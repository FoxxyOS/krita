/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
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

#ifndef KIS_PRESSURE_OPACITY_OPTION
#define KIS_PRESSURE_OPACITY_OPTION

#include "kis_curve_option.h"
#include <kritapaintop_export.h>

class KisPainter;
/**
 * The pressure opacity option defines a curve that is used to
 * calculate the effect of pressure on opacity
 */
class PAINTOP_EXPORT KisPressureOpacityOption : public KisCurveOption
{
public:

    KisPressureOpacityOption();

    /**
     * Set the opacity of the painter based on the pressure
     * and the curve (if checked) and return the old opacity
     * of the painter.
     */
    quint8 apply(KisPainter* painter, const KisPaintInformation& info) const;

    qreal  getOpacityf(const KisPaintInformation& info);

    virtual void writeOptionSetting(KisPropertiesConfigurationSP setting) const;
    virtual void readOptionSetting(const KisPropertiesConfigurationSP setting);

};

#endif
