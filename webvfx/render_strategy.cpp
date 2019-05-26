#include <QGLFormat>
#include <QGLFramebufferObject>
#include <QGLWidget>
#include <QDebug>
#include <QPainter>
#include "webvfx/image.h"
#include "webvfx/content.h"
#include "webvfx/render_strategy.h"
#include "webvfx/webvfx.h"


namespace WebVfx
{

GLWidgetRenderStrategy::GLWidgetRenderStrategy(QGLWidget* glWidget)
    : glWidget(glWidget)
    , fbo(0)
{
    glWidget->makeCurrent();
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects()
        || !QGLFramebufferObject::hasOpenGLFramebufferBlit()) {
        log("GLWidgetRenderStrategy: FBOs not fully supported, GL rendering will not work");
    }
    glWidget->doneCurrent();
}

GLWidgetRenderStrategy::~GLWidgetRenderStrategy()
{
    delete fbo;
}

void GLWidgetRenderStrategy::createFBO(const QSize& size)
{
    if (fbo && fbo->size() == size)
        return;

    delete fbo;
    fbo = new QGLFramebufferObject(size, GL_TEXTURE_2D);
}


bool GLWidgetRenderStrategy::render(Content* content, Image* renderImage)
{
    if (!renderImage)
        return false;

    glWidget->makeCurrent();
    QSize size(renderImage->width(), renderImage->height());
    createFBO(size);

    // Render frame into QGLWidget.
    // This isn't really valid for a hidden QGLWidget due to the OpenGL
    // "pixel ownership test". But it works with some graphics drivers.

    fbo->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    QPainter painter(fbo);
    painter.translate(0, size.height());
    painter.scale(1, -1);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform, true);
    content->paintContent(&painter);
    painter.end();

    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    if (renderImage->hasAlpha()) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH,   0);
        glPixelStorei(GL_UNPACK_ALIGNMENT,    4);
        glReadPixels(0, 0, renderImage->width(), renderImage->height(),
                 GL_RGBA, GL_UNSIGNED_BYTE, renderImage->pixels());
    }
    else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH,   0);
        glPixelStorei(GL_UNPACK_ALIGNMENT,    3);
        glReadPixels(0, 0, renderImage->width(), renderImage->height(),
                 GL_RGB, GL_UNSIGNED_BYTE, renderImage->pixels());
    }
    glPopClientAttrib();
    fbo->release();
    glWidget->doneCurrent();

    return true;
}

FBORenderStrategy::FBORenderStrategy(QGLWidget* glWidget)
    : glWidget(glWidget)
    , multisampleFBO(0)
    , resolveFBO(0)
{
    glWidget->makeCurrent();
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects()
        || !QGLFramebufferObject::hasOpenGLFramebufferBlit()) {
        log("FBORenderStrategy: FBOs not fully supported, antialiasing will not work");
    }
    glWidget->doneCurrent();
}

FBORenderStrategy::~FBORenderStrategy()
{
    delete multisampleFBO;
    delete resolveFBO;
}

void FBORenderStrategy::createFBOs(const QSize& size)
{
    if (multisampleFBO && resolveFBO && resolveFBO->size() == size)
        return;

    // Create a multisample FBO and an FBO to resolve into
    QGLFramebufferObjectFormat fboFormat;
    fboFormat.setSamples(4);
    fboFormat.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
    delete multisampleFBO;
    multisampleFBO = new QGLFramebufferObject(size, fboFormat);
    delete resolveFBO;
    resolveFBO = new QGLFramebufferObject(size);
}

bool FBORenderStrategy::render(Content* content, Image* renderImage)
{
    if (!renderImage)
        return false;

    glWidget->makeCurrent();
    QSize size(renderImage->width(), renderImage->height());
    createFBOs(size);

    // Render frame into multisample FBO
    QPainter painter(multisampleFBO);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform, true);
    content->paintContent(&painter);
    painter.end();

    // Blit from multisample to resolve FBO.
    // Rects are setup so the image is vertically flipped when blitted
    // so when we read the pixels back they are the right way up.
    // OpenGL does everything "upside down".
    QRect srcRect(0, 0, renderImage->width(), renderImage->height());
    QRect dstRect(0, renderImage->height(),
                  renderImage->width(), -renderImage->height());
    QGLFramebufferObject::blitFramebuffer(resolveFBO, srcRect,
                                          multisampleFBO, dstRect);

    // Read back the pixels from the resolve FBO
    resolveFBO->bind();
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    
    if (renderImage->hasAlpha()) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH,   0);
        glPixelStorei(GL_UNPACK_ALIGNMENT,    4);
        glReadPixels(0, 0, renderImage->width(), renderImage->height(),
                 GL_RGBA, GL_UNSIGNED_BYTE, renderImage->pixels());
    }
    else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH,   0);
        glPixelStorei(GL_UNPACK_ALIGNMENT,    3);
        glReadPixels(0, 0, renderImage->width(), renderImage->height(),
                 GL_RGB, GL_UNSIGNED_BYTE, renderImage->pixels());
    }
    
    glPopClientAttrib();

    resolveFBO->release();
    glWidget->doneCurrent();

    return true;
}

bool ImageRenderStrategy::render(Content* content, Image* renderImage)
{
    if (!renderImage || !renderImage->pixels() || !renderImage->width() || !renderImage->height())
        return false;
    // QImage referencing our Image bits
    QImage image((uchar*)renderImage->pixels(), renderImage->width(),
                 renderImage->height(), renderImage->bytesPerLine(),
                  renderImage->hasAlpha()? QImage::Format_RGBA8888 : QImage::Format_RGB888);
    // Paint into image
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform, true);
    content->paintContent(&painter);
    painter.end();
    return true;
}

}
