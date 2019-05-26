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

    mlt_position position = mlt_transition_get_position(transition, aFrame);
    mlt_position length = mlt_transition_get_length(transition);

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
        if (!consumer || !mlt_consumer_is_stopped(consumer))
            manager->render(&renderedImage, position, length);
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
