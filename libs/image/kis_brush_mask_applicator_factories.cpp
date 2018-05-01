/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_brush_mask_applicator_factories.h"

#include "kis_circle_mask_generator.h"
#include "kis_circle_mask_generator_p.h"
#include "kis_brush_mask_applicators.h"
#include "kis_brush_mask_applicator_base.h"

#define a(_s) #_s
#define b(_s) a(_s)

template<>
template<>
MaskApplicatorFactory<KisMaskGenerator, KisBrushMaskScalarApplicator>::ReturnType
MaskApplicatorFactory<KisMaskGenerator, KisBrushMaskScalarApplicator>::create<Vc::CurrentImplementation::current()>(ParamType maskGenerator)
{
    return new KisBrushMaskScalarApplicator<KisMaskGenerator,Vc::CurrentImplementation::current()>(maskGenerator);
}

template<>
template<>
MaskApplicatorFactory<KisCircleMaskGenerator, KisBrushMaskVectorApplicator>::ReturnType
MaskApplicatorFactory<KisCircleMaskGenerator, KisBrushMaskVectorApplicator>::create<Vc::CurrentImplementation::current()>(ParamType maskGenerator)
{
    return new KisBrushMaskVectorApplicator<KisCircleMaskGenerator,Vc::CurrentImplementation::current()>(maskGenerator);
}

#if defined HAVE_VC

struct KisCircleMaskGenerator::FastRowProcessor
{
    FastRowProcessor(KisCircleMaskGenerator *maskGenerator)
        : d(maskGenerator->d.data()) {}

    template<Vc::Implementation _impl>
    void process(float* buffer, int width, float y, float cosa, float sina,
                 float centerX, float centerY);

    KisCircleMaskGenerator::Private *d;
};

template<> void KisCircleMaskGenerator::
FastRowProcessor::process<Vc::CurrentImplementation::current()>(float* buffer, int width, float y, float cosa, float sina,
                                   float centerX, float centerY)
{
    const bool useSmoothing = d->copyOfAntialiasEdges;
    const bool noFading = d->noFading;

    float y_ = y - centerY;
    float sinay_ = sina * y_;
    float cosay_ = cosa * y_;

    float* bufferPointer = buffer;

    Vc::float_v currentIndices = Vc::float_v::IndexesFromZero();

    Vc::float_v increment((float)Vc::float_v::size());
    Vc::float_v vCenterX(centerX);

    Vc::float_v vCosa(cosa);
    Vc::float_v vSina(sina);
    Vc::float_v vCosaY_(cosay_);
    Vc::float_v vSinaY_(sinay_);

    Vc::float_v vXCoeff(d->xcoef);
    Vc::float_v vYCoeff(d->ycoef);

    Vc::float_v vTransformedFadeX(d->transformedFadeX);
    Vc::float_v vTransformedFadeY(d->transformedFadeY);

    Vc::float_v vOne(Vc::One);

    for (int i=0; i < width; i+= Vc::float_v::size()){

        Vc::float_v x_ = currentIndices - vCenterX;

        Vc::float_v xr = x_ * vCosa - vSinaY_;
        Vc::float_v yr = x_ * vSina + vCosaY_;

        Vc::float_v n = pow2(xr * vXCoeff) + pow2(yr * vYCoeff);
        Vc::float_m outsideMask = n > vOne;

        if (!outsideMask.isFull()) {

            if (noFading) {
                Vc::float_v vFade(Vc::Zero);
                vFade(outsideMask) = vOne;
                vFade.store(bufferPointer, Vc::Aligned);
            } else {
                if (useSmoothing) {
                    xr = Vc::abs(xr) + vOne;
                    yr = Vc::abs(yr) + vOne;
                }

                Vc::float_v vNormFade = pow2(xr * vTransformedFadeX) + pow2(yr * vTransformedFadeY);

                //255 * n * (normeFade - 1) / (normeFade - n)
                Vc::float_v vFade = n * (vNormFade - vOne) / (vNormFade - n);

                // Mask in the inner circe of the mask
                Vc::float_m mask = vNormFade < vOne;
                vFade.setZero(mask);

                // Mask out the outer circe of the mask
                vFade(outsideMask) = vOne;

                vFade.store(bufferPointer, Vc::Aligned);
            }
        } else {
            // Mask out everything outside the circle
            vOne.store(bufferPointer, Vc::Aligned);
        }

        currentIndices = currentIndices + increment;

        bufferPointer += Vc::float_v::size();
    }
}

#endif /* defined HAVE_VC */
