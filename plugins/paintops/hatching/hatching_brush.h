/*
 *  Copyright (c) 2008,2009 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (c) 2010 José Luis Vergara <pentalis@gmail.com>
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

#ifndef HATCHING_BRUSH_H_
#define HATCHING_BRUSH_H_

#include <KoColor.h>

#include "kis_hatching_paintop_settings.h"

#include <kis_painter.h>
#include <kis_paint_device.h>
#include <brushengine/kis_paint_information.h>

#include "kis_hatching_options.h"

class HatchingBrush
{

public:
    HatchingBrush(KisHatchingPaintOpSettingsSP settings);
    ~HatchingBrush();
    HatchingBrush(KoColor inkColor);

    /**
     *  Performs a single hatching pass according to specifications
     */
    void hatch(KisPaintDeviceSP dev, qreal x, qreal y, double width, double height, double givenangle, const KoColor &color, qreal additionalScale);


private:
    void init();
    KoColor m_inkColor;
    int m_counter;
    int m_radius;
    KisHatchingPaintOpSettingsSP m_settings;
    KisPainter m_painter;

    /** Thickness in pixels of each hatch line */
    int thickness;

    /** Angle in degrees of all the lines in a single hatching pass*/
    double angle;

    /** Distance separating one line from the other, in pixels */
    double separation;

    /** Height of the imaginary square to be hatched, the "hatching area" */
    double height_;

    /** Width of the imaginary square to be hatched, the "hatching area" */
    double width_;

    /** X coordinate of the point that determines the base line */
    double origin_x;

    /** Y coordinate of the point that determines the base line */
    double origin_y;

    /** Intercept of the base line */
    double baseLineIntercept;

    /** Intercept of the first line _found to_ pass or be neighbour of a line
     *  that passes through the hatching area, this line is used as a base
     *  to start iterating with HatchingBrush::iterateLines()
     */
    double hotIntercept;

    /** Intercept of each line as it is scanned, this value changes constantly */
    double scanIntercept;

    /** X position of the first vertical line _found to_ pass or be neighbour
    *   of a line that passes through the hatching area, this line is used as
    *   a base to start iterating with HatchingBrush::iterateVerticalLines()
    */
    double verticalHotX;

    /** X position of the vertical lines as they are scanned, this value changes constantly */
    double verticalScanX;

    /** Angle of the lines expressed algebraically, as in slope*x + intercept = y */
    double slope;

    /** Unused variable, distance separating non-vertical lines in the X axis*/
    double dx;

    /** Distance separating non-vertical lines in the Y axis*/
    double dy;

    /** Intercept of the line that extends from the mouse cursor position, calculated from
     *  the point (x, y) of the cursor and 'slope'
     */
    double cursorLineIntercept;

    /** Function that begins exploring the field from hotIntercept and
     *  moves in the direction of dy (forward==true) or -dy (forward==false)
     *  to draw all the lines it finds to KisPaintDeviceSP 'dev'
     */
    void iterateLines(bool forward, int lineindex, bool oneline);

    /** Function that begins exploring the field from verticalHotX and
    *   moves in the direction of separation (forward==true) or
    *   -separation (forward==false) to draw all the lines it finds
    *   to KisPaintDeviceSP 'dev'. This function should only be called
    *   when (angle == 90) or (angle == -90)
    */
    void iterateVerticalLines(bool forward, int lineindex, bool oneline);

    /** Simple function that returns a new distance equal to a multiple or
    *   divisor of separation depending on the magnitude of 'parameter' and
    *   the number of intervals of its magnitude.
    *   The multiples and divisors used are all powers of 2 to prevent
    *   desynchronization of the lines during drawing.
    */
    double separationAsFunctionOfParameter(double parameter, double separation, int numintervals);
};

#endif
