/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KIS_PRESSURE_SHARPNESS_OPTION_H
#define KIS_PRESSURE_SHARPNESS_OPTION_H

#include "kis_curve_option.h"
#include <brushengine/kis_paint_information.h>
#include <kritapaintop_export.h>
#include <kis_types.h>


const QString SHARPNESS_FACTOR = "Sharpness/factor";
const QString SHARPNESS_THRESHOLD  = "Sharpness/threshold";

/**
 * This option is responsible to mimic pencil effect from former Pixel Pencil brush engine.auto
 */
class PAINTOP_EXPORT KisPressureSharpnessOption : public KisCurveOption
{
public:
    KisPressureSharpnessOption();

    /**
    * First part of the sharpness is the coordinates: in pen mode they are integers without fractions
    */
    void apply(const KisPaintInformation &info, const QPointF &pt, qint32 &x, qint32 &y, qreal &xFraction, qreal &yFraction) const;

    /**
    * Apply threshold specified by user
    */
    void applyThreshold(KisFixedPaintDeviceSP dab);

    void writeOptionSetting(KisPropertiesConfigurationSP setting) const;
    void readOptionSetting(const KisPropertiesConfigurationSP setting);

    /// threshold has 100 levels (like opacity)
    void setThreshold(qint32 threshold) {
        m_threshold = qBound<qint32>(0, threshold, 100);
    }
    qint32 threshold() {
        return m_threshold;
    }

    void setSharpnessFactor(qreal factor) {
        KisCurveOption::setValue(factor);
    }
    qreal sharpnessFactor() {
        return KisCurveOption::value();
    }

private:
    qint32 m_threshold;
};

#endif
