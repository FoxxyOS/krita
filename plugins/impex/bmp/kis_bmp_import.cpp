/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_bmp_import.h"

#include <QCheckBox>
#include <QSlider>
#include <QApplication>
#include <QFileInfo>

#include <kpluginfactory.h>


#include <KoColorSpace.h>
#include <KisFilterChain.h>
#include <KoColorSpaceRegistry.h>

#include <kis_transaction.h>
#include <kis_paint_device.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_node.h>
#include <kis_group_layer.h>


K_PLUGIN_FACTORY_WITH_JSON(KisBMPImportFactory, "krita_bmp_import.json", registerPlugin<KisBMPImport>();)

KisBMPImport::KisBMPImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisBMPImport::~KisBMPImport()
{
}

KisImportExportFilter::ConversionStatus KisBMPImport::convert(const QByteArray& from, const QByteArray& to, KisPropertiesConfigurationSP configuration)
{
    Q_UNUSED(configuration);

    dbgFile << "BMP import! From:" << from << ", To:" << to << 0;

    if (to != "application/x-krita")
        return KisImportExportFilter::BadMimeType;

    KisDocument * doc = outputDocument();

    if (!doc)
        return KisImportExportFilter::NoDocumentCreated;

    QString filename = inputFile();

    doc->prepareForImport();

    if (!filename.isEmpty()) {

        QFileInfo fi(filename);
        if (!fi.exists()) {
            return KisImportExportFilter::FileNotFound;
        }

        QImage img(filename);

        const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
        KisImageSP image = new KisImage(doc->createUndoStore(), img.width(), img.height(), colorSpace, "imported from bmp");

        KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), 255);
        layer->paintDevice()->convertFromQImage(img, 0, 0, 0);
        image->addNode(layer.data(), image->rootLayer().data());

        doc->setCurrentImage(image);
        return KisImportExportFilter::OK;
    }
    return KisImportExportFilter::StorageCreationError;

}

#include "kis_bmp_import.moc"

