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
#ifndef PSD_COLORMODE_BLOCK_H
#define PSD_COLORMODE_BLOCK_H

#include <psd.h>
#include "psd_header.h"

#include <QByteArray>
#include <QColor>

class PSDColorModeBlock
{
public:

    PSDColorModeBlock(psd_color_mode colormode);

    bool read(QIODevice* io);
    bool write(QIODevice* io);
    bool valid();

    quint32 blocksize;
    psd_color_mode colormode;
    QByteArray data;

    QString error;

    /* to store rgb colormap values of indexed image
    */
    QList<QColor> colormap;
    QByteArray duotoneSpecification; // Krita should save this in an annotation and write it back, if present

};

#endif // PSD_COLORMODE_BLOCK_H
