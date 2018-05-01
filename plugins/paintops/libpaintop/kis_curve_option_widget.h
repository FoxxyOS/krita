/* This file is part of the KDE project
 * Copyright (C) 2008 Boudewijn Rempt <boud@valdyas.org>
 * Copyright (C) 2009 Sven Langkamp   <sven.langkamp@gmail.com>
 * Copyright (C) 2011 Silvio Heinrich <plassy@web.de>
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

#ifndef KIS_CURVE_OPTION_WIDGET_H
#define KIS_CURVE_OPTION_WIDGET_H

#include <kis_paintop_option.h>

class Ui_WdgCurveOption;
class KisCurveOption;

#include <kis_dynamic_sensor.h>

/**
 * XXX; Add a reset button!
 */
class PAINTOP_EXPORT KisCurveOptionWidget : public KisPaintOpOption
{
    Q_OBJECT
public:
    KisCurveOptionWidget(KisCurveOption* curveOption, const QString &minLabel, const QString &maxLabel, bool hideSlider = false);
    ~KisCurveOptionWidget();

    virtual void writeOptionSetting(KisPropertiesConfigurationSP setting) const;
    virtual void readOptionSetting(const KisPropertiesConfigurationSP setting);
    virtual void lodLimitations(KisPaintopLodLimitations *l) const;

    bool isCheckable() const;
    bool isChecked() const;
    void setChecked(bool checked);

protected:

    KisCurveOption* curveOption();
    QWidget* curveWidget();

private Q_SLOTS:

    void transferCurve();
    void updateSensorCurveLabels(KisDynamicSensorSP sensor);
    void updateCurve(KisDynamicSensorSP sensor);
    void updateValues();
    void updateLabelsOfCurrentSensor();
    void disableWidgets(bool disable);

private:
    QWidget* m_widget;
    Ui_WdgCurveOption* m_curveOptionWidget;
    KisCurveOption* m_curveOption;
};

#endif // KIS_CURVE_OPTION_WIDGET_H
