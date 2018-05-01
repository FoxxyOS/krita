/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (c) 2011 Geoffry Song <goffrie@gmail.com>
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

#include <compositeops/KoVcMultiArchBuildSupport.h> //MSVC requires that Vc come first
#include <cmath>
#include <algorithm>

#include <QDomDocument>
#include <QVector>
#include <QPointF>

#include <KoColorSpaceConstants.h>

#include "kis_fast_math.h"

#include "kis_base_mask_generator.h"
#include "kis_gauss_rect_mask_generator.h"
#include "kis_antialiasing_fade_maker.h"

#define M_SQRT_2 1.41421356237309504880

#ifdef Q_OS_WIN
// on windows we get our erf() from boost
#include <boost/math/special_functions/erf.hpp>
#define erf(x) boost::math::erf(x)
#endif

struct Q_DECL_HIDDEN KisGaussRectangleMaskGenerator::Private
{
    Private(bool enableAntialiasing)
        : fadeMaker(*this, enableAntialiasing)
    {
    }

    Private(const Private &rhs)
        : xfade(rhs.xfade),
        yfade(rhs.yfade),
        halfWidth(rhs.halfWidth),
        halfHeight(rhs.halfHeight),
        alphafactor(rhs.alphafactor),
        fadeMaker(rhs.fadeMaker, *this)
    {
    }

    qreal xfade, yfade;
    qreal halfWidth, halfHeight;
    qreal alphafactor;

    KisAntialiasingFadeMaker2D <Private> fadeMaker;

    inline quint8 value(qreal x, qreal y) const;
};

KisGaussRectangleMaskGenerator::KisGaussRectangleMaskGenerator(qreal diameter, qreal ratio, qreal fh, qreal fv, int spikes, bool antialiasEdges)
    : KisMaskGenerator(diameter, ratio, fh, fv, spikes, antialiasEdges, RECTANGLE, GaussId), d(new Private(antialiasEdges))
{
    setScale(1.0, 1.0);
}

KisGaussRectangleMaskGenerator::KisGaussRectangleMaskGenerator(const KisGaussRectangleMaskGenerator &rhs)
    : KisMaskGenerator(rhs),
      d(new Private(*rhs.d))
{
}

KisMaskGenerator* KisGaussRectangleMaskGenerator::clone() const
{
    return new KisGaussRectangleMaskGenerator(*this);
}

void KisGaussRectangleMaskGenerator::setScale(qreal scaleX, qreal scaleY)
{
    KisMaskGenerator::setScale(scaleX, scaleY);

    qreal width = effectiveSrcWidth();
    qreal height = effectiveSrcHeight();

    qreal xfade = (1.0 - horizontalFade()/2.0) * width * 0.1;
    qreal yfade = (1.0 - verticalFade()/2.0) * height * 0.1;
    d->xfade = 1.0 / (M_SQRT_2 * xfade);
    d->yfade = 1.0 / (M_SQRT_2 * yfade);
    d->halfWidth = width * 0.5 - 2.5 * xfade;
    d->halfHeight = height * 0.5 - 2.5 * yfade;
    d->alphafactor = 255.0 / (4.0 * erf(d->halfWidth * d->xfade) * erf(d->halfHeight * d->yfade));

    d->fadeMaker.setLimits(0.5 * width, 0.5 * height);
}

KisGaussRectangleMaskGenerator::~KisGaussRectangleMaskGenerator()
{
}

inline quint8 KisGaussRectangleMaskGenerator::Private::value(qreal xr, qreal yr) const
{
    return (quint8) 255 - (quint8) (alphafactor * (erf((halfWidth + xr) * xfade) + erf((halfWidth - xr) * xfade))
                                    * (erf((halfHeight + yr) * yfade) + erf((halfHeight - yr) * yfade)));
}

quint8 KisGaussRectangleMaskGenerator::valueAt(qreal x, qreal y) const
{
    if (isEmpty()) return 255;
    qreal xr = x;
    qreal yr = qAbs(y);
    fixRotation(xr, yr);

    quint8 value;
    if (d->fadeMaker.needFade(xr, yr, &value)) {
        return value;
    }

    return d->value(xr, yr);
}
