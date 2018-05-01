/*
 *  Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org>
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
#include "psd_import.h"

#include <kpluginfactory.h>
#include <QFileInfo>

#include <KisFilterChain.h>

#include <KisDocument.h>
#include <kis_image.h>

#include "psd_loader.h"

K_PLUGIN_FACTORY_WITH_JSON(ImportFactory, "krita_psd_import.json", registerPlugin<psdImport>();)

        psdImport::psdImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

psdImport::~psdImport()
{
}

KisImportExportFilter::ConversionStatus psdImport::convert(const QByteArray&, const QByteArray& to, KisPropertiesConfigurationSP configuration)
{
    Q_UNUSED(configuration);
    dbgFile <<"Importing using PSDImport!";

    if (to != "application/x-krita")
        return KisImportExportFilter::BadMimeType;

    KisDocument * doc = outputDocument();

    if (!doc)
        return KisImportExportFilter::NoDocumentCreated;

    QString filename = inputFile();

    doc->prepareForImport();

    if (!filename.isEmpty()) {

        if (!QFileInfo(filename).exists()) {
            return KisImportExportFilter::FileNotFound;
        }

        PSDLoader ib(doc);

        KisImageBuilder_Result result = ib.buildImage(filename);

        switch (result) {
        case KisImageBuilder_RESULT_UNSUPPORTED:
            return KisImportExportFilter::NotImplemented;
        case KisImageBuilder_RESULT_INVALID_ARG:
            return KisImportExportFilter::BadMimeType;
        case KisImageBuilder_RESULT_NO_URI:
        case KisImageBuilder_RESULT_NOT_EXIST:
        case KisImageBuilder_RESULT_NOT_LOCAL:
            qDebug() << "ib returned KisImageBuilder_RESULT_NOT_LOCAL";
            return KisImportExportFilter::FileNotFound;
        case KisImageBuilder_RESULT_BAD_FETCH:
        case KisImageBuilder_RESULT_EMPTY:
            return KisImportExportFilter::ParsingError;
        case KisImageBuilder_RESULT_FAILURE:
            return KisImportExportFilter::InternalError;
        case KisImageBuilder_RESULT_OK:
            doc -> setCurrentImage( ib.image());
            return KisImportExportFilter::OK;
        default:
            return KisImportExportFilter::StorageCreationError;
            //dbgFile << "Result was: " << result;
        }
    }
    return KisImportExportFilter::StorageCreationError;
}

#include <psd_import.moc>

