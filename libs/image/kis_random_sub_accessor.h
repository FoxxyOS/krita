/*
 *  This file is part of the KDE project
 *
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_RANDOM_SUB_ACCESSOR_H
#define KIS_RANDOM_SUB_ACCESSOR_H


#include "kis_random_accessor_ng.h"
#include "kis_types.h"
#include <kritaimage_export.h>
#include "kis_shared.h"

/**
 * Gives a random access to the sampled subpixels of an image. Use the
 * moveTo function to select the pixel. And then rawData to access the
 * value of a pixel.
 */
class  KRITAIMAGE_EXPORT KisRandomSubAccessor : public KisShared
{
public:
    KisRandomSubAccessor(KisPaintDeviceSP device);
    ~KisRandomSubAccessor();
    /**
     * Copy the sampled old value to destination
     */
    void sampledOldRawData(quint8* dst);

    /**
     * Copy the sampled value to destination
     */
    void sampledRawData(quint8* dst);

    inline void moveTo(double x, double y) {
        m_currentPoint.setX(x); m_currentPoint.setY(y);
    }
    inline void moveTo(const QPointF& p) {
        m_currentPoint = p;
    }
private:
    KisPaintDeviceSP m_device;
    QPointF m_currentPoint;
    KisRandomConstAccessorSP m_randomAccessor;
};

#endif
