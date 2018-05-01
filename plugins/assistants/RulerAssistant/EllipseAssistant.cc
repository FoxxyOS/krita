/*
 * Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
 * Copyright (c) 2010 Geoffry Song <goffrie@gmail.com>
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

#include "EllipseAssistant.h"

#include <klocalizedstring.h>
#include "kis_debug.h"
#include <QPainter>
#include <QLinearGradient>
#include <QTransform>

#include <kis_canvas2.h>
#include <kis_coordinates_converter.h>

#include <math.h>

EllipseAssistant::EllipseAssistant()
        : KisPaintingAssistant("ellipse", i18n("Ellipse assistant"))
{
}

QPointF EllipseAssistant::project(const QPointF& pt) const
{
    Q_ASSERT(handles().size() == 3);
    e.set(*handles()[0], *handles()[1], *handles()[2]);
    return e.project(pt);
}

QPointF EllipseAssistant::adjustPosition(const QPointF& pt, const QPointF& /*strokeBegin*/)
{
    return project(pt);

}

void EllipseAssistant::drawAssistant(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter* converter, bool cached, KisCanvas2* canvas, bool assistantVisible, bool previewVisible)
{
    gc.save();
    gc.resetTransform();
    QPoint mousePos;
    
    if (canvas){
        //simplest, cheapest way to get the mouse-position//
        mousePos= canvas->canvasWidget()->mapFromGlobal(QCursor::pos());
    }
    else {
        //...of course, you need to have access to a canvas-widget for that.//
        mousePos = QCursor::pos();//this'll give an offset//
        dbgFile<<"canvas does not exist in the ellipse assistant, you may have passed arguments incorrectly:"<<canvas;
    }

    QTransform initialTransform = converter->documentToWidgetTransform();

    if (outline()==true && boundingRect().contains(initialTransform.inverted().map(mousePos), false) && previewVisible==true){
        if (handles().size() > 2){    
            if (e.set(*handles()[0], *handles()[1], *handles()[2])) {
                // valid ellipse
                gc.setTransform(initialTransform);
                gc.setTransform(e.getInverse(), true);
                QPainterPath path;
                //path.moveTo(QPointF(-e.semiMajor(), 0)); path.lineTo(QPointF(e.semiMajor(), 0));
                //path.moveTo(QPointF(0, -e.semiMinor())); path.lineTo(QPointF(0, e.semiMinor()));
                // Draw the ellipse
                path.addEllipse(QPointF(0, 0), e.semiMajor(), e.semiMinor());
                drawPreview(gc, path);
            }
        }
    }
    gc.restore();
    KisPaintingAssistant::drawAssistant(gc, updateRect, converter, cached, canvas, assistantVisible, previewVisible);

}


void EllipseAssistant::drawCache(QPainter& gc, const KisCoordinatesConverter *converter, bool assistantVisible)
{

    if (assistantVisible==false){return;}
    if (handles().size() < 2) return;
        QTransform initialTransform = converter->documentToWidgetTransform();
    if (handles().size() == 2) {
        // just draw the axis
        gc.setTransform(initialTransform);
        QPainterPath path;
        path.moveTo(*handles()[0]);
        path.lineTo(*handles()[1]);
        drawPath(gc, path, snapping());
        return;
    }
    if (e.set(*handles()[0], *handles()[1], *handles()[2])) {
        // valid ellipse

        gc.setTransform(initialTransform);
        gc.setTransform(e.getInverse(), true);
        QPainterPath path;
        path.moveTo(QPointF(-e.semiMajor(), 0)); path.lineTo(QPointF(e.semiMajor(), 0));
        path.moveTo(QPointF(0, -e.semiMinor())); path.lineTo(QPointF(0, e.semiMinor()));
        // Draw the ellipse
        path.addEllipse(QPointF(0, 0), e.semiMajor(), e.semiMinor());
        drawPath(gc, path, snapping());
    }
}

QRect EllipseAssistant::boundingRect() const
{
    if (handles().size() != 3) return KisPaintingAssistant::boundingRect();
    if (e.set(*handles()[0], *handles()[1], *handles()[2])) {
        return e.boundingRect().adjusted(-2, -2, 2, 2).toAlignedRect();
    } else {
        return QRect();
    }
}

QPointF EllipseAssistant::buttonPosition() const
{
    return (*handles()[0] + *handles()[1]) * 0.5;
}

EllipseAssistantFactory::EllipseAssistantFactory()
{
}

EllipseAssistantFactory::~EllipseAssistantFactory()
{
}

QString EllipseAssistantFactory::id() const
{
    return "ellipse";
}

QString EllipseAssistantFactory::name() const
{
    return i18n("Ellipse");
}

KisPaintingAssistant* EllipseAssistantFactory::createPaintingAssistant() const
{
    return new EllipseAssistant;
}
