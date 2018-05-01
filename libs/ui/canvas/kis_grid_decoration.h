/*
 * This file is part of Krita
 *
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2014 Sven Langkamp <sven.langkamp@gmail.com>
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

#ifndef KIS_GRID_DECORATION_H
#define KIS_GRID_DECORATION_H

#include <QScopedPointer>
#include <kis_canvas_decoration.h>

class KisGridConfig;


class KisGridDecoration : public KisCanvasDecoration
{
    Q_OBJECT
public:
    KisGridDecoration(KisView* parent);
    virtual ~KisGridDecoration();

    void setGridConfig(const KisGridConfig &config);

protected:
    virtual void drawDecoration(QPainter& gc, const QRectF& updateArea, const KisCoordinatesConverter* converter, KisCanvas2* canvas);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KIS_GRID_DECORATION_H
