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

#ifndef KIS_TRANSFORM_WORKER_TEST_H
#define KIS_TRANSFORM_WORKER_TEST_H

#include <QtTest>

class KisTransformWorkerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void testCreation();

    void testMirrorX_Even();
    void testMirrorX_Odd();
    void testMirrorY_Even();
    void testMirrorY_Odd();

    void benchmarkMirrorX();
    void benchmarkMirrorY();

    void testOffset();
    void testMirrorTransactionX();
    void testMirrorTransactionY();
    void testIdentity();
    void testScaleUp();
    void testXScaleUp();
    void testYScaleUp();
    void testScaleDown();
    void testXScaleDown();
    void testYScaleDown();
    void testXShear();
    void testYShear();
    void testRotation();
    void testMatrices();
    void testRotationSpecialCases();
    void testScaleUp5times();
    void rotate90Left();
    void rotate90Right();
    void rotate180();

    void benchmarkScale();
    void benchmarkRotate();
    void benchmarkRotate1Q();
    void benchmarkShear();
    void benchmarkScaleRotateShear();

    void testPartialProcessing();

private:
    void generateTestImages();
};

#endif
