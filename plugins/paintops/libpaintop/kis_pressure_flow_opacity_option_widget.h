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

#ifndef KIS_PRESSURE_FLOW_OPACITY_OPTION_WIDGET_H
#define KIS_PRESSURE_FLOW_OPACITY_OPTION_WIDGET_H

#include "kis_pressure_flow_opacity_option.h"
#include "kis_curve_option_widget.h"

class KisDoubleSliderSpinBox;
class KisCurveOptionWidget;

class PAINTOP_EXPORT KisFlowOpacityOptionWidget: public KisCurveOptionWidget
{
    Q_OBJECT

public:
    KisFlowOpacityOptionWidget();

    virtual void readOptionSetting(const KisPropertiesConfigurationSP setting);

private Q_SLOTS:
    void slotSliderValueChanged();

private:
    KisDoubleSliderSpinBox* m_opacitySlider;
};

#endif // KIS_PRESSURE_FLOW_OPACITY_OPTION_WIDGET_H
