/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#ifndef _KIS_CURVE_CIRCLE_MASK_GENERATOR_H_
#define _KIS_CURVE_CIRCLE_MASK_GENERATOR_H_

#include <QScopedPointer>
#include "kritaimage_export.h"

#include <QList>
#include <QVector>

class KisCubicCurve;
class QDomElement;
class QDomDocument;

class QPointF;


/**
 * This mask generator use softness/hardness defined by user curve
 * It used to be soft brush paintop. 
 */
class KRITAIMAGE_EXPORT KisCurveCircleMaskGenerator : public KisMaskGenerator
{

public:

    KisCurveCircleMaskGenerator(qreal radius, qreal ratio, qreal fh, qreal fv, int spikes,const KisCubicCurve& curve, bool antialiasEdges);
    KisCurveCircleMaskGenerator(const KisCurveCircleMaskGenerator &rhs);
    virtual ~KisCurveCircleMaskGenerator();
    KisMaskGenerator* clone() const;

    virtual quint8 valueAt(qreal x, qreal y) const;

    void setScale(qreal scaleX, qreal scaleY);

    bool shouldSupersample() const;

    virtual void toXML(QDomDocument& , QDomElement&) const;
    virtual void setSoftness(qreal softness);

    static void transformCurveForSoftness(qreal softness,const QList<QPointF> &points, int curveResolution, QVector<qreal> &result);

private:

    qreal norme(qreal a, qreal b) const {
        return a*a + b*b;
    }

private:
    struct Private;
    const QScopedPointer<Private> d;
};

#endif
