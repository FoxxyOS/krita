/*
 * Copyright (c) 2011 Silvio Heinrich <plassy@web.de>
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

#include "kis_pressure_flow_opacity_option.h"
#include "kis_paint_action_type_option.h"

#include <klocalizedstring.h>

#include <kis_painter.h>
#include <brushengine/kis_paint_information.h>
#include <kis_indirect_painting_support.h>
#include <kis_node.h>
#include <widgets/kis_curve_widget.h>

KisFlowOpacityOption::KisFlowOpacityOption(KisNodeSP currentNode)
    : KisCurveOption("Opacity", KisPaintOpOption::GENERAL, true, 1.0, 0.0, 1.0)
    , m_flow(1.0)
{
    setCurveUsed(true);
    setSeparateCurveValue(true);

    m_checkable = false;

    m_nodeHasIndirectPaintingSupport =
        currentNode &&
        dynamic_cast<KisIndirectPaintingSupport*>(currentNode.data());
}

void KisFlowOpacityOption::writeOptionSetting(KisPropertiesConfigurationSP setting) const
{
    KisCurveOption::writeOptionSetting(setting);
    setting->setProperty("FlowValue", m_flow);
}

void KisFlowOpacityOption::readOptionSetting(const KisPropertiesConfigurationSP setting)
{
    KisCurveOption::readOptionSetting(setting);
    setFlow(setting->getDouble("FlowValue", 1.0));
    m_paintActionType = setting->getInt("PaintOpAction", BUILDUP);
}

qreal KisFlowOpacityOption::getFlow() const
{
    return m_flow;
}

qreal KisFlowOpacityOption::getStaticOpacity() const
{
    return value();
}

qreal KisFlowOpacityOption::getDynamicOpacity(const KisPaintInformation& info) const
{
    return computeSizeLikeValue(info);
}

void KisFlowOpacityOption::setFlow(qreal flow)
{
    m_flow = qBound(qreal(0), flow, qreal(1));
}

void KisFlowOpacityOption::setOpacity(qreal opacity)
{
    setValue(opacity);
}

void KisFlowOpacityOption::apply(KisPainter* painter, const KisPaintInformation& info)
{
    if (m_paintActionType == WASH && m_nodeHasIndirectPaintingSupport)
        painter->setOpacityUpdateAverage(quint8(getDynamicOpacity(info) * 255.0));
    else
        painter->setOpacityUpdateAverage(quint8(getStaticOpacity() * getDynamicOpacity(info) * 255.0));

    painter->setFlow(quint8(getFlow() * 255.0));
}
