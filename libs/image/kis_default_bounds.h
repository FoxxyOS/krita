/*
 *  Copyright (c) 2010 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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
#ifndef KIS_DEFAULT_BOUNDS_H
#define KIS_DEFAULT_BOUNDS_H

#include <QRect>
#include "kis_types.h"
#include "kis_default_bounds_base.h"

class KisDefaultBounds;
class KisSelectionDefaultBounds;
class KisSelectionEmptyBounds;
typedef KisSharedPtr<KisDefaultBounds> KisDefaultBoundsSP;
typedef KisSharedPtr<KisSelectionDefaultBounds> KisSelectionDefaultBoundsSP;
typedef KisSharedPtr<KisSelectionEmptyBounds> KisSelectionEmptyBoundsSP;

class KRITAIMAGE_EXPORT KisDefaultBounds :  public KisDefaultBoundsBase
{
public:
    KisDefaultBounds(KisImageWSP image = 0);
    virtual ~KisDefaultBounds();

    QRect bounds() const;
    bool wrapAroundMode() const;
    int currentLevelOfDetail() const;
    int currentTime() const;
    bool externalFrameActive() const;

protected:
    friend class KisPaintDeviceTest;
    static const QRect infiniteRect;

private:
    Q_DISABLE_COPY(KisDefaultBounds)

    struct Private;
    Private * const m_d;
};

class KRITAIMAGE_EXPORT KisSelectionDefaultBounds : public KisDefaultBounds
{
public:
    KisSelectionDefaultBounds(KisPaintDeviceSP parentPaintDevice = 0, KisImageWSP image = 0);
    ~KisSelectionDefaultBounds();

    QRect bounds() const;

private:
    Q_DISABLE_COPY(KisSelectionDefaultBounds)

    struct Private;
    Private * const m_d;
};

class KRITAIMAGE_EXPORT KisSelectionEmptyBounds : public KisDefaultBounds
{
public:
    KisSelectionEmptyBounds(KisImageWSP image);
    virtual ~KisSelectionEmptyBounds();
    QRect bounds() const;
};

#endif // KIS_DEFAULT_BOUNDS_H
