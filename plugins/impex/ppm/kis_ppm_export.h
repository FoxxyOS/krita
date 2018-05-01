/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_PPM_EXPORT_H_
#define _KIS_PPM_EXPORT_H_

#include <QVariant>

#include <KisImportExportFilter.h>
#include <kis_config_widget.h>
#include "ui_kis_wdg_options_ppm.h"

class KisWdgOptionsPPM : public KisConfigWidget, public Ui::WdgOptionsPPM
{
    Q_OBJECT

public:
    KisWdgOptionsPPM(QWidget *parent)
        : KisConfigWidget(parent)
    {
        setupUi(this);
    }

    void setConfiguration(const KisPropertiesConfigurationSP  cfg);
    KisPropertiesConfigurationSP configuration() const;
};



class KisPPMExport : public KisImportExportFilter
{
    Q_OBJECT
public:
    KisPPMExport(QObject *parent, const QVariantList &);
    virtual ~KisPPMExport();
public:
    virtual KisImportExportFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to, KisPropertiesConfigurationSP configuration = 0);
    KisPropertiesConfigurationSP defaultConfiguration(const QByteArray& from = "", const QByteArray& to = "") const;
    KisPropertiesConfigurationSP lastSavedConfiguration(const QByteArray &from = "", const QByteArray &to = "") const;
    KisConfigWidget *createConfigurationWidget(QWidget *parent, const QByteArray& from = "", const QByteArray& to = "") const;
};

#endif
