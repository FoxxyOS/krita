/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __MOVE_SELECTION_STROKE_STRATEGY_H
#define __MOVE_SELECTION_STROKE_STRATEGY_H

#include "kis_stroke_strategy_undo_command_based.h"
#include "kis_types.h"

class KisPostExecutionUndoAdapter;
class KisUpdatesFacade;


class MoveSelectionStrokeStrategy : public KisStrokeStrategyUndoCommandBased
{
public:
    MoveSelectionStrokeStrategy(KisPaintLayerSP paintLayer,
                                KisSelectionSP selection,
                                KisUpdatesFacade *updatesFacade,
                                KisStrokeUndoFacade *undoFacade);

    void initStrokeCallback();
    void finishStrokeCallback();
    void cancelStrokeCallback();
    void doStrokeCallback(KisStrokeJobData *data);

private:
    MoveSelectionStrokeStrategy(const MoveSelectionStrokeStrategy &rhs);

    KisStrokeStrategy* createLodClone(int levelOfDetail);

private:
    KisPaintLayerSP m_paintLayer;
    KisSelectionSP m_selection;
    KisUpdatesFacade *m_updatesFacade;
    QPoint m_finalOffset;
    QPoint m_initialDeviceOffset;
};

#endif /* __MOVE_SELECTION_STROKE_STRATEGY_H */
