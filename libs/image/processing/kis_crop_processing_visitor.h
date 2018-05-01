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

#ifndef __KIS_CROP_PROCESSING_VISITOR_H
#define __KIS_CROP_PROCESSING_VISITOR_H

#include "kis_simple_processing_visitor.h"
#include <QRect>
#include "kis_types.h"


class KRITAIMAGE_EXPORT  KisCropProcessingVisitor : public KisSimpleProcessingVisitor
{
public:
    KisCropProcessingVisitor(const QRect &rect, bool cropLayers, bool moveLayers);

private:
    void visitNodeWithPaintDevice(KisNode *node, KisUndoAdapter *undoAdapter);
    void visitExternalLayer(KisExternalLayer *layer, KisUndoAdapter *undoAdapter);

public:

    void visit(KisTransformMask *mask, KisUndoAdapter *undoAdapter);
    void visitColorizeMask(KisColorizeMask *mask, KisUndoAdapter *undoAdapter);
    using KisSimpleProcessingVisitor::visit;

private:
    void moveNodeImpl(KisNode *node, KisUndoAdapter *undoAdapter);
    void cropPaintDeviceImpl(KisPaintDeviceSP device, KisUndoAdapter *undoAdapter);

private:
    QRect m_rect;
    bool m_cropLayers;
    bool m_moveLayers;
};

#endif /* __KIS_CROP_PROCESSING_VISITOR_H */
