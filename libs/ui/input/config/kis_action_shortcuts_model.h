/*
 * This file is part of the KDE project
 * Copyright (C) 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KISACTIONSHORTCUTSMODEL_H
#define KISACTIONSHORTCUTSMODEL_H

#include <QAbstractListModel>

class KisAbstractInputAction;
class KisInputProfile;

/**
 * \brief A QAbstractListModel subclass that lists shortcuts associated with an action from a profile.
 *
 * This class lists all shortcuts from the set profile that correspond to a specific action. This is
 * used to allow editing of shortcuts from the ui.
 *
 * It defines the following columns:
 * - Type:      The type of shortcut, can be one of Key Combination, Mouse Button, Mouse Wheel, Gesture
 * - Input:     What input is used to activate the shortcut. Depends on the type.
 * - Action:    What mode of the action will be activated by the shortcut.
 *
 * \note Before this model will provide any data you should call setAction and setProfile.
 * \note This model is editable and provides an implementation of removeRows.
 */
class KisActionShortcutsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    explicit KisActionShortcutsModel(QObject *parent = 0);
    /**
     * Destructor.
     */
    ~KisActionShortcutsModel();

    /**
     * Reimplemented from QAbstractItemModel::data()
     */
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    /**
     * Reimplemented from QAbstractItemModel::rowCount()
     */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    /**
     * Reimplemented from QAbstractItemModel::columnCount()
     */
    virtual int columnCount(const QModelIndex &) const;
    /**
     * Reimplemented from QAbstractItemModel::headerData()
     */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role =
                                    Qt::DisplayRole) const;
    /**
     * Reimplemented from QAbstractItemModel::flags()
     */
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    /**
     * Reimplemented from QAbstractItemModel::setData()
     */
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    /**
     * Reimplemented from QAbstractItemModel::removeRows.
     *
     * Removes `count` rows starting at `row`.
     *
     * \note The associated shortcuts will also be removed from the profile and completely
     * deleted.
     */
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    /**
     * \return The action used as data constraint for this model.
     */
    KisAbstractInputAction *action() const;
    /**
     * \return The profile used as data source for this model.
     */
    KisInputProfile *profile() const;

    bool canRemoveRow(int row) const;

public Q_SLOTS:
    /**
     * Set the action used as data constraint for this model.
     *
     * \param action The action to use.
     */
    void setAction(KisAbstractInputAction *action);
    /**
     * Set the profile used as data source for this model.
     *
     * \param profile The profile to get the data from.
     */
    void setProfile(KisInputProfile *profile);

private Q_SLOTS:
    void currentProfileChanged();

private:
    class Private;
    Private *const d;
};

#endif // KISACTIONSHORTCUTSMODEL_H
