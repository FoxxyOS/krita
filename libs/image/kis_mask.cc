/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
 *            (c) 2009 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_mask.h"


#include <kis_debug.h>

// to prevent incomplete class types on "delete selection->flatten();"
#include <kundo2command.h>

#include <QScopedPointer>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoCompositeOpRegistry.h>

#include "kis_paint_device.h"
#include "kis_selection.h"
#include "kis_pixel_selection.h"
#include "kis_painter.h"

#include "kis_image.h"
#include "kis_layer.h"

#include "kis_cached_paint_device.h"
#include "kis_mask_projection_plane.h"

#include "kis_raster_keyframe_channel.h"

struct Q_DECL_HIDDEN KisMask::Private {
    Private(KisMask *_q)
        : q(_q),
          projectionPlane(new KisMaskProjectionPlane(q))
    {
    }

    mutable KisSelectionSP selection;
    KisCachedPaintDevice paintDeviceCache;
    KisMask *q;

    /**
     * Due to the design of the Kra format the X,Y offset of the paint
     * device belongs to the node, but not to the device itself.  So
     * the offset is set when the node is created, but not when the
     * selection is initialized. This causes the X,Y values to be
     * lost, since the selection doen not exist at the moment. That is
     * why we save it separately.
     */
    QScopedPointer<QPoint> deferredSelectionOffset;

    KisAbstractProjectionPlaneSP projectionPlane;
    KisCachedSelection cachedSelection;

    void initSelectionImpl(KisSelectionSP copyFrom, KisLayerSP parentLayer, KisPaintDeviceSP copyFromDevice);
};

KisMask::KisMask(const QString & name)
        : KisNode()
        , m_d(new Private(this))
{
    setName(name);
}

KisMask::KisMask(const KisMask& rhs)
        : KisNode(rhs)
        , KisIndirectPaintingSupport()
        , m_d(new Private(this))
{
    setName(rhs.name());

    if (rhs.m_d->selection) {
        m_d->selection = new KisSelection(*rhs.m_d->selection.data());
        m_d->selection->setParentNode(this);
    }
}

KisMask::~KisMask()
{
    delete m_d;
}

void KisMask::setImage(KisImageWSP image)
{
    KisPaintDeviceSP parentPaintDevice = parent() ? parent()->original() : 0;
    KisDefaultBoundsBaseSP defaultBounds = new KisSelectionDefaultBounds(parentPaintDevice, image);
    if (m_d->selection) {
        m_d->selection->setDefaultBounds(defaultBounds);
    }
}

bool KisMask::allowAsChild(KisNodeSP node) const
{
    Q_UNUSED(node);
    return false;
}

const KoColorSpace * KisMask::colorSpace() const
{
    KisNodeSP parentNode = parent();
    return parentNode ? parentNode->colorSpace() : 0;
}

const KoCompositeOp * KisMask::compositeOp() const
{
    /**
     * FIXME: This function duplicates the same function from
     * KisLayer. We can't move it to KisBaseNode as it doesn't
     * know anything about parent() method of KisNode
     * Please think it over...
     */

    const KoColorSpace *colorSpace = this->colorSpace();
    if (!colorSpace) return 0;

    const KoCompositeOp* op = colorSpace->compositeOp(compositeOpId());
    return op ? op : colorSpace->compositeOp(COMPOSITE_OVER);
}

void KisMask::initSelection(KisSelectionSP copyFrom, KisLayerSP parentLayer)
{
    m_d->initSelectionImpl(copyFrom, parentLayer, 0);
}

void KisMask::initSelection(KisPaintDeviceSP copyFromDevice, KisLayerSP parentLayer)
{
    m_d->initSelectionImpl(0, parentLayer, copyFromDevice);
}

void KisMask::initSelection(KisLayerSP parentLayer)
{
    m_d->initSelectionImpl(0, parentLayer, 0);
}

void KisMask::Private::initSelectionImpl(KisSelectionSP copyFrom, KisLayerSP parentLayer, KisPaintDeviceSP copyFromDevice)
{
    Q_ASSERT(parentLayer);

    KisPaintDeviceSP parentPaintDevice = parentLayer->original();

    if (copyFrom) {
        /**
         * We can't use setSelection as we may not have parent() yet
         */
        selection = new KisSelection(*copyFrom);
        selection->setDefaultBounds(new KisSelectionDefaultBounds(parentPaintDevice, parentLayer->image()));
        if (copyFrom->hasShapeSelection()) {
            delete selection->flatten();
        }
    } else if (copyFromDevice) {
        selection = new KisSelection(new KisSelectionDefaultBounds(parentPaintDevice, parentLayer->image()));

        QRect rc(copyFromDevice->extent());
        KisPainter::copyAreaOptimized(rc.topLeft(), copyFromDevice, selection->pixelSelection(), rc);
        selection->pixelSelection()->invalidateOutlineCache();

    } else {
        selection = new KisSelection(new KisSelectionDefaultBounds(parentPaintDevice, parentLayer->image()));
        selection->pixelSelection()->setDefaultPixel(KoColor(Qt::white, selection->pixelSelection()->colorSpace()));

        if (deferredSelectionOffset) {
            selection->setX(deferredSelectionOffset->x());
            selection->setY(deferredSelectionOffset->y());
            deferredSelectionOffset.reset();
        }
    }
    selection->setParentNode(q);
    selection->updateProjection();
}

KisSelectionSP KisMask::selection() const
{
    return m_d->selection;
}

KisPaintDeviceSP KisMask::paintDevice() const
{
    return selection()->pixelSelection();
}

KisPaintDeviceSP KisMask::original() const
{
    return paintDevice();
}

KisPaintDeviceSP KisMask::projection() const
{
    return paintDevice();
}

KisAbstractProjectionPlaneSP KisMask::projectionPlane() const
{
    return m_d->projectionPlane;
}

void KisMask::setSelection(KisSelectionSP selection)
{
    m_d->selection = selection;
    if (parent()) {
        const KisLayer *parentLayer = qobject_cast<const KisLayer*>(parent());
        m_d->selection->setDefaultBounds(new KisDefaultBounds(parentLayer->image()));
    }
    m_d->selection->setParentNode(this);
}

void KisMask::select(const QRect & rc, quint8 selectedness)
{
    KisSelectionSP sel = selection();
    KisPixelSelectionSP psel = sel->pixelSelection();
    psel->select(rc, selectedness);
    sel->updateProjection(rc);
}


QRect KisMask::decorateRect(KisPaintDeviceSP &src,
                            KisPaintDeviceSP &dst,
                            const QRect & rc,
                            PositionToFilthy maskPos) const
{
    Q_UNUSED(src);
    Q_UNUSED(dst);
    Q_UNUSED(maskPos);
    Q_ASSERT_X(0, "KisMask::decorateRect", "Should be overridden by successors");
    return rc;
}

void KisMask::apply(KisPaintDeviceSP projection, const QRect &applyRect, const QRect &needRect, PositionToFilthy maskPos) const
{
    if (selection()) {

        m_d->selection->updateProjection(applyRect);

        KisSelectionSP effectiveSelection = m_d->selection;
        QRect effectiveExtent = effectiveSelection->selectedRect();

        {
            // Access temporary target under the lock held
            KisIndirectPaintingSupport::ReadLocker l(this);

            if (hasTemporaryTarget()) {
                effectiveExtent |= temporaryTarget()->extent();
            }

            if(!effectiveExtent.intersects(applyRect)) {
                return;
            }

            if (hasTemporaryTarget()) {
                effectiveSelection = m_d->cachedSelection.getSelection();
                effectiveSelection->setDefaultBounds(m_d->selection->pixelSelection()->defaultBounds());

                KisPainter::copyAreaOptimized(applyRect.topLeft(),
                                              m_d->selection->pixelSelection(),
                                              effectiveSelection->pixelSelection(), applyRect);

                KisPainter gc(effectiveSelection->pixelSelection());
                setupTemporaryPainter(&gc);
                gc.bitBlt(applyRect.topLeft(), temporaryTarget(), applyRect);
            }
        }

        KisPaintDeviceSP cacheDevice = m_d->paintDeviceCache.getDevice(projection);

        QRect updatedRect = decorateRect(projection, cacheDevice, applyRect, maskPos);

        // masks don't have any compositioning
        KisPainter::copyAreaOptimized(updatedRect.topLeft(), cacheDevice, projection, updatedRect, effectiveSelection);
        m_d->paintDeviceCache.putDevice(cacheDevice);

        if (effectiveSelection != m_d->selection) {
            m_d->cachedSelection.putSelection(effectiveSelection);
        }

    } else {
        KisPaintDeviceSP cacheDevice = m_d->paintDeviceCache.getDevice(projection);

        cacheDevice->makeCloneFromRough(projection, needRect);
        projection->clear(needRect);

        decorateRect(cacheDevice, projection, applyRect, maskPos);

        m_d->paintDeviceCache.putDevice(cacheDevice);
    }
}

QRect KisMask::needRect(const QRect &rect,  PositionToFilthy pos) const
{
    Q_UNUSED(pos);
    QRect resultRect = rect;
    if (m_d->selection)
        resultRect &= m_d->selection->selectedRect();

    return resultRect;
}

QRect KisMask::changeRect(const QRect &rect, PositionToFilthy pos) const
{
    Q_UNUSED(pos);
    QRect resultRect = rect;
    if (m_d->selection)
        resultRect &= m_d->selection->selectedRect();

    return resultRect;
}

QRect KisMask::extent() const
{
    return m_d->selection ? m_d->selection->selectedRect() :
           parent() ? parent()->extent() : QRect();
}

QRect KisMask::exactBounds() const
{
    return m_d->selection ? m_d->selection->selectedExactRect() :
           parent() ? parent()->exactBounds() : QRect();
}

qint32 KisMask::x() const
{
    return m_d->selection ? m_d->selection->x() :
           m_d->deferredSelectionOffset ? m_d->deferredSelectionOffset->x() :
           parent() ? parent()->x() : 0;
}

qint32 KisMask::y() const
{
    return m_d->selection ? m_d->selection->y() :
           m_d->deferredSelectionOffset ? m_d->deferredSelectionOffset->y() :
           parent() ? parent()->y() : 0;
}

void KisMask::setX(qint32 x)
{
    if (m_d->selection) {
        m_d->selection->setX(x);
    } else if (!m_d->deferredSelectionOffset) {
        m_d->deferredSelectionOffset.reset(new QPoint(x, 0));
    } else {
        m_d->deferredSelectionOffset->rx() = x;
    }
}

void KisMask::setY(qint32 y)
{
    if (m_d->selection) {
        m_d->selection->setY(y);
    } else if (!m_d->deferredSelectionOffset) {
        m_d->deferredSelectionOffset.reset(new QPoint(0, y));
    } else {
        m_d->deferredSelectionOffset->ry() = y;
    }
}

QRect KisMask::nonDependentExtent() const
{
    return QRect();
}

QImage KisMask::createThumbnail(qint32 w, qint32 h)
{
    KisPaintDeviceSP originalDevice =
        selection() ? selection()->projection() : 0;

    return originalDevice ?
           originalDevice->createThumbnail(w, h, 1,
                                           KoColorConversionTransformation::internalRenderingIntent(),
                                           KoColorConversionTransformation::internalConversionFlags()) : QImage();
}

void KisMask::testingInitSelection(const QRect &rect, KisLayerSP parentLayer)
{
    if (parentLayer) {
        m_d->selection = new KisSelection(new KisSelectionDefaultBounds(parentLayer->paintDevice(), parentLayer->image()));
    } else {
        m_d->selection = new KisSelection();
    }

    m_d->selection->pixelSelection()->select(rect, OPACITY_OPAQUE_U8);
    m_d->selection->updateProjection(rect);
    m_d->selection->setParentNode(this);
}

KisKeyframeChannel *KisMask::requestKeyframeChannel(const QString &id)
{
    if (id == KisKeyframeChannel::Content.id()) {
        KisPaintDeviceSP device = paintDevice();
        if (device) {
            KisRasterKeyframeChannel *contentChannel = device->createKeyframeChannel(KisKeyframeChannel::Content);
            contentChannel->setFilenameSuffix(".pixelselection");
            return contentChannel;
       }
    }

    return KisNode::requestKeyframeChannel(id);
}

