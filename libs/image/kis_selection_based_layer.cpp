/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2005 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2009 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_selection_based_layer.h"

#include <klocalizedstring.h>
#include "kis_debug.h"

#include <KoCompositeOpRegistry.h>

#include "kis_image.h"
#include "kis_painter.h"
#include "kis_default_bounds.h"

#include "kis_selection.h"
#include "kis_pixel_selection.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter_registry.h"
#include "filter/kis_filter.h"

#include "kis_raster_keyframe_channel.h"


struct Q_DECL_HIDDEN KisSelectionBasedLayer::Private
{
public:
    Private() : useSelectionInProjection(true) {}

    KisSelectionSP selection;
    KisPaintDeviceSP paintDevice;
    bool useSelectionInProjection;
};


KisSelectionBasedLayer::KisSelectionBasedLayer(KisImageWSP image,
        const QString &name,
        KisSelectionSP selection,
        KisFilterConfigurationSP filterConfig,
        bool useGeneratorRegistry)
        : KisLayer(image.data(), name, OPACITY_OPAQUE_U8),
          KisNodeFilterInterface(filterConfig, useGeneratorRegistry),
          m_d(new Private())
{
    if (!selection)
        initSelection();
    else
        setInternalSelection(selection);

    m_d->paintDevice = new KisPaintDevice(this, image->colorSpace(), new KisDefaultBounds(image));
    connect(image.data(), SIGNAL(sigSizeChanged(QPointF,QPointF)), SLOT(slotImageSizeChanged()));
}

KisSelectionBasedLayer::KisSelectionBasedLayer(const KisSelectionBasedLayer& rhs)
        : KisLayer(rhs)
        , KisIndirectPaintingSupport()
        , KisNodeFilterInterface(rhs)
        , m_d(new Private())
{
    setInternalSelection(rhs.m_d->selection);

    m_d->paintDevice = new KisPaintDevice(*rhs.m_d->paintDevice.data());
}


KisSelectionBasedLayer::~KisSelectionBasedLayer()
{
    delete m_d;
}

void KisSelectionBasedLayer::initSelection()
{
    m_d->selection = new KisSelection(new KisDefaultBounds(image()));
    m_d->selection->pixelSelection()->setDefaultPixel(KoColor(Qt::white, m_d->selection->pixelSelection()->colorSpace()));
    m_d->selection->setParentNode(this);
    m_d->selection->updateProjection();
}

void KisSelectionBasedLayer::slotImageSizeChanged()
{
    if (m_d->selection) {
        /**
         * Make sure exactBounds() of the selection got recalculated after
         * the image changed
         */
        m_d->selection->pixelSelection()->setDirty();
        setDirty();
    }
}

void KisSelectionBasedLayer::setImage(KisImageWSP image)
{
    m_d->paintDevice->setDefaultBounds(new KisDefaultBounds(image));
    KisLayer::setImage(image);

    connect(image.data(), SIGNAL(sigSizeChanged(QPointF,QPointF)), SLOT(slotImageSizeChanged()));
}

bool KisSelectionBasedLayer::allowAsChild(KisNodeSP node) const
{
    return node->inherits("KisMask");
}


KisPaintDeviceSP KisSelectionBasedLayer::original() const
{
    return m_d->paintDevice;
}
KisPaintDeviceSP KisSelectionBasedLayer::paintDevice() const
{
    return m_d->selection->pixelSelection();
}


bool KisSelectionBasedLayer::needProjection() const
{
    return m_d->selection;
}

void KisSelectionBasedLayer::setUseSelectionInProjection(bool value) const
{
    m_d->useSelectionInProjection = value;
}

KisSelectionSP KisSelectionBasedLayer::fetchComposedInternalSelection(const QRect &rect) const
{
    if (!m_d->selection) return 0;
    m_d->selection->updateProjection(rect);

    KisSelectionSP tempSelection = m_d->selection;

    KisIndirectPaintingSupport::ReadLocker l(this);

    if (hasTemporaryTarget()) {
        /**
         * Cloning a selection with COW
         * FIXME: check whether it's faster than usual bitBlt'ing
         */
        tempSelection = new KisSelection(*tempSelection);

        KisPainter gc2(tempSelection->pixelSelection());
        setupTemporaryPainter(&gc2);
        gc2.bitBlt(rect.topLeft(), temporaryTarget(), rect);
    }

    return tempSelection;
}

void KisSelectionBasedLayer::copyOriginalToProjection(const KisPaintDeviceSP original,
        KisPaintDeviceSP projection,
        const QRect& rect) const
{
    KisSelectionSP tempSelection;

    if (m_d->useSelectionInProjection) {
        tempSelection = fetchComposedInternalSelection(rect);
    }

    projection->clear(rect);
    KisPainter::copyAreaOptimized(rect.topLeft(), original, projection, rect, tempSelection);
}

QRect KisSelectionBasedLayer::cropChangeRectBySelection(const QRect &rect) const
{
    return m_d->selection ?
        rect & m_d->selection->selectedRect() :
        rect;
}

QRect KisSelectionBasedLayer::needRect(const QRect &rect, PositionToFilthy pos) const
{
    Q_UNUSED(pos);
    return rect;
}

void KisSelectionBasedLayer::resetCache()
{
    if (!image()) {
        return;
    }

    if (!m_d->paintDevice || *m_d->paintDevice->colorSpace() != *image()->colorSpace()) {
        m_d->paintDevice = KisPaintDeviceSP(new KisPaintDevice(KisNodeWSP(this), image()->colorSpace(), new KisDefaultBounds(image())));
    } else {
        m_d->paintDevice->clear();
  }
}

KisSelectionSP KisSelectionBasedLayer::internalSelection() const
{
    return m_d->selection;
}

void KisSelectionBasedLayer::setInternalSelection(KisSelectionSP selection)
{
    if (selection) {
        m_d->selection = new KisSelection(*selection.data());
        m_d->selection->setParentNode(this);
        m_d->selection->updateProjection();
    } else {
        m_d->selection = 0;
    }

    if (selection->pixelSelection()->defaultBounds()->bounds() != image()->bounds()) {
        qWarning() << "WARNING: KisSelectionBasedLayer::setInternalSelection"
                   << "New selection has suspicious default bounds";
        qWarning() << "WARNING:" << ppVar(selection->pixelSelection()->defaultBounds()->bounds());
        qWarning() << "WARNING:" << ppVar(image()->bounds());
    }
}

qint32 KisSelectionBasedLayer::x() const
{
    return m_d->selection ? m_d->selection->x() : 0;
}

qint32 KisSelectionBasedLayer::y() const
{
    return m_d->selection ? m_d->selection->y() : 0;
}

void KisSelectionBasedLayer::setX(qint32 x)
{
    if (m_d->selection) {
        m_d->selection->setX(x);
    }
}

void KisSelectionBasedLayer::setY(qint32 y)
{
    if (m_d->selection) {
        m_d->selection->setY(y);
    }
}

void KisSelectionBasedLayer::setDirty(const QRect & rect)
{
    KisLayer::setDirty(rect);
}

KisKeyframeChannel *KisSelectionBasedLayer::requestKeyframeChannel(const QString &id)
{
    if (id == KisKeyframeChannel::Content.id()) {
        KisRasterKeyframeChannel *contentChannel = m_d->selection->pixelSelection()->createKeyframeChannel(KisKeyframeChannel::Content);
        contentChannel->setFilenameSuffix(".pixelselection");
        return contentChannel;
    }

    return KisLayer::requestKeyframeChannel(id);
}

void KisSelectionBasedLayer::setDirty()
{
    Q_ASSERT(image());

    setDirty(image()->bounds());
}

QRect KisSelectionBasedLayer::extent() const
{
    Q_ASSERT(image());

    return m_d->selection ?
           m_d->selection->selectedRect() : image()->bounds();
}

QRect KisSelectionBasedLayer::exactBounds() const
{
    Q_ASSERT(image());

    return m_d->selection ?
           m_d->selection->selectedExactRect() : image()->bounds();
}

QImage KisSelectionBasedLayer::createThumbnail(qint32 w, qint32 h)
{
    KisSelectionSP originalSelection = internalSelection();
    KisPaintDeviceSP originalDevice = original();

    return originalDevice && originalSelection ?
           originalDevice->createThumbnail(w, h, 1,
                                           KoColorConversionTransformation::internalRenderingIntent(),
                                           KoColorConversionTransformation::internalConversionFlags()) :
           QImage();
}

