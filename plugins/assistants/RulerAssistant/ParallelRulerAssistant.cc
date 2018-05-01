/*
 * Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
 * Copyright (c) 2010 Geoffry Song <goffrie@gmail.com>
 * Copyright (c) 2014 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ParallelRulerAssistant.h"

#include "kis_debug.h"
#include <klocalizedstring.h>

#include <QPainter>
#include <QLinearGradient>
#include <QTransform>

#include <kis_canvas2.h>
#include <kis_coordinates_converter.h>
#include <kis_algebra_2d.h>

#include <math.h>

ParallelRulerAssistant::ParallelRulerAssistant()
        : KisPaintingAssistant("parallel ruler", i18n("Parallel Ruler assistant"))
{
}

QPointF ParallelRulerAssistant::project(const QPointF& pt, const QPointF& strokeBegin)
{
    Q_ASSERT(handles().size() == 2);
    //code nicked from the perspective ruler.
    qreal
            dx = pt.x() - strokeBegin.x(),
            dy = pt.y() - strokeBegin.y();
        if (dx * dx + dy * dy < 4.0) {
            // allow some movement before snapping
            return strokeBegin;
        }
    //dbgKrita<<strokeBegin<< ", " <<*handles()[0];
    QLineF snapLine = QLineF(*handles()[0], *handles()[1]);
    QPointF translation = (*handles()[0]-strokeBegin)*-1.0;
    snapLine= snapLine.translated(translation);
    
    
        dx = snapLine.dx();
        dy = snapLine.dy();
    const qreal
        dx2 = dx * dx,
        dy2 = dy * dy,
        invsqrlen = 1.0 / (dx2 + dy2);
    QPointF r(dx2 * pt.x() + dy2 * snapLine.x1() + dx * dy * (pt.y() - snapLine.y1()),
              dx2 * snapLine.y1() + dy2 * pt.y() + dx * dy * (pt.x() - snapLine.x1()));
    r *= invsqrlen;
    return r;
    //return pt;
}

QPointF ParallelRulerAssistant::adjustPosition(const QPointF& pt, const QPointF& strokeBegin)
{
    return project(pt, strokeBegin);
}

void ParallelRulerAssistant::drawAssistant(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter* converter, bool cached, KisCanvas2* canvas, bool assistantVisible, bool previewVisible)
{
    gc.save();
    gc.resetTransform();
    QPointF delta(0,0);//this is the difference between the vanishing point and the mouse-position//
    QPointF mousePos(0,0);
    QPointF endPoint(0,0);//this is the final point that the line is being extended to, we seek it just outside the view port//
    
    if (canvas){
        //simplest, cheapest way to get the mouse-position//
        mousePos= canvas->canvasWidget()->mapFromGlobal(QCursor::pos());
    }
    else {
        //...of course, you need to have access to a canvas-widget for that.//
        mousePos = QCursor::pos();//this'll give an offset//
        dbgFile<<"canvas does not exist in ruler, you may have passed arguments incorrectly:"<<canvas;
    }
    
    if (handles().size() > 1 && outline()==true && previewVisible==true) {
        //don't draw if invalid.
        QTransform initialTransform = converter->documentToWidgetTransform();
        QLineF snapLine= QLineF(initialTransform.map(*handles()[0]), initialTransform.map(*handles()[1]));
        QPointF translation = (initialTransform.map(*handles()[0])-mousePos)*-1.0;
        snapLine= snapLine.translated(translation);
            
        QRect viewport= gc.viewport();
        KisAlgebra2D::intersectLineRect(snapLine, viewport);
        
        
        QPainterPath path;
        path.moveTo(snapLine.p1());
        path.lineTo(snapLine.p2());
        
        drawPreview(gc, path);//and we draw the preview.
    }
    gc.restore();
    
    KisPaintingAssistant::drawAssistant(gc, updateRect, converter, cached, canvas, assistantVisible, previewVisible);

}

void ParallelRulerAssistant::drawCache(QPainter& gc, const KisCoordinatesConverter *converter, bool assistantVisible)
{
    if (assistantVisible==false){return;}
    if (handles().size() < 2) return;

    QTransform initialTransform = converter->documentToWidgetTransform();

    // Draw the line
    QPointF p1 = *handles()[0];
    QPointF p2 = *handles()[1];

    gc.setTransform(initialTransform);
    QPainterPath path;
    path.moveTo(p1);
    path.lineTo(p2);
    drawPath(gc, path, snapping());
    
}

QPointF ParallelRulerAssistant::buttonPosition() const
{
    return (*handles()[0] + *handles()[1]) * 0.5;
}

ParallelRulerAssistantFactory::ParallelRulerAssistantFactory()
{
}

ParallelRulerAssistantFactory::~ParallelRulerAssistantFactory()
{
}

QString ParallelRulerAssistantFactory::id() const
{
    return "parallel ruler";
}

QString ParallelRulerAssistantFactory::name() const
{
    return i18n("Parallel Ruler");
}

KisPaintingAssistant* ParallelRulerAssistantFactory::createPaintingAssistant() const
{
    return new ParallelRulerAssistant;
}
