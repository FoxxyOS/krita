/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004,2010 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_PAINTOP_H_
#define KIS_PAINTOP_H_

#include <kis_distance_information.h>
#include "kis_shared.h"
#include "kis_types.h"

#include <kritaimage_export.h>

class QPointF;
class KoColorSpace;

class KisPainter;
class KisPaintInformation;

/**
 * KisPaintOp are use by tools to draw on a paint device. A paintop takes settings
 * and input information, like pressure, tilt or motion and uses that to draw pixels
 */
class KRITAIMAGE_EXPORT KisPaintOp : public KisShared
{
    struct Private;
public:

    KisPaintOp(KisPainter * painter);
    virtual ~KisPaintOp();

    /**
     * Paint at the subpixel point pos using the specified paint
     * information..
     *
     * The distance between two calls of the paintAt is always
     * specified by spacing, which is automatically saved into the
     * current distance information object
     */
    void paintAt(const KisPaintInformation& info, KisDistanceInformation *currentDistance);

    /**
     * Draw a line between pos1 and pos2 using the currently set brush and color.
     * If savedDist is less than zero, the brush is painted at pos1 before being
     * painted along the line using the spacing setting.
     *
     * @return the drag distance, that is the remains of the distance
     * between p1 and p2 not covered because the currenlty set brush
     * has a spacing greater than that distance.
     */
    virtual void paintLine(const KisPaintInformation &pi1,
                           const KisPaintInformation &pi2,
                           KisDistanceInformation *currentDistance);

    /**
     * Draw a Bezier curve between pos1 and pos2 using control points 1 and 2.
     * If savedDist is less than zero, the brush is painted at pos1 before being
     * painted along the curve using the spacing setting.
     * @return the drag distance, that is the remains of the distance between p1 and p2 not covered
     * because the currenlty set brush has a spacing greater than that distance.
     */
    virtual void paintBezierCurve(const KisPaintInformation &pi1,
                                  const QPointF &control1,
                                  const QPointF &control2,
                                  const KisPaintInformation &pi2,
                                  KisDistanceInformation *currentDistance);


    /**
    * Whether this paintop can paint. Can be false in case that some setting isn't read correctly.
    * @return if paintop is ready for painting, default is true
    */
    virtual bool canPaint() const {
        return true;
    }

    /**
     * Split the coordinate into whole + fraction, where fraction is always >= 0.
     */
    static void splitCoordinate(qreal coordinate, qint32 *whole, qreal *fraction);

protected:
    friend class KisPaintInformation;
    /**
     * The implementation of painting of a dab
     */
    virtual KisSpacingInformation paintAt(const KisPaintInformation& info) = 0;

    KisFixedPaintDeviceSP cachedDab();
    KisFixedPaintDeviceSP cachedDab(const KoColorSpace *cs);

    /**
     * Return the painter this paintop is owned by
     */
    KisPainter* painter() const;

    /**
     * Return the paintdevice the painter this paintop is owned by
     */
    KisPaintDeviceSP source() const;

private:
    friend class KisPressureRotationOption;
    void setFanCornersInfo(bool fanCornersEnabled, qreal fanCornersStep);

private:
    Private* const d;
};


#endif // KIS_PAINTOP_H_
