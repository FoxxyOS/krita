/*
 *  Copyright (c) 2008-2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include "kis_hairy_paintop.h"
#include "kis_hairy_paintop_settings.h"

#include <cmath>
#include <QRect>

#include <kis_image.h>
#include <kis_debug.h>

#include "kis_paint_device.h"
#include "kis_painter.h"
#include <kis_vec.h>

#include <kis_hairy_ink_option.h>
#include <kis_hairy_bristle_option.h>
#include <kis_brush_option.h>
#include <kis_brush_based_paintop_settings.h>
#include <kis_fixed_paint_device.h>
#include <kis_lod_transform.h>


#include "kis_brush.h"

KisHairyPaintOp::KisHairyPaintOp(const KisPaintOpSettingsSP settings, KisPainter * painter, KisNodeSP node, KisImageSP image)
    : KisPaintOp(painter)
{
    Q_UNUSED(image)
    Q_ASSERT(settings);

    m_dev = node ? node->paintDevice() : 0;

    KisBrushOption brushOption;
    brushOption.readOptionSettingForceCopy(settings);
    KisBrushSP brush = brushOption.brush();
    KisFixedPaintDeviceSP dab = cachedDab(painter->device()->compositionSourceColorSpace());
    if (brush->brushType() == IMAGE || brush->brushType() == PIPE_IMAGE) {
        dab = brush->paintDevice(source()->colorSpace(), KisDabShape(), KisPaintInformation());
    }
    else {
        brush->mask(dab, painter->paintColor(), KisDabShape(), KisPaintInformation());
    }

    m_brush.fromDabWithDensity(dab, settings->getDouble(HAIRY_BRISTLE_DENSITY) * 0.01);
    m_brush.setInkColor(painter->paintColor());

    loadSettings(static_cast<const KisBrushBasedPaintOpSettings*>(settings.data()));
    m_brush.setProperties(&m_properties);

    m_rotationOption.readOptionSetting(settings);
    m_opacityOption.readOptionSetting(settings);
    m_sizeOption.readOptionSetting(settings);
    m_rotationOption.resetAllSensors();
    m_opacityOption.resetAllSensors();
    m_sizeOption.resetAllSensors();
}

void KisHairyPaintOp::loadSettings(const KisBrushBasedPaintOpSettings *settings)
{
    m_properties.inkAmount = settings->getInt(HAIRY_INK_AMOUNT);
    //TODO: wait for the transfer function with variable size

    m_properties.inkDepletionCurve = settings->getCubicCurve(HAIRY_INK_DEPLETION_CURVE).floatTransfer(m_properties.inkAmount);

    m_properties.inkDepletionEnabled = settings->getBool(HAIRY_INK_DEPLETION_ENABLED);
    m_properties.useSaturation = settings->getBool(HAIRY_INK_USE_SATURATION);
    m_properties.useOpacity = settings->getBool(HAIRY_INK_USE_OPACITY);
    m_properties.useWeights = settings->getBool(HAIRY_INK_USE_WEIGHTS);

    m_properties.pressureWeight = settings->getDouble(HAIRY_INK_PRESSURE_WEIGHT) / 100.0;
    m_properties.bristleLengthWeight = settings->getDouble(HAIRY_INK_BRISTLE_LENGTH_WEIGHT) / 100.0;
    m_properties.bristleInkAmountWeight = settings->getDouble(HAIRY_INK_BRISTLE_INK_AMOUNT_WEIGHT) / 100.0;
    m_properties.inkDepletionWeight = settings->getDouble(HAIRY_INK_DEPLETION_WEIGHT);
    m_properties.useSoakInk = settings->getBool(HAIRY_INK_SOAK);

    m_properties.useMousePressure = settings->getBool(HAIRY_BRISTLE_USE_MOUSEPRESSURE);
    m_properties.shearFactor = settings->getDouble(HAIRY_BRISTLE_SHEAR);
    m_properties.randomFactor = settings->getDouble(HAIRY_BRISTLE_RANDOM);
    m_properties.scaleFactor = settings->getDouble(HAIRY_BRISTLE_SCALE);
    m_properties.threshold = settings->getBool(HAIRY_BRISTLE_THRESHOLD);
    m_properties.antialias = settings->getBool(HAIRY_BRISTLE_ANTI_ALIASING);
    m_properties.useCompositing = settings->getBool(HAIRY_BRISTLE_USE_COMPOSITING);
    m_properties.connectedPath = settings->getBool(HAIRY_BRISTLE_CONNECTED);
}


KisSpacingInformation KisHairyPaintOp::paintAt(const KisPaintInformation& info)
{
    Q_UNUSED(info);
    return KisSpacingInformation(0.5);
}


void KisHairyPaintOp::paintLine(const KisPaintInformation &pi1, const KisPaintInformation &pi2, KisDistanceInformation *currentDistance)
{
    Q_UNUSED(currentDistance);
    if (!painter()) return;

    if (!m_dab) {
        m_dab = source()->createCompositionSourceDevice();
    }
    else {
        m_dab->clear();
    }

    // Hairy Brush is capable of working with zero scale,
    // so no additional checks for 'zero'ness are needed
    qreal scale = m_sizeOption.apply(pi2);
    scale *= KisLodTransform::lodToScale(painter()->device());
    qreal rotation = m_rotationOption.apply(pi2);
    quint8 origOpacity = m_opacityOption.apply(painter(), pi2);


    m_brush.paintLine(m_dab, m_dev, pi1, pi2, scale * m_properties.scaleFactor, rotation);

    //QRect rc = m_dab->exactBounds();
    QRect rc = m_dab->extent();
    painter()->bitBlt(rc.topLeft(), m_dab, rc);
    painter()->renderMirrorMask(rc, m_dab);
    painter()->setOpacity(origOpacity);
}
