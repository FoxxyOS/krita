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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_VIDEO_EXPORT_H_
#define _KIS_VIDEO_EXPORT_H_

#include <QVariant>

#include <KisImportExportFilter.h>

class KisVideoExport : public KisImportExportFilter
{
    Q_OBJECT
public:
    KisVideoExport(QObject *parent, const QVariantList &);
    virtual ~KisVideoExport();
public:
    virtual KisImportExportFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to, KisPropertiesConfigurationSP configuration = 0);

    KisPropertiesConfigurationSP defaultConfiguration(const QByteArray& from, const QByteArray& to) const;
    KisPropertiesConfigurationSP lastSavedConfiguration(const QByteArray &from, const QByteArray &to) const;
    KisConfigWidget *createConfigurationWidget(QWidget *parent, const QByteArray& from = "", const QByteArray& to = "") const;
};

#endif
