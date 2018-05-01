/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KIS_STRATEGY_COLORSPACE_CMYK_U16_H_
#define KIS_STRATEGY_COLORSPACE_CMYK_U16_H_

#include <LcmsColorSpace.h>
#include <KoCmykColorSpaceTraits.h>

#include "KoColorModelStandardIds.h"

typedef KoCmykTraits<quint16> CmykU16Traits;

#define TYPE_CMYKA_16           (COLORSPACE_SH(PT_CMYK)|EXTRA_SH(1)|CHANNELS_SH(4)|BYTES_SH(2))

class CmykU16ColorSpace : public LcmsColorSpace<CmykU16Traits>
{
public:
    CmykU16ColorSpace(const QString &name, KoColorProfile *p);

    virtual bool willDegrade(ColorSpaceIndependence independence) const;

    virtual KoID colorModelId() const
    {
        return CMYKAColorModelID;
    }

    virtual KoID colorDepthId() const
    {
        return Integer16BitsColorDepthID;
    }

    virtual KoColorSpace *clone() const;

    virtual void colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const;

    virtual void colorFromXML(quint8* pixel, const QDomElement& elt) const;
    virtual void toHSY(const QVector<double> &channelValues, qreal *hue, qreal *sat, qreal *luma) const;
    virtual QVector <double> fromHSY(qreal *hue, qreal *sat, qreal *luma) const;
    virtual void toYUV(const QVector<double> &channelValues, qreal *y, qreal *u, qreal *v) const;
    virtual QVector <double> fromYUV(qreal *y, qreal *u, qreal *v) const;
    
    static QString colorSpaceId()
    {
        return "CMYKAU16";
    }

};

class CmykU16ColorSpaceFactory : public LcmsColorSpaceFactory
{
public:

    CmykU16ColorSpaceFactory()
        : LcmsColorSpaceFactory(TYPE_CMYKA_16, cmsSigCmykData)
    {
    }

    virtual bool userVisible() const
    {
        return true;
    }

    virtual QString id() const
    {
        return CmykU16ColorSpace::colorSpaceId();
    }

    virtual QString name() const
    {
        return QString("%1 (%2)").arg(CMYKAColorModelID.name()).arg(Integer16BitsColorDepthID.name());
    }

    virtual KoID colorModelId() const
    {
        return CMYKAColorModelID;
    }

    virtual KoID colorDepthId() const
    {
        return Integer16BitsColorDepthID;
    }

    virtual int referenceDepth() const
    {
        return 16;
    }

    virtual KoColorSpace *createColorSpace(const KoColorProfile *p) const
    {
        return new CmykU16ColorSpace(name(), p->clone());
    }

    virtual QString defaultProfile() const
    {
        return "Chemical proof";
    }
};

#endif
