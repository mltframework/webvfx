# Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include(../common.pri)
TEMPLATE = lib
#VERSION = 

HEADERS += content.h
HEADERS += content_context.h
HEADERS += effects.h
HEADERS += effects_impl.h
HEADERS += image.h
HEADERS += logger.h
HEADERS += parameters.h
HEADERS += qml_content.h
HEADERS += render_strategy.h
HEADERS += web_content.h
HEADERS += webvfx.h

SOURCES += content.cpp
SOURCES += content_context.cpp
SOURCES += effects.cpp
SOURCES += effects_impl.cpp
SOURCES += image.cpp
SOURCES += logger.cpp
SOURCES += parameters.cpp
SOURCES += qml_content.cpp
SOURCES += render_strategy.cpp
SOURCES += web_content.cpp
SOURCES += webvfx.cpp
macx:OBJECTIVE_SOURCES += webvfx_mac.mm

RESOURCES += resources/resources.qrc

macx:LIBS += -framework Foundation

win32:CONFIG += staticlib
unix:CONFIG += shared thread

isEqual(QT_MAJOR_VERSION, 5) {
    QT += webkitwidgets
} else {
    QT += webkit
}
QT += opengl quick

TARGET = webvfx

target.path = $$PREFIX/lib
INSTALLS += target
