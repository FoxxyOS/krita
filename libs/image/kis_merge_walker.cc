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

#include "kis_merge_walker.h"
#include "kis_projection_leaf.h"



KisMergeWalker::KisMergeWalker(QRect cropRect,  Flags flags)
    : m_flags(flags)
{
    setCropRect(cropRect);
}

KisMergeWalker::~KisMergeWalker()
{
}

KisBaseRectsWalker::UpdateType KisMergeWalker::type() const
{
    return m_flags == DEFAULT ? KisBaseRectsWalker::UPDATE : KisBaseRectsWalker::UPDATE_NO_FILTHY;
}

void KisMergeWalker::startTrip(KisProjectionLeafSP startLeaf)
{
    if(startLeaf->isMask()) {
        startTripWithMask(startLeaf);
        return;
    }

    visitHigherNode(startLeaf,
                    m_flags == DEFAULT ? N_FILTHY : N_ABOVE_FILTHY);

    KisProjectionLeafSP prevLeaf = startLeaf->prevSibling();
    if(prevLeaf)
        visitLowerNode(prevLeaf);
}

void KisMergeWalker::startTripWithMask(KisProjectionLeafSP filthyMask)
{
    /**
     * Under very rare circumstances it may happen that the update
     * queue will contain a job pointing to a node that has
     * already been deleted from the image (direclty or by undo
     * command). If it happens to a layer then the walker will
     * handle it as usual by building a trivial graph pointing to
     * nowhere, but when it happens to a mask... not. Because the
     * mask is always expected to have a parent layer to process.
     *
     * So just handle it here separately.
     */
    KisProjectionLeafSP parentLayer = filthyMask->parent();
    if (!parentLayer) {
        return;
    }

    adjustMasksChangeRect(filthyMask);

    KisProjectionLeafSP nextLeaf = parentLayer->nextSibling();
    KisProjectionLeafSP prevLeaf = parentLayer->prevSibling();

    if (nextLeaf)
        visitHigherNode(nextLeaf, N_ABOVE_FILTHY);
    else if (parentLayer->parent())
        startTrip(parentLayer->parent());

    NodePosition positionToFilthy =
        (m_flags == DEFAULT ? N_FILTHY_PROJECTION : N_ABOVE_FILTHY) |
        calculateNodePosition(parentLayer);
    registerNeedRect(parentLayer, positionToFilthy);

    if(prevLeaf)
        visitLowerNode(prevLeaf);
}

void KisMergeWalker::visitHigherNode(KisProjectionLeafSP leaf, NodePosition positionToFilthy)
{
    positionToFilthy |= calculateNodePosition(leaf);

    registerChangeRect(leaf, positionToFilthy);

    KisProjectionLeafSP nextLeaf = leaf->nextSibling();
    if (nextLeaf)
        visitHigherNode(nextLeaf, N_ABOVE_FILTHY);
    else if (leaf->parent())
        startTrip(leaf->parent());

    registerNeedRect(leaf, positionToFilthy);
}

void KisMergeWalker::visitLowerNode(KisProjectionLeafSP leaf)
{
    NodePosition position =
        N_BELOW_FILTHY | calculateNodePosition(leaf);
    registerNeedRect(leaf, position);

    KisProjectionLeafSP prevLeaf = leaf->prevSibling();
    if (prevLeaf)
        visitLowerNode(prevLeaf);
}
