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

#include "kis_chalk_paintop.h"
#include "kis_chalk_paintop_settings.h"

#include <cmath>
#include <QRect>

#include <KoColor.h>
#include <KoColorSpace.h>

#include <kis_image.h>
#include <kis_debug.h>

#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_types.h>
#include <brushengine/kis_paintop.h>
#include <brushengine/kis_paint_information.h>

#include <kis_pressure_opacity_option.h>
#include <kis_lod_transform.h>


KisChalkPaintOp::KisChalkPaintOp(const KisPaintOpSettingsSP settings, KisPainter * painter, KisNodeSP node, KisImageSP image)
    : KisPaintOp(painter)
{
    Q_UNUSED(image);
    Q_UNUSED(node);
    m_opacityOption.readOptionSetting(settings);
    m_opacityOption.resetAllSensors();

    m_properties.readOptionSetting(settings);

    KoColorTransformation* transfo = 0;
    if (m_properties.inkDepletion && m_properties.useSaturation) {
        transfo = painter->device()->compositionSourceColorSpace()->createColorTransformation("hsv_adjustment", QHash<QString, QVariant>());
    }
    m_chalkBrush = new ChalkBrush(&m_properties, transfo);
}

KisChalkPaintOp::~KisChalkPaintOp()
{
    delete m_chalkBrush;
}

KisSpacingInformation KisChalkPaintOp::paintAt(const KisPaintInformation& info)
{
    if (!painter()) return KisSpacingInformation(1.0);

    if (!m_dab) {
        m_dab = source()->createCompositionSourceDevice();
    }
    else {
        m_dab->clear();
    }

    qreal x1, y1;

    x1 = info.pos().x();
    y1 = info.pos().y();

    const qreal additionalScale = KisLodTransform::lodToScale(painter()->device());

    quint8 origOpacity = m_opacityOption.apply(painter(), info);
    m_chalkBrush->paint(m_dab, x1, y1, painter()->paintColor(), additionalScale);

    QRect rc = m_dab->extent();

    painter()->bitBlt(rc.x(), rc.y(), m_dab, rc.x(), rc.y(), rc.width(), rc.height());
    painter()->renderMirrorMask(rc, m_dab);
    painter()->setOpacity(origOpacity);
    return KisSpacingInformation(1.0);
}
