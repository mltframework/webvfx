// Copyright (c) 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBVFX_PARAMETERS_H_
#define WEBVFX_PARAMETERS_H_

#include <QVariantMap>

class QString;

namespace WebVfx
{

/*!
 * @brief Callback interface to expose named parameter values to an effect.
 *
 * An instance of this class should be passed to WebVfx::createEffects()
 * to provide the parameters for that Effects instance.
 */
class Parameters
{
public:
    Parameters() {};
    virtual ~Parameters() = 0;
    /*!
     * @brief Return a numeric value for parameter @c name.
     *
     * @param name Parameter name
     */
    virtual double getNumberParameter(const QString& name);
    /*!
     * @brief Return a string value for parameter @c name.
     *
     * @param name Parameter name
     */
    virtual QString getStringParameter(const QString& name);
    /*!
     * @brief Return a rectangle value for parameter @c name.
     *
     * Rectangle is a map of doubles with the following keys:
     * - x
     * - y
     * - width
     * - height
     * - opacity
     * @param name Parameter name
     */
    virtual QVariantMap getRectParameter(const QString& name);
};

}

#endif
