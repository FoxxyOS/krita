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

#ifndef _KIS_RECT_MASK_GENERATOR_H_
#define _KIS_RECT_MASK_GENERATOR_H_

#include <QScopedPointer>
#include "kritaimage_export.h"


#include "kis_mask_generator.h"

/**
 * Represent, serialize and deserialize a rectangular 8-bit mask.
 */
class KRITAIMAGE_EXPORT KisRectangleMaskGenerator : public KisMaskGenerator
{

public:

    KisRectangleMaskGenerator(qreal radius, qreal ratio, qreal fh, qreal fv, int spikes, bool antialiasEdges);
    KisRectangleMaskGenerator(const KisRectangleMaskGenerator &rhs);
    virtual ~KisRectangleMaskGenerator();

    KisMaskGenerator* clone() const;

    virtual bool shouldSupersample() const;
    virtual quint8 valueAt(qreal x, qreal y) const;
    void setScale(qreal scaleX, qreal scaleY);
    void setSoftness(qreal softness);

private:
    struct Private;
    const QScopedPointer<Private> d;
};

#endif
