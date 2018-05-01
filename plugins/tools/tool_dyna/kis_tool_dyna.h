/*
 *  Copyright (c) 2009-2011 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#ifndef KIS_TOOL_DYNA_H_
#define KIS_TOOL_DYNA_H_

#include "kis_tool_freehand.h"

#include "KoToolFactoryBase.h"
#include "KoPointerEvent.h"

#include <flake/kis_node_shape.h>
#include <kis_icon.h>
#include <kconfig.h>
#include <kconfiggroup.h>


class KisDoubleSliderSpinBox;
class QCheckBox;
class QGridLayout;


class KoCanvasBase;

class DynaFilter
{
public:
    DynaFilter() {
        curx = 0;
        cury = 0;
        lastx = 0;
        lasty = 0;
        velx = 0.0;
        vely = 0.0;
        accx = 0.0;
        accy = 0.0;
    }

    void init(qreal x, qreal y) {
        curx = x;
        cury = y;
        lastx = x;
        lasty = y;
        velx = 0.0;
        vely = 0.0;
        accx = 0.0;
        accy = 0.0;
    }

    ~DynaFilter() {}

public:
    qreal curx, cury;
    qreal velx, vely, vel;
    qreal accx, accy, acc;
    qreal angx, angy;
    qreal mass, drag;
    qreal lastx, lasty;
    bool fixedangle;
};


class KisToolDyna : public KisToolFreehand
{
    Q_OBJECT

public:
    KisToolDyna(KoCanvasBase * canvas);
    virtual ~KisToolDyna();

    QWidget * createOptionWidget();
    virtual void activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes);
    void beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);

protected:
    virtual void initStroke(KoPointerEvent *event);

protected Q_SLOTS:
    virtual void resetCursorStyle();

private Q_SLOTS:

    void slotSetDynaWidth(double width);
    void slotSetMass(qreal mass);
    void slotSetDrag(qreal drag);
    void slotSetAngle(qreal angle);
    void slotSetWidthRange(double widthRange);
    void slotSetFixedAngle(bool fixedAngle);

private:
    QGridLayout* m_optionLayout;

    // dyna gui
    QCheckBox * m_chkFixedAngle;
    KisDoubleSliderSpinBox * m_massSPBox;
    KisDoubleSliderSpinBox * m_dragSPBox;
    KisDoubleSliderSpinBox * m_angleDSSBox;

    // dyna algorithm
    QVector<QPointF> m_prevPosition;
    qreal m_odelx, m_odely;

    // mouse info
    QPointF m_mousePos;

    qreal m_surfaceWidth;
    qreal m_surfaceHeight;

    // settings variables
    KConfigGroup m_configGroup;
    qreal m_width;
    qreal m_curmass;
    qreal m_curdrag;
    DynaFilter m_mouse;
    qreal m_xangle;
    qreal m_yangle;
    qreal m_widthRange;


    // methods
    qreal flerp(qreal f0, qreal f1, qreal p) {
        return ((f0 *(1.0 - p)) + (f1 * p));
    }
    void setMousePosition(const QPointF &point) {
        m_mousePos.setX(point.x() / m_surfaceWidth );
        m_mousePos.setY(point.y() / m_surfaceHeight);
    }

    void initDyna();
    int applyFilter(qreal mx, qreal my);
    KoPointerEvent filterEvent(KoPointerEvent * event);

};


class KisToolDynaFactory : public KoToolFactoryBase
{

public:
    KisToolDynaFactory()
            : KoToolFactoryBase("KritaShape/KisToolDyna") {

        setToolTip(i18n("Dynamic Brush Tool"));

        // Temporarily
        setSection(TOOL_TYPE_SHAPE);
        setIconName(koIconNameCStr("krita_tool_dyna"));
        // TODO
        //setShortcut(QKeySequence(Qt::Key_F));
        setPriority(10);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
    }

    virtual ~KisToolDynaFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolDyna(canvas);
    }

};


#endif // KIS_TOOL_DYNA_H_

