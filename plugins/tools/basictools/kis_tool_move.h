/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
 *                2003 Patrick Julien  <freak@codepimps.org>
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

#ifndef KIS_TOOL_MOVE_H_
#define KIS_TOOL_MOVE_H_

#include <KoToolFactoryBase.h>
#include <kis_types.h>
#include <kis_tool.h>
#include <flake/kis_node_shape.h>
#include <kis_icon.h>
#include <QKeySequence>
#include <QWidget>
#include <QGroupBox>
#include <QRadioButton>


class KoCanvasBase;
class MoveToolOptionsWidget;
class KisDocument;

class KisToolMove : public KisTool
{
    Q_OBJECT
    Q_ENUMS(MoveToolMode);
    Q_PROPERTY(bool moveInProgress READ moveInProgress NOTIFY moveInProgressChanged);
public:
    KisToolMove(KoCanvasBase * canvas);
    virtual ~KisToolMove();

public Q_SLOTS:
    virtual void activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes);
    void deactivate();

public Q_SLOTS:
    void requestStrokeEnd();
    void requestStrokeCancellation();

protected Q_SLOTS:
    virtual void resetCursorStyle();

public:
    enum MoveToolMode {
        MoveSelectedLayer,
        MoveFirstLayer,
        MoveGroup
    };

    enum MoveDirection {
        Up,
        Down,
        Left,
        Right
    };

    void beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);
    void endPrimaryAction(KoPointerEvent *event);

    void beginAlternateAction(KoPointerEvent *event, AlternateAction action);
    void continueAlternateAction(KoPointerEvent *event, AlternateAction action);
    void endAlternateAction(KoPointerEvent *event, AlternateAction action);

    void startAction(KoPointerEvent *event, MoveToolMode mode);
    void continueAction(KoPointerEvent *event);
    void endAction(KoPointerEvent *event);

    virtual void paint(QPainter& gc, const KoViewConverter &converter);

    virtual QWidget* createOptionWidget();
    void updateUIUnit(int newUnit);

    MoveToolMode moveToolMode() const;
    bool moveInProgress() const;

    void setShowCoordinates(bool value);

public Q_SLOTS:
    void moveDiscrete(MoveDirection direction, bool big);

    void moveBySpinX(int newX);
    void moveBySpinY(int newY);

    void slotNodeChanged(KisNodeList nodes);

Q_SIGNALS:
    void moveToolModeChanged();
    void moveInProgressChanged();
    void moveInNewPosition(QPoint);

private:
    void drag(const QPoint& newPos);
    void cancelStroke();
    QPoint applyModifiers(Qt::KeyboardModifiers modifiers, QPoint pos);

    bool startStrokeImpl(MoveToolMode mode, const QPoint *pos);

private Q_SLOTS:
    void endStroke();

private:

    MoveToolOptionsWidget* m_optionsWidget;
    QPoint m_dragStart; ///< Point where current cursor dragging began
    QPoint m_accumulatedOffset; ///< Total offset including multiple clicks, up/down/left/right keys, etc. added together

    QPoint m_startPosition;

    KisStrokeId m_strokeId;

    bool m_moveInProgress;
    KisNodeList m_currentlyProcessingNodes;

    int m_resolution;

    QAction *m_showCoordinatesAction;
};


class KisToolMoveFactory : public KoToolFactoryBase
{

public:
    KisToolMoveFactory()
            : KoToolFactoryBase("KritaTransform/KisToolMove") {
        setToolTip(i18n("Move Tool"));
        setSection(TOOL_TYPE_TRANSFORM);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
        setPriority(3);
        setIconName(koIconNameCStr("krita_tool_move"));
        setShortcut(QKeySequence( Qt::Key_T));
    }

    virtual ~KisToolMoveFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolMove(canvas);
    }

};

#endif // KIS_TOOL_MOVE_H_

