/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef _KIS_SELECTION_MASK_
#define _KIS_SELECTION_MASK_

#include <QRect>

#include "kis_base_node.h"

#include "kis_types.h"
#include "kis_mask.h"

/**
 * An selection mask is a single channel mask that applies a
 * particular selection to the layer the mask belongs to. A selection
 * can contain both vector and pixel selection components.
*/
class KRITAIMAGE_EXPORT KisSelectionMask : public KisMask
{
    Q_OBJECT
public:

    /**
     * Create an empty selection mask. There is filter and no layer
     * associated with this mask.
     */
    KisSelectionMask(KisImageWSP image);

    virtual ~KisSelectionMask();
    KisSelectionMask(const KisSelectionMask& rhs);

    QIcon icon() const;

    KisNodeSP clone() const {
        return KisNodeSP(new KisSelectionMask(*this));
    }

    /// Set the selection of this adjustment layer to a copy of selection.
    void setSelection(KisSelectionSP selection);

    bool accept(KisNodeVisitor &v);
    void accept(KisProcessingVisitor &visitor, KisUndoAdapter *undoAdapter);

    virtual KisBaseNode::PropertyList sectionModelProperties() const;
    virtual void setSectionModelProperties(const KisBaseNode::PropertyList &properties);

    void setVisible(bool visible, bool isLoading = false);
    bool active() const;
    void setActive(bool active);

    /**
     * This method works like the one in KisSelection, but it
     * compressed the incoming events instead of processing each of
     * them separately.
     */
    void notifySelectionChangedCompressed();

private:
    Q_PRIVATE_SLOT(m_d, void slotSelectionChangedCompressed());

    KisImageWSP image() const;

    struct Private;
    Private * const m_d;
};

#endif //_KIS_SELECTION_MASK_
