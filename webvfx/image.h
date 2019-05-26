// Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBVFX_IMAGE_H_
#define WEBVFX_IMAGE_H_

namespace WebVfx
{

/*!
 * @brief Lightweight class for passing around a reference to a buffer of
 * raw image data.
 *
 * Image is a wrapper around a buffer of RGB(A) image data,
 * along with metadata about it (width, height, bytes per row etc.).
 * The underlying image data buffer is not owned by Image and
 * its lifetime must exceed that of the referencing Image instance.
 */
class Image
{
public:
    //! @deprecated Number of bytes per pixel.
    static const int BytesPerPixel = 3;

    Image()
        : pixels_(0)
        , width_(0)
        , height_(0)
        , byteCount_(0)
	, hasAlpha_(false) {}

    /*!
     * @param pixels 24-bit RGB or 32-bit RGBA image data. Must be valid for the lifetime
     * of the Image.
     * @param width Width of the image in pixels
     * @param height Height of the image in pixels
     * @param byteCount Number of bytes in the image.
     *  A row can have more than @c width * @c BytesPerPixel bytes.
     */
    Image(unsigned char* pixels, int width, int height, int byteCount, bool hasAlpha = false)
        : pixels_(pixels)
        , width_(width)
        , height_(height)
        , byteCount_(byteCount)
        , hasAlpha_(hasAlpha) {}

    bool isNull() { return !pixels_; }
    unsigned char* pixels() { return pixels_; }
    const unsigned char* pixels() const { return pixels_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int bytesPerLine() const { return (height_ > 0)? (byteCount_ / height_) : 0; }
    int byteCount() const { return byteCount_; }
    bool hasAlpha() const { return hasAlpha_; }

    void copyPixelsFrom(const Image& sourceImage);
    void copyPixelsTo(Image& targetImage) const;
    bool compatible(const Image& image) const {
        return (image.pixels_ && pixels_
                && image.pixels_ != pixels_
                && image.width_ == width_
                && image.height_ == height_);
    }

private:
    static void copyPixels(const Image& sourceImage, Image& targetImage);

    unsigned char* pixels_;
    int width_;
    int height_;
    int byteCount_;
    bool hasAlpha_;
};

}

#endif
