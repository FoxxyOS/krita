/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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

#ifndef KIS_TOOL_INVOCATION_ACTION_H
#define KIS_TOOL_INVOCATION_ACTION_H

#include "kis_abstract_input_action.h"

/**
 * \brief Tool Invocation action of KisAbstractInputAction.
 *
 * The Tool Invocation action invokes the current tool, for example,
 * using the brush tool, it will start painting.
 */
class KisToolInvocationAction : public KisAbstractInputAction
{
public:
    enum Shortcut {
        ActivateShortcut,
        ConfirmShortcut,
        CancelShortcut,
        LineToolShortcut
    };
    explicit KisToolInvocationAction();
    virtual ~KisToolInvocationAction();

    void activate(int shortcut);
    void deactivate(int shortcut);

    virtual int priority() const;
    virtual bool canIgnoreModifiers() const;

    void begin(int shortcut, QEvent *event);
    void end(QEvent *event);
    void inputEvent(QEvent* event);

    void processUnhandledEvent(QEvent* event);

    bool supportsHiResInputEvents() const;

    virtual bool isShortcutRequired(int shortcut) const;

private:
    class Private;
    Private * const d;
};

#endif // KISTOOLINVOCATIONACTION_H
