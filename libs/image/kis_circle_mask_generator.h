/*
 *  Copyright (c) 2008-2009 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_CIRCLE_MASK_GENERATOR_H_
#define _KIS_CIRCLE_MASK_GENERATOR_H_

#include "kritaimage_export.h"

#include "kis_mask_generator.h"
#include <QScopedPointer>


/**
 * Create, serialize and deserialize an elliptical 8-bit mask.
 */
class KRITAIMAGE_EXPORT KisCircleMaskGenerator : public KisMaskGenerator
{
public:
    struct FastRowProcessor;
public:
    KisCircleMaskGenerator(qreal radius, qreal ratio, qreal fh, qreal fv, int spikes, bool antialiasEdges);
    KisCircleMaskGenerator(const KisCircleMaskGenerator &rhs);
    virtual ~KisCircleMaskGenerator();
    KisMaskGenerator* clone() const;

    virtual quint8 valueAt(qreal x, qreal y) const;

    virtual bool shouldSupersample() const;

    virtual bool shouldVectorize() const;

    KisBrushMaskApplicatorBase* applicator();

    virtual void setSoftness(qreal softness);
    virtual void setScale(qreal scaleX, qreal scaleY);

private:

    qreal norme(qreal a, qreal b) const {
        return a*a + b * b;
    }

private:
    struct Private;
    const QScopedPointer<Private> d;
};

#endif
