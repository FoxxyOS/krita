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
#ifndef _PSD_CONVERTER_H_
#define _PSD_CONVERTER_H_

#include <stdio.h>

#include <QObject>

#include <QFileInfo>

#include "kis_types.h"
#include <KisImageBuilderResult.h>
class KisDocument;

class PSDSaver : public QObject {

    Q_OBJECT

public:

    PSDSaver(KisDocument *doc);
    virtual ~PSDSaver();

public:

    KisImageBuilder_Result buildFile(const QString &filename);

    KisImageSP image();

public Q_SLOTS:

    virtual void cancel();

private:

    KisImageSP m_image;
    KisDocument *m_doc;
    bool m_stop;
};

#endif
