/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#ifndef KIS_COUNT_VISITOR_TESTER_H
#define KIS_COUNT_VISITOR_TESTER_H

#include <QtTest>

#include "kis_node.h"
#include <sdk/tests/testing_nodes.h>

class RootNode : public TestUtil::DefaultNode
{
    Q_OBJECT
public:
    KisNodeSP clone() const {
        return new RootNode(*this);
    }
};


class TestNodeA : public TestUtil::DefaultNode
{
    Q_OBJECT
public:
    KisNodeSP clone() const {
        return new TestNodeA(*this);
    }
};

class TestNodeB : public TestUtil::DefaultNode
{
    Q_OBJECT
public:
    KisNodeSP clone() const {
        return new TestNodeB(*this);
    }
};

class TestNodeC : public TestUtil::DefaultNode
{
    Q_OBJECT
public:
    KisNodeSP clone() const {
        return new TestNodeC(*this);
    }
};


class KisCountVisitorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testCounting();
};

#endif

