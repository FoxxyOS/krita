/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004-2008 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_DUPLICATEOP_H_
#define KIS_DUPLICATEOP_H_

#include "kis_brush_based_paintop.h"

#include <klocalizedstring.h>

#include <kis_types.h>
#include <brushengine/kis_paintop_factory.h>
#include <brushengine/kis_paintop_settings.h>
#include <kis_pressure_size_option.h>

#include "kis_duplicateop_settings.h"

class KisPaintInformation;


class QPointF;
class KisPainter;


class KisDuplicateOp : public KisBrushBasedPaintOp
{

public:

    KisDuplicateOp(const KisPaintOpSettingsSP settings, KisPainter *painter, KisNodeSP node, KisImageSP image);
    ~KisDuplicateOp();

    KisSpacingInformation paintAt(const KisPaintInformation& info);

private:

    qreal minimizeEnergy(const qreal* m, qreal* sol, int w, int h);

private:

    KisImageSP m_image;
    KisNodeSP m_node;

    KisDuplicateOpSettingsSP m_settings;
    KisPaintDeviceSP m_srcdev;
    KisPaintDeviceSP m_target;
    QPointF m_duplicateStart;
    bool m_duplicateStartIsSet;
    KisPressureSizeOption m_sizeOption;
    bool m_healing;
    bool m_perspectiveCorrection;
    bool m_moveSourcePoint;
    bool m_cloneFromProjection;
};

#endif // KIS_DUPLICATEOP_H_
