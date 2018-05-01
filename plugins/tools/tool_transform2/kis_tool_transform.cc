/*
 *  kis_tool_transform.cc -- part of Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2005 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2010 Marc Pegon <pe.marc@free.fr>
 *  Copyright (c) 2013 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_tool_transform.h"


#include <math.h>
#include <limits>

#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QObject>
#include <QLabel>
#include <QComboBox>
#include <QApplication>
#include <QMatrix4x4>

#include <kis_debug.h>
#include <klocalizedstring.h>

#include <KoPointerEvent.h>
#include <KoID.h>
#include <KoCanvasBase.h>
#include <KoViewConverter.h>
#include <KoSelection.h>
#include <KoCompositeOp.h>

#include <kis_global.h>
#include <canvas/kis_canvas2.h>
#include <KisViewManager.h>
#include <kis_painter.h>
#include <kis_cursor.h>
#include <kis_image.h>
#include <kis_undo_adapter.h>
#include <kis_transaction.h>
#include <kis_selection.h>
#include <kis_filter_strategy.h>
#include <widgets/kis_cmb_idlist.h>
#include <kis_statusbar.h>
#include <kis_transform_worker.h>
#include <kis_perspectivetransform_worker.h>
#include <kis_warptransform_worker.h>
#include <kis_pixel_selection.h>
#include <kis_shape_selection.h>
#include <kis_selection_manager.h>
#include <krita_utils.h>
#include <kis_resources_snapshot.h>

#include <KoShapeTransformCommand.h>

#include "widgets/kis_progress_widget.h"

#include "kis_transform_utils.h"
#include "kis_warp_transform_strategy.h"
#include "kis_cage_transform_strategy.h"
#include "kis_liquify_transform_strategy.h"
#include "kis_free_transform_strategy.h"
#include "kis_perspective_transform_strategy.h"

#include "kis_transform_mask.h"
#include "kis_transform_mask_adapter.h"

#include "strokes/transform_stroke_strategy.h"

KisToolTransform::KisToolTransform(KoCanvasBase * canvas)
    : KisTool(canvas, KisCursor::rotateCursor())
    , m_workRecursively(true)
    , m_changesTracker(&m_transaction)
    , m_warpStrategy(
        new KisWarpTransformStrategy(
            dynamic_cast<KisCanvas2*>(canvas)->coordinatesConverter(),
            m_currentArgs, m_transaction))
    , m_cageStrategy(
        new KisCageTransformStrategy(
            dynamic_cast<KisCanvas2*>(canvas)->coordinatesConverter(),
            m_currentArgs, m_transaction))
    , m_liquifyStrategy(
        new KisLiquifyTransformStrategy(
            dynamic_cast<KisCanvas2*>(canvas)->coordinatesConverter(),
            m_currentArgs, m_transaction, canvas->resourceManager()))
    , m_freeStrategy(
        new KisFreeTransformStrategy(
            dynamic_cast<KisCanvas2*>(canvas)->coordinatesConverter(),
            dynamic_cast<KisCanvas2*>(canvas)->snapGuide(),
            m_currentArgs, m_transaction))
    , m_perspectiveStrategy(
        new KisPerspectiveTransformStrategy(
            dynamic_cast<KisCanvas2*>(canvas)->coordinatesConverter(),
            dynamic_cast<KisCanvas2*>(canvas)->snapGuide(),
            m_currentArgs, m_transaction))
{
    m_canvas = dynamic_cast<KisCanvas2*>(canvas);
    Q_ASSERT(m_canvas);

    setObjectName("tool_transform");
    useCursor(KisCursor::selectCursor());
    m_optionsWidget = 0;

    connect(m_warpStrategy.data(), SIGNAL(requestCanvasUpdate()), SLOT(canvasUpdateRequested()));
    connect(m_cageStrategy.data(), SIGNAL(requestCanvasUpdate()), SLOT(canvasUpdateRequested()));
    connect(m_liquifyStrategy.data(), SIGNAL(requestCanvasUpdate()), SLOT(canvasUpdateRequested()));
    connect(m_liquifyStrategy.data(), SIGNAL(requestCursorOutlineUpdate(const QPointF&)), SLOT(cursorOutlineUpdateRequested(const QPointF&)));
    connect(m_liquifyStrategy.data(), SIGNAL(requestUpdateOptionWidget()), SLOT(updateOptionWidget()));
    connect(m_freeStrategy.data(), SIGNAL(requestCanvasUpdate()), SLOT(canvasUpdateRequested()));
    connect(m_freeStrategy.data(), SIGNAL(requestResetRotationCenterButtons()), SLOT(resetRotationCenterButtonsRequested()));
    connect(m_freeStrategy.data(), SIGNAL(requestShowImageTooBig(bool)), SLOT(imageTooBigRequested(bool)));
    connect(m_perspectiveStrategy.data(), SIGNAL(requestCanvasUpdate()), SLOT(canvasUpdateRequested()));
    connect(m_perspectiveStrategy.data(), SIGNAL(requestShowImageTooBig(bool)), SLOT(imageTooBigRequested(bool)));

    connect(&m_changesTracker, SIGNAL(sigConfigChanged()),
            this, SLOT(slotTrackerChangedConfig()));
}

KisToolTransform::~KisToolTransform()
{
    cancelStroke();
}

void KisToolTransform::outlineChanged()
{
    emit freeTransformChanged();
    m_canvas->updateCanvas();
}

void KisToolTransform::canvasUpdateRequested()
{
    m_canvas->updateCanvas();
}

void KisToolTransform::resetCursorStyle()
{
    setFunctionalCursor();
}

void KisToolTransform::resetRotationCenterButtonsRequested()
{
    if (!m_optionsWidget) return;
    m_optionsWidget->resetRotationCenterButtons();
}

void KisToolTransform::imageTooBigRequested(bool value)
{
    if (!m_optionsWidget) return;
    m_optionsWidget->setTooBigLabelVisible(value);
}

KisTransformStrategyBase* KisToolTransform::currentStrategy() const
{
    if (m_currentArgs.mode() == ToolTransformArgs::FREE_TRANSFORM) {
        return m_freeStrategy.data();
    } else if (m_currentArgs.mode() == ToolTransformArgs::WARP) {
        return m_warpStrategy.data();
    } else if (m_currentArgs.mode() == ToolTransformArgs::CAGE) {
        return m_cageStrategy.data();
    } else if (m_currentArgs.mode() == ToolTransformArgs::LIQUIFY) {
        return m_liquifyStrategy.data();
    } else /* if (m_currentArgs.mode() == ToolTransformArgs::PERSPECTIVE_4POINT) */ {
        return m_perspectiveStrategy.data();
    }
}

void KisToolTransform::paint(QPainter& gc, const KoViewConverter &converter)
{
    Q_UNUSED(converter);

    if (!m_strokeData.strokeId()) return;

    QRectF newRefRect = KisTransformUtils::imageToFlake(m_canvas->coordinatesConverter(), QRectF(0.0,0.0,1.0,1.0));
    if (m_refRect != newRefRect) {
        m_refRect = newRefRect;
        currentStrategy()->externalConfigChanged();
    }

    gc.save();
    if (m_optionsWidget && m_optionsWidget->showDecorations()) {
        gc.setOpacity(0.3);
        gc.fillPath(m_selectionPath, Qt::black);
    }
    gc.restore();

    currentStrategy()->paint(gc);


    if (!m_cursorOutline.isEmpty()) {
        QPainterPath mappedOutline =
            KisTransformUtils::imageToFlakeTransform(
                m_canvas->coordinatesConverter()).map(m_cursorOutline);
        paintToolOutline(&gc, mappedOutline);
    }
}

void KisToolTransform::setFunctionalCursor()
{
    if (overrideCursorIfNotEditable()) {
        return;
    }

    if (!m_strokeData.strokeId()) {
        useCursor(KisCursor::pointingHandCursor());
    } else {
        useCursor(currentStrategy()->getCurrentCursor());
    }
}

void KisToolTransform::cursorOutlineUpdateRequested(const QPointF &imagePos)
{
    QRect canvasUpdateRect;

    if (!m_cursorOutline.isEmpty()) {
        canvasUpdateRect = m_canvas->coordinatesConverter()->
            imageToDocument(m_cursorOutline.boundingRect()).toAlignedRect();
    }

    m_cursorOutline = currentStrategy()->
        getCursorOutline().translated(imagePos);

    if (!m_cursorOutline.isEmpty()) {
        canvasUpdateRect |=
            m_canvas->coordinatesConverter()->
            imageToDocument(m_cursorOutline.boundingRect()).toAlignedRect();
    }

    if (!canvasUpdateRect.isEmpty()) {
        // grow rect a bit to follow interpolation fuzziness
        canvasUpdateRect = kisGrowRect(canvasUpdateRect, 2);
        m_canvas->updateCanvas(canvasUpdateRect);
    }
}

void KisToolTransform::beginActionImpl(KoPointerEvent *event, bool usePrimaryAction, KisTool::AlternateAction action)
{
    if (!nodeEditable()) {
        event->ignore();
        return;
    }

    if (currentNode()->inherits("KisShapeLayer")) {
        QString message = i18n("The transform tool cannot transform a vector layer.");
        KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
        kiscanvas->viewManager()->showFloatingMessage(message, koIcon("object-locked"));
        event->ignore();
        return;
    }

    if (!m_strokeData.strokeId()) {
        startStroke(m_currentArgs.mode(), false);
    } else {
        bool result = false;

        if (usePrimaryAction) {
            result = currentStrategy()->beginPrimaryAction(event);
        } else {
            result = currentStrategy()->beginAlternateAction(event, action);
        }

        if (result) {
            setMode(KisTool::PAINT_MODE);
        }
    }

    m_actuallyMoveWhileSelected = false;

    outlineChanged();
}

void KisToolTransform::continueActionImpl(KoPointerEvent *event, bool usePrimaryAction, KisTool::AlternateAction action)
{
    if (mode() != KisTool::PAINT_MODE) return;

    m_actuallyMoveWhileSelected = true;

    if (usePrimaryAction) {
        currentStrategy()->continuePrimaryAction(event);
    } else {
        currentStrategy()->continueAlternateAction(event, action);
    }

    updateOptionWidget();
    outlineChanged();
}

void KisToolTransform::endActionImpl(KoPointerEvent *event, bool usePrimaryAction, KisTool::AlternateAction action)
{
    if (mode() != KisTool::PAINT_MODE) return;

    setMode(KisTool::HOVER_MODE);

    if (m_actuallyMoveWhileSelected ||
        currentStrategy()->acceptsClicks()) {

        bool result = false;

        if (usePrimaryAction) {
            result = currentStrategy()->endPrimaryAction(event);
        } else {
            result = currentStrategy()->endAlternateAction(event, action);
        }

        if (result) {
            commitChanges();
        }

        outlineChanged();
    }

    updateOptionWidget();
    updateApplyResetAvailability();
}

void KisToolTransform::beginPrimaryAction(KoPointerEvent *event)
{
    beginActionImpl(event, true, KisTool::NONE);
}

void KisToolTransform::continuePrimaryAction(KoPointerEvent *event)
{
    continueActionImpl(event, true, KisTool::NONE);
}

void KisToolTransform::endPrimaryAction(KoPointerEvent *event)
{
    endActionImpl(event, true, KisTool::NONE);
}

void KisToolTransform::activatePrimaryAction()
{
    currentStrategy()->activatePrimaryAction();
    setFunctionalCursor();
}

void KisToolTransform::deactivatePrimaryAction()
{
    currentStrategy()->deactivatePrimaryAction();
}

void KisToolTransform::activateAlternateAction(AlternateAction action)
{
    currentStrategy()->activateAlternateAction(action);
    setFunctionalCursor();
}

void KisToolTransform::deactivateAlternateAction(AlternateAction action)
{
    currentStrategy()->deactivateAlternateAction(action);
}

void KisToolTransform::beginAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    beginActionImpl(event, false, action);
}

void KisToolTransform::continueAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    continueActionImpl(event, false, action);
}

void KisToolTransform::endAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    endActionImpl(event, false, action);
}

void KisToolTransform::mousePressEvent(KoPointerEvent *event)
{
    KisTool::mousePressEvent(event);
}

void KisToolTransform::mouseMoveEvent(KoPointerEvent *event)
{
    QPointF mousePos = m_canvas->coordinatesConverter()->documentToImage(event->point);

    cursorOutlineUpdateRequested(mousePos);

    if (!MOVE_CONDITION(event, KisTool::PAINT_MODE)) {
        currentStrategy()->hoverActionCommon(event);
        setFunctionalCursor();
        KisTool::mouseMoveEvent(event);
        return;
    }
}

void KisToolTransform::mouseReleaseEvent(KoPointerEvent *event)
{
    KisTool::mouseReleaseEvent(event);
}

void KisToolTransform::touchEvent( QTouchEvent* event )
{
    //Count all moving touch points
    int touchCount = 0;
    Q_FOREACH ( QTouchEvent::TouchPoint tp, event->touchPoints() ) {
        if( tp.state() == Qt::TouchPointMoved ) {
            touchCount++;
        }
    }

    //Use the touch point count to determine the gesture
    switch( touchCount ) {
    case 1: { //Panning
        QTouchEvent::TouchPoint tp = event->touchPoints().at( 0 );
        QPointF diff = tp.screenPos() - tp.lastScreenPos();

        m_currentArgs.setTransformedCenter( m_currentArgs.transformedCenter() + diff );
        outlineChanged();
        break;
    }
    case 2: { //Scaling
        QTouchEvent::TouchPoint tp1 = event->touchPoints().at( 0 );
        QTouchEvent::TouchPoint tp2 = event->touchPoints().at( 1 );

        float lastZoom = (tp1.lastScreenPos() - tp2.lastScreenPos()).manhattanLength();
        float newZoom = (tp1.screenPos() - tp2.screenPos()).manhattanLength();

        float diff = (newZoom - lastZoom) / 100;

        m_currentArgs.setScaleX( m_currentArgs.scaleX() + diff );
        m_currentArgs.setScaleY( m_currentArgs.scaleY() + diff );

        outlineChanged();
        break;
    }
    case 3: { //Rotation

/* TODO: implement touch-based rotation.

            Vector2f center;
            Q_FOREACH ( const QTouchEvent::TouchPoint &tp, event->touchPoints() ) {
                if( tp.state() == Qt::TouchPointMoved ) {
                    center += Vector2f( tp.screenPos().x(), tp.screenPos().y() );
                }
            }
            center /= touchCount;

            QTouchEvent::TouchPoint tp = event->touchPoints().at(0);

            Vector2f oldPosition = (Vector2f( tp.lastScreenPos().x(), tp.lastScreenPos().y() ) - center).normalized();
            Vector2f newPosition = (Vector2f( tp.screenPos().x(), tp.screenPos().y() ) - center).normalized();

            float oldAngle = qAcos( oldPosition.dot( Vector2f( 0.0f, 0.0f ) ) );
            float newAngle = qAcos( newPosition.dot( Vector2f( 0.0f, 0.0f ) ) );

            float diff = newAngle - oldAngle;

            m_currentArgs.setAZ( m_currentArgs.aZ() + diff );

            outlineChanged();
*/
        break;
    }
    }
}

void KisToolTransform::applyTransform()
{
    slotApplyTransform();
}

KisToolTransform::TransformToolMode KisToolTransform::transformMode() const
{
    TransformToolMode mode = FreeTransformMode;

    switch (m_currentArgs.mode())
    {
    case ToolTransformArgs::FREE_TRANSFORM:
        mode = FreeTransformMode;
        break;
    case ToolTransformArgs::WARP:
        mode = WarpTransformMode;
        break;
    case ToolTransformArgs::CAGE:
        mode = CageTransformMode;
        break;
    case ToolTransformArgs::LIQUIFY:
        mode = LiquifyTransformMode;
        break;
    case ToolTransformArgs::PERSPECTIVE_4POINT:
        mode = PerspectiveTransformMode;
        break;
    default:
        KIS_ASSERT_RECOVER_NOOP(0 && "unexpected transform mode");
    }

    return mode;
}

double KisToolTransform::translateX() const
{
    return m_currentArgs.transformedCenter().x();
}

double KisToolTransform::translateY() const
{
    return m_currentArgs.transformedCenter().y();
}

double KisToolTransform::rotateX() const
{
    return m_currentArgs.aX();
}

double KisToolTransform::rotateY() const
{
    return m_currentArgs.aY();
}

double KisToolTransform::rotateZ() const
{
    return m_currentArgs.aZ();
}

double KisToolTransform::scaleX() const
{
    return m_currentArgs.scaleX();
}

double KisToolTransform::scaleY() const
{
    return m_currentArgs.scaleY();
}

double KisToolTransform::shearX() const
{
    return m_currentArgs.shearX();
}

double KisToolTransform::shearY() const
{
    return m_currentArgs.shearY();
}

KisToolTransform::WarpType KisToolTransform::warpType() const
{
    switch(m_currentArgs.warpType()) {
    case KisWarpTransformWorker::AFFINE_TRANSFORM:
        return AffineWarpType;
    case KisWarpTransformWorker::RIGID_TRANSFORM:
        return RigidWarpType;
    case KisWarpTransformWorker::SIMILITUDE_TRANSFORM:
        return SimilitudeWarpType;
    default:
        return RigidWarpType;
    }
}

double KisToolTransform::warpFlexibility() const
{
    return m_currentArgs.alpha();
}

int KisToolTransform::warpPointDensity() const
{
    return m_currentArgs.numPoints();
}

void KisToolTransform::setTransformMode(KisToolTransform::TransformToolMode newMode)
{
    ToolTransformArgs::TransformMode mode = ToolTransformArgs::FREE_TRANSFORM;

    switch (newMode) {
    case FreeTransformMode:
        mode = ToolTransformArgs::FREE_TRANSFORM;
        break;
    case WarpTransformMode:
        mode = ToolTransformArgs::WARP;
        break;
    case CageTransformMode:
        mode = ToolTransformArgs::CAGE;
        break;
    case LiquifyTransformMode:
        mode = ToolTransformArgs::LIQUIFY;
        break;
    case PerspectiveTransformMode:
        mode = ToolTransformArgs::PERSPECTIVE_4POINT;
        break;
    default:
        KIS_ASSERT_RECOVER_NOOP(0 && "unexpected transform mode");
    }

    if( mode != m_currentArgs.mode() ) {
        if( newMode == FreeTransformMode ) {
            m_optionsWidget->slotSetFreeTransformModeButtonClicked( true );
        } else if( newMode == WarpTransformMode ) {
            m_optionsWidget->slotSetWarpModeButtonClicked( true );
        } else if( newMode == CageTransformMode ) {
            m_optionsWidget->slotSetCageModeButtonClicked( true );
        } else if( newMode == LiquifyTransformMode ) {
            m_optionsWidget->slotSetLiquifyModeButtonClicked( true );
        } else if( newMode == PerspectiveTransformMode ) {
            m_optionsWidget->slotSetPerspectiveModeButtonClicked( true );
        }
        emit transformModeChanged();
    }
}

void KisToolTransform::setRotateX( double rotation )
{
    m_currentArgs.setAX( normalizeAngle(rotation) );
}

void KisToolTransform::setRotateY( double rotation )
{
    m_currentArgs.setAY( normalizeAngle(rotation) );
}

void KisToolTransform::setRotateZ( double rotation )
{
    m_currentArgs.setAZ( normalizeAngle(rotation) );
}

void KisToolTransform::setWarpType( KisToolTransform::WarpType type )
{
    switch( type ) {
    case RigidWarpType:
        m_currentArgs.setWarpType(KisWarpTransformWorker::RIGID_TRANSFORM);
        break;
    case AffineWarpType:
        m_currentArgs.setWarpType(KisWarpTransformWorker::AFFINE_TRANSFORM);
        break;
    case SimilitudeWarpType:
        m_currentArgs.setWarpType(KisWarpTransformWorker::SIMILITUDE_TRANSFORM);
        break;
    default:
        break;
    }
}

void KisToolTransform::setWarpFlexibility( double flexibility )
{
    m_currentArgs.setAlpha( flexibility );
}

void KisToolTransform::setWarpPointDensity( int density )
{
    m_optionsWidget->slotSetWarpDensity(density);
}

bool KisToolTransform::tryInitTransformModeFromNode(KisNodeSP node)
{
    bool result = false;

    if (KisTransformMaskSP mask =
        dynamic_cast<KisTransformMask*>(node.data())) {

        KisTransformMaskParamsInterfaceSP savedParams =
            mask->transformParams();

        KisTransformMaskAdapter *adapter =
            dynamic_cast<KisTransformMaskAdapter*>(savedParams.data());

        if (adapter) {
            m_currentArgs = adapter->transformArgs();
            initGuiAfterTransformMode();
            result = true;
        }
    }

    return result;
}

bool KisToolTransform::tryFetchArgsFromCommandAndUndo(ToolTransformArgs *args, ToolTransformArgs::TransformMode mode, KisNodeSP currentNode)
{
    bool result = false;

    const KUndo2Command *lastCommand = image()->undoAdapter()->presentCommand();
    KisNodeSP oldRootNode;

    if (lastCommand &&
        TransformStrokeStrategy::fetchArgsFromCommand(lastCommand, args, &oldRootNode) &&
        args->mode() == mode &&
        oldRootNode == currentNode) {

        args->saveContinuedState();

        image()->undoAdapter()->undoLastCommand();
        // FIXME: can we make it async?
        image()->waitForDone();

        result = true;
    }

    return result;
}

void KisToolTransform::initTransformMode(ToolTransformArgs::TransformMode mode)
{
    // NOTE: we are requesting an old value of m_currentArgs variable
    //       here, which is global, don't forget about this on higher
    //       levels.
    QString filterId = m_currentArgs.filterId();

    m_currentArgs = ToolTransformArgs();
    m_currentArgs.setOriginalCenter(m_transaction.originalCenterGeometric());
    m_currentArgs.setTransformedCenter(m_transaction.originalCenterGeometric());

    if (mode == ToolTransformArgs::FREE_TRANSFORM) {
        m_currentArgs.setMode(ToolTransformArgs::FREE_TRANSFORM);
    } else if (mode == ToolTransformArgs::WARP) {
        m_currentArgs.setMode(ToolTransformArgs::WARP);
        m_optionsWidget->setDefaultWarpPoints();
        m_currentArgs.setEditingTransformPoints(false);
    } else if (mode == ToolTransformArgs::CAGE) {
        m_currentArgs.setMode(ToolTransformArgs::CAGE);
        m_currentArgs.setEditingTransformPoints(true);
    } else if (mode == ToolTransformArgs::LIQUIFY) {
        m_currentArgs.setMode(ToolTransformArgs::LIQUIFY);
        const QRect srcRect = m_transaction.originalRect().toAlignedRect();
        if (!srcRect.isEmpty()) {
            m_currentArgs.initLiquifyTransformMode(m_transaction.originalRect().toAlignedRect());
        }
    } else if (mode == ToolTransformArgs::PERSPECTIVE_4POINT) {
        m_currentArgs.setMode(ToolTransformArgs::PERSPECTIVE_4POINT);
    }

    initGuiAfterTransformMode();
}

void KisToolTransform::initGuiAfterTransformMode()
{
    currentStrategy()->externalConfigChanged();
    outlineChanged();
    updateOptionWidget();
    updateApplyResetAvailability();
}

void KisToolTransform::updateSelectionPath()
{
    m_selectionPath = QPainterPath();

    KisResourcesSnapshotSP resources =
        new KisResourcesSnapshot(image(), currentNode(), this->canvas()->resourceManager());

    QPainterPath selectionOutline;
    KisSelectionSP selection = resources->activeSelection();

    if (selection && selection->outlineCacheValid()) {
        selectionOutline = selection->outlineCache();
    } else {
        selectionOutline.addRect(m_selectedPortionCache->exactBounds());
    }

    const KisCoordinatesConverter *converter = m_canvas->coordinatesConverter();
    QTransform i2f = converter->imageToDocumentTransform() * converter->documentToFlakeTransform();

    m_selectionPath = i2f.map(selectionOutline);
}

void KisToolTransform::initThumbnailImage(KisPaintDeviceSP previewDevice)
{
    QImage origImg;
    m_selectedPortionCache = previewDevice;

    QTransform thumbToImageTransform;

    const int maxSize = 2000;

    QRect srcRect(m_transaction.originalRect().toAlignedRect());
    int x, y, w, h;
    srcRect.getRect(&x, &y, &w, &h);

    if (w > maxSize || h > maxSize) {
        qreal scale = qreal(maxSize) / (w > h ? w : h);
        QTransform scaleTransform = QTransform::fromScale(scale, scale);

        QRect thumbRect = scaleTransform.mapRect(m_transaction.originalRect()).toAlignedRect();

        origImg = m_selectedPortionCache->
            createThumbnail(thumbRect.width(),
                            thumbRect.height(),
                            srcRect, 1,
                            KoColorConversionTransformation::internalRenderingIntent(),
                            KoColorConversionTransformation::internalConversionFlags());
        thumbToImageTransform = scaleTransform.inverted();

    } else {
        origImg = m_selectedPortionCache->convertToQImage(0, x, y, w, h,
                                                            KoColorConversionTransformation::internalRenderingIntent(),
                                                            KoColorConversionTransformation::internalConversionFlags());
        thumbToImageTransform = QTransform();
    }

    // init both strokes since the thumbnail is initialized only once
    // during the stroke
    m_freeStrategy->setThumbnailImage(origImg, thumbToImageTransform);
    m_perspectiveStrategy->setThumbnailImage(origImg, thumbToImageTransform);
    m_warpStrategy->setThumbnailImage(origImg, thumbToImageTransform);
    m_cageStrategy->setThumbnailImage(origImg, thumbToImageTransform);
    m_liquifyStrategy->setThumbnailImage(origImg, thumbToImageTransform);
}

void KisToolTransform::activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes)
{
    KisTool::activate(toolActivation, shapes);

    if (currentNode()) {
        m_transaction = TransformTransactionProperties(QRectF(), &m_currentArgs, currentNode());
    }

    startStroke(ToolTransformArgs::FREE_TRANSFORM, false);
}

void KisToolTransform::deactivate()
{
    endStroke();
    m_canvas->updateCanvas();
    KisTool::deactivate();
}

void KisToolTransform::requestUndoDuringStroke()
{
    if (!m_strokeData.strokeId()) return;

    m_changesTracker.requestUndo();
}

void KisToolTransform::requestStrokeEnd()
{
    endStroke();
}

void KisToolTransform::requestStrokeCancellation()
{
    cancelStroke();
}

void KisToolTransform::startStroke(ToolTransformArgs::TransformMode mode, bool forceReset)
{
    Q_ASSERT(!m_strokeData.strokeId());

    KisResourcesSnapshotSP resources =
            new KisResourcesSnapshot(image(), currentNode(), this->canvas()->resourceManager());

    KisNodeSP currentNode = resources->currentNode();

    if (!currentNode || !currentNode->isEditable()) {
        return;
    }

    /**
     * FIXME: The transform tool is not completely asynchronous, it
     * needs the content of the layer for creation of the stroke
     * strategy. It means that we cannot start a new stroke until the
     * previous one is finished. Ideally, we should create the
     * m_selectedPortionCache and m_selectionPath somewhere in the
     * stroke and pass it to the tool somehow. But currently, we will
     * just disable starting a new stroke asynchronously
     */
    if (image()->tryBarrierLock()) {
        image()->unlock();
    } else {
        return;
    }

    ToolTransformArgs fetchedArgs;
    bool fetchedFromCommand = false;

    if (!forceReset) {
        fetchedFromCommand = tryFetchArgsFromCommandAndUndo(&fetchedArgs, mode, currentNode);
    }

    if (m_optionsWidget) {
        m_workRecursively = m_optionsWidget->workRecursively() ||
            !currentNode->paintDevice();
    }

    TransformStrokeStrategy *strategy = new TransformStrokeStrategy(currentNode, resources->activeSelection(), image().data());
    KisPaintDeviceSP previewDevice = strategy->previewDevice();

    KisSelectionSP selection = strategy->realSelection();
    QRect srcRect = selection ? selection->selectedExactRect() : previewDevice->exactBounds();

    if (!selection && resources->activeSelection()) {
        KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
        kisCanvas->viewManager()->
            showFloatingMessage(
                i18nc("floating message in transformation tool",
                      "Selections are not used when editing transform masks "),
                QIcon(), 4000, KisFloatingMessage::Low);
    }

    if (srcRect.isEmpty()) {
        delete strategy;

        KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
        kisCanvas->viewManager()->
            showFloatingMessage(
                i18nc("floating message in transformation tool",
                      "Cannot transform empty layer "),
                QIcon(), 1000, KisFloatingMessage::Medium);

        return;
    }

    m_transaction = TransformTransactionProperties(srcRect, &m_currentArgs, currentNode);

    initThumbnailImage(previewDevice);
    updateSelectionPath();

    if (!forceReset && fetchedFromCommand) {
        m_currentArgs = fetchedArgs;
        initGuiAfterTransformMode();
    } else if (forceReset || !tryInitTransformModeFromNode(currentNode)) {
        initTransformMode(mode);
    }

    m_strokeData = StrokeData(image()->startStroke(strategy));

    bool haveInvisibleNodes = clearDevices(m_transaction.rootNode(), m_workRecursively);
    if (haveInvisibleNodes) {
        KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
        kisCanvas->viewManager()->
            showFloatingMessage(
                i18nc("floating message in transformation tool",
                      "Invisible sublayers will also be transformed. Lock layers if you do not want them to be transformed "),
                QIcon(), 4000, KisFloatingMessage::Low);
    }


    Q_ASSERT(m_changesTracker.isEmpty());
    commitChanges();
}

void KisToolTransform::endStroke()
{
    if (!m_strokeData.strokeId()) return;

    if (!m_currentArgs.isIdentity()) {
        transformDevices(m_transaction.rootNode(), m_workRecursively);

        image()->addJob(m_strokeData.strokeId(),
                        new TransformStrokeStrategy::TransformData(
                            TransformStrokeStrategy::TransformData::SELECTION,
                            m_currentArgs,
                            m_transaction.rootNode()));

        image()->endStroke(m_strokeData.strokeId());
    } else {
        image()->cancelStroke(m_strokeData.strokeId());
    }

    m_strokeData.clear();
    m_changesTracker.reset();
}

void KisToolTransform::cancelStroke()
{
    if (!m_strokeData.strokeId()) return;

    if (m_currentArgs.continuedTransform()) {
        m_currentArgs.restoreContinuedState();
        endStroke();
    } else {
        image()->cancelStroke(m_strokeData.strokeId());
        m_strokeData.clear();
        m_changesTracker.reset();
    }
}

void KisToolTransform::commitChanges()
{
    if (!m_strokeData.strokeId()) return;

    m_changesTracker.commitConfig(m_currentArgs);
}

void KisToolTransform::slotTrackerChangedConfig()
{
    slotUiChangedConfig();
    updateOptionWidget();
}

bool KisToolTransform::clearDevices(KisNodeSP node, bool recursive)
{
    bool haveInvisibleNodes = false;
    if (!node->isEditable(false)) return haveInvisibleNodes;

    haveInvisibleNodes = !node->visible(false);

    if (recursive) {
        // simple tail-recursive iteration
        KisNodeSP prevNode = node->lastChild();
        while(prevNode) {
            haveInvisibleNodes |= clearDevices(prevNode, recursive);
            prevNode = prevNode->prevSibling();
        }
    }

    image()->addJob(m_strokeData.strokeId(),
                    new TransformStrokeStrategy::ClearSelectionData(node));

    /**
     * It might happen that the editablity state of the node would
     * change during the stroke, so we need to save the set of
     * applicable nodes right in the beginning of the processing
     */
    m_strokeData.addClearedNode(node);

    return haveInvisibleNodes;
}

void KisToolTransform::transformDevices(KisNodeSP node, bool recursive)
{
    if (!node->isEditable()) return;

    KIS_ASSERT_RECOVER_RETURN(recursive ||
                              (m_strokeData.clearedNodes().size() == 1 &&
                               KisNodeSP(m_strokeData.clearedNodes().first()) == node));

    Q_FOREACH (KisNodeSP currentNode, m_strokeData.clearedNodes()) {
        KIS_ASSERT_RECOVER_RETURN(currentNode);

        image()->addJob(m_strokeData.strokeId(),
                        new TransformStrokeStrategy::TransformData(
                            TransformStrokeStrategy::TransformData::PAINT_DEVICE,
                            m_currentArgs,
                            currentNode));
    }
}

QWidget* KisToolTransform::createOptionWidget() {
    m_optionsWidget = new KisToolTransformConfigWidget(&m_transaction, m_canvas, m_workRecursively, 0);
    Q_CHECK_PTR(m_optionsWidget);
    m_optionsWidget->setObjectName(toolId() + " option widget");

    // See https://bugs.kde.org/show_bug.cgi?id=316896
    QWidget *specialSpacer = new QWidget(m_optionsWidget);
    specialSpacer->setObjectName("SpecialSpacer");
    specialSpacer->setFixedSize(0, 0);
    m_optionsWidget->layout()->addWidget(specialSpacer);


    connect(m_optionsWidget, SIGNAL(sigConfigChanged()),
            this, SLOT(slotUiChangedConfig()));

    connect(m_optionsWidget, SIGNAL(sigApplyTransform()),
            this, SLOT(slotApplyTransform()));

    connect(m_optionsWidget, SIGNAL(sigResetTransform()),
            this, SLOT(slotResetTransform()));

    connect(m_optionsWidget, SIGNAL(sigRestartTransform()),
            this, SLOT(slotRestartTransform()));

    connect(m_optionsWidget, SIGNAL(sigEditingFinished()),
            this, SLOT(slotEditingFinished()));

    updateOptionWidget();

    return m_optionsWidget;
}

void KisToolTransform::updateOptionWidget()
{
    if (!m_optionsWidget) return;

    if (!currentNode()) {
        m_optionsWidget->setEnabled(false);
        return;
    }
    else {
        m_optionsWidget->setEnabled(true);
        m_optionsWidget->updateConfig(m_currentArgs);
    }
}

void KisToolTransform::updateApplyResetAvailability()
{
    if (m_optionsWidget) {
        m_optionsWidget->setApplyResetDisabled(m_currentArgs.isIdentity());
    }
}

void KisToolTransform::slotUiChangedConfig()
{
    if (mode() == KisTool::PAINT_MODE) return;

    currentStrategy()->externalConfigChanged();

    if (m_currentArgs.mode() == ToolTransformArgs::LIQUIFY) {
        m_currentArgs.saveLiquifyTransformMode();
    }

    outlineChanged();
    updateApplyResetAvailability();
}

void KisToolTransform::slotApplyTransform()
{
    QApplication::setOverrideCursor(KisCursor::waitCursor());
    endStroke();
    QApplication::restoreOverrideCursor();
}

void KisToolTransform::slotResetTransform()
{
    if (m_currentArgs.continuedTransform()) {
        ToolTransformArgs::TransformMode savedMode = m_currentArgs.mode();

        /**
         * Our reset transform button can be used for two purposes:
         *
         * 1) Reset current transform to the initial one, which was
         *    loaded from the previous user action.
         *
         * 2) Reset transform frame to infinity when the frame is unchanged
         */

        const bool transformDiffers = !m_currentArgs.continuedTransform()->isSameMode(m_currentArgs);

        if (transformDiffers &&
            m_currentArgs.continuedTransform()->mode() == savedMode) {

            m_currentArgs.restoreContinuedState();
            initGuiAfterTransformMode();
            slotEditingFinished();

        } else {
            cancelStroke();
            image()->waitForDone();
            startStroke(savedMode, true);

            KIS_ASSERT_RECOVER_NOOP(!m_currentArgs.continuedTransform());
        }
    } else {
        initTransformMode(m_currentArgs.mode());
        slotEditingFinished();
    }
}

void KisToolTransform::slotRestartTransform()
{
    if (!m_strokeData.strokeId()) return;

    ToolTransformArgs savedArgs(m_currentArgs);
    cancelStroke();
    image()->waitForDone();
    startStroke(savedArgs.mode(), true);
}

void KisToolTransform::slotEditingFinished()
{
    commitChanges();
}

void KisToolTransform::setShearY(double shear)
{
    m_optionsWidget->slotSetShearY(shear);
}

void KisToolTransform::setShearX(double shear)
{
    m_optionsWidget->slotSetShearX(shear);
}

void KisToolTransform::setScaleY(double scale)
{
    m_optionsWidget->slotSetScaleY(scale);
}

void KisToolTransform::setScaleX(double scale)
{
    m_optionsWidget->slotSetScaleX(scale);
}

void KisToolTransform::setTranslateY(double translation)
{
    m_optionsWidget->slotSetTranslateY(translation);
}

void KisToolTransform::setTranslateX(double translation)
{
    m_optionsWidget->slotSetTranslateX(translation);
}

