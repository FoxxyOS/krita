/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_GAMMA_EXPOSURE_ACTION_H
#define __KIS_GAMMA_EXPOSURE_ACTION_H

#include "kis_abstract_input_action.h"


class KisGammaExposureAction : public KisAbstractInputAction
{
public:
    /**
     * The different behaviours for this action.
     */
    enum Shortcuts {
        ExposureShortcut,
        GammaShortcut,
        AddExposure05Shortcut,
        RemoveExposure05Shortcut,
        AddGamma05Shortcut,
        RemoveGamma05Shortcut,
        AddExposure02Shortcut,
        RemoveExposure02Shortcut,
        AddGamma02Shortcut,
        RemoveGamma02Shortcut,
        ResetExposureAndGammaShortcut
    };
    explicit KisGammaExposureAction();
    virtual ~KisGammaExposureAction();

    virtual int priority() const;

    void activate(int shortcut);
    void deactivate(int shortcut);

    void begin(int shortcut, QEvent *event = 0);
    void cursorMoved(const QPointF &lastPos, const QPointF &pos);

    bool isShortcutRequired(int shortcut) const;

private:
    class Private;
    Private * const d;
};

#endif /* __KIS_GAMMA_EXPOSURE_ACTION_H */
