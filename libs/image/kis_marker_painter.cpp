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

#include "kis_marker_painter.h"

#include <KoColor.h>
#include <KoColorSpace.h>

#include "kis_paint_device.h"

#include "kis_algebra_2d.h"
#include "kis_sequential_iterator.h"


struct KisMarkerPainter::Private
{
    Private(KisPaintDeviceSP _device, const KoColor &_color) : device(_device), color(_color) {}

    KisPaintDeviceSP device;
    const KoColor &color;
};

KisMarkerPainter::KisMarkerPainter(KisPaintDeviceSP device, const KoColor &color)
    : m_d(new Private(device, color))
{
}

KisMarkerPainter::~KisMarkerPainter()
{
}

void KisMarkerPainter::fillHalfBrushDiff(const QPointF &p1, const QPointF &p2, const QPointF &p3,
                                         const QPointF &center, qreal radius)
{
    KoColor currentColor(m_d->color);

    const int pixelSize = m_d->device->pixelSize();
    const KoColorSpace *cs = m_d->device->colorSpace();

    const qreal fadedRadius = radius + 1;
    QRectF boundRect(center.x() - fadedRadius, center.y() - fadedRadius,
                     2 * fadedRadius, 2 * fadedRadius);

    KisAlgebra2D::RightHalfPlane plane1(p1, p2);
    KisAlgebra2D::RightHalfPlane plane2(p2, p3);
    KisAlgebra2D::OuterCircle outer(center, radius);

    boundRect = KisAlgebra2D::cutOffRect(boundRect, plane1);
    boundRect = KisAlgebra2D::cutOffRect(boundRect, plane2);

    KisSequentialIterator it(m_d->device, boundRect.toAlignedRect());
    do {
        QPoint pt(it.x(), it.y());

        qreal value1 = plane1.value(pt);
        if (value1 < 0) continue;

        qreal value2 = plane2.value(pt);
        if (value2 < 0) continue;

        qreal value3 = outer.fadeSq(pt);
        if (value3 > 1.0) continue;

        // qreal fadePos =
        //     value1 < 0 || value2 < 0 ?
        //     qMax(-value1, -value2) : value3;
        qreal fadePos = value3;

        const quint8 srcAlpha = fadePos > 0 ? quint8((1.0 - fadePos) * 255.0) : 255;
        const quint8 dstAlpha = cs->opacityU8(it.rawData());

        if (srcAlpha > dstAlpha) {
            currentColor.setOpacity(srcAlpha);
            memcpy(it.rawData(), currentColor.data(), pixelSize);
        }
    } while(it.nextPixel());
}

void KisMarkerPainter::fillFullCircle(const QPointF &center, qreal radius)
{
    KoColor currentColor(m_d->color);

    const int pixelSize = m_d->device->pixelSize();
    const KoColorSpace *cs = m_d->device->colorSpace();

    const qreal fadedRadius = radius + 1;
    QRectF boundRect(center.x() - fadedRadius, center.y() - fadedRadius,
                     2 * fadedRadius, 2 * fadedRadius);

    KisAlgebra2D::OuterCircle outer(center, radius);

    KisSequentialIterator it(m_d->device, boundRect.toAlignedRect());
    do {
        QPoint pt(it.x(), it.y());

        qreal value3 = outer.fadeSq(pt);
        if (value3 > 1.0) continue;

        const quint8 srcAlpha = value3 > 0 ? quint8((1.0 - value3) * 255.0) : 255;
        const quint8 dstAlpha = cs->opacityU8(it.rawData());

        if (srcAlpha > dstAlpha) {
            currentColor.setOpacity(srcAlpha);
            memcpy(it.rawData(), currentColor.data(), pixelSize);
        }
    } while(it.nextPixel());
}

void KisMarkerPainter::fillCirclesDiff(const QPointF &c1, qreal r1,
                                       const QPointF &c2, qreal r2)
{
    QVector<QPointF> n = KisAlgebra2D::intersectTwoCircles(c1, r1, c2, r2);

    if (n.size() < 2) {
        fillFullCircle(c2, r2);
    } else {
        const QPointF diff = c2 - c1;
        const qreal normDiffInv = 1.0 / KisAlgebra2D::norm(diff);
        const QPointF direction = diff * normDiffInv;
        const QPointF p = c1 + r1 * direction;
        const QPointF q = c2 + r2 * direction;

        fillHalfBrushDiff(n[0], p, q, c2, r2);
        fillHalfBrushDiff(q, p, n[1], c2, r2);
    }
}

