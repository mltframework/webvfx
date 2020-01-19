// Copyright (c) 2010 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

extern "C" {
    #include <mlt/framework/mlt_filter.h>
    #include <mlt/framework/mlt_frame.h>
    #include <mlt/framework/mlt_log.h>
    #include <mlt/framework/mlt_consumer.h>
}
#include <webvfx/image.h>
#include <memory>
#include "factory.h"
#include "service_locker.h"
#include "service_manager.h"


static int filterGetImage(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int /*writable*/) {
    int error = 0;

    // Get the filter
    mlt_filter filter = (mlt_filter)mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    // If not a plain resource, then disable preview scaling.
    const char* resource = mlt_properties_get(properties, "resource");
    int use_preview_scale = mlt_properties_get_int(properties, "mlt_resolution_scale");
    if (!use_preview_scale && resource) {
        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        std::string resource2(resource);
        std::string plain = "plain:";
        if (profile && resource2.substr(0, plain.size()) != plain) {
            *width = profile->width;
            *height = profile->height;
        }
    }

    // Get the source image, we will also write our output to it
    *format = mlt_image_rgb24a;
    if ((error = mlt_frame_get_image(frame, image, format, width, height, 1)) != 0)
        return error;

    // Add mlt_profile_scale_width and mlt_profile_scale_height properties for scripts.
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    double scale = mlt_profile_scale_width(profile, *width);
    mlt_properties_set_double(properties, "mlt_profile_scale_width", scale);
    mlt_properties_set_double(properties, "mlt_profile_scale_height",
                              mlt_profile_scale_height(profile, *height));

    { // Scope the lock
        MLTWebVfx::ServiceLocker locker(MLT_FILTER_SERVICE(filter));
        if (!locker.initialize(*width, *height))
            return 1;

        bool hasAlpha = (*format == mlt_image_rgb24a);
        std::unique_ptr<WebVfx::Image> sourceImage;
        std::unique_ptr<WebVfx::Image> renderedImage;
        uint8_t* buffer = nullptr;
        
        if (mlt_properties_get_int(properties, "transparent")) {
            int size = mlt_image_format_size(*format, *width, *height, NULL);
            // Create a new buffer for the source image.
            buffer = (uint8_t*) mlt_pool_alloc(size);
            memcpy(buffer, *image, size);
            // Make the background white.
            memset(*image, 255  , size);
            size = *width * *height * (hasAlpha? 4 : 3);
            // Make the background transparent.
            for (int i = 0; i < *width * *height; i++)
                (*image)[4 * i + 3] = 0;
            sourceImage.reset(new WebVfx::Image(buffer, *width, *height, size, hasAlpha));
            renderedImage.reset(new WebVfx::Image(*image, *width, *height, size, hasAlpha));
        } else {
            int size = *width * *height * (hasAlpha? 4 : 3);
            sourceImage.reset(new WebVfx::Image(*image, *width, *height, size, hasAlpha));
            renderedImage.reset(new WebVfx::Image(*image, *width, *height, size, hasAlpha));
        }

        MLTWebVfx::ServiceManager* manager = locker.getManager();
        manager->setImageForName(manager->getSourceImageName(), sourceImage.get());
        manager->setupConsumerListener(frame);

        // If there is a consumer set on the frame and the consumer is stopped,
        // skip the render step to avoid deadlock. Another thread could have
        // already called mlt_consumer_stop() thereby triggering
        // ServiceManager::onConsumerStopping() and Effects::renderComplete().
        mlt_consumer consumer = static_cast<mlt_consumer>(
            mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame), "consumer", NULL));
        if (!consumer || !mlt_consumer_is_stopped(consumer))
            manager->render(renderedImage.get(), position, length, scale, hasAlpha);
        mlt_pool_release(buffer);
    }

    return error;
}

static mlt_frame filterProcess(mlt_filter filter, mlt_frame frame) {
    // Push the frame filter
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filterGetImage);

    return frame;
}

mlt_service MLTWebVfx::createFilter() {
    mlt_filter self = mlt_filter_new();
    if (self) {
        self->process = filterProcess;
        return MLT_FILTER_SERVICE(self);
    }
    return 0;
}
