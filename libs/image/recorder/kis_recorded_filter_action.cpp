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

#include "recorder/kis_recorded_filter_action.h"
#include <QDomElement>
#include <QString>


#include "kis_image.h"
#include "filter/kis_filter.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter_registry.h"
#include "kis_layer.h"
#include "kis_node.h"
#include "kis_selection.h"
#include "kis_transaction.h"
#include "kis_undo_adapter.h"
#include "kis_selection_mask.h"
#include "kis_config_widget.h"
#include "kis_node_query_path.h"
#include "kis_play_info.h"

struct Q_DECL_HIDDEN KisRecordedFilterAction::Private {
    Private()
        : filterConfiguration(0)
    {

    }

    const KisFilter* filter;
    QRect rect;

    KisFilterConfigurationSP configuration() {
        if (!filterConfiguration) {
            filterConfiguration = filter->defaultConfiguration();
            if (filterConfiguration) {
                filterConfiguration->fromXML(configstr);
            }
        }
        return filterConfiguration;
    }

    void setConfiguration(KisFilterConfigurationSP conf) {
        filterConfiguration = conf;
        configstr = conf->toXML();
    }

    void setConfig(const QString& cfg) {
        filterConfiguration = 0;
        configstr = cfg;
    }

    const QString& config() {
        return configstr;
    }

private:
    QString configstr;
    KisFilterConfigurationSP filterConfiguration;
};

KisRecordedFilterAction::KisRecordedFilterAction(QString name, const KisNodeQueryPath& path, const KisFilter* filter, const KisFilterConfigurationSP fc) : KisRecordedNodeAction("FilterAction", name, path), d(new Private)
{
    Q_ASSERT(filter);
    d->filter = filter;
    if (fc) {
        d->setConfig(fc->toXML());
    }
}

KisRecordedFilterAction::KisRecordedFilterAction(const KisRecordedFilterAction& rhs) : KisRecordedNodeAction(rhs), d(new Private(*rhs.d))
{
}

KisRecordedFilterAction::~KisRecordedFilterAction()
{
    delete d;
}

void KisRecordedFilterAction::play(KisNodeSP node, const KisPlayInfo& _info, KoUpdater* _updater) const
{
    KisFilterConfigurationSP  kfc = d->configuration();
    KisPaintDeviceSP dev = node->paintDevice();
    KisLayerSP layer = dynamic_cast<KisLayer*>(node.data());
    QRect r1 = dev->extent();
    KisTransaction transaction(kundo2_i18n("Filter: \"%1\"", d->filter->name()), dev);

    KisImageWSP image = _info.image();
    r1 = r1.intersected(image->bounds());
    if (layer && layer->selection()) {
        r1 = r1.intersected(layer->selection()->selectedExactRect());
    }

    d->filter->process(dev, dev, layer->selection(), r1, kfc, _updater);
    node->setDirty(r1);

    transaction.commit(_info.undoAdapter());
}

void KisRecordedFilterAction::toXML(QDomDocument& doc, QDomElement& elt, KisRecordedActionSaveContext* context) const
{
    KisRecordedAction::toXML(doc, elt, context);
    elt.setAttribute("filter", d->filter->id());
    // Save configuration
    KisFilterConfigurationSP  kfc = d->configuration();
    if (kfc) {
        QDomElement filterConfigElt = doc.createElement("Params");
        kfc->toXML(doc, filterConfigElt);
        elt.appendChild(filterConfigElt);
    }
}

KisRecordedAction* KisRecordedFilterAction::clone() const
{
    return new KisRecordedFilterAction(*this);
}

const KisFilter* KisRecordedFilterAction::filter() const
{
    return d->filter;
}

const KisFilterConfigurationSP KisRecordedFilterAction::filterConfiguration() const
{
    return d->configuration();
}

void KisRecordedFilterAction::setFilterConfiguration(KisFilterConfigurationSP config)
{
    d->setConfiguration(config);
}


KisRecordedFilterActionFactory::KisRecordedFilterActionFactory() :
        KisRecordedActionFactory("FilterAction")
{
}

KisRecordedFilterActionFactory::~KisRecordedFilterActionFactory()
{

}

KisRecordedAction* KisRecordedFilterActionFactory::fromXML(const QDomElement& elt, const KisRecordedActionLoadContext*)
{
    QString name = elt.attribute("name");
    KisNodeQueryPath pathnode = KisNodeQueryPath::fromString(elt.attribute("path"));
    const KisFilterSP filter = KisFilterRegistry::instance()->get(elt.attribute("filter"));
    if (filter) {
        KisFilterConfigurationSP config = filter->defaultConfiguration();
        QDomElement paramsElt = elt.firstChildElement("Params");
        if (config && !paramsElt.isNull()) {
            config->fromXML(paramsElt);
        }
        KisRecordedFilterAction* rfa = new KisRecordedFilterAction(name, pathnode, filter, config);
        return rfa;
    } else {
        return 0;
    }
}
