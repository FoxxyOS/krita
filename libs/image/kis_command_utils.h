/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_COMMAND_UTILS_H
#define __KIS_COMMAND_UTILS_H

#include "kundo2command.h"
#include "kis_undo_stores.h"
#include "kritaimage_export.h"
#include <functional>

namespace KisCommandUtils
{
    /**
     * @brief The AggregateCommand struct is a command with delayed
     * initialization. On first redo() populateChildCommands() is called
     * and the descendants add the desired commands to the internal list.
     * After that, the added commands are executed on every undo()/redo().
     *
     * This structure is used when the commands should be populated from
     * the context of the stroke, not from the GUI thread.
     */
    struct KRITAIMAGE_EXPORT AggregateCommand : public KUndo2Command {
        AggregateCommand(KUndo2Command *parent = 0);
        AggregateCommand(const KUndo2MagicString &text,
                         KUndo2Command *parent = 0);

        void redo();
        void undo();

    protected:
        virtual void populateChildCommands() = 0;
        void addCommand(KUndo2Command *cmd);

    private:
        bool m_firstRedo;
        KisSurrogateUndoStore m_store;
    };

    /**
     * @brief The LambdaCommand struct is a shorthand for creation of
     * AggregateCommand commands using C++ lambda feature. Just pass
     * a lambda object into a command and it will be called from within
     * the context of the strokes thread to populate the command content.
     */
    struct KRITAIMAGE_EXPORT LambdaCommand : public AggregateCommand {
        LambdaCommand(std::function<KUndo2Command*()> createCommandFunc);
        LambdaCommand(const KUndo2MagicString &text,
                      std::function<KUndo2Command*()> createCommandFunc);
        LambdaCommand(const KUndo2MagicString &text,
                      KUndo2Command *parent,
                      std::function<KUndo2Command*()> createCommandFunc);
        LambdaCommand(KUndo2Command *parent,
                      std::function<KUndo2Command*()> createCommandFunc);

    protected:
        void populateChildCommands() override;

    private:
        std::function<KUndo2Command*()> m_createCommandFunc;
    };

    struct KRITAIMAGE_EXPORT SkipFirstRedoWrapper : public KUndo2Command {

        SkipFirstRedoWrapper(KUndo2Command *child = 0, KUndo2Command *parent = 0);
        void redo();
        void undo();

    private:
        bool m_firstRedo;
        QScopedPointer<KUndo2Command> m_child;
    };

    struct KRITAIMAGE_EXPORT FlipFlopCommand : public KUndo2Command {
        FlipFlopCommand(bool finalize, KUndo2Command *parent = 0);

        void redo();
        void undo();

    protected:
        virtual void init();
        virtual void end();
        bool isFinalizing() const { return m_finalize; }
        bool isFirstRedo() const { return m_firstRedo; }

    private:
        bool m_finalize;
        bool m_firstRedo;
    };

    struct KRITAIMAGE_EXPORT CompositeCommand : public KUndo2Command {
        CompositeCommand(KUndo2Command *parent = 0);
        ~CompositeCommand();

        void addCommand(KUndo2Command *cmd);

        void redo();
        void undo();

    private:
        QVector<KUndo2Command*> m_commands;
    };
}

#endif /* __KIS_COMMAND_UTILS_H */
