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

#include "kis_vec_test.h"

#include <QTest>
#include "kis_vec.h"

void KisVecTest::testCreation()
{
    KisVector2D v2d = KisVector2D::Zero();
    QVERIFY(v2d.x() == 0.0);
    QVERIFY(v2d.y() == 0.0);

    KisVector3D v3d = KisVector3D::Zero();
    QVERIFY(v3d.x() == 0.0);
    QVERIFY(v3d.y() == 0.0);
    QVERIFY(v3d.z() == 0.0);
}


void KisVecTest::testVec2D()
{
}


void KisVecTest::testVec3D()
{
}


QTEST_MAIN(KisVecTest)
