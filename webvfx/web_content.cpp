#ifdef WEBVFX_GRAPHICSVIEW
#include <QGLWidget>
#include <QGraphicsScene>
#include <QGraphicsWebView>
#endif
#include <QMap>
#include <QPainter>
#include <QSize>
#include <QStringBuilder>
#include <QVariant>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>
#include "webvfx/image.h"
#include "webvfx/render_strategy.h"
#include "webvfx/web_content.h"
#include "webvfx/webvfx.h"

namespace WebVfx
{

WebPage::WebPage(QObject* parent) : QWebPage(parent) {
    mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    settings()->setAttribute(QWebSettings::SiteSpecificQuirksEnabled, false);
    settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, true);
    settings()->setAttribute(QWebSettings::WebGLEnabled, true);
}

bool WebPage::shouldInterruptJavaScript() {
    return false;
}

void WebPage::javaScriptAlert(QWebFrame *, const QString &msg) {
    log(QLatin1Literal("JavaScript alert: ") % msg);
}

void WebPage::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID) {
    QString msg(message);
    if (!sourceID.isEmpty())
        msg % QLatin1Literal(" (") % sourceID.section('/', -1) % QLatin1Literal(":") % QString::number(lineNumber) % QLatin1Literal(")");
    log(msg);
}

bool WebPage::acceptNavigationRequest(QWebFrame*, const QNetworkRequest&, NavigationType) {
    //XXX we want to prevent JS from navigating the page - does this prevent our initial load?
    //return false;
    return true;
}

////////////////////

WebContent::WebContent(const QSize& size, Parameters* parameters)
#ifdef WEBVFX_GRAPHICSVIEW
    : QGraphicsView((QWidget*)0)
    , webView(new QGraphicsWebView)
#else
    : QObject(0)
#endif
    , webPage(new WebPage(this))
    , pageLoadFinished(LoadNotFinished)
    , contextLoadFinished(LoadNotFinished)
    , contentContext(new ContentContext(this, parameters))
    , renderStrategy(0)
{
// Enabling WEBVFX_GRAPHICSVIEW is the most efficient rendering path, especially
// for WebGL. But due to bug https://bugs.webkit.org/show_bug.cgi?id=63946
// we must use GLWidgetRenderStrategy which will not work with all OpenGL drivers.
#ifdef WEBVFX_GRAPHICSVIEW
    // Turn off scrollbars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setInteractive(false);
    setFrameStyle(0);

    webView->setPage(webPage);
    setScene(new QGraphicsScene(this));
    // Scene owns webView
    scene()->addItem(webView);

    // Must use a QGraphicsWebView with a parent QAbstractScrollArea
    // with a QGLWidget viewport, then WebGL will find this QGLWidget
    // and share it for rendering.
    // See WebKit:
    //   GraphicsContext3DQt.cpp GraphicsContext3DInternal::getViewportGLWidget
    // The QGLWidget also makes WebKit use the GL TextureMapper.
    // See WebKit:
    //   PageClientQt.cpp PageClientQGraphicsWidget::setRootGraphicsLayer
    QGLWidget* glWidget = new QGLWidget;
    setViewport(glWidget);
    // OpenGL needs this
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    renderStrategy = new GLWidgetRenderStrategy(glWidget);
#else
    renderStrategy = new ImageRenderStrategy();
#endif

    connect(webPage, SIGNAL(loadFinished(bool)),
            SLOT(webPageLoadFinished(bool)));
    connect(contentContext, SIGNAL(readyRender(bool)),
            SLOT(contentContextLoadFinished(bool)));
    connect(webPage->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()),
            SLOT(injectContentContext()));

    setContentSize(size);
}

WebContent::~WebContent()
{
    delete renderStrategy;
}

void WebContent::injectContentContext()
{
    webPage->mainFrame()->addToJavaScriptWindowObject("webvfx", contentContext);
}

void WebContent::webPageLoadFinished(bool result)
{
    if (pageLoadFinished == LoadNotFinished)
        pageLoadFinished = result ? LoadSucceeded : LoadFailed;

    // This is useful when webvfx.renderReady(true) is not used.
    emit contentPreLoadFinished(pageLoadFinished == LoadSucceeded);

    if (pageLoadFinished == LoadFailed || contextLoadFinished != LoadNotFinished)
        emit contentLoadFinished(contextLoadFinished == LoadSucceeded && pageLoadFinished == LoadSucceeded);
}

void WebContent::contentContextLoadFinished(bool result)
{
    if (contextLoadFinished == LoadNotFinished)
        contextLoadFinished = result ? LoadSucceeded : LoadFailed;
    if (contextLoadFinished == LoadFailed || pageLoadFinished != LoadNotFinished)
        emit contentLoadFinished(contextLoadFinished == LoadSucceeded && pageLoadFinished == LoadSucceeded);
}

void WebContent::loadContent(const QUrl& url)
{
    pageLoadFinished = LoadNotFinished;
    contextLoadFinished = LoadNotFinished;

    webPage->mainFrame()->load(url);
}

void WebContent::setContentSize(const QSize& size) {
#ifdef WEBVFX_GRAPHICSVIEW
    if (size != this->size()) {
        webView->resize(size);
        resize(size);
        viewport()->resize(size);
    }
#else
    if (webPage->viewportSize() != size)
        webPage->setViewportSize(size);
#endif
}

bool WebContent::renderContent(double time, Image* renderImage)
{
    // Allow the page to render for this time
    contentContext->render(time);
    return renderStrategy->render(this, renderImage);
}

void WebContent::paintContent(QPainter* painter)
{
    webPage->mainFrame()->render(painter);
}

void WebContent::setZoom(const qreal zoom)
{
    webPage->mainFrame()->setZoomFactor(zoom);
}

QWidget* WebContent::createView(QWidget* parent)
{
#ifdef WEBVFX_GRAPHICSVIEW
    setParent(parent);
    return this;
#else
    QWebView* webView = new QWebView(parent);
    setParent(webView);
    webView->setPage(webPage);
    return webView;
#endif
}

void WebContent::reload()
{
    webPage->triggerAction(QWebPage::ReloadAndBypassCache);
}

QWebSettings* WebContent::settings()
{
    return webPage->settings();
}

void WebContent::setTransparent()
{
    if (webPage) {
        QPalette pal = webPage->palette();
        pal.setBrush(QPalette::Base, Qt::transparent);
        webPage->setPalette(pal);
    }
}

}
