/*
 * Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com
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

#ifndef KIS_GMIC_FILTER_MODEL_H
#define KIS_GMIC_FILTER_MODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QPointer>

#include "Component.h"
#include "kis_gmic_blacklister.h"

// introduce "rolespace", let's not collide with some other Qt::UserRole + 1 roles e.g. in ui/kis_categorized_list_model.h
static const int GmicRolespace = 100;

enum
{
    CommandRole = Qt::UserRole + GmicRolespace + 1,
    FilterSettingsRole = Qt::UserRole + GmicRolespace + 2
};


class KisGmicFilterModel : public QAbstractItemModel
{
     Q_OBJECT

public:
    // takes ownershop of component
    KisGmicFilterModel(Component * rootComponent, QObject *parent = 0);
    ~KisGmicFilterModel();

    QVariant data(const QModelIndex &index, int role) const;
    // virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    // takes ownership
    void setBlacklister(KisGmicBlacklister * blacklister);

private:
    QPointer<Component> m_rootComponent;
    QPointer<KisGmicBlacklister> m_blacklister;

};

#endif
