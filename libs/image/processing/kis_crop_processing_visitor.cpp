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

#include "kis_crop_processing_visitor.h"

#include <klocalizedstring.h>

#include "commands_new/kis_node_move_command2.h"

#include "kis_external_layer_iface.h"

#include "kis_paint_device.h"
#include "kis_image.h"
#include "kis_transaction.h"
#include "kis_undo_adapter.h"
#include "kis_transform_mask.h"
#include "lazybrush/kis_colorize_mask.h"


KisCropProcessingVisitor::KisCropProcessingVisitor(const QRect &rect, bool cropLayers, bool moveLayers)
    : m_rect(rect),
      m_cropLayers(cropLayers),
      m_moveLayers(moveLayers)

{
}

void KisCropProcessingVisitor::visitExternalLayer(KisExternalLayer *layer, KisUndoAdapter *undoAdapter)
{
    KUndo2Command* command = layer->crop(m_rect);
    undoAdapter->addCommand(command);
}

void KisCropProcessingVisitor::moveNodeImpl(KisNode *node, KisUndoAdapter *undoAdapter)
{
    if (m_moveLayers) {
        QPoint oldPos(node->x(), node->y());
        QPoint newPos(node->x() - m_rect.x(), node->y() - m_rect.y());
        KUndo2Command *command = new KisNodeMoveCommand2(node, oldPos, newPos);
        undoAdapter->addCommand(command);
    }
}

void KisCropProcessingVisitor::cropPaintDeviceImpl(KisPaintDeviceSP device, KisUndoAdapter *undoAdapter)
{
    /**
     * TODO: implement actual robust cropping of the selections,
     * including the cropping of vector (!) selection.
     */

    if (m_cropLayers) {
        KisTransaction transaction(kundo2_noi18n("crop"), device);
        device->crop(m_rect);
        transaction.commit(undoAdapter);
    }
}

void KisCropProcessingVisitor::visitNodeWithPaintDevice(KisNode *node, KisUndoAdapter *undoAdapter)
{
    cropPaintDeviceImpl(node->paintDevice(), undoAdapter);
    moveNodeImpl(node, undoAdapter);
}

void KisCropProcessingVisitor::visit(KisTransformMask *node, KisUndoAdapter *undoAdapter)
{
    moveNodeImpl(node, undoAdapter);
    KisSimpleProcessingVisitor::visit(node, undoAdapter);
}

void KisCropProcessingVisitor::visitColorizeMask(KisColorizeMask *node, KisUndoAdapter *undoAdapter)
{
    QVector<KisPaintDeviceSP> devices = node->allPaintDevices();

    Q_FOREACH (KisPaintDeviceSP device, devices) {
        cropPaintDeviceImpl(device, undoAdapter);
    }

    moveNodeImpl(node, undoAdapter);
}
