/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KIS_SLIDER_BASED_PAINTOP_PROPERTY_H
#define __KIS_SLIDER_BASED_PAINTOP_PROPERTY_H

#include "kis_uniform_paintop_property.h"


/**
 * This is a general class for the properties that can be represented
 * in the GUI as an integer or double slider. The GUI representation
 * creates a slider and connects it to this property using all the
 * information contained in it.
 *
 * Methods of this property basically copy the methods of
 * Kis{,Double}SliderSpinbox
 */

template <typename T>
class KisSliderBasedPaintOpProperty : public KisUniformPaintOpProperty
{
public:
    KisSliderBasedPaintOpProperty(Type type,
                                  const QString &id,
                                  const QString &name,
                                  KisPaintOpSettingsRestrictedSP settings,
                                  QObject *parent)
        : KisUniformPaintOpProperty(type, id, name, settings, parent),
        m_min(T(0)),
        m_max(T(100)),
        m_singleStep(T(1)),
        m_pageStep(T(10)),
        m_exponentRatio(1.0),
        m_decimals(2)
    {
    }

    KisSliderBasedPaintOpProperty(const QString &id,
                                  const QString &name,
                                  KisPaintOpSettingsRestrictedSP settings,
                                  QObject *parent)
        : KisUniformPaintOpProperty(Int, id, name, settings, parent),
        m_min(T(0)),
        m_max(T(100)),
        m_singleStep(T(1)),
        m_pageStep(T(10)),
        m_exponentRatio(1.0),
        m_decimals(2)
    {
        qFatal("Should have never been called!");
    }

    T min() const {
        return m_min;
    }
    T max() const {
        return m_max;
    }
    void setRange(T min, T max) {
        m_min = min;
        m_max = max;
    }

    T singleStep() const {
        return m_singleStep;
    }
    void setSingleStep(T value) {
        m_singleStep = value;
    }

    T pageStep() const {
        return m_pageStep;
    }
    void setPageStep(T value) {
        m_pageStep = value;
    }

    qreal exponentRatio() const {
        return m_exponentRatio;
    }
    void setExponentRatio(qreal value) {
        m_exponentRatio = value;
    }

    int decimals() const {
        return m_decimals;
    }
    void setDecimals(int value) {
        m_decimals = value;
    }

    QString suffix() const {
        return m_suffix;
    }
    void setSuffix(QString value) {
        m_suffix = value;
    }


private:
    T m_min;
    T m_max;

    T m_singleStep;
    T m_pageStep;
    qreal m_exponentRatio;

    int m_decimals;
    QString m_suffix;
};

#include "kis_callback_based_paintop_property.h"

extern template class KRITAIMAGE_EXPORT KisSliderBasedPaintOpProperty<int>;
extern template class KRITAIMAGE_EXPORT KisSliderBasedPaintOpProperty<qreal>;
extern template class KRITAIMAGE_EXPORT KisCallbackBasedPaintopProperty<KisSliderBasedPaintOpProperty<int>>;
extern template class KRITAIMAGE_EXPORT KisCallbackBasedPaintopProperty<KisSliderBasedPaintOpProperty<qreal>>;

typedef KisSliderBasedPaintOpProperty<int> KisIntSliderBasedPaintOpProperty;
typedef KisSliderBasedPaintOpProperty<qreal> KisDoubleSliderBasedPaintOpProperty;

typedef KisCallbackBasedPaintopProperty<KisSliderBasedPaintOpProperty<int>> KisIntSliderBasedPaintOpPropertyCallback;
typedef KisCallbackBasedPaintopProperty<KisSliderBasedPaintOpProperty<qreal>> KisDoubleSliderBasedPaintOpPropertyCallback;



#endif /* __KIS_SLIDER_BASED_PAINTOP_PROPERTY_H */
