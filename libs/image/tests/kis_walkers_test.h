/*
 *  Copyright (c) 2009 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef KIS_WALKERS_TEST_H
#define KIS_WALKERS_TEST_H

#include <QtTest>

#include "kis_image.h"
#include "kis_layer.h"
#include "kis_types.h"
#include "kis_node_visitor.h"
#include "kis_default_bounds.h"
#include "kis_paint_device.h"
#include "kis_merge_walker.h"
#include "kis_image.h"

class TestLayer : public KisLayer
{

    Q_OBJECT

public:

    TestLayer(KisImageWSP image, const QString & name, quint8 opacity)
            : KisLayer(image, name, opacity) {
    }

    KisNodeSP clone() {
        return new TestLayer(*this);
    }
    bool allowAsChild(KisNodeSP) const {
        return true;
    }

    virtual QString nodeType() {
        return "TEST";
    }

    KisPaintDeviceSP original() const {
        // This test doesn't use updateProjection so just return 0
        return 0;
    }

    KisPaintDeviceSP paintDevice() const {
        return 0;
    }

    QIcon icon() const {
        return QIcon();
    }

    KisNodeSP clone() const {
        return new TestLayer(image(), name(), opacity());
    }

    qint32 x() const {
        return 0;
    }

    void setX(qint32) {
    }

    qint32 y() const {
        return 0;
    }

    void setY(qint32) {
    }

    QRect extent() const {
        return QRect();
    }

    QRect exactBounds() const {
        return QRect();
    }

    using KisLayer::accept;

    bool accept(KisNodeVisitor& v) {
        return v.visit(this);
    }
};

class ComplexRectsLayer : public KisLayer
{

    Q_OBJECT

public:

    ComplexRectsLayer(KisImageWSP image, const QString & name, quint8 opacity)
            : KisLayer(image, name, opacity) {

        m_device = new KisPaintDevice(this, image->colorSpace(), new KisDefaultBounds(image), "test device");
    }

    KisNodeSP clone() {
        return new ComplexRectsLayer(*this);
    }
    bool allowAsChild(KisNodeSP) const {
        return true;
    }

    virtual QString nodeType() {
        return "TEST";
    }

    KisPaintDeviceSP original() const {
        return m_device;
    }

    KisPaintDeviceSP paintDevice() const {
        return m_device;
    }

    QIcon icon() const {
        return QIcon();
    }

    KisNodeSP clone() const {
        return new ComplexRectsLayer(image(), name(), opacity());
    }

    qint32 x() const {
        return 0;
    }

    void setX(qint32) {
    }

    qint32 y() const {
        return 0;
    }

    void setY(qint32) {
    }

    QRect extent() const {
        return QRect();
    }

    QRect exactBounds() const {
        return QRect();
    }

    using KisLayer::accept;

    bool accept(KisNodeVisitor& v) {
        return v.visit(this);
    }

    QRect changeRect(const QRect &rect, PositionToFilthy pos = N_FILTHY) const {
        Q_UNUSED(pos);
        const qint32 delta = 3;
        return rect.adjusted(-delta, -delta, delta, delta);
    }

    QRect needRect(const QRect &rect, PositionToFilthy pos = N_FILTHY) const {
        Q_UNUSED(pos);
        const qint32 delta = 7;
        return rect.adjusted(-delta, -delta, delta, delta);
    }
private:
    KisPaintDeviceSP m_device;
};

class CacheLayer : public ComplexRectsLayer
{

    Q_OBJECT

public:
    CacheLayer(KisImageWSP image, const QString & name, quint8 opacity)
            : ComplexRectsLayer(image, name, opacity) {
    }


    QRect needRect(const QRect &rect, PositionToFilthy pos = N_FILTHY) const {
        QRect retval;

        if(pos != KisNode::N_BELOW_FILTHY && pos != N_FILTHY_PROJECTION) {
            const qint32 delta = 7;
            retval = rect.adjusted(-delta, -delta, delta, delta);
        }
        return retval;
    }
};

class ComplexAccessLayer : public ComplexRectsLayer
{

    Q_OBJECT

public:
    ComplexAccessLayer(KisImageWSP image, const QString & name, quint8 opacity)
            : ComplexRectsLayer(image, name, opacity) {
    }


    QRect accessRect(const QRect &rect, PositionToFilthy pos = N_FILTHY) const {
        Q_UNUSED(pos);

        const qint32 delta = 70;
        return rect.translated(delta, 0);
    }

    QRect needRect(const QRect &rect, PositionToFilthy pos = N_FILTHY) const {
        Q_UNUSED(pos);
        return rect;
    }

    QRect changeRect(const QRect &rect, PositionToFilthy pos = N_FILTHY) const {
        Q_UNUSED(pos);
        return rect;
    }
};

class KisBaseRectsWalker;

class KisWalkersTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testUsualVisiting();
    void testVisitingWithTopmostMask();
    void testMergeVisiting();
    void testComplexAccessVisiting();
    void testCloneNotificationsVisiting();
    void testRefreshSubtreeVisiting();
    void testFullRefreshVisiting();
    void testCachedVisiting();
    void testMasksVisiting();
    void testMasksVisitingNoFilthy();
    void testMasksOverlapping();
    void testRectsChecksum();
    void testGraphStructureChecksum();

private:
    void verifyResult(KisBaseRectsWalker &walker, QStringList reference,
                      QRect accessRect, bool changeRectVaries,
                      bool needRectVaries);
    void verifyResult(KisBaseRectsWalker &walker, struct UpdateTestJob &job);

    void checkNotification(const KisMergeWalker::CloneNotification &notification,
                           const QString &name,
                           const QRect &rect);
};

#endif

