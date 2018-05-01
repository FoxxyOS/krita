/*
 * Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com
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

#include <kis_gmic_synchronize_layers_command.h>
#include "kis_import_gmic_processing_visitor.h"
#include <kis_paint_layer.h>
#include <kis_image.h>
#include <kis_selection.h>
#include <kis_types.h>
#include <KoColorSpaceConstants.h>

#include <commands/kis_image_layer_add_command.h>

KisGmicSynchronizeLayersCommand::KisGmicSynchronizeLayersCommand(KisNodeListSP nodes, QSharedPointer< cimg_library::CImgList< float > > images, KisImageWSP image, const QRect &dstRect, KisSelectionSP selection)
    :   KUndo2Command(),
        m_nodes(nodes),
        m_images(images),
        m_image(image),
        m_dstRect(dstRect),
        m_selection(selection),
        m_firstRedo(true)
{
}

KisGmicSynchronizeLayersCommand::~KisGmicSynchronizeLayersCommand()
{
    qDeleteAll(m_imageCommands);
}


void KisGmicSynchronizeLayersCommand::redo()
{
    if (m_firstRedo)
    {
        // if gmic produces more layers
        if (m_nodes->size() < int(m_images->_width))
        {

            if (m_image)
            {

                int nodesCount = m_nodes->size();
                for (unsigned int i = nodesCount; i < m_images->_width; i++)
                {

                    KisPaintDevice * device = new KisPaintDevice(m_image->colorSpace());
                    KisLayerSP paintLayer = new KisPaintLayer(m_image, "New layer from gmic filter", OPACITY_OPAQUE_U8, device);
                    KisImportGmicProcessingVisitor::gmicImageToPaintDevice(m_images->_data[i], device);

                    KisNodeSP aboveThis = m_nodes->last();
                    KisNodeSP parent = m_nodes->at(0)->parent();

                    dbgPlugins << "Adding paint layer " << (i - nodesCount + 1) << " to parent " << parent->name();
                    KisImageLayerAddCommand * addLayerCmd = new KisImageLayerAddCommand(m_image, paintLayer, parent, aboveThis, false, true);
                    addLayerCmd->redo();
                    m_imageCommands.append(addLayerCmd);
                    m_nodes->append(paintLayer);

                }
            }
            else // small preview
            {
                Q_ASSERT(m_nodes->size() > 0);
                for (unsigned int i = m_nodes->size(); i < m_images->_width; i++)
                {
                    KisPaintDevice * device = new KisPaintDevice(m_nodes->at(0)->colorSpace());
                    KisLayerSP paintLayer = new KisPaintLayer(0, "New layer from gmic filter", OPACITY_OPAQUE_U8, device);
                    m_nodes->append(paintLayer);
                }
            }
        } // if gmic produces less layers, we are going to drop some
        else if (m_nodes->size() > int(m_images->_width))
        {
            dbgPlugins << "no support for removing layers yet!!";
        }
    }
    else
    {
        dbgPlugins << "Redo again needed?";
    }
}

void KisGmicSynchronizeLayersCommand::undo()
{
    KisImageCommand * cmd;
    Q_FOREACH (cmd, m_imageCommands)
    {
        cmd->undo();
    }
}
