/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
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

#include "kis_ppm_import.h"

#include <ctype.h>

#include <QApplication>
#include <QFile>

#include <kpluginfactory.h>
#include <QFileInfo>

#include <KoColorSpaceRegistry.h>
#include <KisFilterChain.h>

#include <kis_debug.h>
#include <KisDocument.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <KoColorSpaceTraits.h>
#include <kis_paint_device.h>
#include <kis_transaction.h>
#include <KoColorSpace.h>
#include <qendian.h>
#include <KoColorModelStandardIds.h>
#include "kis_iterator_ng.h"

K_PLUGIN_FACTORY_WITH_JSON(PPMImportFactory, "krita_ppm_import.json", registerPlugin<KisPPMImport>();)

KisPPMImport::KisPPMImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisPPMImport::~KisPPMImport()
{
}

KisImportExportFilter::ConversionStatus KisPPMImport::convert(const QByteArray& from, const QByteArray& to, KisPropertiesConfigurationSP configuration)
{
    Q_UNUSED(from);
    Q_UNUSED(configuration);
    dbgFile << "Importing using PPMImport!";

    if (to != "application/x-krita")
        return KisImportExportFilter::BadMimeType;

    KisDocument * doc = outputDocument();

    if (!doc)
        return KisImportExportFilter::NoDocumentCreated;

    QString filename = inputFile();

    if (filename.isEmpty()) {
        return KisImportExportFilter::FileNotFound;
    }

    if (!QFileInfo(filename).exists()) {
        return KisImportExportFilter::FileNotFound;
    }


    QFile fp(filename);
    doc->prepareForImport();
    return loadFromDevice(&fp, doc);
}

int readNumber(QIODevice* device)
{
    char c;
    int val = 0;
    while (true) {
        if (!device->getChar(&c)) break; // End of the file
        if (isdigit(c)) {
            val = 10 * val + c - '0';
        } else if (c == '#') {
            device->readLine();
            break;
        } else if (isspace((uchar) c)) {
            break;
        }
    }
    return val;
}

class KisPpmFlow
{
public:
    KisPpmFlow() {
    }
    virtual ~KisPpmFlow() {
    }
    virtual void nextRow() = 0;
    virtual bool valid() = 0;
    virtual bool nextUint1() = 0;
    virtual quint8 nextUint8() = 0;
    virtual quint16 nextUint16() = 0;

};

class KisAsciiPpmFlow : public KisPpmFlow
{
public:
    KisAsciiPpmFlow(QIODevice* device) : m_device(device) {
    }
    ~KisAsciiPpmFlow() override {
    }
    void nextRow() override {
    }
    bool valid() override {
        return !m_device->atEnd();
    }
    bool nextUint1() override {
        return readNumber(m_device) == 1;
    }
    quint8 nextUint8() override {
        return readNumber(m_device);
    }
    quint16 nextUint16() override {
        return readNumber(m_device);
    }
private:
    QIODevice* m_device;
};

class KisBinaryPpmFlow : public KisPpmFlow
{
public:
    KisBinaryPpmFlow(QIODevice* device, int lineWidth) : m_pos(0), m_device(device), m_lineWidth(lineWidth) {
    }
    ~KisBinaryPpmFlow() override {
    }
    void nextRow() override {
        m_array = m_device->read(m_lineWidth);
        m_ptr = m_array.data();
    }
    bool valid() override {
        return m_array.size() == m_lineWidth;
    }
    bool nextUint1() override {
        if (m_pos == 0) {
            m_current = nextUint8();
            m_pos = 8;
        }
        bool v = (m_current & 1) == 1;
        --m_pos;
        m_current = m_current >> 1;
        return v;
    }
    quint8 nextUint8() override {
        quint8 v = *reinterpret_cast<quint8*>(m_ptr);
        m_ptr += 1;
        return v;
    }
    quint16 nextUint16() override {
        quint16 v = *reinterpret_cast<quint16*>(m_ptr);
        m_ptr += 2;
        return qFromBigEndian(v);
    }
private:
    int m_pos;
    quint8 m_current;
    char* m_ptr;
    QIODevice* m_device;
    QByteArray m_array;
    int m_lineWidth;
};

KisImportExportFilter::ConversionStatus KisPPMImport::loadFromDevice(QIODevice* device, KisDocument* doc)
{
    dbgFile << "Start decoding file";
    device->open(QIODevice::ReadOnly);
    if (!device->isOpen()) {
        return KisImportExportFilter::CreationError;
    }

    QByteArray array = device->read(2);

    if (array.size() < 2) return KisImportExportFilter::CreationError;

    // Read the type of the ppm file
    enum { Puk, P1, P2, P3, P4, P5, P6 } fileType = Puk; // Puk => unknown

    int channels = -1;
    bool isAscii = false;

    if (array == "P1") {
        fileType = P1;
        isAscii = true;
        channels = 0;
    } else if (array == "P2") {
        fileType = P2;
        channels = 1;
        isAscii = true;
    } else if (array == "P3") {
        fileType = P3;
        channels = 3;
        isAscii = true;
    } else if (array == "P4") {
        fileType = P4;
        channels = 0;
    } else if (array == "P5") { // PGM
        fileType = P5;
        channels = 1;
    } else if (array == "P6") { // PPM
        fileType = P6;
        channels = 3;
    }

    Q_ASSERT(channels != -1);
    char c; device->getChar(&c);
    if (!isspace(c)) return KisImportExportFilter::CreationError; // Invalid file, it should have a separator now

    // Read width
    int width = readNumber(device);
    int height = readNumber(device);
    int maxval = 1;

    if (fileType != P1 && fileType != P4) {
        maxval = readNumber(device);
    }

    dbgFile << "Width = " << width << " height = " << height << "maxval = " << maxval;

    // Select the colorspace depending on the maximum value
    int pixelsize = -1;
    const KoColorSpace* colorSpace = 0;
    if (maxval <= 255) {
        if (channels == 1 || channels == 0) {
            pixelsize = 1;
            colorSpace = KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), 0);
        } else {
            pixelsize = 3;
            colorSpace = KoColorSpaceRegistry::instance()->rgb8();
        }
    } else if (maxval <= 65535) {
        if (channels == 1 || channels == 0) {
            pixelsize = 2;
            colorSpace = KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), 0);
        } else {
            pixelsize = 6;
            colorSpace = KoColorSpaceRegistry::instance()->rgb16();
        }
    } else {
        dbgFile << "Unknown colorspace";
        return KisImportExportFilter::CreationError;
    }

    KisImageSP image = new KisImage(doc->createUndoStore(), width, height, colorSpace, "built image");
    KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), 255);

    KisPpmFlow* ppmFlow = 0;
    if (isAscii) {
        ppmFlow = new KisAsciiPpmFlow(device);
    } else {
        ppmFlow = new KisBinaryPpmFlow(device, pixelsize * width);
    }

    for (int v = 0; v < height; ++v) {
        KisHLineIteratorSP it = layer->paintDevice()->createHLineIteratorNG(0, v, width);
        ppmFlow->nextRow();
        if (!ppmFlow->valid()) return KisImportExportFilter::CreationError;
        if (maxval <= 255) {
            if (channels == 3) {
                do {
                    KoBgrTraits<quint8>::setRed(it->rawData(), ppmFlow->nextUint8());
                    KoBgrTraits<quint8>::setGreen(it->rawData(), ppmFlow->nextUint8());
                    KoBgrTraits<quint8>::setBlue(it->rawData(), ppmFlow->nextUint8());
                    colorSpace->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
                } while (it->nextPixel());
            } else if (channels == 1) {
                do {
                    *reinterpret_cast<quint8*>(it->rawData()) = ppmFlow->nextUint8();
                    colorSpace->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
                } while (it->nextPixel());
            } else if (channels == 0) {
                do {
                    if (ppmFlow->nextUint1()) {
                        *reinterpret_cast<quint8*>(it->rawData()) = 255;
                    } else {
                        *reinterpret_cast<quint8*>(it->rawData()) = 0;
                    }
                    colorSpace->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
                } while (it->nextPixel());
            }
        } else {
            if (channels == 3) {
                do {
                    KoBgrU16Traits::setRed(it->rawData(), ppmFlow->nextUint16());
                    KoBgrU16Traits::setGreen(it->rawData(), ppmFlow->nextUint16());
                    KoBgrU16Traits::setBlue(it->rawData(), ppmFlow->nextUint16());
                    colorSpace->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
                } while (it->nextPixel());
            } else if (channels == 1) {
                do {
                    *reinterpret_cast<quint16*>(it->rawData()) = ppmFlow->nextUint16();
                    colorSpace->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
                } while (it->nextPixel());
            }
        }
    }

    image->addNode(layer.data(), image->rootLayer().data());

    doc->setCurrentImage(image);
    return KisImportExportFilter::OK;
}

#include "kis_ppm_import.moc"
