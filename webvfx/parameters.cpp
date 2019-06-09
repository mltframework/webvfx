#include <QString>
#include "webvfx/parameters.h"

WebVfx::Parameters::~Parameters()
{
}

double WebVfx::Parameters::getNumberParameter(const QString&)
{
    return 0;
}

QString WebVfx::Parameters::getStringParameter(const QString&)
{
    return QString();
}

QVariantMap WebVfx::Parameters::getRectParameter(const QString&)
{
    return QVariantMap();
}
