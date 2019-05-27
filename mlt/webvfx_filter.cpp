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

    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);

    // Get the source image, we will also write our output to it
    *format = mlt_image_rgb24a;
    if ((error = mlt_frame_get_image(frame, image, format, width, height, 1)) != 0)
        return error;

    { // Scope the lock
        MLTWebVfx::ServiceLocker locker(MLT_FILTER_SERVICE(filter));
        if (!locker.initialize(*width, *height))
            return 1;

        bool hasAlpha = (*format == mlt_image_rgb24a);
        std::unique_ptr<WebVfx::Image> sourceImage;
        std::unique_ptr<WebVfx::Image> renderedImage;
        uint8_t* buffer = nullptr;
        
        if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "transparent")) {
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
            manager->render(renderedImage.get(), position, length, hasAlpha);
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
