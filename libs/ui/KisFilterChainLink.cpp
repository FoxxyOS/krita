/* This file is part of the Calligra libraries
   Copyright (C) 2001 Werner Trobin <trobin@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/
#include "KisFilterChainLink.h"
#include <QMetaMethod>
#include <QPluginLoader>
#include <kis_debug.h>
#include "KisFilterEntry.h"
#include "KisImportExportManager.h"
#include "KoProgressUpdater.h"
#include "KoUpdater.h"

namespace
{
    KoUpdater *createUpdater(KisFilterChainSP chain)
    {
        QPointer<KoUpdater> updater = 0;
        Q_ASSERT(chain);
        Q_ASSERT(chain->manager());
        KoProgressUpdater *pu = chain->manager()->progressUpdater();
        if (pu) {
            updater = pu->startSubtask(1, "filter");
            updater->setProgress(0);
        }

        return updater;
    }
}

namespace CalligraFilter {

    ChainLink::ChainLink(KisFilterChainSP chain, KisFilterEntrySP filterEntry,
                         const QByteArray& from, const QByteArray& to)
        : m_chain(chain)
        , m_filterEntry(filterEntry)
        , m_from(from)
        , m_to(to)
        , m_filter(0)
        , m_updater(createUpdater(chain))
    {
    }

    ChainLink::~ChainLink() {
    }

    KisImportExportFilter::ConversionStatus ChainLink::invokeFilter()
    {
        if (!m_filterEntry) {
            errFile << "This filter entry is null. Strange stuff going on." << endl;
            return KisImportExportFilter::FilterEntryNull;
        }

        m_filter = m_filterEntry->createFilter(m_chain);

        if (!m_filter) {
            errFile << "Couldn't create the filter." << endl;
            return KisImportExportFilter::FilterCreationError;
        }

        Q_ASSERT(m_updater);
        if (m_updater) {
            // if there is an updater, use that for progress reporting
            m_filter->setUpdater(m_updater);
        }

        KisImportExportFilter::ConversionStatus status = m_filter->convert(m_from, m_to, m_chain->filterManagerExportConfiguration());
        delete m_filter;
        m_filter = 0;
        if (m_updater) {
            m_updater->setProgress(100);
        }
        return status;
    }

    void ChainLink::dump() const
    {
        dbgFile << "   Link:" << m_filterEntry->loader()->fileName();
    }


}
