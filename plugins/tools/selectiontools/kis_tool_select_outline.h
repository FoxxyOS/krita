/*
 *  kis_tool_select_freehand.h - part of Krayon^WKrita
 *
 *  Copyright (c) 2000 John Califf <jcaliff@compuzone.net>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2015 Michael Abrahams <miabraha@gmail.com>
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

#ifndef KIS_TOOL_SELECT_OUTLINE_H_
#define KIS_TOOL_SELECT_OUTLINE_H_

#include <QPoint>
#include <KoToolFactoryBase.h>
#include <kis_tool_select_base.h>
#include <kis_icon.h>

class QPainterPath;

class KisToolSelectOutline : public KisToolSelect
{
    Q_OBJECT

public:
    KisToolSelectOutline(KoCanvasBase *canvas);
    virtual ~KisToolSelectOutline();
    void beginPrimaryAction(KoPointerEvent *event);
    void continuePrimaryAction(KoPointerEvent *event);
    void endPrimaryAction(KoPointerEvent *event);
    virtual void paint(QPainter& gc, const KoViewConverter &converter);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    void mouseMoveEvent(KoPointerEvent *event);

public Q_SLOTS:
    virtual void deactivate();
    void setSelectionAction(int);

protected:
    using KisToolSelectBase::m_widgetHelper;

private:
    void finishSelectionAction();
    void updateFeedback();
    void updateContinuedMode();
    void updateCanvas();

    QPainterPath m_paintPath;
    vQPointF m_points;
    bool m_continuedMode;
    QPointF m_lastCursorPos;
};

class KisToolSelectOutlineFactory : public KoToolFactoryBase
{
public:
    KisToolSelectOutlineFactory()
        : KoToolFactoryBase("KisToolSelectOutline")
    {
        setToolTip(i18n("Outline Selection Tool"));
        setSection(TOOL_TYPE_SELECTION);
        setIconName(koIconNameCStr("tool_outline_selection"));
        setPriority(3);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
    }

    virtual ~KisToolSelectOutlineFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolSelectOutline(canvas);
    }
};


#endif //__selecttoolfreehand_h__

