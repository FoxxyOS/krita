/*
 *  Copyright (c) 2005 Cyrille Berger <cberger@cberger.net>
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

#include "kis_jpeg_import.h"

#include <QFileInfo>

#include <kpluginfactory.h>

#include <KisFilterChain.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <KisViewManager.h>
#include <KisImportExportManager.h>

#include "kis_jpeg_converter.h"

K_PLUGIN_FACTORY_WITH_JSON(JPEGImportFactory, "krita_jpeg_import.json", registerPlugin<KisJPEGImport>();)

KisJPEGImport::KisJPEGImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisJPEGImport::~KisJPEGImport()
{
}

KisImportExportFilter::ConversionStatus KisJPEGImport::convert(const QByteArray&, const QByteArray& to, KisPropertiesConfigurationSP /*configuration*/)
{
    dbgFile << "Importing using JPEGImport!";

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

        KisJPEGConverter ib(doc, getBatchMode());

//        if (view != 0)
//            view -> canvasSubject() ->  progressDisplay() -> setSubject(&ib, false, true);

        switch (ib.buildImage(filename)) {
        case KisImageBuilder_RESULT_UNSUPPORTED:
        case KisImageBuilder_RESULT_UNSUPPORTED_COLORSPACE:
            return KisImportExportFilter::NotImplemented;
            break;
        case KisImageBuilder_RESULT_INVALID_ARG:
            return KisImportExportFilter::BadMimeType;
            break;
        case KisImageBuilder_RESULT_NO_URI:
        case KisImageBuilder_RESULT_NOT_LOCAL:
            return KisImportExportFilter::FileNotFound;
            break;
        case KisImageBuilder_RESULT_BAD_FETCH:
        case KisImageBuilder_RESULT_EMPTY:
            return KisImportExportFilter::ParsingError;
            break;
        case KisImageBuilder_RESULT_FAILURE:
            return KisImportExportFilter::InternalError;
            break;
        case KisImageBuilder_RESULT_OK:
            doc->setCurrentImage(ib.image());
            return KisImportExportFilter::OK;
        default:
            break;
        }


    }
    return KisImportExportFilter::StorageCreationError;
}

#include <kis_jpeg_import.moc>

