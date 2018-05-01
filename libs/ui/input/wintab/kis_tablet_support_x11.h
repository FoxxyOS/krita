/*
 *  Copyright (c) 2013 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_TABLET_SUPPORT_X11_H
#define __KIS_TABLET_SUPPORT_X11_H

#include <QAbstractNativeEventFilter>
#include <kritaui_export.h>

class KRITAUI_EXPORT KisTabletSupportX11 : public QAbstractNativeEventFilter
{
public:
    static void init();
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
};

#endif /* __KIS_TABLET_SUPPORT_X11_H */
