/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_PALETTE_DELEGATE_H
#define __KIS_PALETTE_DELEGATE_H

#include <QAbstractItemDelegate>

#include "kritaui_export.h"


class KRITAUI_EXPORT KisPaletteDelegate : public QAbstractItemDelegate
{
public:
    KisPaletteDelegate(QObject * parent = 0);
    ~KisPaletteDelegate();

    void setCrossedKeyword(const QString &value);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex &) const;

private:
    QString m_crossedKeyword;
};

#endif /* __KIS_PALETTE_DELEGATE_H */
