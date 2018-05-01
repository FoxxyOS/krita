/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
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

#include "kis_image_commands.h"
#include "kis_image.h"
#include "kis_group_layer.h"

#include <klocalizedstring.h>


KisImageChangeLayersCommand::KisImageChangeLayersCommand(KisImageWSP image, KisNodeSP oldRootLayer, KisNodeSP newRootLayer)
    : KisImageCommand(kundo2_noi18n("change-layer-command"), image)
{
    m_oldRootLayer = oldRootLayer;
    m_newRootLayer = newRootLayer;
}

void KisImageChangeLayersCommand::redo()
{
    m_image->setRootLayer(static_cast<KisGroupLayer*>(m_newRootLayer.data()));

    m_image->refreshGraphAsync();
    m_image->notifyLayersChanged();
}

void KisImageChangeLayersCommand::undo()
{
    m_image->setRootLayer(static_cast<KisGroupLayer*>(m_oldRootLayer.data()));

    m_image->refreshGraphAsync();
    m_image->notifyLayersChanged();
}
