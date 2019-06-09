// Copyright (c) 2010 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>
#include <getopt.h>
#include <webvfx/webvfx.h>
#include <QFileInfo>
#include <QImage>
#include <QStringList>

void usage(const char* name) {
    std::cerr << "Usage: " << name << " -s|--size <width>x<height> [-p|--parameter <name>=<value>]... [-i|--image <name>=<image-filename>] [-t|--times <time0>,<time1>,...] [-c|--comment <comment>] -o|--output <output-filename> [-h|--help] <html-or-qml-filename>" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        usage(argv[0]);
        return 1;
    }

    std::map<const QString, const QString> parameterMap;
    std::map<const QString, const QImage> imageMap;
    QFileInfo outputFile;
    int width = 0, height = 0;
    QStringList renderTimes;
    QString comment;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        // Render size
        {"size", required_argument, 0, 's'},
        // Parameter name=value to expose to effect
        {"parameter", required_argument, 0, 'p'},
        // Name and filename of an input image
        {"image", required_argument, 0, 'i'},
        // Times to render at
        {"times", required_argument, 0, 't'},
        // Comment
        {"comment", required_argument, 0, 'c'},
        // Output filename to write images to
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "hs:p:i:t:c:o:",
                            long_options, &option_index)) != -1) {
        switch (c) {
        case 'h':
            usage(argv[0]);
            return 1;
        case 's': {
            char* p = strchr(optarg, 'x');
            if (!p) {
                usage(argv[0]);
                return 1;
            }
            width = QString::fromUtf8(optarg, p - optarg).toInt();
            height = QString::fromUtf8(p + 1).toInt();
            break;
        }
        case 'p': {
            char* p = strchr(optarg, '=');
            if (!p) {
                usage(argv[0]);
                return 1;
            }
            QString name(QString::fromUtf8(optarg, p - optarg));
            QString value(QString::fromUtf8(p + 1));
            parameterMap.insert(std::pair<const QString, const QString>(name, value));
            break;
        }
        case 'i': {
            char* p = strchr(optarg, '=');
            if (!p) {
                usage(argv[0]);
                return 1;
            }
            QString name(QString::fromUtf8(optarg, p - optarg));
            QImage image(QString::fromUtf8(p + 1));
            image = image.convertToFormat(QImage::Format_RGB888);
            imageMap.insert(std::pair<const QString, const QImage>(name, image));
            break;
        }
        case 't':
            renderTimes = QString::fromUtf8(optarg).split(",");
            break;
        case 'c':
            comment = QString::fromUtf8(optarg);
            break;
        case 'o':
            outputFile = QFileInfo(QString::fromUtf8(optarg));
            break;
        case '?':
        default:
            return 1;
        }
    }

    if (outputFile.filePath().isEmpty() || optind >= argc || width == 0 || height == 0) {
        usage(argv[0]);
        return 1;
    }
    if (renderTimes.empty())
        renderTimes.append("0");

    const char* effectFile = argv[optind];

    class Logger : public WebVfx::Logger {
    public:
        void log(const QString& message) {
            std::cerr << message.toStdString() << std::endl;
        }
    };
    WebVfx::setLogger(new Logger());

    class Parameters : public WebVfx::Parameters {
    public:
        Parameters(std::map<const QString, const QString> &map) : map(map) {}
        QString getStringParameter(const QString& name) {
            return map[name];
        }
        double getNumberParameter(const QString& name) {
            return map[name].toDouble();
        }
        QVariantMap getRectParameter(const QString& name) {
            QStringList parts = map[name].split(' ');
            QVariantMap map;
            int i = 0;
            if (parts.size() > i) {
                map["x"] = parts[i++].toDouble();
                if (parts.size() > i) {
                    map["y"] = parts[i++].toDouble();
                    if (parts.size() > i) {
                        map["width"] = parts[i++].toDouble();
                        if (parts.size() > i) {
                            map["height"] = parts[i++].toDouble();
                            if (parts.size() > i) {
                                map["opacity"] = parts[i++].toDouble();
                            }
                        }
                    }
                }
            }
            return map;
        }
    private:
        std::map<const QString, const QString>& map;
    };

    class AutoWebVfx {
    public:
        //XXX check return code
        AutoWebVfx() { WebVfx::initialize(); }
        ~AutoWebVfx() { WebVfx::shutdown(); }
    };
    AutoWebVfx vfx;

    WebVfx::Effects* effects = WebVfx::createEffects(effectFile, width, height, new Parameters(parameterMap));
    if (!effects) {
        std::cerr << "Failed to create Effects" << std::endl;
        return 1;
    }

    uchar data[width * height * 3];
    WebVfx::Image renderImage(data, width, height, sizeof(data));

    QString outputPathTemplate(outputFile.path() + "/" + outputFile.completeBaseName()
                               + "-%1." + outputFile.suffix());

    for (int i = 0; i < renderTimes.size(); ++i) {
        // Set input images
        std::map<const QString, const QImage>::iterator it;
        for (it = imageMap.begin(); it != imageMap.end(); it++) {
            QImage image((*it).second);
            // Use constBits to avoid a deep copy
            WebVfx::Image inputImage(const_cast<unsigned char*>(image.constBits()),
                                     image.width(), image.height(),
                                     image.byteCount());
            effects->setImage((*it).first, &inputImage);
        }

        // Render
        double time = renderTimes.at(i).toDouble();
        if (!effects->render(time, &renderImage)) {
            std::cerr << "Failed to render time " << time << std::endl;
            return 1;
        }

        // Save image to disk
        QImage outputImage((const uchar*)renderImage.pixels(),
                           renderImage.width(), renderImage.height(),
                           renderImage.bytesPerLine(), QImage::Format_RGB888);
        if (!comment.isEmpty())
            outputImage.setText("Comment", comment);
        outputImage.save(outputPathTemplate.arg(renderTimes.at(i)));
    }

    effects->destroy(); effects = 0;

    return 0;
}
