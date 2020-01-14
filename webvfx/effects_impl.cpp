#include <QApplication>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSize>
#include <QStringBuilder>
#include <QThread>
#include <QUrl>
#include <QWaitCondition>
#include "webvfx/content.h"
#include "webvfx/effects_impl.h"
#include "webvfx/image.h"
#include "webvfx/parameters.h"
#include "webvfx/qml_content.h"
#include "webvfx/web_content.h"
#include "webvfx/webvfx.h"


namespace WebVfx
{

EffectsImpl::EffectsImpl()
    : QObject(0)
    , content(0)
    , mutex(0)
    , waitCondition(0)
    , initializeResult(false)
    , renderResult(false)
{
}

EffectsImpl::~EffectsImpl()
{
    delete content;
}

bool EffectsImpl::initialize(const QString& fileName, int width, int height, Parameters* parameters, bool isTransparent)
{
    if (onUIThread()) {
        log("WebVfx::Effects cannot be initialized on main UI thread.");
        return false;
    }

    QUrl url(fileName);
    bool isPlain = (url.scheme() == "plain");
    if (isPlain) {
        // Remove the "plain:" prefix interpeted as the scheme.
        url = QUrl(url.toString(QUrl::RemoveScheme));
    }
    // If no scheme or the scheme is a Windows drive letter.
    if (url.scheme().size() < 2) {
        // Prepend the file scheme.
        url = QUrl::fromLocalFile(QFileInfo(url.toString()).absoluteFilePath());
        if (!url.isValid()) {
            log(QLatin1Literal("Invalid URL: ") % fileName);
            return false;
        }
    }

    QSize size(width, height);

    QMutex mutex;
    QWaitCondition waitCondition;
    this->mutex = &mutex;
    this->waitCondition = &waitCondition;
    {
        QMutexLocker locker(&mutex);
        // Move ourself onto GUI thread and create our Content there.
        // Invoke this async then wait for result.
        this->moveToThread(QApplication::instance()->thread());
        QMetaObject::invokeMethod(this, "initializeInvokable",
                                  Qt::QueuedConnection,
                                  Q_ARG(QUrl, url), Q_ARG(QSize, size),
                                  Q_ARG(Parameters*, parameters),
                                  Q_ARG(bool, isPlain),
                                  Q_ARG(bool, isTransparent));
        //XXX should we wait with a timeout and fail if expires?
        waitCondition.wait(&mutex);
    }
    this->mutex = 0;
    this->waitCondition = 0;
    return initializeResult;
}

void EffectsImpl::initializeComplete(bool result)
{
    if (mutex && waitCondition) {
        QMutexLocker locker(mutex);
        initializeResult = result;
        waitCondition->wakeAll();
    }
}

void EffectsImpl::destroy()
{
    deleteLater();
}

bool EffectsImpl::onUIThread() {
    return QThread::currentThread() == QApplication::instance()->thread();
}

const Effects::ImageTypeMap& EffectsImpl::getImageTypeMap()
{
    return content->getImageTypeMap();
}

void EffectsImpl::setImage(const QString& name, Image* image)
{
    // This may create a QImage and modify QHash - both of those classes
    // are reentrant, so should be safe to do on calling thread as long
    // as access to this EffectsImpl is synchronized.
    content->setImage(name, image);
}

void EffectsImpl::setZoom(const qreal zoom)
{
    content->setZoom(zoom);
}

bool EffectsImpl::render(double time, Image* renderImage)
{
    if (onUIThread()) {
        renderInvokable(time, renderImage);
    }
    else {
        QMutex mutex;
        QWaitCondition waitCondition;
        this->mutex = &mutex;
        this->waitCondition = &waitCondition;
        {
            QMutexLocker locker(&mutex);
            QMetaObject::invokeMethod(this, "renderInvokable",
                                      Qt::QueuedConnection,
                                      Q_ARG(double, time),
                                      Q_ARG(Image*, renderImage));
            //XXX should we wait with a timeout and fail if expires?
            waitCondition.wait(&mutex);
        }
        this->mutex = 0;
        this->waitCondition = 0;
    }
    return renderResult;
}

void EffectsImpl::renderComplete(bool result)
{
    if (mutex && waitCondition) {
        QMutexLocker locker(mutex);
        renderResult = result;
        waitCondition->wakeAll();
    }
}

void EffectsImpl::reload()
{
    if (onUIThread()) {
        reloadInvokable();
    }
    else {
        QMutex mutex;
        QWaitCondition waitCondition;
        this->mutex = &mutex;
        this->waitCondition = &waitCondition;
        {
            QMutexLocker locker(&mutex);
            QMetaObject::invokeMethod(this, "reloadInvokable", Qt::QueuedConnection);
            //XXX should we wait with a timeout and fail if expires?
            waitCondition.wait(&mutex);
        }
        this->mutex = 0;
        this->waitCondition = 0;
    }
}

void EffectsImpl::initializeInvokable(const QUrl& url, const QSize& size, Parameters* parameters, bool isPlain, bool isTransparent)
{
    QString path(url.path());
    // We can't parent QmlContent since we aren't a QWidget.
    // So don't parent either content, and destroy them explicitly.
    if (path.endsWith(".html", Qt::CaseInsensitive) || path.endsWith(".htm", Qt::CaseInsensitive) || !url.isLocalFile()) {
        WebContent* webContent = new WebContent(size, parameters);
        content = webContent;
        if (isTransparent)
            webContent->setTransparent();

        if (isPlain) {
            connect(webContent, SIGNAL(contentPreLoadFinished(bool)), SLOT(initializeComplete(bool)));
        }
        else {
            connect(webContent, SIGNAL(contentLoadFinished(bool)), SLOT(initializeComplete(bool)));
        }
    }
    else if (path.endsWith(".qml", Qt::CaseInsensitive)) {
        QmlContent* qmlContent = new QmlContent(size, parameters);
        content = qmlContent;
        if (isPlain) {
            connect(qmlContent, SIGNAL(contentPreLoadFinished(bool)), SLOT(initializeComplete(bool)));
        }
        else {
            connect(qmlContent, SIGNAL(contentLoadFinished(bool)), SLOT(initializeComplete(bool)));
        }
    }
    else {
        log(QLatin1Literal("WebVfx Filename must end with '.html', '.htm', or '.qml': ") % path);
        return;
    }

    content->loadContent(url);
}

void EffectsImpl::renderInvokable(double time, Image* renderImage)
{
    content->setContentSize(QSize(renderImage->width(), renderImage->height()));
    renderComplete(content->renderContent(time, renderImage));
}

void EffectsImpl::reloadInvokable()
{
    content->reload();
}

}
