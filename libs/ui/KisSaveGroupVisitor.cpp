/*
 *  Copyright (C) 2016 Boudewijn Rempt <boud@valdyas.org>
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
#include "KisSaveGroupVisitor.h"

#include <KisDocument.h>
#include <kis_painter.h>
#include <kis_paint_layer.h>
#include <KisPart.h>

KisSaveGroupVisitor::KisSaveGroupVisitor(KisImageWSP image,
                                         bool saveInvisible,
                                         bool saveTopLevelOnly,
                                         const QString &path,
                                         const QString &baseName,
                                         const QString &extension,
                                         const QString &mimeFilter)
    : m_image(image)
    , m_saveInvisible(saveInvisible)
    , m_saveTopLevelOnly(saveTopLevelOnly)
    , m_path(path)
    , m_baseName(baseName)
    , m_extension(extension)
    , m_mimeFilter(mimeFilter)
{
}

KisSaveGroupVisitor::~KisSaveGroupVisitor()
{
}

bool KisSaveGroupVisitor::visit(KisNode* ) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisPaintLayer *) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisAdjustmentLayer *) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisExternalLayer *) {
    return true;
}


bool KisSaveGroupVisitor::visit(KisCloneLayer *) {
    return true;
}


bool KisSaveGroupVisitor::visit(KisFilterMask *) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisTransformMask *) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisTransparencyMask *) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisGeneratorLayer * ) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisSelectionMask* ) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisColorizeMask* ) {
    return true;
}

bool KisSaveGroupVisitor::visit(KisGroupLayer *layer)
{
    if (layer == m_image->rootLayer()) {
        KisLayerSP child = dynamic_cast<KisLayer*>(layer->firstChild().data());
        while (child) {
            child->accept(*this);
            child = dynamic_cast<KisLayer*>(child->nextSibling().data());
        }

    }
    else if (layer->visible() || m_saveInvisible) {

        QRect r = m_image->bounds();

        KisDocument *exportDocument = KisPart::instance()->createDocument();

        exportDocument->prepareForImport();

        KisImageWSP dst = new KisImage(exportDocument->createUndoStore(), r.width(), r.height(), m_image->colorSpace(), layer->name());
        dst->setResolution(m_image->xRes(), m_image->yRes());
        exportDocument->setCurrentImage(dst);
        KisPaintLayer* paintLayer = new KisPaintLayer(dst, "projection", layer->opacity());
        KisPainter gc(paintLayer->paintDevice());
        gc.bitBlt(QPoint(0, 0), layer->projection(), r);
        dst->addNode(paintLayer, dst->rootLayer(), KisLayerSP(0));

        dst->refreshGraph();

        exportDocument->setOutputMimeType(m_mimeFilter.toLatin1());
        exportDocument->setFileBatchMode(true);

        QString path = m_path + "/" + m_baseName + "_" + layer->name().replace(' ', '_') + '.' + m_extension;
        QUrl url = QUrl::fromLocalFile(path);
        exportDocument->exportDocument(url);

        if (!m_saveTopLevelOnly) {
            KisGroupLayerSP child = dynamic_cast<KisGroupLayer*>(layer->firstChild().data());
            while (child) {
                child->accept(*this);
                child = dynamic_cast<KisGroupLayer*>(child->nextSibling().data());
            }
        }
        delete exportDocument;
    }

    return true;
}


