/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_layer_projection_plane.h"

#include <QBitArray>
#include <KoColorSpace.h>
#include <KoChannelInfo.h>
#include <KoCompositeOpRegistry.h>
#include "kis_painter.h"
#include "kis_projection_leaf.h"


struct KisLayerProjectionPlane::Private
{
    KisLayer *layer;
};


KisLayerProjectionPlane::KisLayerProjectionPlane(KisLayer *layer)
    : m_d(new Private)
{
    m_d->layer = layer;
}

KisLayerProjectionPlane::~KisLayerProjectionPlane()
{
}

QRect KisLayerProjectionPlane::recalculate(const QRect& rect, KisNodeSP filthyNode)
{
    return m_d->layer->updateProjection(rect, filthyNode);
}

void KisLayerProjectionPlane::apply(KisPainter *painter, const QRect &rect)
{
    KisPaintDeviceSP device = m_d->layer->projection();
    if (!device) return;

    QRect needRect = rect;

    if (m_d->layer->compositeOpId() != COMPOSITE_COPY) {
        needRect &= device->extent();
    }

    if(needRect.isEmpty()) return;

    QBitArray channelFlags = m_d->layer->projectionLeaf()->channelFlags();


    // if the color spaces don't match we will have a problem with the channel flags
    // because the channel flags from the source layer doesn't match with the colorspace of the projection device
    // this leads to the situation that the wrong channels will be enabled/disabled
    const KoColorSpace* srcCS = device->colorSpace();
    const KoColorSpace* dstCS = painter->device()->colorSpace();

    if (!channelFlags.isEmpty() && srcCS != dstCS) {
        bool alphaFlagIsSet        = (srcCS->channelFlags(false,true) & channelFlags) == srcCS->channelFlags(false,true);
        bool allColorFlagsAreSet   = (srcCS->channelFlags(true,false) & channelFlags) == srcCS->channelFlags(true,false);
        bool allColorFlagsAreUnset = (srcCS->channelFlags(true,false) & channelFlags).count(true) == 0;

        if(allColorFlagsAreSet) {
            channelFlags = dstCS->channelFlags(true, alphaFlagIsSet);
        } else if(allColorFlagsAreUnset) {
            channelFlags = dstCS->channelFlags(false, alphaFlagIsSet);
        } else {
            //TODO: convert the cannel flags properly
            //      for now just the alpha channel bit is copied and the other channels are left alone
            for (quint32 i=0; i < dstCS->channelCount(); ++i) {
                if (dstCS->channels()[i]->channelType() == KoChannelInfo::ALPHA) {
                    channelFlags.setBit(i, alphaFlagIsSet);
                    break;
                }
            }
        }
    }

    painter->setChannelFlags(channelFlags);
    painter->setCompositeOp(m_d->layer->compositeOpId());
    painter->setOpacity(m_d->layer->projectionLeaf()->opacity());
    painter->bitBlt(needRect.topLeft(), device, needRect);
}

KisPaintDeviceList KisLayerProjectionPlane::getLodCapableDevices() const
{
    return KisPaintDeviceList() << m_d->layer->projection();
}

QRect KisLayerProjectionPlane::needRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->needRect(rect, pos);
}

QRect KisLayerProjectionPlane::changeRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->changeRect(rect, pos);
}

QRect KisLayerProjectionPlane::accessRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->accessRect(rect, pos);
}

