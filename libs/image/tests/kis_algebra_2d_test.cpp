/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_algebra_2d_test.h"

#include <QTest>

#include "kis_algebra_2d.h"
#include "kis_debug.h"

namespace KisAlgebra2D {

}

void KisAlgebra2DTest::testHalfPlane()
{
    {
        QPointF a(10,10);
        QPointF b(5,5);

        KisAlgebra2D::RightHalfPlane p(a, b);

        QVERIFY(p.value(QPointF(7, 5)) > 0);
        QVERIFY(p.value(QPointF(3, 5)) < 0);
        QVERIFY(p.value(QPointF(3, 3)) == 0);
    }

    {
        QPointF a(10,10);
        QPointF b(15,10);

        KisAlgebra2D::RightHalfPlane p(a, b);
        QCOMPARE(p.value(QPointF(3, 5)), -5.0);
        QCOMPARE(p.value(QPointF(500, 15)), 5.0);
        QCOMPARE(p.value(QPointF(1000, 10)), 0.0);

        QCOMPARE(p.valueSq(QPointF(3, 5)), -25.0);
        QCOMPARE(p.valueSq(QPointF(500, 15)), 25.0);
        QCOMPARE(p.valueSq(QPointF(1000, 10)), 0.0);

        QCOMPARE(p.pos(QPointF(3, 5)), -1);
        QCOMPARE(p.pos(QPointF(500, 15)), 1);
        QCOMPARE(p.pos(QPointF(1000, 10)), 0);
    }
}

void KisAlgebra2DTest::testOuterCircle()
{
    QPointF a(10,10);
    KisAlgebra2D::OuterCircle p(a, 5);

    QVERIFY(p.value(QPointF(3, 5)) > 0);
    QVERIFY(p.value(QPointF(7, 7)) < 0);
    QVERIFY(p.value(QPointF(10, 5)) == 0);

    QCOMPARE(p.value(QPointF(10, 12)), -3.0);
    QCOMPARE(p.value(QPointF(10, 15)), 0.0);
    QCOMPARE(p.value(QPointF(10, 17)), 2.0);
}

void KisAlgebra2DTest::testQuadraticEquation()
{
    int result = 0;
    qreal x1 = 0;
    qreal x2 = 0;

    result = KisAlgebra2D::quadraticEquation(3, -11, 6, &x1, &x2);

    QCOMPARE(result, 2);
    QCOMPARE(x2, 2.0 / 3.0);
    QCOMPARE(x1, 3.0);

    result = KisAlgebra2D::quadraticEquation(9, -12, 4, &x1, &x2);

    QCOMPARE(result, 1);
    QCOMPARE(x1, 2.0 / 3.0);

    result = KisAlgebra2D::quadraticEquation(9, -1, 4, &x1, &x2);
    QCOMPARE(result, 0);
}

void KisAlgebra2DTest::testIntersections()
{
    QVector<QPointF> points;

    points = KisAlgebra2D::intersectTwoCircles(QPointF(10,10), 5.0,
                                               QPointF(20,10), 5.0);

    QCOMPARE(points.size(), 1);
    QCOMPARE(points[0], QPointF(15, 10));

    points = KisAlgebra2D::intersectTwoCircles(QPointF(10,10), 5.0,
                                               QPointF(18,10), 5.0);

    QCOMPARE(points.size(), 2);
    QCOMPARE(points[0], QPointF(14, 13));
    QCOMPARE(points[1], QPointF(14, 7));

    points = KisAlgebra2D::intersectTwoCircles(QPointF(10,10), 5.0,
                                               QPointF(10,20), 5.0);

    QCOMPARE(points.size(), 1);
    QCOMPARE(points[0], QPointF(10, 15));

    points = KisAlgebra2D::intersectTwoCircles(QPointF(10,10), 5.0,
                                               QPointF(10,18), 5.0);

    QCOMPARE(points.size(), 2);
    QCOMPARE(points[0], QPointF(7, 14));
    QCOMPARE(points[1], QPointF(13, 14));

    points = KisAlgebra2D::intersectTwoCircles(QPointF(10,10), 5.0,
                                               QPointF(17,17), 5.0);

    QCOMPARE(points.size(), 2);
    QCOMPARE(points[0], QPointF(13, 14));
    QCOMPARE(points[1], QPointF(14, 13));

    points = KisAlgebra2D::intersectTwoCircles(QPointF(10,10), 5.0,
                                               QPointF(10,100), 5.0);

    QCOMPARE(points.size(), 0);
}

void KisAlgebra2DTest::testWeirdIntersections()
{
    QVector<QPointF> points;

    QPointF c1 = QPointF(5369.14,3537.98);
    QPointF c2 = QPointF(5370.24,3536.71);
    qreal r1 = 8.5;
    qreal r2 = 10;

    points = KisAlgebra2D::intersectTwoCircles(c1, r1, c2, r2);

    QCOMPARE(points.size(), 2);
    //QCOMPARE(points[0], QPointF(15, 10));
}

QTEST_MAIN(KisAlgebra2DTest)
