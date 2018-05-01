/*
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

#include "kis_selection_commands.h"

#include <klocalizedstring.h>

#include "kis_image.h"
#include "kis_selection.h"
#include "kis_undo_adapter.h"
#include "kis_selection_mask.h"
#include "kis_pixel_selection.h"

KisReselectGlobalSelectionCommand::KisReselectGlobalSelectionCommand(KisImageWSP image, KUndo2Command * parent) :
        KUndo2Command(kundo2_i18n("Reselect"), parent)
        , m_image(image)
{
}

KisReselectGlobalSelectionCommand::~KisReselectGlobalSelectionCommand()
{
}

void KisReselectGlobalSelectionCommand::redo()
{
    m_canReselect = m_image->canReselectGlobalSelection();

    if (m_canReselect) {
        m_image->reselectGlobalSelection();
    }
}

void KisReselectGlobalSelectionCommand::undo()
{
    if (m_canReselect) {
        m_image->deselectGlobalSelection();
    }
}

