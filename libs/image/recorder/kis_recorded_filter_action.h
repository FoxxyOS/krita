/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_RECORDED_FILTER_ACTION_H_
#define _KIS_RECORDED_FILTER_ACTION_H_

#include "recorder/kis_recorded_node_action.h"

#include <kritaimage_export.h>

class QString;
class KisFilterConfiguration;

/**
 * Action representing a filter.
 */
class KRITAIMAGE_EXPORT KisRecordedFilterAction : public KisRecordedNodeAction
{
public:
    /**
     * @param config the filter configuration, the ownership of config remains in the caller.
     */
    KisRecordedFilterAction(QString name, const KisNodeQueryPath& path, const KisFilter* filter, const KisFilterConfigurationSP config);
    KisRecordedFilterAction(const KisRecordedFilterAction&);
    virtual ~KisRecordedFilterAction();
    using KisRecordedNodeAction::play;
    virtual void play(KisNodeSP node, const KisPlayInfo& _info, KoUpdater* _updater = 0) const;
    virtual void toXML(QDomDocument& doc, QDomElement& elt, KisRecordedActionSaveContext* ) const;
    virtual KisRecordedAction* clone() const;
    const KisFilter* filter() const;
    const KisFilterConfigurationSP filterConfiguration() const;
    /**
     * Set the configuration, and takes the ownership of the config object.
     */
    void setFilterConfiguration(KisFilterConfigurationSP config);
private:
    struct Private;
    Private* const d;
};

class KisRecordedFilterActionFactory : public KisRecordedActionFactory
{
public:
    KisRecordedFilterActionFactory();
    virtual ~KisRecordedFilterActionFactory();
    virtual KisRecordedAction* fromXML(const QDomElement& elt, const KisRecordedActionLoadContext*);
};

#endif
