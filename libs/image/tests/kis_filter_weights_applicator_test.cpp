/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_filter_weights_applicator_test.h"

#include <QTest>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include "kis_paint_device.h"

//#define DEBUG_ENABLED
#include "kis_filter_weights_applicator.h"

void debugSpan(const KisFilterWeightsApplicator::BlendSpan &span)
{
    dbgKrita << ppVar(span.weights->centerIndex);
    for (int i = 0; i < span.weights->span; i++) {
        dbgKrita << "Weights" << i << span.weights->weight[i];
    }

    dbgKrita << ppVar(span.firstBlendPixel);
    dbgKrita << ppVar(span.offset);
    dbgKrita << ppVar(span.offsetInc);
}

void testSpan(qreal scale, qreal dx, int dst_l,
              int expectedFirstPixel,
              qreal expectedOffset,
              qreal expectedOffsetInc)
{
    KisFilterStrategy *filter = new KisBilinearFilterStrategy();

    KisFilterWeightsBuffer buf(filter, qAbs(scale));
    KisFilterWeightsApplicator applicator(0, 0, scale, 0.0, dx, false);
    KisFilterWeightsApplicator::BlendSpan span;
    span = applicator.calculateBlendSpan(dst_l, 0, &buf);

    //debugSpan(span);

    if (span.firstBlendPixel != expectedFirstPixel ||
        span.offset != KisFixedPoint(expectedOffset) ||
        span.offsetInc != KisFixedPoint(expectedOffsetInc)) {

        dbgKrita << "Failed to generate a span:";
        dbgKrita << ppVar(scale) << ppVar(dx) << ppVar(dst_l);
        dbgKrita << ppVar(span.firstBlendPixel) << ppVar(expectedFirstPixel);
        dbgKrita << ppVar(span.offset) << ppVar(KisFixedPoint(expectedOffset));
        dbgKrita << ppVar(span.offsetInc) << ppVar(KisFixedPoint(expectedOffsetInc));
        QFAIL("fail");
    }
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_2_0_Aligned()
{
    testSpan(2.0, 0.0, 0, -1, 0.25, 1.0);
    testSpan(2.0, 0.0, 1, 0, 0.75, 1.0);
    testSpan(2.0, 0.0, -1, -1, 0.75, 1.0);
    testSpan(2.0, 0.0, -2, -2, 0.25, 1.0);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_2_0_Shift_0_5()
{
    testSpan(2.0, 0.5, 0, -1, 0.5, 1.0);
    testSpan(2.0, 0.5, 1, -1, 0.0, 1.0);
    testSpan(2.0, 0.5, -1, -2, 0.0, 1.0);
    testSpan(2.0, 0.5, -2, -2, 0.5, 1.0);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_2_0_Shift_0_75()
{
    testSpan(2.0, 0.75, 0, -1, 0.625, 1.0);
    testSpan(2.0, 0.75, 1, -1, 0.125, 1.0);
    testSpan(2.0, 0.75, -1, -2, 0.125, 1.0);
    testSpan(2.0, 0.75, -2, -2, 0.625, 1.0);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Aligned()
{
    testSpan(0.5, 0.0, 0, -1, 0.25, 0.5);
    testSpan(0.5, 0.0, 1, 1, 0.25, 0.5);
    testSpan(0.5, 0.0, -1, -3, 0.25, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Shift_0_5()
{
    testSpan(0.5, 0.5, 0, -2, 0.25, 0.5);
    testSpan(0.5, 0.5, 1, 0, 0.25, 0.5);
    testSpan(0.5, 0.5, -1, -4, 0.25, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Shift_0_25()
{
    testSpan(0.5, 0.25, 0, -2, 0.0, 0.5);
    testSpan(0.5, 0.25, 1, 0, 0.0, 0.5);
    testSpan(0.5, 0.25, -1, -4, 0.0, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Shift_0_375()
{
    testSpan(0.5, 0.375, 0, -2, 0.125, 0.5);
    testSpan(0.5, 0.375, 1, 0, 0.125, 0.5);
    testSpan(0.5, 0.375, -1, -4, 0.125, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Shift_m0_5()
{
    testSpan(0.5, -0.5, 0, 0, 0.25, 0.5);
    testSpan(0.5, -0.5, 1, 2, 0.25, 0.5);
    testSpan(0.5, -0.5, -1, -2, 0.25, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Shift_m0_25()
{
    testSpan(0.5, -0.25, 0, -1, 0.0, 0.5);
    testSpan(0.5, -0.25, 1, 1, 0.0, 0.5);
    testSpan(0.5, -0.25, -1, -3, 0.0, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Shift_m0_375()
{
    testSpan(0.5, -0.375, 0, 0, 0.375, 0.5);
    testSpan(0.5, -0.375, 1, 2, 0.375, 0.5);
    testSpan(0.5, -0.375, -1, -2, 0.375, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_1_0_Aligned_Mirrored()
{
    testSpan(-1.0, 0.0, 0, -2, 0.0, 1.0);
    testSpan(-1.0, 0.0, 1, -3, 0.0, 1.0);
    testSpan(-1.0, 0.0, -1, -1, 0.0, 1.0);
    testSpan(-1.0, 0.0, -2, 0, 0.0, 1.0);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Aligned_Mirrored()
{
    testSpan(-0.5, 0.0, 0, -3, 0.25, 0.5);
    testSpan(-0.5, 0.0, 1, -5, 0.25, 0.5);
    testSpan(-0.5, 0.0, -1, -1, 0.25, 0.5);
    testSpan(-0.5, 0.0, -2, 1, 0.25, 0.5);
}

void KisFilterWeightsApplicatorTest::testSpan_Scale_0_5_Shift_0_125_Mirrored()
{
    testSpan(-0.5, 0.125, 0, -3, 0.125, 0.5);
    testSpan(-0.5, 0.125, 1, -5, 0.125, 0.5);
    testSpan(-0.5, 0.125, -1, -1, 0.125, 0.5);
    testSpan(-0.5, 0.125, -2, 1, 0.125, 0.5);
}

void printPixels(KisPaintDeviceSP dev, int x0, int len, bool horizontal)
{
    for (int i = x0; i < x0 + len; i++) {
        QColor c;

        int x = horizontal ? i : 0;
        int y = horizontal ? 0 : i;

        dev->pixel(x, y, &c);
        dbgKrita << "px" << x << y << "|" << c.red() << c.green() << c.blue() << c.alpha();
    }
}

void checkRA(KisPaintDeviceSP dev, int x0, int len, quint8 r[], quint8 a[], bool horizontal)
{
    for (int i = 0; i < len; i++) {
        QColor c;

        int x = horizontal ? x0 + i : 0;
        int y = horizontal ? 0 : x0 + i;

        dev->pixel(x, y, &c);

        if (c.red() != r[i] ||
            c.alpha() != a[i]) {

            dbgKrita << "Failed to compare RA channels:" << ppVar(x0 + i);
            dbgKrita << "Red:" << c.red() << "Expected:" << r[i];
            dbgKrita << "Alpha:" << c.alpha() << "Expected:" << a[i];
            QFAIL("failed");
        }
    }
}

void testLineImpl(qreal scale, qreal dx, quint8 expR[], quint8 expA[], int x0, int len, bool clampToEdge, bool horizontal)
{
    const KoColorSpace *cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP dev = new KisPaintDevice(cs);
    KisFilterStrategy *filter = new KisBilinearFilterStrategy();

    KisFilterWeightsBuffer buf(filter, qAbs(scale));
    KisFilterWeightsApplicator applicator(dev, dev, scale, 0.0, dx, clampToEdge);

    for (int i = 0; i < 4; i++) {
        int x = horizontal ? i : 0;
        int y = horizontal ? 0 : i;
        dev->setPixel(x,y,QColor(10 + i * 10, 20 + i * 10, 40 + i * 10));
    }

    {
        quint8 r[] = {  0, 10, 20, 30, 40,  0,  0};
        quint8 a[] = {  0,255,255,255,255,  0,  0};
        checkRA(dev, -1, 6, r, a, horizontal);
    }

    KisFilterWeightsApplicator::LinePos srcPos(0,4);
    KisFilterWeightsApplicator::LinePos dstPos;

    if (horizontal) {
        dstPos = applicator.processLine<KisHLineIteratorSP>(srcPos,0,&buf, filter->support());
    } else {
        dstPos = applicator.processLine<KisVLineIteratorSP>(srcPos,0,&buf, filter->support());
    }

    QRect rc = dev->exactBounds();

    if (horizontal) {
        QVERIFY(rc.left() >= dstPos.start());
        QVERIFY(rc.left() + rc.width() <= dstPos.end());
    } else {
        QVERIFY(rc.top() >= dstPos.start());
        QVERIFY(rc.top() + rc.height() <= dstPos.end());
    }

    //printPixels(dev, x0, len, horizontal);
    checkRA(dev, x0, len, expR, expA, horizontal);
}

void testLine(qreal scale, qreal dx, quint8 expR[], quint8 expA[], int x0, int len, bool clampToEdge = false)
{
    testLineImpl(scale, dx, expR, expA, x0, len, clampToEdge, true);
    testLineImpl(scale, dx, expR, expA, x0, len, clampToEdge, false);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Aligned()
{
    qreal scale = 1.0;
    qreal dx = 0.0;

    quint8 r[] = {  0, 10, 20, 30, 40,  0,  0};
    quint8 a[] = {  0,255,255,255,255,  0,  0};

    testLine(scale, dx, r, a, -1, 7);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Shift_0_5()
{
    qreal scale = 1.0;
    qreal dx = 0.5;

    quint8 r[] = {  0, 10, 15, 25, 35, 40,  0};
    quint8 a[] = {  0,128,255,255,255,127,  0};

    testLine(scale, dx, r, a, -1, 7);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Shift_m0_5()
{
    qreal scale = 1.0;
    qreal dx = -0.5;

    quint8 r[] = { 10, 15, 25, 35, 40,  0,  0};
    quint8 a[] = {128,255,255,255,127,  0,  0};

    testLine(scale, dx, r, a, -1, 7);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Shift_0_25()
{
    qreal scale = 1.0;
    qreal dx = 0.25;

    quint8 r[] = {  0, 10, 17, 27, 37, 40,  0};
    quint8 a[] = {  0,191,255,255,255, 64,  0};

    testLine(scale, dx, r, a, -1, 7);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Shift_m0_25()
{
    qreal scale = 1.0;
    qreal dx = -0.25;

    quint8 r[] = { 10, 12, 22, 32, 40,  0,  0};
    quint8 a[] = { 64,255,255,255,191,  0,  0};

    testLine(scale, dx, r, a, -1, 7);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_0_5_Aligned()
{
    qreal scale = 0.5;
    qreal dx = 0.0;

    quint8 r[] = { 10, 17, 32, 40,  0,  0,  0};
    quint8 a[] = { 32,223,223, 32,  0,  0,  0};

    testLine(scale, dx, r, a, -1, 7);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_0_5_Shift_0_25()
{
    qreal scale = 0.5;
    qreal dx = 0.25;

    quint8 r[] = {  0, 13, 30, 40,  0,  0,  0};
    quint8 a[] = {  0,191,255, 64,  0,  0,  0};

    testLine(scale, dx, r, a, -1, 7);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_2_0_Aligned()
{
    qreal scale = 2.0;
    qreal dx = 0.0;

    quint8 r[] = {  0, 10, 10, 12, 17, 22, 27, 32, 37, 40, 40,  0};
    quint8 a[] = {  0, 64,191,255,255,255,255,255,255,191, 64,  0};

    testLine(scale, dx, r, a, -2, 12);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_2_0_Shift_0_25()
{
    qreal scale = 2.0;
    qreal dx = 0.25;

    quint8 r[] = {  0, 10, 10, 11, 16, 21, 26, 31, 36, 40, 40,  0};
    quint8 a[] = {  0, 32,159,255,255,255,255,255,255,223, 96,  0};

    testLine(scale, dx, r, a, -2, 12);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_2_0_Shift_0_5()
{
    qreal scale = 2.0;
    qreal dx = 0.5;

    quint8 r[] = {  0,  0, 10, 10, 15, 20, 25, 30, 35, 40, 40,  0};
    quint8 a[] = {  0,  0,128,255,255,255,255,255,255,255,127,  0};

    testLine(scale, dx, r, a, -2, 12);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Aligned_Clamped()
{
    qreal scale = 1.0;
    qreal dx = 0.0;

    quint8 r[] = {  0, 10, 20, 30, 40,  0,  0};
    quint8 a[] = {  0,255,255,255,255,  0,  0};

    testLine(scale, dx, r, a, -1, 7, true);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_0_5_Aligned_Clamped()
{
    qreal scale = 0.5;
    qreal dx = 0.0;

    quint8 r[] = {  0, 16, 33,  0,  0,  0,  0};
    quint8 a[] = {  0,255,255,  0,  0,  0,  0};

    testLine(scale, dx, r, a, -1, 7, true);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_2_0_Aligned_Clamped()
{
    qreal scale = 2.0;
    qreal dx = 0.0;

    quint8 r[] = {  0,  0, 10, 12, 17, 22, 27, 32, 37, 40,  0,  0};
    quint8 a[] = {  0,  0,255,255,255,255,255,255,255,255,  0,  0};

    testLine(scale, dx, r, a, -2, 12, true);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Aligned_Mirrored()
{
    qreal scale = -1.0;
    qreal dx = 0.0;

    quint8 r[] = {  0,  0, 40, 30, 20, 10,  0,  0,  0,  0};
    quint8 a[] = {  0,  0,255,255,255,255,  0,  0,  0,  0};

    testLine(scale, dx, r, a, -6, 10);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_1_0_Shift_0_25_Mirrored()
{
    qreal scale = -1.0;
    qreal dx = 0.25;

    quint8 r[] = {  0,  0, 40, 32, 22, 12, 10,  0,  0,  0};
    quint8 a[] = {  0,  0,191,255,255,255, 64,  0,  0,  0};

    testLine(scale, dx, r, a, -6, 10);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_0_5_Aligned_Mirrored_Clamped()
{
    qreal scale = -0.5;
    qreal dx = 0.0;

    quint8 r[] = {  0,  0,  0,  0, 33, 16,  0,  0,  0,  0};
    quint8 a[] = {  0,  0,  0,  0,255,255,  0,  0,  0,  0};

    testLine(scale, dx, r, a, -6, 10, true);
}

void KisFilterWeightsApplicatorTest::testProcessLine_Scale_0_5_Shift_0_125_Mirrored()
{
    qreal scale = -0.5;
    qreal dx = 0.125;

    quint8 r[] = {  0,  0,  0, 40, 34, 18, 10,  0,  0,  0};
    quint8 a[] = {  0,  0,  0, 16,207,239, 48,  0,  0,  0};

    testLine(scale, dx, r, a, -6, 10);
}

void KisFilterWeightsApplicatorTest::benchmarkProcesssLine()
{
    const KoColorSpace *cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP dev = new KisPaintDevice(cs);
    KisFilterStrategy *filter = new KisBilinearFilterStrategy();

    const qreal scale = 0.873;
    const qreal dx = 0.0387;

    KisFilterWeightsBuffer buf(filter, qAbs(scale));
    KisFilterWeightsApplicator applicator(dev, dev, scale, 0.0, dx, false);

    for (int i = 0; i < 32767; i++) {
        dev->setPixel(i,0,QColor(10 + i%240,20,40));
    }

    KisFilterWeightsApplicator::LinePos linePos(0,32767);

    QBENCHMARK {
        applicator.processLine<KisHLineIteratorSP>(linePos,0,&buf, filter->support());
    }
}

QTEST_MAIN(KisFilterWeightsApplicatorTest)
