/*
 * This file is part of Krita
 *
 * Copyright (c) 2016 Spencer Brown <sbrown655@gmail.com>
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

#pragma once

#include "QObject"
#include "ui_wdg_gradientmap.h"
#include "kis_properties_configuration.h"
#include "filter/kis_color_transformation_configuration.h"
#include "kis_config_widget.h"

class WdgGradientMap : public QWidget, public Ui::WdgGradientMap
{
    Q_OBJECT

public:
    WdgGradientMap(QWidget *parent) : QWidget(parent) {
        setupUi(this);
    }
};


class KritaGradientMapFilterConfiguration : public KisColorTransformationConfiguration
{
public:
    KritaGradientMapFilterConfiguration();
    virtual ~KritaGradientMapFilterConfiguration();

    virtual void setGradient(const KoResource* gradient);

    virtual const KoResource* gradient() const;

private:
    KoResource const* m_gradient;
};

class KritaGradientMapConfigWidget : public KisConfigWidget
{
    Q_OBJECT
public:
    KritaGradientMapConfigWidget(QWidget *parent, KisPaintDeviceSP dev, Qt::WFlags f = 0);
    virtual ~KritaGradientMapConfigWidget();

    virtual KisPropertiesConfigurationSP configuration() const;
    virtual void setConfiguration(const KisPropertiesConfigurationSP config);

    WdgGradientMap * m_page;
    void setView(KisViewManager *view);
    void gradientResourceChanged(KoResource *gradient);
};

class KritaGradientMap : public QObject
{
    Q_OBJECT
public:
    KritaGradientMap(QObject *parent, const QVariantList &);
    virtual ~KritaGradientMap();
};

