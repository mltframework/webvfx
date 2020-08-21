# IMPORTANT

THIS PROJECT IS DEPRECATED BY THE MLT TEAM AND NO LONGER MAINTAINED.

The QtWebKit library it uses was deprecated by the Qt project since 2015. We are aware of the GitHub project to keep
the library alive. However, we no longer want to maintain this sub-project due to minimal resources and lack of
contribution. Also, the integration of QtWebKit into Qt 6 is uncertain.

# Overview

WebVfx is a video effects library that allows effects to be implemented using [WebKit HTML](http://trac.webkit.org/wiki/QtWebKit) or [Qt QML](http://doc.qt.io/qt-5/qmlapplications.html).

## Prerequisites

You will need [Qt](http://qt-project.org/downloads/) installed (5.2 or later recommended). Qt includes QtWebKit and QML, but since v5.6, you need to [build QtWebKit](http://trac.webkit.org/wiki/QtWebKit#BuildInstructions) yourself.
You may also want to build [Qt3D](http://doc.qt.io/qt-5/qt3d-index.html) - 3D extensions to QML. WebVfx includes support for Qt3D, and Qt include Qt3D as of v5.6.

WebVfx includes an MLT service that exposes producers, filters and transitions implemented in WebVfx. Install [MLT Framework](https://www.mltframework.org/) 0.7.2 or greater to build the plugin.

## Building

In the webvfx directory run `qmake -r PREFIX=/usr/local` and then `make install`. `PREFIX` determines where WebVfx will be installed. If MLT is installed in a non-standard location, you may need to set the `PKG_CONFIG_PATH` environment variable to where its pkgconfig file lives, e.g. `PKG_CONFIG_PATH=/usr/local/lib/pkgconfig`.

The [MLT melt](https://www.mltframework.org/twiki/bin/view/MLT/MltMelt) command
will not work with WebVfx on Windows or macOS because the Qt event loop must run
on the main thread. If you set `MLT_SOURCE` to the root of your MLT source code
directory, then a `qmelt` executable will be installed which behaves the same as
`melt` but works with WebVfx on Windows or macOS. e.g. `qmake -r
PREFIX=/usr/local MLT_SOURCE=~/Projects/mlt`.
The MLT source is available as a git submodule to make this more convenient. If
you checkout the git submodule, then it is detected and there is no need to set
MLT_SOURCE.

`make doxydoc` to generate the documentation using Doxygen.
You can also `make uninstall`, `make clean` and `make distclean`.

## Demos

See the [documentation](https://www.mltframework.org/doxygen/webvfx/) for examples.

## License

Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
