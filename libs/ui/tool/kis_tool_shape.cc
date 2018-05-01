/*
 *  Copyright (c) 2005 Adrian Page <adrian@pagenet.plus.com>
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

#include "kis_tool_shape.h"

#include <QWidget>
#include <QLayout>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>

#include <KoShape.h>
#include <KoGradientBackground.h>
#include <KoCanvasBase.h>
#include <KoShapeController.h>
#include <KoColorBackground.h>
#include <KoPatternBackground.h>
#include <KoDocumentResourceManager.h>
#include <KoPathShape.h>

#include <klocalizedstring.h>
#include <ksharedconfig.h>

#include <kis_debug.h>
#include <kis_canvas_resource_provider.h>
#include <brushengine/kis_paintop_registry.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <recorder/kis_recorded_paint_action.h>
#include <recorder/kis_recorded_path_paint_action.h>
#include "kis_figure_painting_tool_helper.h"
#include <recorder/kis_node_query_path.h>
#include <recorder/kis_action_recorder.h>

KisToolShape::KisToolShape(KoCanvasBase * canvas, const QCursor & cursor)
        : KisToolPaint(canvas, cursor)
{
    m_shapeOptionsWidget = 0;
}

KisToolShape::~KisToolShape()
{
    // in case the widget hasn't been shown
    if (m_shapeOptionsWidget && !m_shapeOptionsWidget->parent()) {
        delete m_shapeOptionsWidget;
    }
}

void KisToolShape::activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes)
{
    KisToolPaint::activate(toolActivation, shapes);
    m_configGroup =  KSharedConfig::openConfig()->group(toolId());
}


int KisToolShape::flags() const
{
    return KisTool::FLAG_USES_CUSTOM_COMPOSITEOP|KisTool::FLAG_USES_CUSTOM_PRESET;
}

QWidget * KisToolShape::createOptionWidget()
{
    m_shapeOptionsWidget = new WdgGeometryOptions(0);

    m_shapeOptionsWidget->cmbOutline->setCurrentIndex(KisPainter::StrokeStyleBrush);

    //connect two combo box event. Inherited classes can call the slots to make appropriate changes
    connect(m_shapeOptionsWidget->cmbOutline, SIGNAL(currentIndexChanged(int)), this, SLOT(outlineSettingChanged(int)));
    connect(m_shapeOptionsWidget->cmbFill, SIGNAL(currentIndexChanged(int)), this, SLOT(fillSettingChanged(int)));

    m_shapeOptionsWidget->cmbOutline->setCurrentIndex(m_configGroup.readEntry("outlineType", 0));
    m_shapeOptionsWidget->cmbFill->setCurrentIndex(m_configGroup.readEntry("fillType", 0));

    //if both settings are empty, force the outline to brush so the tool will work when first activated
    if (  m_shapeOptionsWidget->cmbFill->currentIndex() == 0 &&
          m_shapeOptionsWidget->cmbOutline->currentIndex() == 0)
    {
        m_shapeOptionsWidget->cmbOutline->setCurrentIndex(1); // brush
    }

    return m_shapeOptionsWidget;
}

void KisToolShape::outlineSettingChanged(int value)
{
    m_configGroup.writeEntry("outlineType", value);
}

void KisToolShape::fillSettingChanged(int value)
{
    m_configGroup.writeEntry("fillType", value);
}

KisPainter::FillStyle KisToolShape::fillStyle(void)
{
    if (m_shapeOptionsWidget) {
        return static_cast<KisPainter::FillStyle>(m_shapeOptionsWidget->cmbFill->currentIndex());
    } else {
        return KisPainter::FillStyleNone;
    }
}

KisPainter::StrokeStyle KisToolShape::strokeStyle(void)
{
    if (m_shapeOptionsWidget) {
        return static_cast<KisPainter::StrokeStyle>(m_shapeOptionsWidget->cmbOutline->currentIndex());
    } else {
        return KisPainter::StrokeStyleNone;
    }
}

void KisToolShape::setupPaintAction(KisRecordedPaintAction* action)
{
    KisToolPaint::setupPaintAction(action);
    action->setFillStyle(fillStyle());
    action->setStrokeStyle(strokeStyle());
    action->setGenerator(currentGenerator());
    action->setPattern(currentPattern());
    action->setGradient(currentGradient());
}

void KisToolShape::addShape(KoShape* shape)
{
    KoImageCollection* imageCollection = canvas()->shapeController()->resourceManager()->imageCollection();
    switch(fillStyle()) {
        case KisPainter::FillStyleForegroundColor:
            shape->setBackground(QSharedPointer<KoColorBackground>(new KoColorBackground(currentFgColor().toQColor())));
            break;
        case KisPainter::FillStyleBackgroundColor:
            shape->setBackground(QSharedPointer<KoColorBackground>(new KoColorBackground(currentBgColor().toQColor())));
            break;
        case KisPainter::FillStylePattern:
            if (imageCollection) {
                QSharedPointer<KoPatternBackground> fill(new KoPatternBackground(imageCollection));
                if (currentPattern()) {
                    fill->setPattern(currentPattern()->pattern());
                    shape->setBackground(fill);
                }
            } else {
                shape->setBackground(QSharedPointer<KoShapeBackground>(0));
            }
            break;
        case KisPainter::FillStyleGradient:
            {
                QLinearGradient *gradient = new QLinearGradient(QPointF(0, 0), QPointF(1, 1));
                gradient->setCoordinateMode(QGradient::ObjectBoundingMode);
                gradient->setStops(currentGradient()->toQGradient()->stops());
                QSharedPointer<KoGradientBackground>  gradientFill(new KoGradientBackground(gradient));
                shape->setBackground(gradientFill);
            }
            break;
        case KisPainter::FillStyleNone:
        default:
            shape->setBackground(QSharedPointer<KoShapeBackground>(0));
            break;
    }
    KUndo2Command * cmd = canvas()->shapeController()->addShape(shape);
    canvas()->addCommand(cmd);
}

void KisToolShape::addPathShape(KoPathShape* pathShape, const KUndo2MagicString& name)
{
    KisNodeSP node = currentNode();
    if (!node || !blockUntillOperationsFinished()) {
        return;
    }
    // Get painting options
    KisPaintOpPresetSP preset = currentPaintOpPreset();

    // Compute the outline
    KisImageWSP image = this->image();
    QTransform matrix;
    matrix.scale(image->xRes(), image->yRes());
    matrix.translate(pathShape->position().x(), pathShape->position().y());
    QPainterPath mapedOutline = matrix.map(pathShape->outline());

    // Recorde the paint action
    KisRecordedPathPaintAction bezierCurvePaintAction(
            KisNodeQueryPath::absolutePath(node),
            preset );
    bezierCurvePaintAction.setPaintColor(currentFgColor());
    QPointF lastPoint, nextPoint;
    int elementCount = mapedOutline.elementCount();
    for (int i = 0; i < elementCount; i++) {
        QPainterPath::Element element = mapedOutline.elementAt(i);
        switch (element.type) {
        case QPainterPath::MoveToElement:
            if (i != 0) {
                qFatal("Unhandled"); // XXX: I am not sure the tool can produce such element, deal with it when it can
            }
            lastPoint =  QPointF(element.x, element.y);
            break;
        case QPainterPath::LineToElement:
            nextPoint =  QPointF(element.x, element.y);
            bezierCurvePaintAction.addLine(KisPaintInformation(lastPoint), KisPaintInformation(nextPoint));
            lastPoint = nextPoint;
            break;
        case QPainterPath::CurveToElement:
            nextPoint =  QPointF(mapedOutline.elementAt(i + 2).x, mapedOutline.elementAt(i + 2).y);
            bezierCurvePaintAction.addCurve(KisPaintInformation(lastPoint),
                                             QPointF(mapedOutline.elementAt(i).x,
                                                     mapedOutline.elementAt(i).y),
                                             QPointF(mapedOutline.elementAt(i + 1).x,
                                                     mapedOutline.elementAt(i + 1).y),
                                             KisPaintInformation(nextPoint));
            lastPoint = nextPoint;
            break;
        default:
            continue;
        }
    }
    image->actionRecorder()->addAction(bezierCurvePaintAction);

    if (node->hasEditablePaintDevice()) {
        KisFigurePaintingToolHelper helper(name,
                                           image,
                                           node,
                                           canvas()->resourceManager(),
                                           strokeStyle(),
                                           fillStyle());
        helper.paintPainterPath(mapedOutline);
    } else if (node->inherits("KisShapeLayer")) {
        pathShape->normalize();
        addShape(pathShape);

    }

    notifyModified();
}

