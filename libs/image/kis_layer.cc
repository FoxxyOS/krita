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

#include "kis_layer.h"


#include <klocalizedstring.h>
#include <QImage>
#include <QBitArray>
#include <QStack>
#include <QMutex>
#include <QMutexLocker>

#include <KoIcon.h>
#include <kis_icon.h>
#include <KoProperties.h>
#include <KoCompositeOpRegistry.h>
#include <KoColorSpace.h>

#include "kis_debug.h"
#include "kis_image.h"

#include "kis_painter.h"
#include "kis_mask.h"
#include "kis_effect_mask.h"
#include "kis_selection_mask.h"
#include "kis_meta_data_store.h"
#include "kis_selection.h"
#include "kis_paint_layer.h"
#include "kis_raster_keyframe_channel.h"

#include "kis_clone_layer.h"

#include "kis_psd_layer_style.h"
#include "kis_layer_projection_plane.h"
#include "layerstyles/kis_layer_style_projection_plane.h"

#include "krita_utils.h"
#include "kis_layer_properties_icons.h"
#include "kis_layer_utils.h"


class KisSafeProjection {
public:
    KisPaintDeviceSP getDeviceLazy(KisPaintDeviceSP prototype) {
        QMutexLocker locker(&m_lock);

        if (!m_reusablePaintDevice) {
            m_reusablePaintDevice = new KisPaintDevice(*prototype);
        }
        if(!m_projection ||
           *m_projection->colorSpace() != *prototype->colorSpace()) {
            m_projection = m_reusablePaintDevice;
            m_projection->makeCloneFromRough(prototype, prototype->extent());
            m_projection->setProjectionDevice(true);
        }

        return m_projection;
    }

    void freeDevice() {
        QMutexLocker locker(&m_lock);
        m_projection = 0;
        if(m_reusablePaintDevice) {
            m_reusablePaintDevice->clear();
        }
    }

private:
    QMutex m_lock;
    KisPaintDeviceSP m_projection;
    KisPaintDeviceSP m_reusablePaintDevice;
};

class KisCloneLayersList {
public:
    void addClone(KisCloneLayerWSP cloneLayer) {
        m_clonesList.append(cloneLayer);
    }

    void removeClone(KisCloneLayerWSP cloneLayer) {
        m_clonesList.removeOne(cloneLayer);
    }

    void setDirty(const QRect &rect) {
        Q_FOREACH (KisCloneLayerWSP clone, m_clonesList) {
            clone->setDirtyOriginal(rect);
        }
    }

    const QList<KisCloneLayerWSP> registeredClones() const {
        return m_clonesList;
    }

    bool hasClones() const {
        return !m_clonesList.isEmpty();
    }

private:
    QList<KisCloneLayerWSP> m_clonesList;
};

struct Q_DECL_HIDDEN KisLayer::Private
{
    KisImageWSP image;
    QBitArray channelFlags;
    KisMetaData::Store* metaDataStore;
    KisSafeProjection safeProjection;
    KisCloneLayersList clonesList;

    KisPSDLayerStyleSP layerStyle;
    KisAbstractProjectionPlaneSP layerStyleProjectionPlane;

    KisAbstractProjectionPlaneSP projectionPlane;
};


KisLayer::KisLayer(KisImageWSP image, const QString &name, quint8 opacity)
        : KisNode()
        , m_d(new Private)
{
    setName(name);
    setOpacity(opacity);
    m_d->image = image;
    m_d->metaDataStore = new KisMetaData::Store();
    m_d->projectionPlane = toQShared(new KisLayerProjectionPlane(this));
}

KisLayer::KisLayer(const KisLayer& rhs)
        : KisNode(rhs)
        , m_d(new Private())
{
    if (this != &rhs) {
        m_d->image = rhs.m_d->image;
        m_d->metaDataStore = new KisMetaData::Store(*rhs.m_d->metaDataStore);
        m_d->channelFlags = rhs.m_d->channelFlags;

        setName(rhs.name());
        m_d->projectionPlane = toQShared(new KisLayerProjectionPlane(this));

        if (rhs.m_d->layerStyle) {
            setLayerStyle(rhs.m_d->layerStyle->clone());
        }
    }
}

KisLayer::~KisLayer()
{
    delete m_d->metaDataStore;
    delete m_d;
}

const KoColorSpace * KisLayer::colorSpace() const
{
    if (m_d->image)
        return m_d->image->colorSpace();
    return 0;
}

const KoCompositeOp * KisLayer::compositeOp() const
{
    /**
     * FIXME: This function duplicates the same function from
     * KisMask. We can't move it to KisBaseNode as it doesn't
     * know anything about parent() method of KisNode
     * Please think it over...
     */

    KisNodeSP parentNode = parent();
    if (!parentNode) return 0;

    if (!parentNode->colorSpace()) return 0;
    const KoCompositeOp* op = parentNode->colorSpace()->compositeOp(compositeOpId());
    return op ? op : parentNode->colorSpace()->compositeOp(COMPOSITE_OVER);
}

KisPSDLayerStyleSP KisLayer::layerStyle() const
{
    return m_d->layerStyle;
}

void KisLayer::setLayerStyle(KisPSDLayerStyleSP layerStyle)
{
    if (layerStyle) {
        m_d->layerStyle = layerStyle;

        KisAbstractProjectionPlaneSP plane = !layerStyle->isEmpty() ?
            KisAbstractProjectionPlaneSP(new KisLayerStyleProjectionPlane(this)) :
            KisAbstractProjectionPlaneSP(0);

        m_d->layerStyleProjectionPlane = plane;
    } else {
        m_d->layerStyleProjectionPlane.clear();
        m_d->layerStyle.clear();
    }
}

KisBaseNode::PropertyList KisLayer::sectionModelProperties() const
{
    KisBaseNode::PropertyList l = KisBaseNode::sectionModelProperties();
    l << KisBaseNode::Property(KoID("opacity", i18n("Opacity")), i18n("%1%", percentOpacity()));

    const KoCompositeOp * compositeOp = this->compositeOp();

    if (compositeOp) {
        l << KisBaseNode::Property(KoID("compositeop", i18n("Composite Mode")), compositeOp->description());
    }

    if (m_d->layerStyle && !m_d->layerStyle->isEmpty()) {
        l << KisLayerPropertiesIcons::getProperty(KisLayerPropertiesIcons::layerStyle, m_d->layerStyle->isEnabled());
    }

    l << KisLayerPropertiesIcons::getProperty(KisLayerPropertiesIcons::inheritAlpha, alphaChannelDisabled());

    return l;
}

void KisLayer::setSectionModelProperties(const KisBaseNode::PropertyList &properties)
{
    KisBaseNode::setSectionModelProperties(properties);

    Q_FOREACH (const KisBaseNode::Property &property, properties) {
        if (property.id == KisLayerPropertiesIcons::inheritAlpha.id()) {
            disableAlphaChannel(property.state.toBool());
        }

        if (property.id == KisLayerPropertiesIcons::layerStyle.id()) {
            if (m_d->layerStyle &&
                m_d->layerStyle->isEnabled() != property.state.toBool()) {

                m_d->layerStyle->setEnabled(property.state.toBool());

                baseNodeChangedCallback();
                baseNodeInvalidateAllFramesCallback();
            }
        }
    }
}

void KisLayer::disableAlphaChannel(bool disable)
{
    QBitArray newChannelFlags = m_d->channelFlags;

    if(newChannelFlags.isEmpty())
        newChannelFlags = colorSpace()->channelFlags(true, true);

    if(disable)
        newChannelFlags &= colorSpace()->channelFlags(true, false);
    else
        newChannelFlags |= colorSpace()->channelFlags(false, true);

    setChannelFlags(newChannelFlags);
}

bool KisLayer::alphaChannelDisabled() const
{
    QBitArray flags = colorSpace()->channelFlags(false, true) & m_d->channelFlags;
    return flags.count(true) == 0 && !m_d->channelFlags.isEmpty();
}


void KisLayer::setChannelFlags(const QBitArray & channelFlags)
{
    Q_ASSERT(channelFlags.isEmpty() ||((quint32)channelFlags.count() == colorSpace()->channelCount()));

    if (KritaUtils::compareChannelFlags(channelFlags,
                                        this->channelFlags())) {
        return;
    }

    if (!channelFlags.isEmpty() &&
        channelFlags == QBitArray(channelFlags.size(), true)) {

        m_d->channelFlags.clear();
    } else {
        m_d->channelFlags = channelFlags;
    }

    baseNodeChangedCallback();
    baseNodeInvalidateAllFramesCallback();
}

QBitArray & KisLayer::channelFlags() const
{
    return m_d->channelFlags;
}

bool KisLayer::temporary() const
{
    return nodeProperties().boolProperty("temporary", false);
}

void KisLayer::setTemporary(bool t)
{
    nodeProperties().setProperty("temporary", t);
}

KisImageWSP KisLayer::image() const
{
    return m_d->image;
}

void KisLayer::setImage(KisImageWSP image)
{
    m_d->image = image;

    KisNodeSP node = firstChild();
    while (node) {
        KisLayerUtils::recursiveApplyNodes(node,
                                           [image] (KisNodeSP node) {
                                               node->setImage(image);
                                           });

        node = node->nextSibling();
    }
}

bool KisLayer::canMergeAndKeepBlendOptions(KisLayerSP otherLayer)
{
    return
        this->compositeOpId() == otherLayer->compositeOpId() &&
        this->opacity() == otherLayer->opacity() &&
        this->channelFlags() == otherLayer->channelFlags() &&
        !this->layerStyle() && !otherLayer->layerStyle() &&
        (this->colorSpace() == otherLayer->colorSpace() ||
         *this->colorSpace() == *otherLayer->colorSpace());
}

KisLayerSP KisLayer::createMergedLayerTemplate(KisLayerSP prevLayer)
{
    const bool keepBlendingOptions = canMergeAndKeepBlendOptions(prevLayer);

    KisLayerSP newLayer = new KisPaintLayer(image(), prevLayer->name(), OPACITY_OPAQUE_U8);

    if (keepBlendingOptions) {
        newLayer->setCompositeOpId(compositeOpId());
        newLayer->setOpacity(opacity());
        newLayer->setChannelFlags(channelFlags());
    }

    return newLayer;
}

void KisLayer::fillMergedLayerTemplate(KisLayerSP dstLayer, KisLayerSP prevLayer)
{
    const bool keepBlendingOptions = canMergeAndKeepBlendOptions(prevLayer);

    QRect layerProjectionExtent = this->projection()->extent();
    QRect prevLayerProjectionExtent = prevLayer->projection()->extent();
    bool alphaDisabled = this->alphaChannelDisabled();
    bool prevAlphaDisabled = prevLayer->alphaChannelDisabled();

    KisPaintDeviceSP mergedDevice = dstLayer->paintDevice();

    if (!keepBlendingOptions) {

        KisNodeSP parentNode = parent();
        KisPainter gc(mergedDevice);

        //Copy the pixels of previous layer with their actual alpha value
        prevLayer->disableAlphaChannel(false);

        prevLayer->projectionPlane()->apply(&gc, prevLayerProjectionExtent | image()->bounds());

        //Restore the previous prevLayer disableAlpha status for correct undo/redo
        prevLayer->disableAlphaChannel(prevAlphaDisabled);

        //Paint the pixels of the current layer, using their actual alpha value
        if (alphaDisabled == prevAlphaDisabled) {
            this->disableAlphaChannel(false);
        }

        this->projectionPlane()->apply(&gc, layerProjectionExtent | image()->bounds());

        //Restore the layer disableAlpha status for correct undo/redo
        this->disableAlphaChannel(alphaDisabled);
    }
    else {
        //Copy prevLayer
        KisPaintDeviceSP srcDev = prevLayer->projection();
        mergedDevice->makeCloneFrom(srcDev, srcDev->extent());

        //Paint layer on the copy
        KisPainter gc(mergedDevice);
        gc.bitBlt(layerProjectionExtent.topLeft(), this->projection(), layerProjectionExtent);
    }
}

void KisLayer::registerClone(KisCloneLayerWSP clone)
{
    m_d->clonesList.addClone(clone);
}

void KisLayer::unregisterClone(KisCloneLayerWSP clone)
{
    m_d->clonesList.removeClone(clone);
}

const QList<KisCloneLayerWSP> KisLayer::registeredClones() const
{
    return m_d->clonesList.registeredClones();
}

bool KisLayer::hasClones() const
{
    return m_d->clonesList.hasClones();
}

void KisLayer::updateClones(const QRect &rect)
{
    m_d->clonesList.setDirty(rect);
}

KisSelectionMaskSP KisLayer::selectionMask() const
{
    KoProperties properties;
    properties.setProperty("active", true);
    QList<KisNodeSP> masks = childNodes(QStringList("KisSelectionMask"), properties);

    // return the first visible mask
    Q_FOREACH (KisNodeSP mask, masks) {
        if (mask->visible()) {
            return dynamic_cast<KisSelectionMask*>(mask.data());
        }
    }
    return 0;
}

KisSelectionSP KisLayer::selection() const
{
    KisSelectionMaskSP mask = selectionMask();

    if (mask) {
        return mask->selection();
    }
    else if (m_d->image) {
        return m_d->image->globalSelection();
    }
    else {
        return 0;
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

QList<KisEffectMaskSP> KisLayer::effectMasks(KisNodeSP lastNode) const
{
    QList<KisEffectMaskSP> masks;

    if (childCount() > 0) {
        KoProperties properties;
        properties.setProperty("visible", true);
        QList<KisNodeSP> nodes = childNodes(QStringList("KisEffectMask"), properties);

        Q_FOREACH (const KisNodeSP& node,  nodes) {
            if (node == lastNode) break;

            KisEffectMaskSP mask = dynamic_cast<KisEffectMask*>(const_cast<KisNode*>(node.data()));
            if (mask)
                masks.append(mask);
        }
    }
    return masks;
}

bool KisLayer::hasEffectMasks() const
{
    if (childCount() == 0) return false;

    KisNodeSP node = firstChild();
    while (node) {
        if (node->inherits("KisEffectMask") && node->visible()) {
            return true;
        }
        node = node->nextSibling();
    }

    return false;
}

QRect KisLayer::masksChangeRect(const QList<KisEffectMaskSP> &masks,
                                const QRect &requestedRect,
                                bool &rectVariesFlag) const
{
    rectVariesFlag = false;

    QRect prevChangeRect = requestedRect;

    /**
     * We set default value of the change rect for the case
     * when there is no mask at all
     */
    QRect changeRect = requestedRect;

    Q_FOREACH (const KisEffectMaskSP& mask, masks) {
        changeRect = mask->changeRect(prevChangeRect);

        if (changeRect != prevChangeRect)
            rectVariesFlag = true;

        prevChangeRect = changeRect;
    }

    return changeRect;
}

QRect KisLayer::masksNeedRect(const QList<KisEffectMaskSP> &masks,
                              const QRect &changeRect,
                              QStack<QRect> &applyRects,
                              bool &rectVariesFlag) const
{
    rectVariesFlag = false;

    QRect prevNeedRect = changeRect;
    QRect needRect;

    for (qint32 i = masks.size() - 1; i >= 0; i--) {
        applyRects.push(prevNeedRect);

        needRect = masks[i]->needRect(prevNeedRect);

        if (prevNeedRect != needRect)
            rectVariesFlag = true;

        prevNeedRect = needRect;
    }

    return needRect;
}

KisNode::PositionToFilthy calculatePositionToFilthy(KisNodeSP nodeInQuestion,
                                           KisNodeSP filthy,
                                           KisNodeSP parent)
{
    if (parent == filthy || parent != filthy->parent()) {
        return KisNode::N_ABOVE_FILTHY;
    }

    if (nodeInQuestion == filthy) {
        return KisNode::N_FILTHY;
    }

    KisNodeSP node = nodeInQuestion->prevSibling();
    while (node) {
        if (node == filthy) {
            return KisNode::N_ABOVE_FILTHY;
        }
        node = node->prevSibling();
    }

    return KisNode::N_BELOW_FILTHY;
}

QRect KisLayer::applyMasks(const KisPaintDeviceSP source,
                           KisPaintDeviceSP destination,
                           const QRect &requestedRect,
                           KisNodeSP filthyNode,
                           KisNodeSP lastNode) const
{
    Q_ASSERT(source);
    Q_ASSERT(destination);

    QList<KisEffectMaskSP> masks = effectMasks(lastNode);
    QRect changeRect;
    QRect needRect;

    if (masks.isEmpty()) {
        changeRect = requestedRect;
        if (source != destination) {
            copyOriginalToProjection(source, destination, requestedRect);
        }
    } else {
        QStack<QRect> applyRects;
        bool changeRectVaries;
        bool needRectVaries;

        /**
         * FIXME: Assume that varying of the changeRect has already
         * been taken into account while preparing walkers
         */
        changeRectVaries = false;
        changeRect = requestedRect;
        //changeRect = masksChangeRect(masks, requestedRect,
        //                             changeRectVaries);

        needRect = masksNeedRect(masks, changeRect,
                                 applyRects, needRectVaries);

        if (!changeRectVaries && !needRectVaries) {
            /**
             * A bit of optimization:
             * All filters will read/write exactly from/to the requested
             * rect so we needn't create temporary paint device,
             * just apply it onto destination
             */
            Q_ASSERT(needRect == requestedRect);

            if (source != destination) {
                copyOriginalToProjection(source, destination, needRect);
            }

            Q_FOREACH (const KisEffectMaskSP& mask, masks) {
                const QRect maskApplyRect = applyRects.pop();
                const QRect maskNeedRect =
                    applyRects.isEmpty() ? needRect : applyRects.top();

                PositionToFilthy maskPosition = calculatePositionToFilthy(mask, filthyNode, const_cast<KisLayer*>(this));
                mask->apply(destination, maskApplyRect, maskNeedRect, maskPosition);
            }
            Q_ASSERT(applyRects.isEmpty());
        } else {
            /**
             * We can't eliminate additional copy-op
             * as filters' behaviour may be quite insane here,
             * so let them work on their own paintDevice =)
             */

            KisPaintDeviceSP tempDevice = new KisPaintDevice(colorSpace());
            tempDevice->prepareClone(source);
            copyOriginalToProjection(source, tempDevice, needRect);

            QRect maskApplyRect = applyRects.pop();
            QRect maskNeedRect = needRect;

            Q_FOREACH (const KisEffectMaskSP& mask, masks) {
                PositionToFilthy maskPosition = calculatePositionToFilthy(mask, filthyNode, const_cast<KisLayer*>(this));
                mask->apply(tempDevice, maskApplyRect, maskNeedRect, maskPosition);

                if (!applyRects.isEmpty()) {
                    maskNeedRect = maskApplyRect;
                    maskApplyRect = applyRects.pop();
                }
            }
            Q_ASSERT(applyRects.isEmpty());

            KisPainter::copyAreaOptimized(changeRect.topLeft(), tempDevice, destination, changeRect);
        }
    }

    return changeRect;
}

QRect KisLayer::updateProjection(const QRect& rect, KisNodeSP filthyNode)
{
    QRect updatedRect = rect;
    KisPaintDeviceSP originalDevice = original();
    if (!rect.isValid() ||
            !visible() ||
            !originalDevice) return QRect();

    if (!needProjection() && !hasEffectMasks()) {
        m_d->safeProjection.freeDevice();
    } else {

        if (!updatedRect.isEmpty()) {
            KisPaintDeviceSP projection =
                m_d->safeProjection.getDeviceLazy(originalDevice);

            updatedRect = applyMasks(originalDevice, projection,
                                     updatedRect, filthyNode, 0);
        }
    }

    return updatedRect;
}

QRect KisLayer::partialChangeRect(KisNodeSP lastNode, const QRect& rect)
{
    bool changeRectVaries = false;
    QRect changeRect = outgoingChangeRect(rect);
    changeRect = masksChangeRect(effectMasks(lastNode), changeRect,
                                 changeRectVaries);

    return changeRect;
}

/**
 * \p rect is a dirty rect in layer's original() coordinates!
 */
void KisLayer::buildProjectionUpToNode(KisPaintDeviceSP projection, KisNodeSP lastNode, const QRect& rect)
{
    QRect changeRect = partialChangeRect(lastNode, rect);

    KisPaintDeviceSP originalDevice = original();

    KIS_ASSERT_RECOVER_RETURN(needProjection() || hasEffectMasks());

    if (!changeRect.isEmpty()) {
        applyMasks(originalDevice, projection,
                   changeRect, this, lastNode);
    }
}

bool KisLayer::needProjection() const
{
    return false;
}

void KisLayer::copyOriginalToProjection(const KisPaintDeviceSP original,
                                        KisPaintDeviceSP projection,
                                        const QRect& rect) const
{
    KisPainter::copyAreaOptimized(rect.topLeft(), original, projection, rect);
}

KisAbstractProjectionPlaneSP KisLayer::projectionPlane() const
{
    return m_d->layerStyleProjectionPlane ?
        m_d->layerStyleProjectionPlane : m_d->projectionPlane;
}

KisAbstractProjectionPlaneSP KisLayer::internalProjectionPlane() const
{
    return m_d->projectionPlane;
}

KisPaintDeviceSP KisLayer::projection() const
{
    KisPaintDeviceSP originalDevice = original();

    return needProjection() || hasEffectMasks() ?
        m_d->safeProjection.getDeviceLazy(originalDevice) : originalDevice;
}

QRect KisLayer::changeRect(const QRect &rect, PositionToFilthy pos) const
{
    QRect changeRect = rect;
    changeRect = incomingChangeRect(changeRect);

    if(pos == KisNode::N_FILTHY) {
        QRect projectionToBeUpdated = projection()->exactBoundsAmortized() & changeRect;

        bool changeRectVaries;
        changeRect = outgoingChangeRect(changeRect);
        changeRect = masksChangeRect(effectMasks(), changeRect, changeRectVaries);

        /**
         * If the projection contains some dirty areas we should also
         * add them to the change rect, because they might have
         * changed. E.g. when a visibility of the mask has chnaged
         * while the parent layer was invinisble.
         */

        if (!projectionToBeUpdated.isEmpty() &&
            !changeRect.contains(projectionToBeUpdated)) {

            changeRect |= projectionToBeUpdated;
        }
    }

    // TODO: string comparizon: optimize!
    if (pos != KisNode::N_FILTHY &&
        pos != KisNode::N_FILTHY_PROJECTION &&
        compositeOpId() != COMPOSITE_COPY) {

        changeRect |= rect;
    }

    return changeRect;
}

QRect KisLayer::incomingChangeRect(const QRect &rect) const
{
    return rect;
}

QRect KisLayer::outgoingChangeRect(const QRect &rect) const
{
    return rect;
}

QImage KisLayer::createThumbnail(qint32 w, qint32 h)
{
    if (w == 0 || h == 0) {
        return QImage();
    }

    KisPaintDeviceSP originalDevice = original();

    return originalDevice ?
           originalDevice->createThumbnail(w, h, 1,
                                           KoColorConversionTransformation::internalRenderingIntent(),
                                           KoColorConversionTransformation::internalConversionFlags()) : QImage();
}

QImage KisLayer::createThumbnailForFrame(qint32 w, qint32 h, int time)
{
    if (w == 0 || h == 0) {
        return QImage();
    }

    KisPaintDeviceSP originalDevice = original();
    if (originalDevice) {
        KisRasterKeyframeChannel *channel = originalDevice->keyframeChannel();

        if (channel) {
            KisPaintDeviceSP targetDevice = new KisPaintDevice(colorSpace());
            KisKeyframeSP keyframe = channel->activeKeyframeAt(time);
            channel->fetchFrame(keyframe, targetDevice);
            return targetDevice->createThumbnail(w, h, 1,
                                                 KoColorConversionTransformation::internalRenderingIntent(),
                                                 KoColorConversionTransformation::internalConversionFlags());
        }
    }

    return createThumbnail(w, h);
}

qint32 KisLayer::x() const
{
    KisPaintDeviceSP originalDevice = original();
    return originalDevice ? originalDevice->x() : 0;
}
qint32 KisLayer::y() const
{
    KisPaintDeviceSP originalDevice = original();
    return originalDevice ? originalDevice->y() : 0;
}
void KisLayer::setX(qint32 x)
{
    KisPaintDeviceSP originalDevice = original();
    if (originalDevice)
        originalDevice->setX(x);
}
void KisLayer::setY(qint32 y)
{
    KisPaintDeviceSP originalDevice = original();
    if (originalDevice)
        originalDevice->setY(y);
}

QRect KisLayer::extent() const
{
    QRect additionalMaskExtent = QRect();
    QList<KisEffectMaskSP> effectMasks = this->effectMasks();

    Q_FOREACH(KisEffectMaskSP mask, effectMasks) {
        additionalMaskExtent |= mask->nonDependentExtent();
    }

    KisPaintDeviceSP originalDevice = original();
    QRect layerExtent = originalDevice ? originalDevice->extent() : QRect();

    return layerExtent | additionalMaskExtent;
}

QRect KisLayer::exactBounds() const
{
    QRect additionalMaskExtent = QRect();
    QList<KisEffectMaskSP> effectMasks = this->effectMasks();

    Q_FOREACH(KisEffectMaskSP mask, effectMasks) {
        additionalMaskExtent |= mask->nonDependentExtent();
    }

    KisPaintDeviceSP originalDevice = original();
    QRect layerExtent = originalDevice ? originalDevice->exactBounds() : QRect();

    return layerExtent | additionalMaskExtent;
}

KisLayerSP KisLayer::parentLayer() const
{
    return dynamic_cast<KisLayer*>(parent().data());
}

KisMetaData::Store* KisLayer::metaData()
{
    return m_d->metaDataStore;
}

