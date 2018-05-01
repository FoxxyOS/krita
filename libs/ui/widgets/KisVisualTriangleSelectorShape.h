/*
 * Copyright (C) Wolthera van Hovell tot Westerflier <griffinvalley@gmail.com>, (C) 2016
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
#ifndef KIS_VISUAL_TRIANGLE_SELECTOR_SHAPE_H
#define KIS_VISUAL_TRIANGLE_SELECTOR_SHAPE_H

#include <QWidget>
#include <QScopedPointer>
#include <QPixmap>
#include <QRegion>
#include <QMouseEvent>

#include <KoColor.h>
#include <KoColorSpace.h>
#include "KoColorDisplayRendererInterface.h"

#include "KisColorSelectorConfiguration.h"
#include "KisVisualColorSelectorShape.h"

#include "kritaui_export.h"

class KisVisualTriangleSelectorShape : public KisVisualColorSelectorShape
{
    Q_OBJECT
public:
    enum singelDTypes{border, borderMirrored};
    explicit KisVisualTriangleSelectorShape(QWidget *parent,
                                            Dimensions dimension,
                                            ColorModel model,
                                            const KoColorSpace *cs,
                                            int channel1, int channel2,
                                            const KoColorDisplayRendererInterface *displayRenderer = KoDumbColorDisplayRenderer::instance(),
                                            int barwidth=20
            );
    ~KisVisualTriangleSelectorShape();

    void setBorderWidth(int width);
    void setTriangle();

    /**
     * @brief getSpaceForSquare
     * @param geom the full widget rectangle
     * @return rectangle with enough space for second widget
     */
    virtual QRect getSpaceForSquare(QRect geom);
    virtual QRect getSpaceForCircle(QRect geom);
    virtual QRect getSpaceForTriangle(QRect geom);

protected:
    void resizeEvent(QResizeEvent *);

private:

    virtual QPointF convertShapeCoordinateToWidgetCoordinate(QPointF coordinate);
    virtual QPointF convertWidgetCoordinateToShapeCoordinate(QPoint coordinate);

    singelDTypes m_type;
    int m_barWidth;
    QPolygon m_triangle;
    QPointF m_center;
    qreal m_radius;
    virtual QRegion getMaskMap();
    virtual void drawCursor();
};
#endif
