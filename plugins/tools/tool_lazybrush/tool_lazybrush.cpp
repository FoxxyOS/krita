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

#include "tool_lazybrush.h"
#include <QStringList>

#include <kis_debug.h>
#include <kpluginfactory.h>

#include <kis_tool.h>
#include <KoToolRegistry.h>

#include "kis_paint_device.h"
#include "kis_tool_lazy_brush.h"

#include "kis_config.h"


K_PLUGIN_FACTORY_WITH_JSON(DefaultToolsFactory, "kritatoollazybrush.json", registerPlugin<ToolLazyBrush>();)


ToolLazyBrush::ToolLazyBrush(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    if (!KisConfig().disableColorizeMaskFeature()) {
        KoToolRegistry::instance()->add(new KisToolLazyBrushFactory());
    }
}

ToolLazyBrush::~ToolLazyBrush()
{
}

#include "tool_lazybrush.moc"
