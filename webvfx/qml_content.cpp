#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QResizeEvent>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QQuickImageProvider>
#include <QWidget>
#include "webvfx/image.h"
#include "webvfx/qml_content.h"
#include "webvfx/render_strategy.h"
#include "webvfx/webvfx.h"


namespace WebVfx
{

class PixmapProvider : public QQuickImageProvider
{
 public:
     PixmapProvider(ContentContext* contentContext)
         : QQuickImageProvider(QQuickImageProvider::Pixmap)
         , contentContext(contentContext)
     {
     }

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
    {
        // URLs are of the form image://webvfx/<name>/<count>
        // where <count> is a unique ID to force refresh and is ignored.
        QImage image(contentContext->getImage(id.section('/', 0, 0)));
        QPixmap pixmap(QPixmap::fromImage(image));

        if (size)
            *size = pixmap.size();

        if (!requestedSize.isEmpty())
            return pixmap.scaled(requestedSize, Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);

        return pixmap;
    }

private:
    ContentContext* contentContext;
 };

////////////////////

QmlContent::QmlContent(const QSize& size, Parameters* parameters)
    : QQuickView(0)
    , pageLoadFinished(LoadNotFinished)
    , contextLoadFinished(LoadNotFinished)
    , contentContext(new ContentContext(this, parameters))
{
    // Add root of our qrc:/ resource path so embedded QML components are available.
    engine()->addImportPath(":/");

    /* setInteractive(false); */
    setResizeMode(QQuickView::SizeRootObjectToView);
    /* setResizeAnchor(QDeclarativeView::AnchorViewCenter); */
    resize(size);
    setColor(Qt::transparent);

    // Expose context to the QML
    rootContext()->setContextProperty("webvfx", contentContext);

    // Register image provider for image://webvfx/<name>/<counter>
    engine()->addImageProvider(QLatin1String("webvfx"), new PixmapProvider(contentContext));

    connect(this, SIGNAL(statusChanged(QQuickView::Status)), SLOT(qmlViewStatusChanged(QQuickView::Status)));
    connect(engine(), SIGNAL(warnings(QList<QQmlError>)), SLOT(logWarnings(QList<QQmlError>)));
    connect(contentContext, SIGNAL(readyRender(bool)), SLOT(contentContextLoadFinished(bool)));
}

QmlContent::~QmlContent()
{
}

void QmlContent::qmlViewStatusChanged(QQuickView::Status status)
{
    if (status != QQuickView::Ready && status != QQuickView::Error)
        return;

    if (pageLoadFinished == LoadNotFinished)
        pageLoadFinished = (status == QQuickView::Ready) ? LoadSucceeded : LoadFailed;

    // This is useful when webvfx.renderReady(true) is not used.
    emit contentPreLoadFinished(pageLoadFinished == LoadSucceeded);

    if (pageLoadFinished == LoadFailed || contextLoadFinished != LoadNotFinished) {
        logWarnings(errors());
        emit contentLoadFinished(contextLoadFinished == LoadSucceeded && pageLoadFinished == LoadSucceeded);
    }
}

QWidget* QmlContent::createView(QWidget* parent)
{
    return QWidget::createWindowContainer(this, parent);
}

void QmlContent::contentContextLoadFinished(bool result)
{
    if (contextLoadFinished == LoadNotFinished)
        contextLoadFinished = result ? LoadSucceeded : LoadFailed;
    if (contextLoadFinished == LoadFailed || pageLoadFinished != LoadNotFinished) {
        logWarnings(errors());
        emit contentLoadFinished(contextLoadFinished == LoadSucceeded && pageLoadFinished == LoadSucceeded);
    }
}

void QmlContent::logWarnings(const QList<QQmlError>& warnings)
{
    foreach (const QQmlError& warning, warnings) {
        log(warning.toString());
    }
}

void QmlContent::loadContent(const QUrl& url)
{
    pageLoadFinished = LoadNotFinished;
    contextLoadFinished = LoadNotFinished;

    QSize originalSize(size());

    setSource(url);

    /* grabWindow will not work until a gl context has been initialized, which does not happen until
     * the quickwindow has been shown once */
    show();
}

void QmlContent::setContentSize(const QSize& size)
{
    resize(size);
}

bool QmlContent::renderContent(double time, Image* renderImage)
{
    contentContext->render(time);

    if (renderImage) {
        hide();
        QImage sourceImage = grabWindow();
        QImage targetImage((uchar*)renderImage->pixels(), renderImage->width(),
                renderImage->height(), renderImage->bytesPerLine(),
                QImage::Format_RGB888);
        QPainter p(&targetImage);
        p.drawImage(QPoint(), sourceImage);
        m_mostRecentImage = sourceImage;
    }
    logWarnings(errors());
    return true;
}

void QmlContent::paintContent(QPainter* painter)
{
    painter->drawImage(QPoint(), m_mostRecentImage);
}

void QmlContent::reload()
{
    engine()->clearComponentCache();
    setSource(source());

}

}
