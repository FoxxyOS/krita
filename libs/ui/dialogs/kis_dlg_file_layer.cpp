/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2013
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

#include "kis_dlg_file_layer.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QDesktopServices>

#include <klocalizedstring.h>

#include <KoFileDialog.h>
#include <KisApplication.h>
#include <KisImportExportManager.h>
#include <kis_file_name_requester.h>
#include <kis_config_widget.h>
#include <kis_paint_device.h>
#include <kis_transaction.h>
#include <kis_node.h>
#include <kis_file_layer.h>

KisDlgFileLayer::KisDlgFileLayer(const QString &basePath, const QString & name, QWidget * parent)
    : KoDialog(parent)
    , m_basePath(basePath)
{
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    QWidget * page = new QWidget(this);
    dlgWidget.setupUi(page);
    dlgWidget.wdgUrlRequester->setMimeTypeFilters(KisImportExportManager::mimeFilter(KisImportExportManager::Import));
    setMainWidget(page);

    //dlgWidget.wdgUrlRequester->setBasePath(m_basePath);
    dlgWidget.wdgUrlRequester->setStartDir(m_basePath);

    dlgWidget.txtLayerName->setText(name);

    connect(dlgWidget.wdgUrlRequester, SIGNAL(textChanged(const QString &)),
            SLOT(slotNameChanged(const QString &)));

    enableButtonOk(false);
}

void KisDlgFileLayer::slotNameChanged(const QString & text)
{
    enableButtonOk(!text.isEmpty());
}

QString KisDlgFileLayer::layerName() const
{
    return dlgWidget.txtLayerName->text();
}

KisFileLayer::ScalingMethod KisDlgFileLayer::scaleToImageResolution() const
{
    if (dlgWidget.radioDontScale->isChecked()) {
        return KisFileLayer::None;
    }
    else if (dlgWidget.radioScaleToImageSize->isChecked()) {
        return KisFileLayer::ToImageSize;
    }
    else {
        return KisFileLayer::ToImagePPI;
    }
}

QString KisDlgFileLayer::fileName() const
{
    QString path = dlgWidget.wdgUrlRequester->fileName();

    if (!m_basePath.isEmpty() && QFileInfo(path).isAbsolute()) {
        QDir directory(m_basePath);
        path = directory.relativeFilePath(path);
    }

    return path;
}

