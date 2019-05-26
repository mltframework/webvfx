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
        int size = *width * *height * (hasAlpha? 4 : 3);
        WebVfx::Image renderedImage(*image, *width, *height, size, hasAlpha);
        MLTWebVfx::ServiceManager* manager = locker.getManager();
        manager->setImageForName(manager->getSourceImageName(), &renderedImage);
        manager->setupConsumerListener(frame);

        // If there is a consumer set on the frame and the consumer is stopped,
        // skip the render step to avoid deadlock. Another thread could have
        // already called mlt_consumer_stop() thereby triggering
        // ServiceManager::onConsumerStopping() and Effects::renderComplete().
        mlt_consumer consumer = static_cast<mlt_consumer>(
            mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame), "consumer", NULL));
        if (!consumer || !mlt_consumer_is_stopped(consumer))
            manager->render(&renderedImage, position, length, hasAlpha);
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
