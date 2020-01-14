// Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBVFX_EFFECTS_H_
#define WEBVFX_EFFECTS_H_

#include <webvfx/image.h>
#include <QMap>
#include <QString>


namespace WebVfx
{

/*!
 * @brief An effects implementation that can consume video frame images and
 *   render output.
 *
 * Instances of this class are created with WebVfx::createEffects()
 * and can be accessed from any thread, but the class is not threadsafe
 * so access should be synchronized.
 */
class Effects
{
public:
    /*!
     * @brief Describes the type of an image.
     */
    enum ImageType {
        /*!
         * The source (origin/from) image in a transition,
         * or the image being processed in a filter.
         */
        SourceImageType=1,
        /*!
         * The target (destination/to) image in a transition.
         */
        TargetImageType,
        /*!
         * An extra image not directly participating in a
         *   filter or transition.
         */
        ExtraImageType
    };

    typedef QMap<QString, ImageType> ImageTypeMap;
    typedef QMapIterator<QString, ImageType> ImageTypeMapIterator;

    /*!
     * @brief Describes the image names this effect will request.
     *
     * Images for these names will need to be set using setImage()
     * each time before render() is called.
     * @return a map mapping image names to their ImageType
     */
    virtual const ImageTypeMap& getImageTypeMap() = 0;

    /*!
     * @brief Set an Image for the given @c name.
     *
     * @param name Name of the image
     * @param image Image data. The Image pixels must remain valid until
     *   render() is called.
     */
    virtual void setImage(const QString& name, Image* image) = 0;

    /*!
     * @brief Set the zoom level.
     *
     * @param zoom the zoom factor
     */
    virtual void setZoom(const qreal zoom) = 0;

    /*!
     * @brief Renders the effect for the given @c time.
     *
     * Prior to calling render() each time, all named images must
     * be set via setImage().
     * @param time Time to render image for, must be from 0 to 1.0.
     * @param renderImage Image buffer to render into.
     */
    virtual bool render(double time, Image* renderImage) = 0;

    /*!
     * @brief Destroy the effect
     */
    virtual void destroy() = 0;

    /*!
     * @brief Indicate that rendering is done.
     *
     * @param result This is the success/fail value to return from render().
     */
    virtual void renderComplete(bool result) = 0;

    /*!
     * @brief Reload the content.
     */
    virtual void reload() = 0;

protected:
    Effects() {};
    virtual ~Effects() = 0;
};

}

#endif
