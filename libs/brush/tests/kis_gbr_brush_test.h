/*
 *  Copyright (c) 2007 Boudewijn Rempt boud@valdyas.org
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

#ifndef KIS_BRUSH_TEST_H
#define KIS_BRUSH_TEST_H

#include <QtTest>

class KisGbrBrushTest : public QObject
{
    Q_OBJECT

    // XXX disabled until I figure out why they don't work from here, while the brushes do work from Krita
    void testMaskGenerationNoColor();
    void testMaskGenerationSingleColor();
    void testMaskGenerationDevColor();
    void testMaskGenerationDefaultColor();

private Q_SLOTS:

    void testImageGeneration();

    void benchmarkPyramidCreation();
    void benchmarkScaling();
    void benchmarkRotation();
    void benchmarkMaskScaling();

    void testPyramidLevelRounding();
    void testPyramidDabTransform();

    void testQPainterTransformationBorder();
};

#endif
