/*
 *
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
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

#ifndef KIS_TOOL_MEASURE_H_
#define KIS_TOOL_MEASURE_H_

#include <QLabel>

#include <KoUnit.h>

#include "kis_tool.h"
#include "kis_global.h"
#include "kis_types.h"
#include "KoToolFactoryBase.h"
#include "flake/kis_node_shape.h"
#include <kis_icon.h>

class QPointF;
class QWidget;

class KoCanvasBase;


class KisToolMeasureOptionsWidget : public QWidget
{
    Q_OBJECT

public:
    KisToolMeasureOptionsWidget(QWidget* parent, double resolution);

public Q_SLOTS:
    void slotSetDistance(double distance);
    void slotSetAngle(double angle);
    void slotUnitChanged(int index);

private:
    void updateDistance();

    double m_resolution;
    QLabel* m_distanceLabel;
    QLabel* m_angleLabel;
    double m_distance;
    KoUnit m_unit;
};

class KisToolMeasure : public KisTool
{

    Q_OBJECT

public:
    KisToolMeasure(KoCanvasBase * canvas);
    virtual ~KisToolMeasure();

    void beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);
    void endPrimaryAction(KoPointerEvent *event);

    virtual void paint(QPainter& gc, const KoViewConverter &converter);

    QWidget * createOptionWidget();

Q_SIGNALS:
    void sigDistanceChanged(double distance);
    void sigAngleChanged(double angle);

private:
    QRectF boundingRect();
    double angle();
    double distance();

    double deltaX() {
        return m_endPos.x() - m_startPos.x();
    }
    double deltaY() {
        return m_startPos.y() - m_endPos.y();
    }

private:
    KisToolMeasureOptionsWidget *m_optionsWidget;

    QPointF m_startPos;
    QPointF m_endPos;
};


class KisToolMeasureFactory : public KoToolFactoryBase
{

public:

    KisToolMeasureFactory()
            : KoToolFactoryBase("KritaShape/KisToolMeasure") {
        setSection(TOOL_TYPE_VIEW);
        setToolTip(i18n("Measure Tool"));
        setIconName(koIconNameCStr("krita_tool_measure"));
        setPriority(1);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
    }

    virtual ~KisToolMeasureFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolMeasure(canvas);
    }

};




#endif //KIS_TOOL_MEASURE_H_

