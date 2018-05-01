/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_GREEN_COORDINATES_MATH_H
#define __KIS_GREEN_COORDINATES_MATH_H

#include <QScopedPointer>
#include <QVector>
#include <QPointF>

#include "kritaimage_export.h"

class KRITAIMAGE_EXPORT KisGreenCoordinatesMath
{
public:
    KisGreenCoordinatesMath();
    ~KisGreenCoordinatesMath();

    /**
     * Prepare the transformation framework by computing internal
     * coordinates of the points in cage.
     *
     * Please note that the points in \p points will later be accessed
     * with indexes only.
     */
    void precalculateGreenCoordinates(const QVector<QPointF> &originalCage, const QVector<QPointF> &points);

    /**
     * Precalculate coefficients of the destination cage. Should be
     * called once for every cage change
     */
    void generateTransformedCageNormals(const QVector<QPointF> &transformedCage);

    /**
     * Transform one point according to its index
     */
    QPointF transformedPoint(int pointIndex, const QVector<QPointF> &transformedCage);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_GREEN_COORDINATES_MATH_H */
