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

#include "kis_layer_utils.h"

#include <algorithm>

#include <QUuid>
#include <KoColorSpaceConstants.h>

#include "kis_painter.h"
#include "kis_image.h"
#include "kis_node.h"
#include "kis_layer.h"
#include "kis_paint_layer.h"
#include "kis_clone_layer.h"
#include "kis_group_layer.h"
#include "kis_selection.h"
#include "kis_selection_mask.h"
#include "kis_meta_data_merge_strategy.h"
#include <kundo2command.h>
#include "commands/kis_image_layer_add_command.h"
#include "commands/kis_image_layer_remove_command.h"
#include "commands/kis_image_layer_move_command.h"
#include "commands/kis_image_change_layers_command.h"
#include "commands_new/kis_activate_selection_mask_command.h"
#include "kis_abstract_projection_plane.h"
#include "kis_processing_applicator.h"
#include "kis_image_animation_interface.h"
#include "kis_keyframe_channel.h"
#include "kis_command_utils.h"
#include "kis_processing_applicator.h"
#include "commands_new/kis_change_projection_color_command.h"
#include "kis_layer_properties_icons.h"
#include "lazybrush/kis_colorize_mask.h"
#include "commands/kis_node_property_list_command.h"


namespace KisLayerUtils {

    void fetchSelectionMasks(KisNodeList mergedNodes, QVector<KisSelectionMaskSP> &selectionMasks)
    {
        foreach (KisNodeSP node, mergedNodes) {
            KisLayerSP layer = dynamic_cast<KisLayer*>(node.data());

            KisSelectionMaskSP mask;

            if (layer && (mask = layer->selectionMask())) {
                selectionMasks.append(mask);
            }
        }
    }

    struct MergeDownInfoBase {
        MergeDownInfoBase(KisImageSP _image)
            : image(_image),
              storage(new SwitchFrameCommand::SharedStorage())
        {
        }
        virtual ~MergeDownInfoBase() {}

        KisImageWSP image;

        QVector<KisSelectionMaskSP> selectionMasks;

        KisNodeSP dstNode;

        SwitchFrameCommand::SharedStorageSP storage;
        QSet<int> frames;

        virtual KisNodeList allSrcNodes() = 0;
        virtual KisLayerSP dstLayer() { return 0; }
    };

    struct MergeDownInfo : public MergeDownInfoBase {
        MergeDownInfo(KisImageSP _image,
                      KisLayerSP _prevLayer,
                      KisLayerSP _currLayer)
            : MergeDownInfoBase(_image),
              prevLayer(_prevLayer),
              currLayer(_currLayer)
        {
            frames =
                fetchLayerFramesRecursive(prevLayer) |
                fetchLayerFramesRecursive(currLayer);
        }

        KisLayerSP prevLayer;
        KisLayerSP currLayer;

        KisNodeList allSrcNodes() override {
            KisNodeList mergedNodes;
            mergedNodes << currLayer;
            mergedNodes << prevLayer;
            return mergedNodes;
        }

        KisLayerSP dstLayer() override {
            return dynamic_cast<KisLayer*>(dstNode.data());
        }
    };

    struct MergeMultipleInfo : public MergeDownInfoBase {
        MergeMultipleInfo(KisImageSP _image,
                          KisNodeList _mergedNodes)
            : MergeDownInfoBase(_image),
              mergedNodes(_mergedNodes)
        {
            foreach (KisNodeSP node, mergedNodes) {
                frames |= fetchLayerFramesRecursive(node);
            }
        }

        KisNodeList mergedNodes;

        KisNodeList allSrcNodes() override {
            return mergedNodes;
        }
    };

    typedef QSharedPointer<MergeDownInfoBase> MergeDownInfoBaseSP;
    typedef QSharedPointer<MergeDownInfo> MergeDownInfoSP;
    typedef QSharedPointer<MergeMultipleInfo> MergeMultipleInfoSP;

    struct FillSelectionMasks : public KUndo2Command {
        FillSelectionMasks(MergeDownInfoBaseSP info) : m_info(info) {}

        void redo() override {
            fetchSelectionMasks(m_info->allSrcNodes(), m_info->selectionMasks);
        }

    private:
        MergeDownInfoBaseSP m_info;
    };

    struct DisableColorizeKeyStrokes : public KisCommandUtils::AggregateCommand {
        DisableColorizeKeyStrokes(MergeDownInfoBaseSP info) : m_info(info) {}

        void populateChildCommands() override {
            Q_FOREACH (KisNodeSP node, m_info->allSrcNodes()) {
                recursiveApplyNodes(node,
                                    [this] (KisNodeSP node) {
                                        if (dynamic_cast<KisColorizeMask*>(node.data()) &&
                                            KisLayerPropertiesIcons::nodeProperty(node, KisLayerPropertiesIcons::colorizeEditKeyStrokes, true).toBool()) {

                                            KisBaseNode::PropertyList props = node->sectionModelProperties();
                                            KisLayerPropertiesIcons::setNodeProperty(&props,
                                                                                     KisLayerPropertiesIcons::colorizeEditKeyStrokes,
                                                                                     false);

                                            addCommand(new KisNodePropertyListCommand(node, props));
                                        }
                                    });
            }
        }

    private:
        MergeDownInfoBaseSP m_info;
    };

    struct DisableOnionSkins : public KisCommandUtils::AggregateCommand {
        DisableOnionSkins(MergeDownInfoBaseSP info) : m_info(info) {}

        void populateChildCommands() override {
            Q_FOREACH (KisNodeSP node, m_info->allSrcNodes()) {
                recursiveApplyNodes(node,
                                    [this] (KisNodeSP node) {
                                        if (KisLayerPropertiesIcons::nodeProperty(node, KisLayerPropertiesIcons::onionSkins, false).toBool()) {

                                            KisBaseNode::PropertyList props = node->sectionModelProperties();
                                            KisLayerPropertiesIcons::setNodeProperty(&props,
                                                                                     KisLayerPropertiesIcons::onionSkins,
                                                                                     false);

                                            addCommand(new KisNodePropertyListCommand(node, props));
                                        }
                                    });
            }
        }

    private:
        MergeDownInfoBaseSP m_info;
    };

    struct DisablePassThroughForHeadsOnly : public KisCommandUtils::AggregateCommand {
        DisablePassThroughForHeadsOnly(MergeDownInfoBaseSP info, bool skipIfDstIsGroup = false)
            : m_info(info),
              m_skipIfDstIsGroup(skipIfDstIsGroup)
        {
        }

        void populateChildCommands() override {
            if (m_skipIfDstIsGroup &&
                m_info->dstLayer() &&
                m_info->dstLayer()->inherits("KisGroupLayer")) {

                return;
            }


            Q_FOREACH (KisNodeSP node, m_info->allSrcNodes()) {
                if (KisLayerPropertiesIcons::nodeProperty(node, KisLayerPropertiesIcons::passThrough, false).toBool()) {

                    KisBaseNode::PropertyList props = node->sectionModelProperties();
                    KisLayerPropertiesIcons::setNodeProperty(&props,
                                                             KisLayerPropertiesIcons::passThrough,
                                                             false);

                    addCommand(new KisNodePropertyListCommand(node, props));
                }
            }
        }

    private:
        MergeDownInfoBaseSP m_info;
        bool m_skipIfDstIsGroup;
    };

    struct RefreshHiddenAreas : public KUndo2Command {
        RefreshHiddenAreas(MergeDownInfoBaseSP info) : m_info(info) {}

        void redo() override {
            KisImageAnimationInterface *interface = m_info->image->animationInterface();
            const QRect preparedRect = !interface->externalFrameActive() ?
                m_info->image->bounds() : QRect();

            foreach (KisNodeSP node, m_info->allSrcNodes()) {
                refreshHiddenAreaAsync(node, preparedRect);
            }
        }

    private:
        QRect realNodeExactBounds(KisNodeSP rootNode, QRect currentRect = QRect()) {
            KisNodeSP node = rootNode->firstChild();

            while(node) {
                currentRect |= realNodeExactBounds(node, currentRect);
                node = node->nextSibling();
            }

            // TODO: it would be better to count up changeRect inside
            // node's extent() method
            currentRect |= rootNode->projectionPlane()->changeRect(rootNode->exactBounds());

            return currentRect;
        }

        void refreshHiddenAreaAsync(KisNodeSP rootNode, const QRect &preparedArea) {
            QRect realNodeRect = realNodeExactBounds(rootNode);
            if (!preparedArea.contains(realNodeRect)) {

                QRegion dirtyRegion = realNodeRect;
                dirtyRegion -= preparedArea;

                foreach(const QRect &rc, dirtyRegion.rects()) {
                    m_info->image->refreshGraphAsync(rootNode, rc, realNodeRect);
                }
            }
        }
    private:
        MergeDownInfoBaseSP m_info;
    };

    struct KeepMergedNodesSelected : public KisCommandUtils::AggregateCommand {
        KeepMergedNodesSelected(MergeDownInfoSP info, bool finalizing)
            : m_singleInfo(info),
              m_finalizing(finalizing) {}

        KeepMergedNodesSelected(MergeMultipleInfoSP info, KisNodeSP putAfter, bool finalizing)
            : m_multipleInfo(info),
              m_finalizing(finalizing),
              m_putAfter(putAfter) {}

        void populateChildCommands() override {
            KisNodeSP prevNode;
            KisNodeSP nextNode;
            KisNodeList prevSelection;
            KisNodeList nextSelection;
            KisImageSP image;

            if (m_singleInfo) {
                prevNode = m_singleInfo->currLayer;
                nextNode = m_singleInfo->dstNode;
                image = m_singleInfo->image;
            } else if (m_multipleInfo) {
                prevNode = m_putAfter;
                nextNode = m_multipleInfo->dstNode;
                prevSelection = m_multipleInfo->allSrcNodes();
                image = m_multipleInfo->image;
            }

            if (!m_finalizing) {
                addCommand(new KeepNodesSelectedCommand(prevSelection, KisNodeList(),
                                                        prevNode, KisNodeSP(),
                                                        image, false));
            } else {
                addCommand(new KeepNodesSelectedCommand(KisNodeList(), nextSelection,
                                                        KisNodeSP(), nextNode,
                                                        image, true));
            }
        }

    private:
        MergeDownInfoSP m_singleInfo;
        MergeMultipleInfoSP m_multipleInfo;
        bool m_finalizing;
        KisNodeSP m_putAfter;
    };

    struct CreateMergedLayer : public KisCommandUtils::AggregateCommand {
        CreateMergedLayer(MergeDownInfoSP info) : m_info(info) {}

        void populateChildCommands() override {
            // actual merging done by KisLayer::createMergedLayer (or specialized decendant)
            m_info->dstNode = m_info->currLayer->createMergedLayerTemplate(m_info->prevLayer);

            if (m_info->frames.size() > 0) {
                m_info->dstNode->enableAnimation();
                m_info->dstNode->getKeyframeChannel(KisKeyframeChannel::Content.id(), true);
            }
        }

    private:
        MergeDownInfoSP m_info;
    };

    struct CreateMergedLayerMultiple : public KisCommandUtils::AggregateCommand {
        CreateMergedLayerMultiple(MergeMultipleInfoSP info, const QString name = QString() ) 
            : m_info(info),
              m_name(name) {}

        void populateChildCommands() override {
            QString mergedLayerName;
            
            if (m_name.isEmpty()){
                const QString mergedLayerSuffix = i18n("Merged");
                mergedLayerName = m_info->mergedNodes.first()->name();

                if (!mergedLayerName.endsWith(mergedLayerSuffix)) {
                    mergedLayerName = QString("%1 %2")
                        .arg(mergedLayerName).arg(mergedLayerSuffix);
                }
            } else {
                mergedLayerName = m_name;
            }
                
            m_info->dstNode = new KisPaintLayer(m_info->image, mergedLayerName, OPACITY_OPAQUE_U8);

            if (m_info->frames.size() > 0) {
                m_info->dstNode->enableAnimation();
                m_info->dstNode->getKeyframeChannel(KisKeyframeChannel::Content.id(), true);
            }

            QString compositeOpId;
            QBitArray channelFlags;
            bool compositionVaries = false;

            foreach (KisNodeSP node, m_info->allSrcNodes()) {
                if (compositeOpId.isEmpty()) {
                    compositeOpId = node->compositeOpId();
                } else if (compositeOpId != node->compositeOpId()) {
                    compositionVaries = true;
                    break;
                }

                KisLayerSP layer = dynamic_cast<KisLayer*>(node.data());
                if (layer && layer->layerStyle()) {
                    compositionVaries = true;
                    break;
                }
            }

            if (!compositionVaries) {
                if (!compositeOpId.isEmpty()) {
                    m_info->dstNode->setCompositeOpId(compositeOpId);
                }
                if (m_info->dstLayer() && !channelFlags.isEmpty()) {
                    m_info->dstLayer()->setChannelFlags(channelFlags);
                }
            }
        }

    private:
        MergeMultipleInfoSP m_info;
        QString m_name;
    };

    struct MergeLayers : public KisCommandUtils::AggregateCommand {
        MergeLayers(MergeDownInfoSP info) : m_info(info) {}

        void populateChildCommands() override {
            // actual merging done by KisLayer::createMergedLayer (or specialized decendant)
            m_info->currLayer->fillMergedLayerTemplate(m_info->dstLayer(), m_info->prevLayer);
        }

    private:
        MergeDownInfoSP m_info;
    };

    struct MergeLayersMultiple : public KisCommandUtils::AggregateCommand {
        MergeLayersMultiple(MergeMultipleInfoSP info) : m_info(info) {}

        void populateChildCommands() override {
            KisPainter gc(m_info->dstNode->paintDevice());

            foreach (KisNodeSP node, m_info->allSrcNodes()) {
                QRect rc = node->exactBounds() | m_info->image->bounds();
                node->projectionPlane()->apply(&gc, rc);
            }
        }

    private:
        MergeMultipleInfoSP m_info;
    };

    struct MergeMetaData : public KUndo2Command {
        MergeMetaData(MergeDownInfoSP info, const KisMetaData::MergeStrategy* strategy)
            : m_info(info),
              m_strategy(strategy) {}

        void redo() override {
            QRect layerProjectionExtent = m_info->currLayer->projection()->extent();
            QRect prevLayerProjectionExtent = m_info->prevLayer->projection()->extent();
            int prevLayerArea = prevLayerProjectionExtent.width() * prevLayerProjectionExtent.height();
            int layerArea = layerProjectionExtent.width() * layerProjectionExtent.height();

            QList<double> scores;
            double norm = qMax(prevLayerArea, layerArea);
            scores.append(prevLayerArea / norm);
            scores.append(layerArea / norm);

            QList<const KisMetaData::Store*> srcs;
            srcs.append(m_info->prevLayer->metaData());
            srcs.append(m_info->currLayer->metaData());
            m_strategy->merge(m_info->dstLayer()->metaData(), srcs, scores);
        }

    private:
        MergeDownInfoSP m_info;
        const KisMetaData::MergeStrategy *m_strategy;
    };

    KeepNodesSelectedCommand::KeepNodesSelectedCommand(const KisNodeList &selectedBefore,
                                                       const KisNodeList &selectedAfter,
                                                       KisNodeSP activeBefore,
                                                       KisNodeSP activeAfter,
                                                       KisImageSP image,
                                                       bool finalize, KUndo2Command *parent)
        : FlipFlopCommand(finalize, parent),
          m_selectedBefore(selectedBefore),
          m_selectedAfter(selectedAfter),
          m_activeBefore(activeBefore),
          m_activeAfter(activeAfter),
          m_image(image)
    {
    }

    void KeepNodesSelectedCommand::end() {
        KisImageSignalType type;
        if (isFinalizing()) {
            type = ComplexNodeReselectionSignal(m_activeAfter, m_selectedAfter);
        } else {
            type = ComplexNodeReselectionSignal(m_activeBefore, m_selectedBefore);
        }
        m_image->signalRouter()->emitNotification(type);
    }

    KisLayerSP constructDefaultLayer(KisImageSP image) {
        return new KisPaintLayer(image.data(), image->nextLayerName(), OPACITY_OPAQUE_U8, image->colorSpace());
    }

    RemoveNodeHelper::~RemoveNodeHelper()
    {
    }

    /**
     * The removal of two nodes in one go may be a bit tricky, because one
     * of them may be the clone of another. If we remove the source of a
     * clone layer, it will reincarnate into a paint layer. In this case
     * the pointer to the second layer will be lost.
     *
     * That's why we need to care about the order of the nodes removal:
     * the clone --- first, the source --- last.
     */
    void RemoveNodeHelper::safeRemoveMultipleNodes(KisNodeList nodes, KisImageSP image) {
        const bool lastLayer = scanForLastLayer(image, nodes);

        while (!nodes.isEmpty()) {
            KisNodeList::iterator it = nodes.begin();

            while (it != nodes.end()) {
                if (!checkIsSourceForClone(*it, nodes)) {
                    KisNodeSP node = *it;
                    addCommandImpl(new KisImageLayerRemoveCommand(image, node, false, true));
                    it = nodes.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (lastLayer) {
            KisLayerSP newLayer = constructDefaultLayer(image);
            addCommandImpl(new KisImageLayerAddCommand(image, newLayer,
                                                       image->root(),
                                                       KisNodeSP(),
                                                       false, false));
        }
    }

    bool RemoveNodeHelper::checkIsSourceForClone(KisNodeSP src, const KisNodeList &nodes) {
        foreach (KisNodeSP node, nodes) {
            if (node == src) continue;

            KisCloneLayer *clone = dynamic_cast<KisCloneLayer*>(node.data());

            if (clone && KisNodeSP(clone->copyFrom()) == src) {
                return true;
            }
        }

        return false;
    }

    bool RemoveNodeHelper::scanForLastLayer(KisImageWSP image, KisNodeList nodesToRemove) {
        bool removeLayers = false;
        Q_FOREACH(KisNodeSP nodeToRemove, nodesToRemove) {
            if (dynamic_cast<KisLayer*>(nodeToRemove.data())) {
                removeLayers = true;
                break;
            }
        }
        if (!removeLayers) return false;

        bool lastLayer = true;
        KisNodeSP node = image->root()->firstChild();
        while (node) {
            if (!nodesToRemove.contains(node) &&
                dynamic_cast<KisLayer*>(node.data())) {

                lastLayer = false;
                break;
            }
            node = node->nextSibling();
        }

        return lastLayer;
    }

    SimpleRemoveLayers::SimpleRemoveLayers(const KisNodeList &nodes,
                                           KisImageSP image)
        : m_nodes(nodes),
          m_image(image)
    {
    }

    void SimpleRemoveLayers::populateChildCommands() {
        if (m_nodes.isEmpty()) return;
        safeRemoveMultipleNodes(m_nodes, m_image);
    }

    void SimpleRemoveLayers::addCommandImpl(KUndo2Command *cmd) {
        addCommand(cmd);
    }

    struct InsertNode : public KisCommandUtils::AggregateCommand {
        InsertNode(MergeDownInfoBaseSP info, KisNodeSP putAfter)
            : m_info(info), m_putAfter(putAfter) {}
        
        void populateChildCommands() override {
            addCommand(new KisImageLayerAddCommand(m_info->image,
                                                           m_info->dstNode,
                                                           m_putAfter->parent(),
                                                           m_putAfter,
                                                           true, false));
        
        }

    private:
        virtual void addCommandImpl(KUndo2Command *cmd) {
            addCommand(cmd);
        }

    private:
        MergeDownInfoBaseSP m_info;
        KisNodeSP m_putAfter;
    };


    struct CleanUpNodes : private RemoveNodeHelper, public KisCommandUtils::AggregateCommand {
        CleanUpNodes(MergeDownInfoBaseSP info, KisNodeSP putAfter)
            : m_info(info), m_putAfter(putAfter) {}

        static void findPerfectParent(KisNodeList nodesToDelete, KisNodeSP &putAfter, KisNodeSP &parent) {
            if (!putAfter) {
                putAfter = nodesToDelete.last();
            }

            // Add the new merged node on top of the active node -- checking
            // whether the parent is going to be deleted
            parent = putAfter->parent();
            while (parent && nodesToDelete.contains(parent)) {
                parent = parent->parent();
            }
        }

        void populateChildCommands() override {
            KisNodeList nodesToDelete = m_info->allSrcNodes();

            KisNodeSP parent;
            findPerfectParent(nodesToDelete, m_putAfter, parent);

            if (!parent) {
                KisNodeSP oldRoot = m_info->image->root();
                KisNodeSP newRoot =
                    new KisGroupLayer(m_info->image, "root", OPACITY_OPAQUE_U8);

                addCommand(new KisImageLayerAddCommand(m_info->image,
                                                       m_info->dstNode,
                                                       newRoot,
                                                       KisNodeSP(),
                                                       true, false));
                addCommand(new KisImageChangeLayersCommand(m_info->image, oldRoot, newRoot));

            }
            else {
                if (parent == m_putAfter->parent()) {
                    addCommand(new KisImageLayerAddCommand(m_info->image,
                                                           m_info->dstNode,
                                                           parent,
                                                           m_putAfter,
                                                           true, false));
                }
                else {
                    addCommand(new KisImageLayerAddCommand(m_info->image,
                                                           m_info->dstNode,
                                                           parent,
                                                           parent->lastChild(),
                                                           true, false));
                }

                reparentSelectionMasks(m_info->image,
                                       m_info->dstLayer(),
                                       m_info->selectionMasks);

                safeRemoveMultipleNodes(m_info->allSrcNodes(), m_info->image);
            }


        }

    private:
        void addCommandImpl(KUndo2Command *cmd) override {
            addCommand(cmd);
        }

        void reparentSelectionMasks(KisImageSP image,
                                    KisLayerSP newLayer,
                                    const QVector<KisSelectionMaskSP> &selectionMasks) {

            foreach (KisSelectionMaskSP mask, selectionMasks) {
                addCommand(new KisImageLayerMoveCommand(image, mask, newLayer, newLayer->lastChild()));
                addCommand(new KisActivateSelectionMaskCommand(mask, false));
            }
        }
    private:
        MergeDownInfoBaseSP m_info;
        KisNodeSP m_putAfter;
    };

    SwitchFrameCommand::SharedStorage::~SharedStorage() {
    }

    SwitchFrameCommand::SwitchFrameCommand(KisImageSP image, int time, bool finalize, SharedStorageSP storage)
        : FlipFlopCommand(finalize),
          m_image(image),
          m_newTime(time),
          m_storage(storage) {}

    SwitchFrameCommand::~SwitchFrameCommand() {}

    void SwitchFrameCommand::init() {
        KisImageAnimationInterface *interface = m_image->animationInterface();
        const int currentTime = interface->currentTime();
        if (currentTime == m_newTime) {
            m_storage->value = m_newTime;
            return;
        }

        interface->image()->disableUIUpdates();
        interface->saveAndResetCurrentTime(m_newTime, &m_storage->value);
    }

    void SwitchFrameCommand::end() {
        KisImageAnimationInterface *interface = m_image->animationInterface();
        const int currentTime = interface->currentTime();
        if (currentTime == m_storage->value) {
            return;
        }

        interface->restoreCurrentTime(&m_storage->value);
        interface->image()->enableUIUpdates();
    }

    struct AddNewFrame : public KisCommandUtils::AggregateCommand {
        AddNewFrame(MergeDownInfoBaseSP info, int frame) : m_info(info), m_frame(frame) {}

        void populateChildCommands() override {
            KUndo2Command *cmd = new KisCommandUtils::SkipFirstRedoWrapper();
            KisKeyframeChannel *channel = m_info->dstNode->getKeyframeChannel(KisKeyframeChannel::Content.id());
            channel->addKeyframe(m_frame, cmd);

            addCommand(cmd);
        }

    private:
        MergeDownInfoBaseSP m_info;
        int m_frame;
    };

    QSet<int> fetchLayerFrames(KisNodeSP node) {
        KisKeyframeChannel *channel = node->getKeyframeChannel(KisKeyframeChannel::Content.id());
        if (!channel) return QSet<int>();

        return channel->allKeyframeIds();
    }

    QSet<int> fetchLayerFramesRecursive(KisNodeSP rootNode) {
        QSet<int> frames = fetchLayerFrames(rootNode);

        KisNodeSP node = rootNode->firstChild();
        while(node) {
            frames |= fetchLayerFramesRecursive(node);
            node = node->nextSibling();
        }

        return frames;
    }

    void updateFrameJobs(FrameJobs *jobs, KisNodeSP node) {
        QSet<int> frames = fetchLayerFrames(node);

        if (frames.isEmpty()) {
            (*jobs)[0].insert(node);
        } else {
            foreach (int frame, frames) {
                (*jobs)[frame].insert(node);
            }
        }
    }

    void updateFrameJobsRecursive(FrameJobs *jobs, KisNodeSP rootNode) {
        updateFrameJobs(jobs, rootNode);

        KisNodeSP node = rootNode->firstChild();
        while(node) {
            updateFrameJobsRecursive(jobs, node);
            node = node->nextSibling();
        }
    }

    void mergeDown(KisImageSP image, KisLayerSP layer, const KisMetaData::MergeStrategy* strategy)
    {
        if (!layer->prevSibling()) return;

        // XXX: this breaks if we allow free mixing of masks and layers
        KisLayerSP prevLayer = dynamic_cast<KisLayer*>(layer->prevSibling().data());
        if (!prevLayer) return;

        if (!layer->visible() && !prevLayer->visible()) {
            return;
        }

        KisImageSignalVector emitSignals;
        emitSignals << ModifiedSignal;

        KisProcessingApplicator applicator(image, 0,
                                           KisProcessingApplicator::NONE,
                                           emitSignals,
                                           kundo2_i18n("Merge Down"));

        if (layer->visible() && prevLayer->visible()) {
            MergeDownInfoSP info(new MergeDownInfo(image, prevLayer, layer));

            // disable key strokes on all colorize masks, all onion skins on
            // paint layers and wait until update is finished with a barrier
            applicator.applyCommand(new DisableColorizeKeyStrokes(info));
            applicator.applyCommand(new DisableOnionSkins(info));
            applicator.applyCommand(new KUndo2Command(), KisStrokeJobData::BARRIER);

            applicator.applyCommand(new KeepMergedNodesSelected(info, false));
            applicator.applyCommand(new FillSelectionMasks(info));
            applicator.applyCommand(new CreateMergedLayer(info), KisStrokeJobData::BARRIER);

            // in two-layer mode we disable pass trhough only when the destination layer
            // is not a group layer
            applicator.applyCommand(new DisablePassThroughForHeadsOnly(info, true));
            applicator.applyCommand(new KUndo2Command(), KisStrokeJobData::BARRIER);

            if (info->frames.size() > 0) {
                foreach (int frame, info->frames) {
                    applicator.applyCommand(new SwitchFrameCommand(info->image, frame, false, info->storage));

                    applicator.applyCommand(new AddNewFrame(info, frame));
                    applicator.applyCommand(new RefreshHiddenAreas(info));
                    applicator.applyCommand(new MergeLayers(info), KisStrokeJobData::BARRIER);

                    applicator.applyCommand(new SwitchFrameCommand(info->image, frame, true, info->storage));
                }
            } else {
                applicator.applyCommand(new RefreshHiddenAreas(info));
                applicator.applyCommand(new MergeLayers(info), KisStrokeJobData::BARRIER);
            }

            applicator.applyCommand(new MergeMetaData(info, strategy), KisStrokeJobData::BARRIER);
            applicator.applyCommand(new CleanUpNodes(info, layer),
                                    KisStrokeJobData::SEQUENTIAL,
                                    KisStrokeJobData::EXCLUSIVE);
            applicator.applyCommand(new KeepMergedNodesSelected(info, true));
        } else if (layer->visible()) {
            applicator.applyCommand(new KeepNodesSelectedCommand(KisNodeList(), KisNodeList(),
                                                                 layer, KisNodeSP(),
                                                                 image, false));

            applicator.applyCommand(
                new SimpleRemoveLayers(KisNodeList() << prevLayer,
                                       image),
                KisStrokeJobData::SEQUENTIAL,
                KisStrokeJobData::EXCLUSIVE);

            applicator.applyCommand(new KeepNodesSelectedCommand(KisNodeList(), KisNodeList(),
                                                                 KisNodeSP(), layer,
                                                                 image, true));
        } else if (prevLayer->visible()) {
            applicator.applyCommand(new KeepNodesSelectedCommand(KisNodeList(), KisNodeList(),
                                                                 layer, KisNodeSP(),
                                                                 image, false));

            applicator.applyCommand(
                new SimpleRemoveLayers(KisNodeList() << layer,
                                       image),
                KisStrokeJobData::SEQUENTIAL,
                KisStrokeJobData::EXCLUSIVE);

            applicator.applyCommand(new KeepNodesSelectedCommand(KisNodeList(), KisNodeList(),
                                                                 KisNodeSP(), prevLayer,
                                                                 image, true));
        }

        applicator.end();
    }

    bool checkIsChildOf(KisNodeSP node, const KisNodeList &parents)
    {
        KisNodeList nodeParents;

        KisNodeSP parent = node->parent();
        while (parent) {
            nodeParents << parent;
            parent = parent->parent();
        }

        foreach(KisNodeSP perspectiveParent, parents) {
            if (nodeParents.contains(perspectiveParent)) {
                return true;
            }
        }

        return false;
    }

    bool checkIsCloneOf(KisNodeSP node, const KisNodeList &nodes)
    {
        bool result = false;

        KisCloneLayer *clone = dynamic_cast<KisCloneLayer*>(node.data());
        if (clone) {
            KisNodeSP cloneSource = KisNodeSP(clone->copyFrom());

            Q_FOREACH(KisNodeSP subtree, nodes) {
                result =
                    recursiveFindNode(subtree,
                                      [cloneSource](KisNodeSP node) -> bool
                                      {
                                          return node == cloneSource;
                                      });

                if (!result) {
                    result = checkIsCloneOf(cloneSource, nodes);
                }

                if (result) {
                    break;
                }
            }
        }

        return result;
    }

    void filterMergableNodes(KisNodeList &nodes, bool allowMasks)
    {
        KisNodeList::iterator it = nodes.begin();

        while (it != nodes.end()) {
            if ((!allowMasks && !dynamic_cast<KisLayer*>(it->data())) ||
                checkIsChildOf(*it, nodes)) {

                qDebug() << "Skipping node" << ppVar((*it)->name());
                it = nodes.erase(it);
            } else {
                ++it;
            }
        }
    }

    void sortMergableNodes(KisNodeSP root, KisNodeList &inputNodes, KisNodeList &outputNodes)
    {
        KisNodeList::iterator it = std::find(inputNodes.begin(), inputNodes.end(), root);

        if (it != inputNodes.end()) {
            outputNodes << *it;
            inputNodes.erase(it);
        }

        if (inputNodes.isEmpty()) {
            return;
        }

        KisNodeSP child = root->firstChild();
        while (child) {
            sortMergableNodes(child, inputNodes, outputNodes);
            child = child->nextSibling();
        }

        /**
         * By the end of recursion \p inputNodes must be empty
         */
        KIS_ASSERT_RECOVER_NOOP(root->parent() || inputNodes.isEmpty());
    }

    KisNodeList sortMergableNodes(KisNodeSP root, KisNodeList nodes)
    {
        KisNodeList result;
        sortMergableNodes(root, nodes, result);
        return result;
    }

    KisNodeList sortAndFilterMergableInternalNodes(KisNodeList nodes, bool allowMasks)
    {
        KIS_ASSERT_RECOVER(!nodes.isEmpty()) { return nodes; }

        KisNodeSP root;
        Q_FOREACH(KisNodeSP node, nodes) {
            KisNodeSP localRoot = node;
            while (localRoot->parent()) {
                localRoot = localRoot->parent();
            }

            if (!root) {
                root = localRoot;
            }
            KIS_ASSERT_RECOVER(root == localRoot) { return nodes; }
        }

        KisNodeList result;
        sortMergableNodes(root, nodes, result);
        filterMergableNodes(result, allowMasks);
        return result;
    }

    void addCopyOfNameTag(KisNodeSP node)
    {
        const QString prefix = i18n("Copy of");
        QString newName = node->name();
        if (!newName.startsWith(prefix)) {
            newName = QString("%1 %2").arg(prefix).arg(newName);
            node->setName(newName);
        }
    }

    KisNodeList findNodesWithProps(KisNodeSP root, const KoProperties &props, bool excludeRoot)
    {
        KisNodeList nodes;

        if ((!excludeRoot || root->parent()) && root->check(props)) {
            nodes << root;
        }

        KisNodeSP node = root->firstChild();
        while (node) {
            nodes += findNodesWithProps(node, props, excludeRoot);
            node = node->nextSibling();
        }

        return nodes;
    }

    KisNodeList filterInvisibleNodes(const KisNodeList &nodes, KisNodeList *invisibleNodes, KisNodeSP *putAfter)
    {
        KIS_ASSERT_RECOVER(invisibleNodes) { return nodes; }
        KIS_ASSERT_RECOVER(putAfter) { return nodes; }

        KisNodeList visibleNodes;
        int putAfterIndex = -1;

        Q_FOREACH(KisNodeSP node, nodes) {
            if (node->visible()) {
                visibleNodes << node;
            } else {
                *invisibleNodes << node;

                if (node == *putAfter) {
                    putAfterIndex = visibleNodes.size() - 1;
                }
            }
        }

        if (!visibleNodes.isEmpty() && putAfterIndex >= 0) {
            putAfterIndex = qBound(0, putAfterIndex, visibleNodes.size() - 1);
            *putAfter = visibleNodes[putAfterIndex];
        }

        return visibleNodes;
    }

    void changeImageDefaultProjectionColor(KisImageSP image, const KoColor &color)
    {
        KisImageSignalVector emitSignals;
        emitSignals << ModifiedSignal;

        KisProcessingApplicator applicator(image,
                                           image->root(),
                                           KisProcessingApplicator::RECURSIVE,
                                           emitSignals,
                                           kundo2_i18n("Change projection color"),
                                           0,
                                           142857 + 1);
        applicator.applyCommand(new KisChangeProjectionColorCommand(image, color), KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE);
        applicator.end();
    }

    void mergeMultipleLayersImpl(KisImageSP image, KisNodeList mergedNodes, KisNodeSP putAfter, 
                                           bool flattenSingleLayer, const KUndo2MagicString &actionName, 
                                           bool cleanupNodes = true, const QString layerName = QString())
    {
        filterMergableNodes(mergedNodes);
        {
            KisNodeList tempNodes;
            qSwap(mergedNodes, tempNodes);
            sortMergableNodes(image->root(), tempNodes, mergedNodes);
        }

        if (mergedNodes.size() <= 1 &&
            (!flattenSingleLayer && mergedNodes.size() == 1)) return;

        KisImageSignalVector emitSignals;
        emitSignals << ModifiedSignal;
        emitSignals << ComplexNodeReselectionSignal(KisNodeSP(), KisNodeList(), KisNodeSP(), mergedNodes);

        KisProcessingApplicator applicator(image, 0,
                                           KisProcessingApplicator::NONE,
                                           emitSignals,
                                           actionName);


        KisNodeList originalNodes = mergedNodes;
        KisNodeList invisibleNodes;
        mergedNodes = filterInvisibleNodes(originalNodes, &invisibleNodes, &putAfter);

        if (!invisibleNodes.isEmpty()) {
            applicator.applyCommand(
                new SimpleRemoveLayers(invisibleNodes,
                                       image),
                KisStrokeJobData::SEQUENTIAL,
                KisStrokeJobData::EXCLUSIVE);
        }

        if (mergedNodes.size() > 1 || invisibleNodes.isEmpty()) {
            MergeMultipleInfoSP info(new MergeMultipleInfo(image, mergedNodes));

            // disable key strokes on all colorize masks, all onion skins on
            // paint layers and wait until update is finished with a barrier
            applicator.applyCommand(new DisableColorizeKeyStrokes(info));
            applicator.applyCommand(new DisableOnionSkins(info));
            applicator.applyCommand(new DisablePassThroughForHeadsOnly(info));
            applicator.applyCommand(new KUndo2Command(), KisStrokeJobData::BARRIER);

            applicator.applyCommand(new KeepMergedNodesSelected(info, putAfter, false));
            applicator.applyCommand(new FillSelectionMasks(info));
            applicator.applyCommand(new CreateMergedLayerMultiple(info, layerName), KisStrokeJobData::BARRIER);

            if (info->frames.size() > 0) {
                foreach (int frame, info->frames) {
                    applicator.applyCommand(new SwitchFrameCommand(info->image, frame, false, info->storage));

                    applicator.applyCommand(new AddNewFrame(info, frame));
                    applicator.applyCommand(new RefreshHiddenAreas(info));
                    applicator.applyCommand(new MergeLayersMultiple(info), KisStrokeJobData::BARRIER);

                    applicator.applyCommand(new SwitchFrameCommand(info->image, frame, true, info->storage));
                }
            } else {
                applicator.applyCommand(new RefreshHiddenAreas(info));
                applicator.applyCommand(new MergeLayersMultiple(info), KisStrokeJobData::BARRIER);
            }

            //applicator.applyCommand(new MergeMetaData(info, strategy), KisStrokeJobData::BARRIER);
            if (cleanupNodes){
                applicator.applyCommand(new CleanUpNodes(info, putAfter),
                                            KisStrokeJobData::SEQUENTIAL,
                                        KisStrokeJobData::EXCLUSIVE);
            } else {
                applicator.applyCommand(new InsertNode(info, putAfter),
                                            KisStrokeJobData::SEQUENTIAL,
                                        KisStrokeJobData::EXCLUSIVE);
            }
            applicator.applyCommand(new KeepMergedNodesSelected(info, putAfter, true));
        }

        applicator.end();

    }

    void mergeMultipleLayers(KisImageSP image, KisNodeList mergedNodes, KisNodeSP putAfter)
    {
        mergeMultipleLayersImpl(image, mergedNodes, putAfter, false, kundo2_i18n("Merge Selected Nodes"));
    }
    
    void newLayerFromVisible(KisImageSP image, KisNodeSP putAfter)
    {
        KisNodeList mergedNodes;
        mergedNodes << image->root();

        mergeMultipleLayersImpl(image, mergedNodes, putAfter, true, kundo2_i18n("New From Visible"), false, i18nc("New layer created from all the visible layers", "Visible"));
    }

    struct MergeSelectionMasks : public KisCommandUtils::AggregateCommand {
        MergeSelectionMasks(MergeDownInfoBaseSP info, KisNodeSP putAfter)
            : m_info(info),
              m_putAfter(putAfter){}

        void populateChildCommands() override {
            KisNodeSP parent;
            CleanUpNodes::findPerfectParent(m_info->allSrcNodes(), m_putAfter, parent);

            KisLayerSP parentLayer;
            do {
                parentLayer = dynamic_cast<KisLayer*>(parent.data());

                parent = parent->parent();
            } while(!parentLayer && parent);

            KisSelectionSP selection = new KisSelection();

            foreach (KisNodeSP node, m_info->allSrcNodes()) {
                KisMaskSP mask = dynamic_cast<KisMask*>(node.data());
                if (!mask) continue;

                selection->pixelSelection()->applySelection(
                    mask->selection()->pixelSelection(), SELECTION_ADD);
            }

            KisSelectionMaskSP mergedMask = new KisSelectionMask(m_info->image);
            mergedMask->initSelection(parentLayer);
            mergedMask->setSelection(selection);

            m_info->dstNode = mergedMask;
        }

    private:
        MergeDownInfoBaseSP m_info;
        KisNodeSP m_putAfter;
    };

    struct ActivateSelectionMask : public KisCommandUtils::AggregateCommand {
        ActivateSelectionMask(MergeDownInfoBaseSP info)
            : m_info(info) {}

        void populateChildCommands() override {
            KisSelectionMaskSP mergedMask = dynamic_cast<KisSelectionMask*>(m_info->dstNode.data());
            addCommand(new KisActivateSelectionMaskCommand(mergedMask, true));
        }

    private:
        MergeDownInfoBaseSP m_info;
    };

    bool tryMergeSelectionMasks(KisImageSP image, KisNodeList mergedNodes, KisNodeSP putAfter)
    {
        QList<KisSelectionMaskSP> selectionMasks;

        for (auto it = mergedNodes.begin(); it != mergedNodes.end(); /*noop*/) {
            KisSelectionMaskSP mask = dynamic_cast<KisSelectionMask*>(it->data());
            if (!mask) {
                it = mergedNodes.erase(it);
            } else {
                selectionMasks.append(mask);
                ++it;
            }
        }

        if (mergedNodes.isEmpty()) return false;

        KisLayerSP parentLayer = dynamic_cast<KisLayer*>(selectionMasks.first()->parent().data());
        KIS_ASSERT_RECOVER(parentLayer) { return 0; }

        KisImageSignalVector emitSignals;
        emitSignals << ModifiedSignal;

        KisProcessingApplicator applicator(image, 0,
                                           KisProcessingApplicator::NONE,
                                           emitSignals,
                                           kundo2_i18n("Merge Selection Masks"));

        MergeMultipleInfoSP info(new MergeMultipleInfo(image, mergedNodes));


        applicator.applyCommand(new MergeSelectionMasks(info, putAfter));
        applicator.applyCommand(new CleanUpNodes(info, putAfter),
                                KisStrokeJobData::SEQUENTIAL,
                                KisStrokeJobData::EXCLUSIVE);
        applicator.applyCommand(new ActivateSelectionMask(info));
        applicator.end();

        return true;
    }

    void flattenLayer(KisImageSP image, KisLayerSP layer)
    {
        if (!layer->childCount() && !layer->layerStyle())
            return;

        KisNodeList mergedNodes;
        mergedNodes << layer;

        mergeMultipleLayersImpl(image, mergedNodes, layer, true, kundo2_i18n("Flatten Layer"));
    }

    void flattenImage(KisImageSP image)
    {
        KisNodeList mergedNodes;
        mergedNodes << image->root();

        mergeMultipleLayersImpl(image, mergedNodes, 0, true, kundo2_i18n("Flatten Image"));
    }

    KisSimpleUpdateCommand::KisSimpleUpdateCommand(KisNodeList nodes, bool finalize, KUndo2Command *parent)
        : FlipFlopCommand(finalize, parent),
          m_nodes(nodes)
    {
    }
    void KisSimpleUpdateCommand::end()
    {
        updateNodes(m_nodes);
    }
    void KisSimpleUpdateCommand::updateNodes(const KisNodeList &nodes)
    {
        Q_FOREACH(KisNodeSP node, nodes) {
            node->setDirty(node->extent());
        }
    }

    void recursiveApplyNodes(KisNodeSP node, std::function<void(KisNodeSP)> func)
    {
        func(node);

        node = node->firstChild();
        while (node) {
            recursiveApplyNodes(node, func);
            node = node->nextSibling();
        }
    }

    KisNodeSP recursiveFindNode(KisNodeSP node, std::function<bool(KisNodeSP)> func)
    {
        if (func(node)) {
            return node;
        }

        node = node->firstChild();
        while (node) {
            KisNodeSP resultNode = recursiveFindNode(node, func);
            if (resultNode) {
                return resultNode;
            }
            node = node->nextSibling();
        }

        return 0;
    }

    KisNodeSP findNodeByUuid(KisNodeSP root, const QUuid &uuid)
    {
        return recursiveFindNode(root,
            [uuid] (KisNodeSP node) {
                return node->uuid() == uuid;
            });
    }

}
