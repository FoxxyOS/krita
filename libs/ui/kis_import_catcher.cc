/*
 *  Copyright (c) 2006 Boudewijn Rempt  <boud@valdyas.org>
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

#include "kis_import_catcher.h"
#include <kis_debug.h>

#include <klocalizedstring.h>
#include <kis_debug.h>
#include <QUrl>

#include <KisImportExportManager.h>

#include "kis_node_manager.h"
#include "kis_count_visitor.h"
#include "KisViewManager.h"
#include "KisDocument.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_painter.h"
#include "kis_selection.h"
#include "kis_node_commands_adapter.h"
#include "kis_group_layer.h"
#include "kis_statusbar.h"
#include "kis_progress_widget.h"
#include "kis_config.h"
#include "KisPart.h"

struct KisImportCatcher::Private
{
public:
    KisDocument* doc;
    KisViewManager* view;
    QUrl url;
    QString layerType;

    QString prettyLayerName() const;
    void importAsPaintLayer(KisPaintDeviceSP device);
    void importAsTransparencyMask(KisPaintDeviceSP device);
};

QString KisImportCatcher::Private::prettyLayerName() const
{
    QString name = url.fileName();
    return !name.isEmpty() ? name : url.toDisplayString();
}

void KisImportCatcher::Private::importAsPaintLayer(KisPaintDeviceSP device)
{
    KisLayerSP newLayer = new KisPaintLayer(view->image(),
                                            prettyLayerName(),
                                            OPACITY_OPAQUE_U8,
                                            device);

    KisNodeSP parent = 0;
    KisLayerSP currentActiveLayer = view->activeLayer();

    if (currentActiveLayer) {
        parent = currentActiveLayer->parent();
    }

    if (parent.isNull()) {
        parent = view->image()->rootLayer();
    }

    KisNodeCommandsAdapter adapter(view);
    adapter.addNode(newLayer, parent, currentActiveLayer);
}

KisImportCatcher::KisImportCatcher(const QUrl &url, KisViewManager *view, const QString &layerType)
    : m_d(new Private)
{
    m_d->doc = KisPart::instance()->createDocument();

    KoProgressProxy *progressProxy = view->statusBar()->progress()->progressProxy();
    m_d->doc->setProgressProxy(progressProxy);
    m_d->view = view;
    m_d->url = url;
    m_d->layerType = layerType;

    connect(m_d->doc, SIGNAL(sigLoadingFinished()), this, SLOT(slotLoadingFinished()));
    bool result = m_d->doc->openUrl(url);

    if (!result) {
        deleteMyself();
    }
}

void KisImportCatcher::slotLoadingFinished()
{
    KisImageWSP importedImage = m_d->doc->image();
    importedImage->waitForDone();

    if (importedImage && importedImage->projection()->exactBounds().isValid()) {
        if (m_d->layerType != "KisPaintLayer") {
            m_d->view->nodeManager()->createNode(m_d->layerType, false, importedImage->projection());
        }
        else {
            KisPaintDeviceSP dev = importedImage->projection();
            adaptClipToImageColorSpace(dev, m_d->view->image());
            m_d->importAsPaintLayer(dev);
        }
    }

    deleteMyself();
}

void KisImportCatcher::deleteMyself()
{
    m_d->doc->deleteLater();
    deleteLater();
}

KisImportCatcher::~KisImportCatcher()
{
    delete m_d;
}

void KisImportCatcher::adaptClipToImageColorSpace(KisPaintDeviceSP dev, KisImageSP image)
{
    KisConfig cfg;
    qDebug() << "dev" << dev->colorSpace() << "image" << image->colorSpace() << "cfg" << cfg.convertToImageColorspaceOnImport();
    if (cfg.convertToImageColorspaceOnImport() && *dev->colorSpace() != *image->colorSpace()) {
        /// XXX: do we need intent here?
        KUndo2Command* cmd = dev->convertTo(image->colorSpace());
        delete cmd;
    }
}

