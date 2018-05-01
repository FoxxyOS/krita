/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ora_save_context.h"

#include <QDomDocument>

#include <KoStore.h>
#include <KoStoreDevice.h>

#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_image.h>

#include <metadata/kis_meta_data_store.h>

#include "kis_png_converter.h"

OraSaveContext::OraSaveContext(KoStore* _store) : m_id(0), m_store(_store)
{

}

QString OraSaveContext::saveDeviceData(KisPaintDeviceSP dev, KisMetaData::Store* metaData, const QRect &imageRect, const qreal xRes, const qreal yRes)
{
    QString filename = QString("data/layer%1.png").arg(m_id++);
    if (KisPNGConverter::saveDeviceToStore(filename, imageRect, xRes, yRes, dev, m_store, metaData)) {
        return filename;
    }
    return "";
}


void OraSaveContext::saveStack(const QDomDocument& doc)
{
    if (m_store->open("stack.xml")) {
        KoStoreDevice io(m_store);
        io.write(doc.toByteArray());
        io.close();
        m_store->close();
    } else {
        dbgFile << "Opening of the stack.xml file failed :";
    }
}
