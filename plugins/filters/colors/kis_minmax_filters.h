/*
 * This file is part of Krita
 *
 * Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_MINMAX_FILTERS_H
#define KIS_MINMAX_FILTERS_H

#include "filter/kis_filter.h"

class KisFilterMax : public KisFilter
{
public:

    KisFilterMax();

    void processImpl(KisPaintDeviceSP src,
                     const QRect& size,
                     const KisFilterConfigurationSP config,
                     KoUpdater* progressUpdater
                     ) const;

    static inline KoID id() {
        return KoID("maximize", i18n("Maximize Channel"));
    }

};

class KisFilterMin : public KisFilter
{
public:
    KisFilterMin();
public:

    void processImpl(KisPaintDeviceSP device,
                     const QRect& rect,
                     const KisFilterConfigurationSP config,
                     KoUpdater* progressUpdater
                     ) const;
    static inline KoID id() {
        return KoID("minimize", i18n("Minimize Channel"));
    }
};

#endif
