# Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include(../common.pri)
TEMPLATE = app

SOURCES += main.cpp
SOURCES += image_color.cpp
SOURCES += render_dialog.cpp
SOURCES += viewer.cpp
HEADERS += image_color.h
HEADERS += render_dialog.h
HEADERS += viewer.h
FORMS += render_dialog.ui
FORMS += viewer.ui

isEqual(QT_MAJOR_VERSION, 5) {
    QT += webkitwidgets
} else {
    QT += webkit
}
QT += quick

!mac:TARGET = webvfx_viewer
mac:TARGET = "WebVfx Viewer"
LIBS += -L$$DESTDIR -lwebvfx

QMAKE_RPATHDIR += $$PREFIX/lib

target.path = $$PREFIX/bin
INSTALLS += target
