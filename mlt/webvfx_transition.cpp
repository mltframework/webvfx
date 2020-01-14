// Copyright (c) 2010 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

extern "C" {
    #include <mlt/framework/mlt_transition.h>
    #include <mlt/framework/mlt_frame.h>
    #include <mlt/framework/mlt_log.h>
    #include <mlt/framework/mlt_consumer.h>
}
#include <cstring>
#include <webvfx/image.h>
#include "factory.h"
#include "service_locker.h"
#include "service_manager.h"


static int transitionGetImage(mlt_frame aFrame, uint8_t **image, mlt_image_format *format, int *width, int *height, int /*writable*/) {
    int error = 0;

    mlt_frame bFrame = mlt_frame_pop_frame(aFrame);
    mlt_transition transition = (mlt_transition)mlt_frame_pop_service(aFrame);
    mlt_properties properties = MLT_TRANSITION_PROPERTIES(transition);

    mlt_position position = mlt_transition_get_position(transition, aFrame);
    mlt_position length = mlt_transition_get_length(transition);

    // If not a plain resource, then disable preview scaling.
    const char* resource = mlt_properties_get(properties, "resource");
    int use_preview_scale = mlt_properties_get_int(properties, "mlt_resolution_scale");
    if (!use_preview_scale && resource) {
        mlt_profile profile = mlt_service_profile(MLT_TRANSITION_SERVICE(transition));
        std::string resource2(resource);
        std::string plain = "plain:";
        if (profile && resource2.substr(0, plain.size()) != plain) {
            *width = profile->width;
            *height = profile->height;
            mlt_properties_set_double(MLT_FRAME_PROPERTIES(aFrame), "consumer_scale", 1.0);
        }
    }

    // Make a mlt_frame_resolution_scale filter property available for scripts.
    double scale = mlt_frame_resolution_scale(aFrame);
    mlt_properties_set_double(properties, "mlt_frame_resolution_scale", scale);

    // Get the aFrame image, we will write our output to it
    *format = mlt_image_rgb24;
    if ((error = mlt_frame_get_image(aFrame, image, format, width, height, 1)) != 0)
        return error;
    // Get the bFrame image, we won't write to it
    uint8_t *bImage = NULL;
    int bWidth = 0, bHeight = 0;
    if ((error = mlt_frame_get_image(bFrame, &bImage, format, &bWidth, &bHeight, 0)) != 0)
        return error;

    { // Scope the lock
        MLTWebVfx::ServiceLocker locker(MLT_TRANSITION_SERVICE(transition));
        if (!locker.initialize(*width, *height))
            return 1;

        bool hasAlpha = (*format == mlt_image_rgb24a);
        int size = *width * *height * (hasAlpha? 4 : 3);
        MLTWebVfx::ServiceManager* manager = locker.getManager();
        WebVfx::Image renderedImage(*image, *width, *height, size, hasAlpha);
        manager->setImageForName(manager->getSourceImageName(), &renderedImage);
        size = bWidth * bHeight * (hasAlpha? 4 : 3);
        WebVfx::Image targetImage(bImage, bWidth, bHeight, size, hasAlpha);
        manager->setImageForName(manager->getTargetImageName(), &targetImage);
        manager->setupConsumerListener(aFrame);

        // If there is a consumer set on the frame and the consumer is stopped,
        // skip the render step to avoid deadlock. Another thread could have
        // already called mlt_consumer_stop() thereby triggering
        // ServiceManager::onConsumerStopping() and Effects::renderComplete().
        mlt_consumer consumer = static_cast<mlt_consumer>(
            mlt_properties_get_data(MLT_FRAME_PROPERTIES(aFrame), "consumer", NULL));
        if (!consumer || !mlt_consumer_is_stopped(consumer)) {
            manager->render(&renderedImage,
                            position, length,
                            mlt_frame_resolution_scale(aFrame));
        }
    }

    return error;
}

static mlt_frame transitionProcess(mlt_transition transition, mlt_frame aFrame, mlt_frame bFrame) {
    mlt_frame_push_service(aFrame, transition);
    mlt_frame_push_frame(aFrame, bFrame);
    mlt_frame_push_get_image(aFrame, transitionGetImage);
    return aFrame;
}

mlt_service MLTWebVfx::createTransition() {
    mlt_transition self = mlt_transition_new();
    if (self) {
        self->process = transitionProcess;
        // Video only transition
        mlt_properties_set_int(MLT_TRANSITION_PROPERTIES(self), "_transition_type", 1);
        return MLT_TRANSITION_SERVICE(self);
    }
    return 0;
}
