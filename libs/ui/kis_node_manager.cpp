/*
 *  Copyright (C) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_node_manager.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QSignalMapper>

#include <kactioncollection.h>

#include <QKeySequence>

#include <kis_icon.h>
#include <KoSelection.h>
#include <KoShapeManager.h>
#include <KoShape.h>
#include <KoShapeLayer.h>
#include <KisImportExportManager.h>
#include <KoFileDialog.h>
#include <KoToolManager.h>
#include <KoProperties.h>


#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>

#include <kis_types.h>
#include <kis_node.h>
#include <kis_selection.h>
#include <kis_selection_mask.h>
#include <kis_layer.h>
#include <kis_mask.h>
#include <kis_image.h>
#include <kis_painter.h>
#include <kis_paint_layer.h>
#include <KisMimeDatabase.h>

#include "KisPart.h"
#include "canvas/kis_canvas2.h"
#include "kis_shape_controller.h"
#include "kis_canvas_resource_provider.h"
#include "KisViewManager.h"
#include "KisDocument.h"
#include "kis_mask_manager.h"
#include "kis_group_layer.h"
#include "kis_layer_manager.h"
#include "kis_selection_manager.h"
#include "kis_node_commands_adapter.h"
#include "kis_action.h"
#include "kis_action_manager.h"
#include "kis_processing_applicator.h"
#include "kis_sequential_iterator.h"
#include "kis_transaction.h"
#include "kis_node_selection_adapter.h"
#include "kis_node_insertion_adapter.h"
#include "kis_node_juggler_compressed.h"
#include "kis_clipboard.h"
#include "kis_node_dummies_graph.h"
#include "kis_mimedata.h"
#include "kis_layer_utils.h"
#include "krita_utils.h"

#include "processing/kis_mirror_processing_visitor.h"
#include "KisView.h"
#include "kis_config.h"

struct KisNodeManager::Private {

    Private(KisNodeManager *_q, KisViewManager *v)
        : q(_q)
        , view(v)
        , imageView(0)
        , layerManager(v)
        , maskManager(v)
        , commandsAdapter(v)
        , nodeSelectionAdapter(new KisNodeSelectionAdapter(q))
        , nodeInsertionAdapter(new KisNodeInsertionAdapter(q))
    {
    }

    KisNodeManager * q;
    KisViewManager * view;
    QPointer<KisView>imageView;
    KisLayerManager layerManager;
    KisMaskManager maskManager;
    KisNodeCommandsAdapter commandsAdapter;
    QScopedPointer<KisNodeSelectionAdapter> nodeSelectionAdapter;
    QScopedPointer<KisNodeInsertionAdapter> nodeInsertionAdapter;

    KisNodeList selectedNodes;
    QPointer<KisNodeJugglerCompressed> nodeJuggler;

    bool activateNodeImpl(KisNodeSP node);

    QSignalMapper nodeCreationSignalMapper;
    QSignalMapper nodeConversionSignalMapper;

    void saveDeviceAsImage(KisPaintDeviceSP device,
                           const QString &defaultName,
                           const QRect &bounds,
                           qreal xRes,
                           qreal yRes,
                           quint8 opacity);

    void mergeTransparencyMaskAsAlpha(bool writeToLayers);
    KisNodeJugglerCompressed* lazyGetJuggler(const KUndo2MagicString &actionName);
};

bool KisNodeManager::Private::activateNodeImpl(KisNodeSP node)
{
    Q_ASSERT(view);
    Q_ASSERT(view->canvasBase());
    Q_ASSERT(view->canvasBase()->globalShapeManager());
    Q_ASSERT(imageView);
    if (node && node == q->activeNode()) {
        return false;
    }

    // Set the selection on the shape manager to the active layer
    // and set call KoSelection::setActiveLayer( KoShapeLayer* layer )
    // with the parent of the active layer.
    KoSelection *selection = view->canvasBase()->globalShapeManager()->selection();
    Q_ASSERT(selection);
    selection->deselectAll();

    if (!node) {
        selection->setActiveLayer(0);
        imageView->setCurrentNode(0);
        maskManager.activateMask(0);
        layerManager.activateLayer(0);
    } else {

        KoShape * shape = view->document()->shapeForNode(node);
        KIS_ASSERT_RECOVER_RETURN_VALUE(shape, false);

        selection->select(shape);
        KoShapeLayer * shapeLayer = dynamic_cast<KoShapeLayer*>(shape);

        KIS_ASSERT_RECOVER_RETURN_VALUE(shapeLayer, false);

//         shapeLayer->setGeometryProtected(node->userLocked());
//         shapeLayer->setVisible(node->visible());
        selection->setActiveLayer(shapeLayer);

        imageView->setCurrentNode(node);
        if (KisLayerSP layer = dynamic_cast<KisLayer*>(node.data())) {
            maskManager.activateMask(0);
            layerManager.activateLayer(layer);
        } else if (KisMaskSP mask = dynamic_cast<KisMask*>(node.data())) {
            maskManager.activateMask(mask);
            // XXX_NODE: for now, masks cannot be nested.
            layerManager.activateLayer(static_cast<KisLayer*>(node->parent().data()));
        }

    }
    return true;
}

KisNodeManager::KisNodeManager(KisViewManager *view)
    : m_d(new Private(this, view))
{

    connect(&m_d->layerManager, SIGNAL(sigLayerActivated(KisLayerSP)), SIGNAL(sigLayerActivated(KisLayerSP)));
}

KisNodeManager::~KisNodeManager()
{
    delete m_d;
}

void KisNodeManager::setView(QPointer<KisView>imageView)
{
    m_d->maskManager.setView(imageView);
    m_d->layerManager.setView(imageView);

    if (m_d->imageView) {
        KisShapeController *shapeController = dynamic_cast<KisShapeController*>(m_d->imageView->document()->shapeController());
        Q_ASSERT(shapeController);
        shapeController->disconnect(SIGNAL(sigActivateNode(KisNodeSP)), this);
        m_d->imageView->image()->disconnect(this);
    }

    m_d->imageView = imageView;

    if (m_d->imageView) {
        KisShapeController *shapeController = dynamic_cast<KisShapeController*>(m_d->imageView->document()->shapeController());
        Q_ASSERT(shapeController);
        connect(shapeController, SIGNAL(sigActivateNode(KisNodeSP)), SLOT(slotNonUiActivatedNode(KisNodeSP)));
        connect(m_d->imageView->image(), SIGNAL(sigIsolatedModeChanged()),this, SLOT(slotUpdateIsolateModeAction()));
        connect(m_d->imageView->image(), SIGNAL(sigRequestNodeReselection(KisNodeSP, const KisNodeList&)),this, SLOT(slotImageRequestNodeReselection(KisNodeSP, const KisNodeList&)));
        m_d->imageView->resourceProvider()->slotNodeActivated(m_d->imageView->currentNode());
    }

}

#define NEW_LAYER_ACTION(id, layerType)                                 \
    {                                                                   \
        action = actionManager->createAction(id);                       \
        m_d->nodeCreationSignalMapper.setMapping(action, layerType);    \
        connect(action, SIGNAL(triggered()),                            \
                &m_d->nodeCreationSignalMapper, SLOT(map()));           \
    }

#define CONVERT_NODE_ACTION_2(id, layerType, exclude)                   \
    {                                                                   \
        action = actionManager->createAction(id);                       \
        action->setExcludedNodeTypes(QStringList(exclude));             \
        actionManager->addAction(id, action);                           \
        m_d->nodeConversionSignalMapper.setMapping(action, layerType);  \
        connect(action, SIGNAL(triggered()),                            \
                &m_d->nodeConversionSignalMapper, SLOT(map()));         \
    }

#define CONVERT_NODE_ACTION(id, layerType)              \
    CONVERT_NODE_ACTION_2(id, layerType, layerType)

void KisNodeManager::setup(KActionCollection * actionCollection, KisActionManager* actionManager)
{
    m_d->layerManager.setup(actionManager);
    m_d->maskManager.setup(actionCollection, actionManager);

    KisAction * action  = actionManager->createAction("mirrorNodeX");
    connect(action, SIGNAL(triggered()), this, SLOT(mirrorNodeX()));

    action  = actionManager->createAction("mirrorNodeY");
    connect(action, SIGNAL(triggered()), this, SLOT(mirrorNodeY()));

    action = actionManager->createAction("activateNextLayer");
    connect(action, SIGNAL(triggered()), this, SLOT(activateNextNode()));

    action = actionManager->createAction("activatePreviousLayer");
    connect(action, SIGNAL(triggered()), this, SLOT(activatePreviousNode()));

    action  = actionManager->createAction("save_node_as_image");
    connect(action, SIGNAL(triggered()), this, SLOT(saveNodeAsImage()));

    action = actionManager->createAction("duplicatelayer");
    connect(action, SIGNAL(triggered()), this, SLOT(duplicateActiveNode()));

    action = actionManager->createAction("copy_layer_clipboard");
    connect(action, SIGNAL(triggered()), this, SLOT(copyLayersToClipboard()));

    action = actionManager->createAction("cut_layer_clipboard");
    connect(action, SIGNAL(triggered()), this, SLOT(cutLayersToClipboard()));

    action = actionManager->createAction("paste_layer_from_clipboard");
    connect(action, SIGNAL(triggered()), this, SLOT(pasteLayersFromClipboard()));

    action = actionManager->createAction("create_quick_group");
    connect(action, SIGNAL(triggered()), this, SLOT(createQuickGroup()));

    action = actionManager->createAction("create_quick_clipping_group");
    connect(action, SIGNAL(triggered()), this, SLOT(createQuickClippingGroup()));

    action = actionManager->createAction("quick_ungroup");
    connect(action, SIGNAL(triggered()), this, SLOT(quickUngroup()));

    action = actionManager->createAction("select_all_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectAllNodes()));

    action = actionManager->createAction("select_visible_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectVisibleNodes()));

    action = actionManager->createAction("select_locked_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectLockedNodes()));

    action = actionManager->createAction("select_invisible_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectInvisibleNodes()));

    action = actionManager->createAction("select_unlocked_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectUnlockedNodes()));

    action = actionManager->createAction("new_from_visible");
    connect(action, SIGNAL(triggered()), this, SLOT(createFromVisible()));
    
    NEW_LAYER_ACTION("add_new_paint_layer", "KisPaintLayer");

    NEW_LAYER_ACTION("add_new_group_layer", "KisGroupLayer");

    NEW_LAYER_ACTION("add_new_clone_layer", "KisCloneLayer");

    NEW_LAYER_ACTION("add_new_shape_layer", "KisShapeLayer");

    NEW_LAYER_ACTION("add_new_adjustment_layer", "KisAdjustmentLayer");

    NEW_LAYER_ACTION("add_new_fill_layer", "KisGeneratorLayer");

    NEW_LAYER_ACTION("add_new_file_layer", "KisFileLayer");

    NEW_LAYER_ACTION("add_new_transparency_mask", "KisTransparencyMask");

    NEW_LAYER_ACTION("add_new_filter_mask", "KisFilterMask");

    if (!KisConfig().disableColorizeMaskFeature()) {
        NEW_LAYER_ACTION("add_new_colorize_mask", "KisColorizeMask");
    }

    NEW_LAYER_ACTION("add_new_transform_mask", "KisTransformMask");

    NEW_LAYER_ACTION("add_new_selection_mask", "KisSelectionMask");

    connect(&m_d->nodeCreationSignalMapper, SIGNAL(mapped(const QString &)),
            this, SLOT(createNode(const QString &)));

    CONVERT_NODE_ACTION("convert_to_paint_layer", "KisPaintLayer");

    CONVERT_NODE_ACTION_2("convert_to_selection_mask", "KisSelectionMask", QStringList() << "KisSelectionMask" << "KisColorizeMask");

    CONVERT_NODE_ACTION_2("convert_to_filter_mask", "KisFilterMask", QStringList() << "KisFilterMask" << "KisColorizeMask");

    CONVERT_NODE_ACTION_2("convert_to_transparency_mask", "KisTransparencyMask", QStringList() << "KisTransparencyMask" << "KisColorizeMask");

    CONVERT_NODE_ACTION("convert_to_animated", "animated");

    connect(&m_d->nodeConversionSignalMapper, SIGNAL(mapped(const QString &)),
            this, SLOT(convertNode(const QString &)));

    action = actionManager->createAction("isolate_layer");
    connect(action, SIGNAL(triggered(bool)), this, SLOT(toggleIsolateMode(bool)));

    action  = actionManager->createAction("split_alpha_into_mask");
    connect(action, SIGNAL(triggered()), this, SLOT(slotSplitAlphaIntoMask()));

    action  = actionManager->createAction("split_alpha_write");
    connect(action, SIGNAL(triggered()), this, SLOT(slotSplitAlphaWrite()));

    // HINT: we can save even when the nodes are not editable
    action  = actionManager->createAction("split_alpha_save_merged");
    connect(action, SIGNAL(triggered()), this, SLOT(slotSplitAlphaSaveMerged()));

    connect(this, SIGNAL(sigNodeActivated(KisNodeSP)), SLOT(slotUpdateIsolateModeAction()));
    connect(this, SIGNAL(sigNodeActivated(KisNodeSP)), SLOT(slotTryFinishIsolatedMode()));
}

void KisNodeManager::updateGUI()
{
    // enable/disable all relevant actions
    m_d->layerManager.updateGUI();
    m_d->maskManager.updateGUI();
}

KisNodeSP KisNodeManager::activeNode()
{
    if (m_d->imageView) {
        return m_d->imageView->currentNode();
    }
    return 0;
}

KisLayerSP KisNodeManager::activeLayer()
{
    return m_d->layerManager.activeLayer();
}

const KoColorSpace* KisNodeManager::activeColorSpace()
{
    if (m_d->maskManager.activeDevice()) {
        return m_d->maskManager.activeDevice()->colorSpace();
    } else {
        Q_ASSERT(m_d->layerManager.activeLayer());
        if (m_d->layerManager.activeLayer()->parentLayer())
            return m_d->layerManager.activeLayer()->parentLayer()->colorSpace();
        else
            return m_d->view->image()->colorSpace();
    }
}

void KisNodeManager::moveNodeAt(KisNodeSP node, KisNodeSP parent, int index)
{
    if (parent->allowAsChild(node)) {
        if (node->inherits("KisSelectionMask") && parent->inherits("KisLayer")) {
            KisSelectionMask *m = dynamic_cast<KisSelectionMask*>(node.data());
            KisLayer *l = dynamic_cast<KisLayer*>(parent.data());
            KisSelectionMaskSP selMask = l->selectionMask();
            if (m && m->active() && l && l->selectionMask())
                selMask->setActive(false);
        }
        m_d->commandsAdapter.moveNode(node, parent, index);
    }
}

void KisNodeManager::moveNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    KUndo2MagicString actionName = kundo2_i18n("Move Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->moveNode(nodes, parent, aboveThis);
}

void KisNodeManager::copyNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    KUndo2MagicString actionName = kundo2_i18n("Copy Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->copyNode(nodes, parent, aboveThis);
}

void KisNodeManager::addNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    KUndo2MagicString actionName = kundo2_i18n("Add Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->addNode(nodes, parent, aboveThis);
}

void KisNodeManager::toggleIsolateActiveNode()
{
    KisImageWSP image = m_d->view->image();
    KisNodeSP activeNode = this->activeNode();
    KIS_ASSERT_RECOVER_RETURN(activeNode);

    if (activeNode == image->isolatedModeRoot()) {
        toggleIsolateMode(false);
    } else {
        toggleIsolateMode(true);
    }
}

void KisNodeManager::toggleIsolateMode(bool checked)
{
    KisImageWSP image = m_d->view->image();

    if (checked) {
        KisNodeSP activeNode = this->activeNode();
        // Transform and colorize masks don't have pixel data...
        if (activeNode->inherits("KisTransformMask") ||
            activeNode->inherits("KisColorizeMask")) return;

        KIS_ASSERT_RECOVER_RETURN(activeNode);
        if (!image->startIsolatedMode(activeNode)) {
            KisAction *action = m_d->view->actionManager()->actionByName("isolate_layer");
            action->setChecked(false);
        }
    } else {
        image->stopIsolatedMode();
    }
}

void KisNodeManager::slotUpdateIsolateModeAction()
{
    KisAction *action = m_d->view->actionManager()->actionByName("isolate_layer");
    Q_ASSERT(action);

    KisNodeSP activeNode = this->activeNode();
    KisNodeSP isolatedRootNode = m_d->view->image()->isolatedModeRoot();

    action->setChecked(isolatedRootNode && isolatedRootNode == activeNode);
}

void KisNodeManager::slotTryFinishIsolatedMode()
{
    KisNodeSP isolatedRootNode = m_d->view->image()->isolatedModeRoot();
    if (!isolatedRootNode) return;

    this->toggleIsolateMode(true);
}

void KisNodeManager::createNode(const QString & nodeType, bool quiet, KisPaintDeviceSP copyFrom)
{
    if (!m_d->view->blockUntillOperationsFinished(m_d->view->image())) {
        return;
    }

    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) {
        activeNode = m_d->view->image()->root();
    }

    KIS_ASSERT_RECOVER_RETURN(activeNode);

    // XXX: make factories for this kind of stuff,
    //      with a registry

    if (nodeType == "KisPaintLayer") {
        m_d->layerManager.addLayer(activeNode);
    } else if (nodeType == "KisGroupLayer") {
        m_d->layerManager.addGroupLayer(activeNode);
    } else if (nodeType == "KisAdjustmentLayer") {
        m_d->layerManager.addAdjustmentLayer(activeNode);
    } else if (nodeType == "KisGeneratorLayer") {
        m_d->layerManager.addGeneratorLayer(activeNode);
    } else if (nodeType == "KisShapeLayer") {
        m_d->layerManager.addShapeLayer(activeNode);
    } else if (nodeType == "KisCloneLayer") {
        m_d->layerManager.addCloneLayer(activeNode);
    } else if (nodeType == "KisTransparencyMask") {
        m_d->maskManager.createTransparencyMask(activeNode, copyFrom, false);
    } else if (nodeType == "KisFilterMask") {
        m_d->maskManager.createFilterMask(activeNode, copyFrom, quiet, false);
    } else if (nodeType == "KisColorizeMask") {
        m_d->maskManager.createColorizeMask(activeNode);
    } else if (nodeType == "KisTransformMask") {
        m_d->maskManager.createTransformMask(activeNode);
    } else if (nodeType == "KisSelectionMask") {
        m_d->maskManager.createSelectionMask(activeNode, copyFrom, false);
    } else if (nodeType == "KisFileLayer") {
        m_d->layerManager.addFileLayer(activeNode);
    }

}

void KisNodeManager::createFromVisible()
{
    KisLayerUtils::newLayerFromVisible(m_d->view->image(), m_d->view->image()->root()->lastChild());
}

KisLayerSP KisNodeManager::createPaintLayer()
{
    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) {
        activeNode = m_d->view->image()->root();
    }

    return m_d->layerManager.addLayer(activeNode);
}

void KisNodeManager::convertNode(const QString &nodeType)
{
    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) return;

    if (nodeType == "KisPaintLayer") {
        m_d->layerManager.convertNodeToPaintLayer(activeNode);
    } else if (nodeType == "KisSelectionMask" ||
               nodeType == "KisFilterMask" ||
               nodeType == "KisTransparencyMask") {

        KisPaintDeviceSP copyFrom = activeNode->paintDevice() ?
            activeNode->paintDevice() : activeNode->projection();

        m_d->commandsAdapter.beginMacro(kundo2_i18n("Convert to a Selection Mask"));

        if (nodeType == "KisSelectionMask") {
            m_d->maskManager.createSelectionMask(activeNode, copyFrom, true);
        } else if (nodeType == "KisFilterMask") {
            m_d->maskManager.createFilterMask(activeNode, copyFrom, false, true);
        } else if (nodeType == "KisTransparencyMask") {
            m_d->maskManager.createTransparencyMask(activeNode, copyFrom, true);
        }

        m_d->commandsAdapter.removeNode(activeNode);
        m_d->commandsAdapter.endMacro();

    } else {
        warnKrita << "Unsupported node conversion type:" << nodeType;
    }
}

void KisNodeManager::slotSomethingActivatedNodeImpl(KisNodeSP node)
{
    KIS_ASSERT_RECOVER_RETURN(node != activeNode());
    if (m_d->activateNodeImpl(node)) {
        emit sigUiNeedChangeActiveNode(node);
        emit sigNodeActivated(node);
        nodesUpdated();
        if (node) {
            bool toggled =  m_d->view->actionCollection()->action("view_show_canvas_only")->isChecked();
            if (toggled) {
                m_d->view->showFloatingMessage( activeLayer()->name(), QIcon(), 1600, KisFloatingMessage::Medium, Qt::TextSingleLine);
            }
        }
    }
}

void KisNodeManager::slotNonUiActivatedNode(KisNodeSP node)
{
    if (node == activeNode()) return;
    slotSomethingActivatedNodeImpl(node);

    if (node) {
        bool toggled =  m_d->view->actionCollection()->action("view_show_canvas_only")->isChecked();
        if (toggled) {
            m_d->view->showFloatingMessage( activeLayer()->name(), QIcon(), 1600, KisFloatingMessage::Medium, Qt::TextSingleLine);
        }
    }
}

void KisNodeManager::slotUiActivatedNode(KisNodeSP node)
{
    if (node == activeNode()) return;

    slotSomethingActivatedNodeImpl(node);

    if (node) {
        QStringList vectorTools = QStringList()
                << "InteractionTool"
                << "KarbonPatternTool"
                << "KarbonGradientTool"
                << "KarbonCalligraphyTool"
                << "CreateShapesTool"
                << "PathTool";

        QStringList pixelTools = QStringList()
                << "KritaShape/KisToolBrush"
                << "KritaShape/KisToolDyna"
                << "KritaShape/KisToolMultiBrush"
                << "KritaFill/KisToolFill"
                << "KritaFill/KisToolGradient";

        if (node->inherits("KisShapeLayer")) {
            if (pixelTools.contains(KoToolManager::instance()->activeToolId())) {
                KoToolManager::instance()->switchToolRequested("InteractionTool");
            }
        }
        else {
            if (vectorTools.contains(KoToolManager::instance()->activeToolId())) {
                KoToolManager::instance()->switchToolRequested("KritaShape/KisToolBrush");
            }
        }
    }
}

void KisNodeManager::nodesUpdated()
{
    KisNodeSP node = activeNode();
    if (!node) return;

    m_d->layerManager.layersUpdated();
    m_d->maskManager.masksUpdated();

    m_d->view->updateGUI();
    m_d->view->selectionManager()->selectionChanged();

}

KisPaintDeviceSP KisNodeManager::activePaintDevice()
{
    return m_d->maskManager.activeMask() ?
        m_d->maskManager.activeDevice() :
        m_d->layerManager.activeDevice();
}

void KisNodeManager::nodeProperties(KisNodeSP node)
{
    if (selectedNodes().size() > 1 || node->inherits("KisLayer")) {
        m_d->layerManager.layerProperties();
    } else if (node->inherits("KisMask")) {
        m_d->maskManager.maskProperties();
    }
}

qint32 KisNodeManager::convertOpacityToInt(qreal opacity)
{
    /**
     * Scales opacity from the range 0...100
     * to the integer range 0...255
     */

    return qMin(255, int(opacity * 2.55 + 0.5));
}

void KisNodeManager::setNodeOpacity(KisNodeSP node, qint32 opacity,
                                    bool finalChange)
{
    if (!node) return;
    if (node->opacity() == opacity) return;

    if (!finalChange) {
        node->setOpacity(opacity);
        node->setDirty();
    } else {
        m_d->commandsAdapter.setOpacity(node, opacity);
    }
}

void KisNodeManager::setNodeCompositeOp(KisNodeSP node,
                                        const KoCompositeOp* compositeOp)
{
    if (!node) return;
    if (node->compositeOp() == compositeOp) return;

    m_d->commandsAdapter.setCompositeOp(node, compositeOp);
}

void KisNodeManager::slotImageRequestNodeReselection(KisNodeSP activeNode, const KisNodeList &selectedNodes)
{
    if (activeNode) {
        slotNonUiActivatedNode(activeNode);
    }
    if (!selectedNodes.isEmpty()) {
        slotSetSelectedNodes(selectedNodes);
    }
}

void KisNodeManager::slotSetSelectedNodes(const KisNodeList &nodes)
{
    m_d->selectedNodes = nodes;
    emit sigUiNeedChangeSelectedNodes(nodes);
}

KisNodeList KisNodeManager::selectedNodes()
{
    return m_d->selectedNodes;
}

KisNodeSelectionAdapter* KisNodeManager::nodeSelectionAdapter() const
{
    return m_d->nodeSelectionAdapter.data();
}

KisNodeInsertionAdapter* KisNodeManager::nodeInsertionAdapter() const
{
    return m_d->nodeInsertionAdapter.data();
}

void KisNodeManager::nodeOpacityChanged(qreal opacity, bool finalChange)
{
    KisNodeSP node = activeNode();

    setNodeOpacity(node, convertOpacityToInt(opacity), finalChange);
}

void KisNodeManager::nodeCompositeOpChanged(const KoCompositeOp* op)
{
    KisNodeSP node = activeNode();

    setNodeCompositeOp(node, op);
}

void KisNodeManager::duplicateActiveNode()
{
    KUndo2MagicString actionName = kundo2_i18n("Duplicate Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->duplicateNode(selectedNodes());
}

KisNodeJugglerCompressed* KisNodeManager::Private::lazyGetJuggler(const KUndo2MagicString &actionName)
{
    KisImageWSP image = view->image();

    if (!nodeJuggler ||
        (nodeJuggler &&
         !nodeJuggler->canMergeAction(actionName))) {

        nodeJuggler = new KisNodeJugglerCompressed(actionName, image, q, 750);
        nodeJuggler->setAutoDelete(true);
    }

    return nodeJuggler;
}

void KisNodeManager::raiseNode()
{
    KUndo2MagicString actionName = kundo2_i18n("Raise Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->raiseNode(selectedNodes());
}

void KisNodeManager::lowerNode()
{
    KUndo2MagicString actionName = kundo2_i18n("Lower Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->lowerNode(selectedNodes());
}

void KisNodeManager::removeSingleNode(KisNodeSP node)
{
    if (!node || !node->parent()) {
        return;
    }

    KisNodeList nodes;
    nodes << node;
    removeSelectedNodes(nodes);
}

void KisNodeManager::removeSelectedNodes(KisNodeList nodes)
{
    KUndo2MagicString actionName = kundo2_i18n("Remove Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->removeNode(nodes);
}

void KisNodeManager::removeNode()
{
    removeSelectedNodes(selectedNodes());
}

void KisNodeManager::mirrorNodeX()
{
    KisNodeSP node = activeNode();

    KUndo2MagicString commandName;
    if (node->inherits("KisLayer")) {
        commandName = kundo2_i18n("Mirror Layer X");
    } else if (node->inherits("KisMask")) {
        commandName = kundo2_i18n("Mirror Mask X");
    }
    mirrorNode(node, commandName, Qt::Horizontal);
}

void KisNodeManager::mirrorNodeY()
{
    KisNodeSP node = activeNode();

    KUndo2MagicString commandName;
    if (node->inherits("KisLayer")) {
        commandName = kundo2_i18n("Mirror Layer Y");
    } else if (node->inherits("KisMask")) {
        commandName = kundo2_i18n("Mirror Mask Y");
    }
    mirrorNode(node, commandName, Qt::Vertical);
}

inline bool checkForGlobalSelection(KisNodeSP node) {
    return dynamic_cast<KisSelectionMask*>(node.data()) && node->parent() && !node->parent()->parent();
}

void KisNodeManager::activateNextNode()
{
    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) return;

    KisNodeSP node = activeNode->nextSibling();

    while (node && node->childCount() > 0) {
           node = node->firstChild();
    }

    if (!node && activeNode->parent() && activeNode->parent()->parent()) {
        node = activeNode->parent();
    }

    while(node && checkForGlobalSelection(node)) {
        node = node->nextSibling();
    }

    if (node) {
        slotNonUiActivatedNode(node);
    }
}

void KisNodeManager::activatePreviousNode()
{
    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) return;

    KisNodeSP node;

    if (activeNode->childCount() > 0) {
        node = activeNode->lastChild();
    }
    else {
        node = activeNode->prevSibling();
    }

    while (!node && activeNode->parent()) {
        node = activeNode->parent()->prevSibling();
        activeNode = activeNode->parent();
    }

    while(node && checkForGlobalSelection(node)) {
        node = node->prevSibling();
    }

    if (node) {
        slotNonUiActivatedNode(node);
    }
}

void KisNodeManager::mergeLayer()
{
    m_d->layerManager.mergeLayer();
}

void KisNodeManager::rotate(double radians)
{
    // XXX: implement rotation for masks as well
    m_d->layerManager.rotateLayer(radians);

}

void KisNodeManager::rotate180()
{
    rotate(M_PI);
}

void KisNodeManager::rotateLeft90()
{
   rotate(-M_PI / 2);
}

void KisNodeManager::rotateRight90()
{
    rotate(M_PI / 2);
}

void KisNodeManager::shear(double angleX, double angleY)
{
    // XXX: implement shear for masks as well
    m_d->layerManager.shearLayer(angleX, angleY);
}

void KisNodeManager::scale(double sx, double sy, KisFilterStrategy *filterStrategy)
{
    KisNodeSP node = activeNode();
    KIS_ASSERT_RECOVER_RETURN(node);

    m_d->view->image()->scaleNode(node, sx, sy, filterStrategy);

    nodesUpdated();
}

void KisNodeManager::mirrorNode(KisNodeSP node, const KUndo2MagicString& actionName, Qt::Orientation orientation)
{
    KisImageSignalVector emitSignals;
    emitSignals << ModifiedSignal;

    KisProcessingApplicator applicator(m_d->view->image(), node,
                                       KisProcessingApplicator::RECURSIVE,
                                       emitSignals, actionName);

    KisProcessingVisitorSP visitor =
        new KisMirrorProcessingVisitor(m_d->view->image()->bounds(), orientation);

    applicator.applyVisitor(visitor, KisStrokeJobData::CONCURRENT);
    applicator.end();

    nodesUpdated();
}

void KisNodeManager::Private::saveDeviceAsImage(KisPaintDeviceSP device,
                                                const QString &defaultName,
                                                const QRect &bounds,
                                                qreal xRes,
                                                qreal yRes,
                                                quint8 opacity)
{
    KoFileDialog dialog(view->mainWindow(), KoFileDialog::SaveFile, "savenodeasimage");
    dialog.setCaption(i18n("Export \"%1\"", defaultName));
    dialog.setDefaultDir(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
    dialog.setMimeTypeFilters(KisImportExportManager::mimeFilter(KisImportExportManager::Export));
    QString filename = dialog.filename();

    if (filename.isEmpty()) return;

    QUrl url = QUrl::fromLocalFile(filename);

    if (url.isEmpty()) return;

    QString mimefilter = KisMimeDatabase::mimeTypeForFile(filename);;

    QScopedPointer<KisDocument> d(KisPart::instance()->createDocument());
    d->prepareForImport();

    KisImageSP dst = new KisImage(d->createUndoStore(),
                                  bounds.width(),
                                  bounds.height(),
                                  device->compositionSourceColorSpace(),
                                  defaultName);
    dst->setResolution(xRes, yRes);
    d->setCurrentImage(dst);
    KisPaintLayer* paintLayer = new KisPaintLayer(dst, "paint device", opacity);
    paintLayer->paintDevice()->makeCloneFrom(device, bounds);
    dst->addNode(paintLayer, dst->rootLayer(), KisLayerSP(0));

    dst->initialRefreshGraph();

    d->setOutputMimeType(mimefilter.toLatin1());
    d->exportDocument(url);
}

void KisNodeManager::saveNodeAsImage()
{
    KisNodeSP node = activeNode();

    if (!node) {
        warnKrita << "BUG: Save Node As Image was called without any node selected";
        return;
    }

    KisImageWSP image = m_d->view->image();
    QRect saveRect = image->bounds() | node->exactBounds();

    KisPaintDeviceSP device = node->paintDevice();
    if (!device) {
        device = node->projection();
    }

    m_d->saveDeviceAsImage(device, node->name(),
                           saveRect,
                           image->xRes(), image->yRes(),
                           node->opacity());
}

void KisNodeManager::slotSplitAlphaIntoMask()
{
    KisNodeSP node = activeNode();

    // guaranteed by KisActionManager
    KIS_ASSERT_RECOVER_RETURN(node->hasEditablePaintDevice());

    KisPaintDeviceSP srcDevice = node->paintDevice();
    const KoColorSpace *srcCS = srcDevice->colorSpace();
    const QRect processRect =
        srcDevice->exactBounds() |
        srcDevice->defaultBounds()->bounds();

    KisPaintDeviceSP selectionDevice =
        new KisPaintDevice(KoColorSpaceRegistry::instance()->alpha8());

    m_d->commandsAdapter.beginMacro(kundo2_i18n("Split Alpha into a Mask"));
    KisTransaction transaction(kundo2_noi18n("__split_alpha_channel__"), srcDevice);

    KisSequentialIterator srcIt(srcDevice, processRect);
    KisSequentialIterator dstIt(selectionDevice, processRect);

    do {
        quint8 *srcPtr = srcIt.rawData();
        quint8 *alpha8Ptr = dstIt.rawData();

        *alpha8Ptr = srcCS->opacityU8(srcPtr);
        srcCS->setOpacity(srcPtr, OPACITY_OPAQUE_U8, 1);
    } while (srcIt.nextPixel() && dstIt.nextPixel());

    m_d->commandsAdapter.addExtraCommand(transaction.endAndTake());

    createNode("KisTransparencyMask", false, selectionDevice);
    m_d->commandsAdapter.endMacro();
}

void KisNodeManager::Private::mergeTransparencyMaskAsAlpha(bool writeToLayers)
{
    KisNodeSP node = q->activeNode();
    KisNodeSP parentNode = node->parent();

    // guaranteed by KisActionManager
    KIS_ASSERT_RECOVER_RETURN(node->inherits("KisTransparencyMask"));

    if (writeToLayers && !parentNode->hasEditablePaintDevice()) {
        QMessageBox::information(view->mainWindow(),
                                 i18nc("@title:window", "Layer %1 is not editable").arg(parentNode->name()),
                                 i18n("Cannot write alpha channel of "
                                      "the parent layer \"%1\".\n"
                                      "The operation will be cancelled.").arg(parentNode->name()));
        return;
    }

    KisPaintDeviceSP dstDevice;
    if (writeToLayers) {
        KIS_ASSERT_RECOVER_RETURN(parentNode->paintDevice());
        dstDevice = parentNode->paintDevice();
    } else {
        KisPaintDeviceSP copyDevice = parentNode->paintDevice();
        if (!copyDevice) {
            copyDevice = parentNode->original();
        }
        dstDevice = new KisPaintDevice(*copyDevice);
    }

    const KoColorSpace *dstCS = dstDevice->colorSpace();

    KisPaintDeviceSP selectionDevice = node->paintDevice();
    KIS_ASSERT_RECOVER_RETURN(selectionDevice->colorSpace()->pixelSize() == 1);

    const QRect processRect =
        selectionDevice->exactBounds() |
        dstDevice->exactBounds() |
        selectionDevice->defaultBounds()->bounds();

    QScopedPointer<KisTransaction> transaction;

    if (writeToLayers) {
        commandsAdapter.beginMacro(kundo2_i18n("Write Alpha into a Layer"));
        transaction.reset(new KisTransaction(kundo2_noi18n("__write_alpha_channel__"), dstDevice));
    }

    KisSequentialIterator srcIt(selectionDevice, processRect);
    KisSequentialIterator dstIt(dstDevice, processRect);

    do {
        quint8 *alpha8Ptr = srcIt.rawData();
        quint8 *dstPtr = dstIt.rawData();

        dstCS->setOpacity(dstPtr, *alpha8Ptr, 1);
    } while (srcIt.nextPixel() && dstIt.nextPixel());

    if (writeToLayers) {
        commandsAdapter.addExtraCommand(transaction->endAndTake());
        commandsAdapter.removeNode(node);
        commandsAdapter.endMacro();
    } else {
        KisImageWSP image = view->image();
        QRect saveRect = image->bounds();

        saveDeviceAsImage(dstDevice, parentNode->name(),
                          saveRect,
                          image->xRes(), image->yRes(),
                          OPACITY_OPAQUE_U8);
    }
}


void KisNodeManager::slotSplitAlphaWrite()
{
    m_d->mergeTransparencyMaskAsAlpha(true);
}

void KisNodeManager::slotSplitAlphaSaveMerged()
{
    m_d->mergeTransparencyMaskAsAlpha(false);
}

void KisNodeManager::cutLayersToClipboard()
{
    KisNodeList nodes = this->selectedNodes();
    if (nodes.isEmpty()) return;

    KisClipboard::instance()->setLayers(nodes, m_d->view->image(), false);

    KUndo2MagicString actionName = kundo2_i18n("Cut Nodes");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
    juggler->removeNode(nodes);
}

void KisNodeManager::copyLayersToClipboard()
{
    KisNodeList nodes = this->selectedNodes();
    KisClipboard::instance()->setLayers(nodes, m_d->view->image(), true);
}

void KisNodeManager::pasteLayersFromClipboard()
{
    const QMimeData *data = KisClipboard::instance()->layersMimeData();
    if (!data) return;

    KisNodeSP activeNode = this->activeNode();

    KisShapeController *shapeController = dynamic_cast<KisShapeController*>(m_d->imageView->document()->shapeController());
    Q_ASSERT(shapeController);

    KisDummiesFacadeBase *dummiesFacade = dynamic_cast<KisDummiesFacadeBase*>(m_d->imageView->document()->shapeController());
    Q_ASSERT(dummiesFacade);

    const bool copyNode = false;
    KisImageSP image = m_d->view->image();
    KisNodeDummy *parentDummy = dummiesFacade->dummyForNode(activeNode);
    KisNodeDummy *aboveThisDummy = parentDummy ? parentDummy->lastChild() : 0;

    KisMimeData::insertMimeLayers(data,
                                  image,
                                  shapeController,
                                  parentDummy,
                                  aboveThisDummy,
                                  copyNode,
                                  nodeInsertionAdapter());
}

void KisNodeManager::createQuickGroupImpl(KisNodeJugglerCompressed *juggler,
                                          const QString &overrideGroupName,
                                          KisNodeSP *newGroup,
                                          KisNodeSP *newLastChild)
{
    KisNodeSP active = activeNode();
    if (!active) return;

    KisImageSP image = m_d->view->image();
    QString groupName = !overrideGroupName.isEmpty() ? overrideGroupName : image->nextLayerName();
    KisGroupLayerSP group = new KisGroupLayer(image.data(), groupName, OPACITY_OPAQUE_U8);

    KisNodeList nodes1;
    nodes1 << group;

    KisNodeList nodes2;
    nodes2 = KisLayerUtils::sortMergableNodes(image->root(), selectedNodes());
    KisLayerUtils::filterMergableNodes(nodes2);

    if (KisLayerUtils::checkIsChildOf(active, nodes2)) {
        active = nodes2.first();
    }

    KisNodeSP parent = active->parent();
    KisNodeSP aboveThis = active;

    juggler->addNode(nodes1, parent, aboveThis);
    juggler->moveNode(nodes2, group, 0);

    *newGroup = group;
    *newLastChild = nodes2.last();
}

void KisNodeManager::createQuickGroup()
{
    KUndo2MagicString actionName = kundo2_i18n("Quick Group");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);

    KisNodeSP parent;
    KisNodeSP above;

    createQuickGroupImpl(juggler, "", &parent, &above);
}

void KisNodeManager::createQuickClippingGroup()
{
    KUndo2MagicString actionName = kundo2_i18n("Quick Clipping Group");
    KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);

    KisNodeSP parent;
    KisNodeSP above;

    KisImageSP image = m_d->view->image();
    createQuickGroupImpl(juggler, image->nextLayerName(i18nc("default name for a clipping group layer", "Clipping Group")), &parent, &above);

    KisPaintLayerSP maskLayer = new KisPaintLayer(image.data(), i18nc("default name for quick clip group mask layer", "Mask Layer"), OPACITY_OPAQUE_U8, image->colorSpace());
    maskLayer->disableAlphaChannel(true);

    juggler->addNode(KisNodeList() << maskLayer, parent, above);
}

void KisNodeManager::quickUngroup()
{
    KisNodeSP active = activeNode();
    if (!active) return;

    KisNodeSP parent = active->parent();
    KisNodeSP aboveThis = active;

    KUndo2MagicString actionName = kundo2_i18n("Quick Ungroup");

    if (parent && dynamic_cast<KisGroupLayer*>(active.data())) {
        KisNodeList nodes = active->childNodes(QStringList(), KoProperties());

        KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
        juggler->moveNode(nodes, parent, active);
        juggler->removeNode(KisNodeList() << active);
    } else if (parent && parent->parent()) {
        KisNodeSP grandParent = parent->parent();

        KisNodeList allChildNodes = parent->childNodes(QStringList(), KoProperties());
        KisNodeList allSelectedNodes = selectedNodes();

        const bool removeParent = KritaUtils::compareListsUnordered(allChildNodes, allSelectedNodes);

        KisNodeJugglerCompressed *juggler = m_d->lazyGetJuggler(actionName);
        juggler->moveNode(allSelectedNodes, grandParent, parent);
        if (removeParent) {
            juggler->removeNode(KisNodeList() << parent);
        }
    }
}

void KisNodeManager::selectLayersImpl(const KoProperties &props, const KoProperties &invertedProps)
{
    KisImageSP image = m_d->view->image();
    KisNodeList nodes = KisLayerUtils::findNodesWithProps(image->root(), props, true);

    KisNodeList selectedNodes = this->selectedNodes();

    if (KritaUtils::compareListsUnordered(nodes, selectedNodes)) {
        nodes = KisLayerUtils::findNodesWithProps(image->root(), invertedProps, true);
    }

    if (!nodes.isEmpty()) {
        slotImageRequestNodeReselection(nodes.last(), nodes);
    }
}

void KisNodeManager::selectAllNodes()
{
    KoProperties props;
    selectLayersImpl(props, props);
}

void KisNodeManager::selectVisibleNodes()
{
    KoProperties props;
    props.setProperty("visible", true);

    KoProperties invertedProps;
    invertedProps.setProperty("visible", false);

    selectLayersImpl(props, invertedProps);
}

void KisNodeManager::selectLockedNodes()
{
    KoProperties props;
    props.setProperty("locked", true);

    KoProperties invertedProps;
    invertedProps.setProperty("locked", false);

    selectLayersImpl(props, invertedProps);
}

void KisNodeManager::selectInvisibleNodes()
{
    KoProperties props;
    props.setProperty("visible", false);

    KoProperties invertedProps;
    invertedProps.setProperty("visible", true);

    selectLayersImpl(props, invertedProps);
}

void KisNodeManager::selectUnlockedNodes()
{
    KoProperties props;
    props.setProperty("locked", false);

    KoProperties invertedProps;
    invertedProps.setProperty("locked", true);

    selectLayersImpl(props, invertedProps);
}
