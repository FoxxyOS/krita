/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_IPTC_IO_H_
#define _KIS_IPTC_IO_H_

#include <metadata/kis_meta_data_io_backend.h>

#include <klocalizedstring.h>

class KisIptcIO : public KisMetaData::IOBackend
{
    struct Private;
public:
    KisIptcIO();
    virtual ~KisIptcIO();
    virtual QString id() const {
        return "iptc";
    }
    virtual QString name() const {
        return i18n("Iptc");
    }
    virtual BackendType type() const {
        return Binary;
    }
    virtual bool supportSaving() const {
        return true;
    }
    virtual bool saveTo(KisMetaData::Store* store, QIODevice* ioDevice, HeaderType headerType = NoHeader) const;
    virtual bool canSaveAllEntries(KisMetaData::Store* store) const;
    virtual bool supportLoading() const {
        return true;
    }
    virtual bool loadFrom(KisMetaData::Store* store, QIODevice* ioDevice) const;
private:
    void initMappingsTable() const;
private:
    Private* const d;
};

#endif
