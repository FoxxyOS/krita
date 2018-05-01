/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KO_STRATEGY_COLORSPACE_RGB_H_
#define KO_STRATEGY_COLORSPACE_RGB_H_

#include <klocalizedstring.h>
#include <LcmsColorSpace.h>
#include "KoColorModelStandardIds.h"

struct KoBgrU8Traits;

class RgbU8ColorSpace : public LcmsColorSpace<KoBgrU8Traits>
{

public:

    RgbU8ColorSpace(const QString &name, KoColorProfile *p);

    virtual bool willDegrade(ColorSpaceIndependence) const
    {
        return false;
    }

    virtual KoColorTransformation *createInvertTransformation() const;

    virtual KoID colorModelId() const
    {
        return RGBAColorModelID;
    }

    virtual KoID colorDepthId() const
    {
        return Integer8BitsColorDepthID;
    }

    virtual KoColorSpace *clone() const;

    virtual void colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const;

    virtual void colorFromXML(quint8 *pixel, const QDomElement &elt) const;

    virtual quint8 intensity8(const quint8 * src) const;
    
    virtual void toHSY(const QVector<double> &channelValues, qreal *hue, qreal *sat, qreal *luma) const;
    virtual QVector <double> fromHSY(qreal *hue, qreal *sat, qreal *luma) const;
    virtual void toYUV(const QVector<double> &channelValues, qreal *y, qreal *u, qreal *v) const;
    virtual QVector <double> fromYUV(qreal *y, qreal *u, qreal *v) const;

    static QString colorSpaceId()
    {
        return QString("RGBA");
    }
};

class RgbU8ColorSpaceFactory : public LcmsColorSpaceFactory
{

public:

    RgbU8ColorSpaceFactory() : LcmsColorSpaceFactory(TYPE_BGRA_8, cmsSigRgbData) {}

    virtual bool userVisible() const
    {
        return true;
    }

    virtual QString id() const
    {
        return RgbU8ColorSpace::colorSpaceId();
    }

    virtual QString name() const
    {
        return QString("%1 (%2)").arg(RGBAColorModelID.name()).arg(Integer8BitsColorDepthID.name());
    }

    virtual KoID colorModelId() const
    {
        return RGBAColorModelID;
    }

    virtual KoID colorDepthId() const
    {
        return Integer8BitsColorDepthID;
    }

    virtual int referenceDepth() const
    {
        return 8;
    }

    virtual KoColorSpace *createColorSpace(const KoColorProfile *p) const
    {
        return new RgbU8ColorSpace(name(), p->clone());
    }

    virtual QString defaultProfile() const
    {
        return "sRGB-elle-V2-srgbtrc.icc";
    }
};

#endif // KO_STRATEGY_COLORSPACE_RGB_H_
