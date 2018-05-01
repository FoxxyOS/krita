/*
 *  Copyright (c) 2003 Patrick Julien <freak@codepimps.org>
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

#ifndef KIS_UNDO_ADAPTER_H_
#define KIS_UNDO_ADAPTER_H_

#include <QObject>

#include <kritaimage_export.h>
#include "kis_undo_store.h"


class KRITAIMAGE_EXPORT KisUndoAdapter : public QObject
{
    Q_OBJECT

public:
    KisUndoAdapter(KisUndoStore *undoStore);
    virtual ~KisUndoAdapter();

public:
    void emitSelectionChanged();

    virtual const KUndo2Command* presentCommand() = 0;
    virtual void undoLastCommand() = 0;
    virtual void addCommand(KUndo2Command *cmd) = 0;
    virtual void beginMacro(const KUndo2MagicString& macroName) = 0;
    virtual void endMacro() = 0;

    inline void setUndoStore(KisUndoStore *undoStore) {
        m_undoStore = undoStore;
    }

Q_SIGNALS:
    void selectionChanged();

protected:
    inline KisUndoStore* undoStore() {
        return m_undoStore;
    }

private:
    Q_DISABLE_COPY(KisUndoAdapter)
    KisUndoStore *m_undoStore;
};


#endif // KIS_UNDO_ADAPTER_H_

