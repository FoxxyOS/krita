/*
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

#ifndef KIS_KEYFRAME_COMMANDS_H
#define KIS_KEYFRAME_COMMANDS_H

#include "kis_keyframe_channel.h"
#include "kundo2command.h"
#include "kritaimage_export.h"

class KRITAIMAGE_EXPORT KisReplaceKeyframeCommand : public KUndo2Command
{
public:
    KisReplaceKeyframeCommand(KisKeyframeChannel *channel, int time, KisKeyframeSP keyframe, KUndo2Command *parentCommand);

    void redo();
    void undo();

private:
    void doSwap(bool insert);

private:
    KisKeyframeChannel *m_channel;
    int m_time;
    KisKeyframeSP m_keyframe;
    KisKeyframeSP m_existingKeyframe;
};

class KRITAIMAGE_EXPORT KisMoveFrameCommand : public KUndo2Command
{
public:
    KisMoveFrameCommand(KisKeyframeChannel *channel, KisKeyframeSP keyframe, int oldTime, int newTime, KUndo2Command *parentCommand);

    void redo();
    void undo();

private:
    KisKeyframeChannel *m_channel;
    KisKeyframeSP m_keyframe;
    int m_oldTime;
    int m_newTime;
};

#endif
