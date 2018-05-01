/*
 *  kis_tool_transform.h - part of Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2005 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2010 Marc Pegon <pe.marc@free.fr>
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

#ifndef KIS_TOOL_TRANSFORM_H_
#define KIS_TOOL_TRANSFORM_H_

#include <kis_icon.h>

#include <QPoint>
#include <QPointF>
#include <QVector2D>
#include <QVector3D>
#include <QButtonGroup>

#include <QKeySequence>

#include <KoToolFactoryBase.h>

#include <kis_shape_selection.h>
#include <kis_undo_adapter.h>
#include <kis_types.h>
#include <flake/kis_node_shape.h>
#include <kis_tool.h>

#include "tool_transform_args.h"
#include "tool_transform_changes_tracker.h"
#include "kis_tool_transform_config_widget.h"
#include "transform_transaction_properties.h"



class KisCanvas2;

class QTouchEvent;
class KisTransformStrategyBase;
class KisWarpTransformStrategy;
class KisCageTransformStrategy;
class KisLiquifyTransformStrategy;
class KisFreeTransformStrategy;
class KisPerspectiveTransformStrategy;


/**
 * Transform tool
 * This tool offers several modes.
 * - Free Transform mode allows the user to translate, scale, shear, rotate and
 *   apply a perspective transformation to a selection or the whole canvas.
 * - Warp mode allows the user to warp the selection of the canvas by grabbing
 *   and moving control points placed on the image. The user can either work
 *   with default control points, like a grid whose density can be modified, or
 *   place the control points manually. The modifications made on the selected
 *   pixels are applied only when the user clicks the Apply button : the
 *   semi-transparent image displayed until the user click that button is only a
 *   preview.
 * - Cage transform is similar to warp transform with control points exactly
 *   placed on the outer boundary. The user draws a boundary polygon, the
 *   vertices of which become control points.
 * - Perspective transform applies a two-point perspective transformation. The
 *   user can manipulate the corners of the selection. If the vanishing points
 *   of the resulting quadrilateral are on screen, the user can manipulate those
 *   as well.
 * - Liquify transform transforms the selection by painting motions, as if the
 *   user were fingerpainting.
 */
class KisToolTransform : public KisTool
{

    Q_OBJECT

    Q_PROPERTY(TransformToolMode transformMode READ transformMode WRITE setTransformMode NOTIFY transformModeChanged)

    Q_PROPERTY(double translateX READ translateX WRITE setTranslateX NOTIFY freeTransformChanged)
    Q_PROPERTY(double translateY READ translateY WRITE setTranslateY NOTIFY freeTransformChanged)

    Q_PROPERTY(double rotateX READ rotateX WRITE setRotateX NOTIFY freeTransformChanged)
    Q_PROPERTY(double rotateY READ rotateY WRITE setRotateY NOTIFY freeTransformChanged)
    Q_PROPERTY(double rotateZ READ rotateZ WRITE setRotateZ NOTIFY freeTransformChanged)

    Q_PROPERTY(double scaleX READ scaleX WRITE setScaleX NOTIFY freeTransformChanged)
    Q_PROPERTY(double scaleY READ scaleY WRITE setScaleY NOTIFY freeTransformChanged)

    Q_PROPERTY(double shearX READ shearX WRITE setShearX NOTIFY freeTransformChanged)
    Q_PROPERTY(double shearY READ shearY WRITE setShearY NOTIFY freeTransformChanged)

    Q_PROPERTY(WarpType warpType READ warpType WRITE setWarpType NOTIFY warpTransformChanged)
    Q_PROPERTY(double warpFlexibility READ warpFlexibility WRITE setWarpFlexibility NOTIFY warpTransformChanged)
    Q_PROPERTY(int warpPointDensity READ warpPointDensity WRITE setWarpPointDensity NOTIFY warpTransformChanged)



public:
    enum TransformToolMode {
        FreeTransformMode,
        WarpTransformMode,
        CageTransformMode,
        LiquifyTransformMode,
        PerspectiveTransformMode
    };
    Q_ENUMS(TransformToolMode)

    enum WarpType {
        RigidWarpType,
        AffineWarpType,
        SimilitudeWarpType
    };
    Q_ENUMS(WarpType)

    KisToolTransform(KoCanvasBase * canvas);
    virtual ~KisToolTransform();

    virtual QWidget* createOptionWidget();

    virtual void mousePressEvent(KoPointerEvent *e);
    virtual void mouseMoveEvent(KoPointerEvent *e);
    virtual void mouseReleaseEvent(KoPointerEvent *e);
    virtual void touchEvent(QTouchEvent *event);

    void beginActionImpl(KoPointerEvent *event, bool usePrimaryAction, KisTool::AlternateAction action);
    void continueActionImpl(KoPointerEvent *event, bool usePrimaryAction, KisTool::AlternateAction action);
    void endActionImpl(KoPointerEvent *event, bool usePrimaryAction, KisTool::AlternateAction action);

    void activatePrimaryAction();
    void deactivatePrimaryAction();
    void beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);
    void endPrimaryAction(KoPointerEvent *event);

    void activateAlternateAction(AlternateAction action);
    void deactivateAlternateAction(AlternateAction action);
    void beginAlternateAction(KoPointerEvent *event, AlternateAction action);
    void continueAlternateAction(KoPointerEvent *event, AlternateAction action);
    void endAlternateAction(KoPointerEvent *event, AlternateAction action);

    void paint(QPainter& gc, const KoViewConverter &converter);

    TransformToolMode transformMode() const;

    double translateX() const;
    double translateY() const;

    double rotateX() const;
    double rotateY() const;
    double rotateZ() const;

    double scaleX() const;
    double scaleY() const;

    double shearX() const;
    double shearY() const;

    WarpType warpType() const;
    double warpFlexibility() const;
    int warpPointDensity() const;

    bool wantsTouch() const { return true; }

public Q_SLOTS:
    virtual void activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes);
    virtual void deactivate();
    // Applies the current transformation to the original paint device and commits it to the undo stack
    void applyTransform();

    void setTransformMode( KisToolTransform::TransformToolMode newMode );

    void setTranslateX(double translateX);
    void setTranslateY(double translateY);

    void setRotateX(double rotation);
    void setRotateY(double rotation);
    void setRotateZ(double rotation);

    void setScaleX(double scaleX);
    void setScaleY(double scaleY);

    void setShearX(double shearX);
    void setShearY(double shearY);

    void setWarpType(WarpType type);
    void setWarpFlexibility(double flexibility);
    void setWarpPointDensity(int density);

protected Q_SLOTS:
    virtual void resetCursorStyle();

Q_SIGNALS:
    void transformModeChanged();
    void freeTransformChanged();
    void warpTransformChanged();

public Q_SLOTS:
    void requestUndoDuringStroke();
    void requestStrokeEnd();
    void requestStrokeCancellation();
    void canvasUpdateRequested();
    void cursorOutlineUpdateRequested(const QPointF &imagePos);

    // Update the widget according to m_currentArgs
    void updateOptionWidget();

    void resetRotationCenterButtonsRequested();
    void imageTooBigRequested(bool value);

private:
    bool clearDevices(KisNodeSP node, bool recursive);
    void transformDevices(KisNodeSP node, bool recursive);

    void startStroke(ToolTransformArgs::TransformMode mode, bool forceReset);
    void endStroke();
    void cancelStroke();

private:
    void outlineChanged();
    // Sets the cursor according to mouse position (doesn't take shearing into account well yet)
    void setFunctionalCursor();
    // Sets m_function according to mouse position and modifier
    void setTransformFunction(QPointF mousePos, Qt::KeyboardModifiers modifiers);

    void commitChanges();


    bool tryInitTransformModeFromNode(KisNodeSP node);
    bool tryFetchArgsFromCommandAndUndo(ToolTransformArgs *args, ToolTransformArgs::TransformMode mode, KisNodeSP currentNode);
    void initTransformMode(ToolTransformArgs::TransformMode mode);
    void initGuiAfterTransformMode();

    void initThumbnailImage(KisPaintDeviceSP previewDevice);
    void updateSelectionPath();
    void updateApplyResetAvailability();

private:
    ToolTransformArgs m_currentArgs;

    bool m_actuallyMoveWhileSelected; // true <=> selection has been moved while clicked

    KisPaintDeviceSP m_selectedPortionCache;

    struct StrokeData {
        StrokeData() {}
        StrokeData(KisStrokeId strokeId) : m_strokeId(strokeId) {}

        void clear() {
            m_strokeId.clear();
            m_clearedNodes.clear();
        }

        const KisStrokeId strokeId() const { return m_strokeId; }
        void addClearedNode(KisNodeSP node) { m_clearedNodes.append(node); }
        const QVector<KisNodeWSP>& clearedNodes() const { return m_clearedNodes; }

    private:
        KisStrokeId m_strokeId;
        QVector<KisNodeWSP> m_clearedNodes;
    };
    StrokeData m_strokeData;

    bool m_workRecursively;

    QPainterPath m_selectionPath; // original (unscaled) selection outline, used for painting decorations

    KisToolTransformConfigWidget *m_optionsWidget;
    KisCanvas2 *m_canvas;

    TransformTransactionProperties m_transaction;
    TransformChangesTracker m_changesTracker;

    /**
     * This artificial rect is used to store the image to flake
     * transformation. We check against this rect to get to know
     * whether zoom has changed.
     */
    QRectF m_refRect;

    QScopedPointer<KisWarpTransformStrategy> m_warpStrategy;
    QScopedPointer<KisCageTransformStrategy> m_cageStrategy;
    QScopedPointer<KisLiquifyTransformStrategy> m_liquifyStrategy;
    QScopedPointer<KisFreeTransformStrategy> m_freeStrategy;
    QScopedPointer<KisPerspectiveTransformStrategy> m_perspectiveStrategy;
    KisTransformStrategyBase* currentStrategy() const;

    QPainterPath m_cursorOutline;

private Q_SLOTS:
    void slotTrackerChangedConfig();
    void slotUiChangedConfig();
    void slotApplyTransform();
    void slotResetTransform();
    void slotRestartTransform();
    void slotEditingFinished();
};

class KisToolTransformFactory : public KoToolFactoryBase
{
public:

    KisToolTransformFactory()
            : KoToolFactoryBase("KisToolTransform") {
        setToolTip(i18n("Transform a layer or a selection"));
        setSection(TOOL_TYPE_TRANSFORM);
        setIconName(koIconNameCStr("krita_tool_transform"));
        setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
        setPriority(2);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
    }

    virtual ~KisToolTransformFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolTransform(canvas);
    }

};



#endif // KIS_TOOL_TRANSFORM_H_

