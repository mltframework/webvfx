// Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBVFX_QML_CONTENT_H_
#define WEBVFX_QML_CONTENT_H_

#include <QQuickView>
#include <QGraphicsEffect>
#include "webvfx/content.h"
#include "webvfx/content_context.h"
#include "webvfx/effects.h"
#include "webvfx/image.h"

class QSize;
class QUrl;

namespace WebVfx
{

class Image;
class Parameters;

class QmlContent : public QQuickView, public virtual Content
{
    Q_OBJECT
public:
    QmlContent(const QSize& size, Parameters* parameters);
    ~QmlContent();

    void loadContent(const QUrl& url);
    void setContentSize(const QSize& size);
    const Effects::ImageTypeMap& getImageTypeMap() { return contentContext->getImageTypeMap(); };
    bool renderContent(double time, Image* renderImage);
    void paintContent(QPainter* painter);
    void setImage(const QString& name, Image* image) { contentContext->setImage(name, image); }
    void reload();

    QWidget* createView(QWidget* parent);

signals:
    void contentLoadFinished(bool result);
    void contentPreLoadFinished(bool result);

private slots:
    void qmlViewStatusChanged(QQuickView::Status status);
    void contentContextLoadFinished(bool result);
    void logWarnings(const QList<QQmlError>& warnings);

private:
    enum LoadStatus { LoadNotFinished, LoadFailed, LoadSucceeded };
    LoadStatus pageLoadFinished;
    LoadStatus contextLoadFinished;
    ContentContext* contentContext;
    QImage m_mostRecentImage;
};

}

#endif
