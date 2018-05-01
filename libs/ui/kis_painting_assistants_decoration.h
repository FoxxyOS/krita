/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_PAINTING_ASSISTANTS_MANAGER_H_
#define _KIS_PAINTING_ASSISTANTS_MANAGER_H_

#include <QPointF>

#include "canvas/kis_canvas_decoration.h"
#include "kis_painting_assistant.h"

#include <kritaui_export.h>

class KisView;

class KisPaintingAssistantsDecoration;
typedef KisSharedPtr<KisPaintingAssistantsDecoration> KisPaintingAssistantsDecorationSP;

/**
 * This class hold a list of painting assistants.
 */
class KRITAUI_EXPORT KisPaintingAssistantsDecoration : public KisCanvasDecoration
{
    Q_OBJECT
public:
    KisPaintingAssistantsDecoration(QPointer<KisView> parent);
    ~KisPaintingAssistantsDecoration();
    void addAssistant(KisPaintingAssistantSP assistant);
    void removeAssistant(KisPaintingAssistantSP assistant);
    void removeAll();
    QPointF adjustPosition(const QPointF& point, const QPointF& strokeBegin);
    void endStroke();
    QList<KisPaintingAssistantHandleSP> handles();
    QList<KisPaintingAssistantSP> assistants();
    /*sets whether the main assistant is visible*/
    void setAssistantVisible(bool set);
    /*sets whether the preview is visible*/
    void setOutlineVisible(bool set);
    /*sets whether we snap to only one assistant*/
    void setOnlyOneAssistantSnap(bool assistant);
    /*returns assistant visibility*/
    bool assistantVisibility();
    /*returns preview visibility*/
    bool outlineVisibility();
    /*uncache all assistants*/
    void uncache();
Q_SIGNALS:
    void assistantChanged();
public Q_SLOTS:
    void toggleAssistantVisible();
    void toggleOutlineVisible();
protected:
    void drawDecoration(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter *converter,KisCanvas2* canvas);

private:
    struct Private;
    Private* const d;
};

#endif
