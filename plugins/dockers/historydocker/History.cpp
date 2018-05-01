/* This file is part of the KDE project
 * Copyright (C) 2010 Matus Talcik <matus.talcik@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "History.h"


#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include <KoDockFactoryBase.h>
#include <KoDockRegistry.h>

#include "HistoryDock.h"

K_PLUGIN_FACTORY_WITH_JSON(HistoryPluginFactory, "kritahistorydocker.json", registerPlugin<HistoryPlugin>();)

class HistoryDockFactory : public KoDockFactoryBase
{
public:
    HistoryDockFactory() {
    }

    QString id() const override {
        return QString("History");
    }

    virtual Qt::DockWidgetArea defaultDockWidgetArea() const {
        return Qt::RightDockWidgetArea;
    }

    QDockWidget* createDockWidget() override {
        HistoryDock * dockWidget = new HistoryDock();
        dockWidget->setObjectName(id());

        return dockWidget;
    }

    DockPosition defaultDockPosition() const override {
        return DockRight;
    }
};


HistoryPlugin::HistoryPlugin(QObject *parent, const QVariantList &)
        : QObject(parent)
{

    KoDockRegistry::instance()->add(new HistoryDockFactory());
}

HistoryPlugin::~HistoryPlugin()
{
}

#include <History.moc>
