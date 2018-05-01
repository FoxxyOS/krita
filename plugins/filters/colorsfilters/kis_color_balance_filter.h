/*
 *  Copyright (c) 2013 Sahil Nagpal <nagpal.sahil01@gmail.com>
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

#ifndef _KIS_COLOR_BALANCE_FILTER_H_
#define _KIS_COLOR_BALANCE_FILTER_H_

#include <QList>
#include "filter/kis_filter.h"
#include "kis_config_widget.h"
#include "ui_wdg_color_balance.h"
#include "filter/kis_color_transformation_filter.h"


class QWidget;
class KoColorTransformation;

class KisColorBalanceFilter : public  KisColorTransformationFilter
{

public:

	KisColorBalanceFilter();
public:
    enum Type {
      SHADOWS,
      MIDTONES,
      HIGHLIGHTS
    };

public:

	virtual KisConfigWidget * createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev) const;

    virtual KoColorTransformation* createTransformation(const KoColorSpace* cs, const KisFilterConfigurationSP config) const;

	static inline KoID id() {
        return KoID("colorbalance", i18n("Color Balance"));
	}

    virtual KisFilterConfigurationSP  factoryConfiguration() const;

};

class KisColorBalanceConfigWidget : public KisConfigWidget
{

	Q_OBJECT

public:
    KisColorBalanceConfigWidget(QWidget * parent);
	virtual ~KisColorBalanceConfigWidget();

	virtual KisPropertiesConfigurationSP  configuration() const;
	virtual void setConfiguration(const KisPropertiesConfigurationSP config);
    Ui_Form * m_page;
    QString m_id;

public Q_SLOTS:
    void slotShadowsClear();
    void slotMidtonesClear();
    void slotHighlightsClear();
};

#endif
