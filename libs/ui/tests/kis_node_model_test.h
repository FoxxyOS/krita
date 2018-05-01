/*
 * Copyright (C) 2007 Boudewijn Rempt <boud@kde.org>
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

#ifndef KISNODEMODEL_TEST_H
#define KISNODEMODEL_TEST_H

#include <QtTest>

#include "empty_nodes_test.h"

class KisDocument;
class KisNameServer;
class KisShapeController;
class KisNodeModel;


class KisNodeModelTest : public QObject, public TestUtil::EmptyNodesTest
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void testSetImage();
    void testAddNode();
    void testRemoveAllNodes();
    void testRemoveIncludingRoot();
    void testSubstituteRootNode();

private:
    KisDocument *m_doc;
    KisNameServer *m_nameServer;
    KisShapeController *m_shapeController;
    KisNodeModel *m_nodeModel;
};

#endif

