/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_legacy_tile_compressor.h"
#include "kis_paint_device_writer.h"
#include <QIODevice>

#define TILE_DATA_SIZE(pixelSize) ((pixelSize) * KisTileData::WIDTH * KisTileData::HEIGHT)

KisLegacyTileCompressor::KisLegacyTileCompressor()
{
}

KisLegacyTileCompressor::~KisLegacyTileCompressor()
{
}

bool KisLegacyTileCompressor::writeTile(KisTileSP tile, KisPaintDeviceWriter &store)
{
    const qint32 tileDataSize = TILE_DATA_SIZE(tile->pixelSize());

    const qint32 bufferSize = maxHeaderLength() + 1;
    quint8 *headerBuffer = new quint8[bufferSize];

    bool retval = writeHeader(tile, headerBuffer);

    store.write((char *)headerBuffer, strlen((char *)headerBuffer));

    tile->lockForRead();
    retval = store.write((char *)tile->data(), tileDataSize);
    tile->unlock();

    delete[] headerBuffer;

    return retval;
}

bool KisLegacyTileCompressor::readTile(QIODevice *stream, KisTiledDataManager *dm)
{
    const qint32 tileDataSize = TILE_DATA_SIZE(pixelSize(dm));

    const qint32 bufferSize = maxHeaderLength() + 1;
    quint8 *headerBuffer = new quint8[bufferSize];

    qint32 x, y;
    qint32 width, height;

    stream->readLine((char *)headerBuffer, bufferSize);
    sscanf((char *) headerBuffer, "%d,%d,%d,%d", &x, &y, &width, &height);

    qint32 row = yToRow(dm, y);
    qint32 col = xToCol(dm, x);

    KisTileSP tile = dm->getTile(col, row, true);

    tile->lockForWrite();
    stream->read((char *)tile->data(), tileDataSize);
    tile->unlock();

    return true;
}

void KisLegacyTileCompressor::compressTileData(KisTileData *tileData,
                                               quint8 *buffer,
                                               qint32 bufferSize,
                                               qint32 &bytesWritten)
{
    bytesWritten = 0;
    const qint32 tileDataSize = TILE_DATA_SIZE(tileData->pixelSize());
    Q_UNUSED(bufferSize);
    Q_ASSERT(bufferSize >= tileDataSize);
    memcpy(buffer, tileData->data(), tileDataSize);
    bytesWritten += tileDataSize;
}

bool KisLegacyTileCompressor::decompressTileData(quint8 *buffer,
                                                 qint32 bufferSize,
                                                 KisTileData *tileData)
{
    const qint32 tileDataSize = TILE_DATA_SIZE(tileData->pixelSize());
    if (bufferSize >= tileDataSize) {
        memcpy(tileData->data(), buffer, tileDataSize);
        return true;
    }
    return false;
}

qint32 KisLegacyTileCompressor::tileDataBufferSize(KisTileData *tileData)
{
    return TILE_DATA_SIZE(tileData->pixelSize());
}

inline qint32 KisLegacyTileCompressor::maxHeaderLength()
{
    static const qint32 LEGACY_MAGIC_NUMBER = 79;
    return LEGACY_MAGIC_NUMBER;
}

inline bool KisLegacyTileCompressor::writeHeader(KisTileSP tile,
                                                 quint8 *buffer)
{
    qint32 x, y;
    qint32 width, height;

    tile->extent().getRect(&x, &y, &width, &height);
    sprintf((char *)buffer, "%d,%d,%d,%d\n", x, y, width, height);

    return true;
}
