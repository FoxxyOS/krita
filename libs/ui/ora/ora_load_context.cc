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

#include "ora_load_context.h"

#include <QDomDocument>

#include <KoStore.h>
#include <KoStoreDevice.h>

#include <kis_image.h>

#include <kis_paint_device.h>

#include "kis_png_converter.h"

OraLoadContext::OraLoadContext(KoStore* _store) : m_store(_store)
{
}

OraLoadContext::~OraLoadContext()
{
}

KisImageWSP OraLoadContext::loadDeviceData(const QString & filename)
{
    if (m_store->open(filename)) {
        KoStoreDevice io(m_store);
        if (!io.open(QIODevice::ReadOnly)) {
            dbgFile << "Could not open for reading:" << filename;
            return 0;
        }
        KisPNGConverter pngConv(0);
        pngConv.buildImage(&io);
        io.close();
        m_store->close();

        return pngConv.image();

    }
    return 0;
}

QDomDocument OraLoadContext::loadStack()
{
    m_store->open("stack.xml");
    KoStoreDevice io(m_store);
    QDomDocument doc;
    doc.setContent(&io, false);
    io.close();
    m_store->close();
    return doc;
}
