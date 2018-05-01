/*
 *  Copyright (c) 2011 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_node_shapes_graph.h"

#include "kis_node_shape.h"


KisNodeShape* KisNodeShapesGraph::addNode(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis)
{
    KisNodeDummy *parentDummy = 0;
    KisNodeDummy *aboveThisDummy = 0;

    KisNodeShape *parentShape = 0;

    if(parent) {
        parentDummy = nodeToDummy(parent);
        parentShape = parentDummy->nodeShape();
    }

    if(aboveThis) {
        aboveThisDummy = nodeToDummy(aboveThis);
    }

    KisNodeShape *newShape = new KisNodeShape(node);
    ((KoShapeLayer*)newShape)->setParent(parentShape);

    KisNodeDummy *newDummy = new KisNodeDummy(newShape, newShape->node());

    m_dummiesGraph.addNode(newDummy, parentDummy, aboveThisDummy);
    return newShape;
}

void KisNodeShapesGraph::moveNode(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis)
{
    KisNodeDummy *nodeDummy = nodeToDummy(node);
    KisNodeDummy *parentDummy = parent ? nodeToDummy(parent) : 0;
    KisNodeDummy *aboveThisDummy = aboveThis ? nodeToDummy(aboveThis) : 0;

    m_dummiesGraph.moveNode(nodeDummy, parentDummy, aboveThisDummy);
}

void KisNodeShapesGraph::removeNode(KisNodeSP node)
{
    KisNodeDummy *nodeDummy = nodeToDummy(node);

    m_dummiesGraph.removeNode(nodeDummy);

    /**
     * The shapes remove their children automatically,
     * so the dummies do not own them. Delete them manually.
     */
    KisNodeShape *tempShape = nodeDummy->nodeShape();

    delete nodeDummy;
    delete tempShape;
}

KisNodeDummy* KisNodeShapesGraph::rootDummy() const
{
    return m_dummiesGraph.rootDummy();
}

KisNodeDummy* KisNodeShapesGraph::nodeToDummy(KisNodeSP node)
{
    return m_dummiesGraph.nodeToDummy(node);
}

KisNodeShape* KisNodeShapesGraph::nodeToShape(KisNodeSP node)
{
    KisNodeDummy *dummy = nodeToDummy(node);
    if (dummy) {
        return dummy->nodeShape();
    }
    return 0;
}

bool KisNodeShapesGraph::containsNode(KisNodeSP node) const
{
    return m_dummiesGraph.containsNode(node);
}

int KisNodeShapesGraph::shapesCount() const
{
    return m_dummiesGraph.dummiesCount();
}
