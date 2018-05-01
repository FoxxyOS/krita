/* This file is part of the KDE project
 * Copyright (C) Nishant Rodrigues <nishantjr@gmail.com>, (C) 2016
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

#pragma once

#include "kis_curve_option.h"
#include <brushengine/kis_paint_information.h>

/** Calculates effect of pressure on aspect ratio of brush tip */
class PAINTOP_EXPORT KisPressureRatioOption : public KisCurveOption
{
public:
    KisPressureRatioOption();
    double apply(const KisPaintInformation & info) const;
};
