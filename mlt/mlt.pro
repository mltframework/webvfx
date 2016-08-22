# Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include(../common.pri)
TEMPLATE = lib

HEADERS += factory.h
HEADERS += service_locker.h
HEADERS += service_manager.h

SOURCES += factory.cpp
SOURCES += panzoom_producer.cpp
SOURCES += service_locker.cpp
SOURCES += service_manager.cpp
SOURCES += webvfx_filter.cpp
SOURCES += webvfx_producer.cpp
SOURCES += webvfx_transition.cpp

CONFIG += plugin shared

TARGET = mltwebvfx

LIBS += -L$$DESTDIR -lwebvfx

macx:isEqual(QT_MAJOR_VERSION, 5) {
    # QMake from Qt 5.1.0 on OSX is messing with the environment in which it runs
    # pkg-config such that the PKG_CONFIG_PATH env var is not set.
    isEmpty(MLT_PREFIX) {
        MLT_PREFIX = /opt/local
    }
    INCLUDEPATH += $$MLT_PREFIX/include
    INCLUDEPATH += $$MLT_PREFIX/include/mlt
    LIBS += -L$$MLT_PREFIX/lib -lmlt
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt-framework
}
win32 {
    QT += opengl quick
    isEqual(QT_MAJOR_VERSION, 5) {
        QT += webkitwidgets
    } else {
        QT += webkit
    }
    LIBS += -lglu32 -lopengl32 -lpthread
}

QMAKE_RPATHDIR += $$PREFIX/lib

# Install in mlt plugins directory
target.path = $$system(pkg-config --variable=libdir mlt-framework)/mlt
INSTALLS += target
# Add mlt plugins to rpath so we can dlopen ourself without a full path.
QMAKE_RPATHDIR += $$target.path
