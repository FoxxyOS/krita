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

#ifndef CATEGORY_H
#define CATEGORY_H

#include <QtGlobal>

#include <Component.h>

#include <QList>

class Category : public Component
{
    Q_OBJECT

public:
    Category(Component * parent = 0);
    virtual ~Category();
    virtual void add(Component *c);
    virtual Component* child(int index) const;
    virtual Component* parent() { return m_parent; }
    void setParent(Component * parent) { m_parent = parent; }

    virtual int row() const;
    virtual int indexOf(const Component* c) const;
    virtual int childCount() const;
    virtual int columnCount() const;

    void processCategoryName(const QString &line);

    virtual void print(int level = 0);

    virtual QVariant data(int column);

    template <typename T>
    int indexOf(const QString &componentName) const
    {
        for (int i = 0; i < m_components.size(); i++)
        {
            const T * component = dynamic_cast<T *>(m_components.at(i));
            if (component)
            {
                if (component->name() == componentName)
                {
                    return i;
                }
            }
        }

        return -1;
    }

    void replace(int position, Component * c);

public:
    QList<Component*> m_components; // categories and commands
private:
    Component * m_parent;

};

#endif
