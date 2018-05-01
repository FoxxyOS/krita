/*
 *  Copyright (c) 2011 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef TASKSET_RESOURCE_H
#define TASKSET_RESOURCE_H

#include <resources/KoResource.h>
#include <QStringList>


class TasksetResource : public KoResource
{

public:
    TasksetResource(const QString& filename);
    virtual ~TasksetResource();
    
    virtual bool load();
    virtual bool loadFromDevice(QIODevice *dev);
    virtual bool save();
    virtual bool saveToDevice(QIODevice* dev) const;

    virtual QString defaultFileExtension() const;
    
    void setActionList(const QStringList actions);
    QStringList actionList();

private:

    QStringList m_actions;
};

#endif // TASKSET_RESOURCE_H
