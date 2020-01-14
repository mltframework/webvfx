// Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBVFX_WEB_CONTENT_H_
#define WEBVFX_WEB_CONTENT_H_


#include <QMap>

#ifdef WEBVFX_GRAPHICSVIEW
#include <QGraphicsView>
#endif
#include <QWebPage>
#include "webvfx/content.h"
#include "webvfx/content_context.h"
#include "webvfx/effects.h"

class QGraphicsWebView;
class QImage;
class QSize;
class QWebFrame;

namespace WebVfx
{

class Image;
class Parameters;
class RenderStrategy;
class WebPage;

class WebContent
#ifdef WEBVFX_GRAPHICSVIEW
    : public QGraphicsView
#else
    : public QObject
#endif
    , public virtual Content
{
    Q_OBJECT
public:
    WebContent(const QSize& size, Parameters* parameters);
    ~WebContent();

    void loadContent(const QUrl& url);
    void setContentSize(const QSize& size);
    const Effects::ImageTypeMap& getImageTypeMap() { return contentContext->getImageTypeMap(); };
    bool renderContent(double time, Image* renderImage);
    void paintContent(QPainter* painter);
    void setImage(const QString& name, Image* image) { contentContext->setImage(name, image); }
    void setZoom(const qreal zoom);

    QWidget* createView(QWidget* parent);
    void reload();

    // For debugging with Viewer
    QWebSettings* settings();

    void setTransparent();

signals:
    void contentLoadFinished(bool result);
    void contentPreLoadFinished(bool result);

private slots:
    void injectContentContext();
    void webPageLoadFinished(bool result);
    void contentContextLoadFinished(bool result);

private:
#ifdef WEBVFX_GRAPHICSVIEW
    QGraphicsWebView* webView;
#endif
    WebPage* webPage;
    enum LoadStatus { LoadNotFinished, LoadFailed, LoadSucceeded };
    LoadStatus pageLoadFinished;
    LoadStatus contextLoadFinished;
    ContentContext* contentContext;
    RenderStrategy* renderStrategy;
};

////////////////////

class QWebFrame;
class QNetworkRequest;

class WebPage : public QWebPage
{
    Q_OBJECT
public:
    WebPage(QObject* parent);

private slots:
    bool shouldInterruptJavaScript();

protected:
    void javaScriptAlert(QWebFrame *, const QString &msg);
    void javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID);
    bool acceptNavigationRequest(QWebFrame*, const QNetworkRequest&, NavigationType);
};

}

#endif
