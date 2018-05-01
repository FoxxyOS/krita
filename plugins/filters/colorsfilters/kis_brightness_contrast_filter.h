/*
 * This file is part of Krita
 *
 * Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_BRIGHTNESS_CONTRAST_FILTER_H_
#define _KIS_BRIGHTNESS_CONTRAST_FILTER_H_

#include <QList>

#include "filter/kis_color_transformation_filter.h"
#include "kis_config_widget.h"
#include "ui_wdg_brightness_contrast.h"
#include <filter/kis_color_transformation_configuration.h>
#include <kis_selection.h>
#include <kis_paint_device.h>
#include <kis_processing_information.h>
#include <KoColor.h>

class QWidget;
class KoColorTransformation;

class WdgBrightnessContrast : public QWidget, public Ui::WdgBrightnessContrast
{
    Q_OBJECT

public:
    WdgBrightnessContrast(QWidget *parent) : QWidget(parent) {
        setupUi(this);
    }
};

class KisBrightnessContrastFilterConfiguration : public KisColorTransformationConfiguration
{

public:
    using KisFilterConfiguration::fromXML;
    using KisFilterConfiguration::toXML;
    using KisFilterConfiguration::fromLegacyXML;

    virtual void fromLegacyXML(const QDomElement& root);

    virtual void fromXML(const QDomElement& e);
    virtual void toXML(QDomDocument& doc, QDomElement& root) const;

    KisBrightnessContrastFilterConfiguration();
    virtual ~KisBrightnessContrastFilterConfiguration();

    virtual void setCurve(const KisCubicCurve &curve);

    const QVector<quint16>& transfer() const;
    virtual const KisCubicCurve& curve() const;

private:
    void updateTransfer();

private:
    KisCubicCurve m_curve;
    QVector<quint16> m_transfer;
};

/**
 * This class affect Intensity Y of the image
 */
class KisBrightnessContrastFilter : public KisColorTransformationFilter
{

public:

    KisBrightnessContrastFilter();

public:

    virtual KoColorTransformation* createTransformation(const KoColorSpace* cs, const KisFilterConfigurationSP config) const;

    static inline KoID id() {
        return KoID("brightnesscontrast", i18n("Brightness / Contrast"));
    }
    virtual KisFilterConfigurationSP factoryConfiguration() const;

    virtual KisConfigWidget * createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev) const;
};


class KisBrightnessContrastConfigWidget : public KisConfigWidget
{
    Q_OBJECT
public:
    KisBrightnessContrastConfigWidget(QWidget * parent, KisPaintDeviceSP dev, Qt::WFlags f = 0);
    virtual ~KisBrightnessContrastConfigWidget();

    virtual KisPropertiesConfigurationSP configuration() const;
    virtual void setConfiguration(const KisPropertiesConfigurationSP config);
    WdgBrightnessContrast * m_page;
    void setView(KisViewManager *view);

public Q_SLOTS:
    void slotDrawLine(const KoColor &color);
};

#endif
