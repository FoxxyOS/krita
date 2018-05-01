/*
 * rotateimage.cc -- Part of Krita
 *
 * Copyright (c) 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
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

#include "rotateimage.h"

#include <math.h>

#include <klocalizedstring.h>
#include <kis_debug.h>
#include <kpluginfactory.h>
#include <kis_icon.h>
#include <kundo2magicstring.h>
#include <kis_image.h>
#include <kis_types.h>
#include <KisViewManager.h>
#include <kis_image_manager.h>
#include <kis_node_manager.h>
#include <kis_canvas_resource_provider.h>
#include <kis_group_layer.h>
#include <kis_action.h>

#include "dlg_rotateimage.h"

K_PLUGIN_FACTORY_WITH_JSON(RotateImageFactory, "kritarotateimage.json", registerPlugin<RotateImage>();)

RotateImage::RotateImage(QObject *parent, const QVariantList &)
        : KisViewPlugin(parent)
{

    KisAction *action  = createAction("rotateimage");
    connect(action, SIGNAL(triggered()), this, SLOT(slotRotateImage()));

    action  = createAction("rotateImageCW90");
    connect(action, SIGNAL(triggered()), this, SLOT(slotRotateImage90()));

    action  = createAction("rotateImage180");
    connect(action, SIGNAL(triggered()), this, SLOT(slotRotateImage180()));

    action  = createAction("rotateImageCCW90");
    connect(action, SIGNAL(triggered()), this, SLOT(slotRotateImage270()));

    action  = createAction("mirrorImageHorizontal");
    connect(action, SIGNAL(triggered()), this, SLOT(slotMirrorImageHorizontal()));

    action  = createAction("mirrorImageVertical");
    connect(action, SIGNAL(triggered()), this, SLOT(slotMirrorImageVertical()));

    action  = createAction("rotatelayer");
    connect(action, SIGNAL(triggered()), this, SLOT(slotRotateLayer()));

    action  = createAction("rotateLayer180");
    connect(action, SIGNAL(triggered()), m_view->nodeManager(), SLOT(rotate180()));

    action  = createAction("rotateLayerCW90");
    connect(action, SIGNAL(triggered()), m_view->nodeManager(), SLOT(rotateRight90()));

    action  = createAction("rotateLayerCCW90");
    connect(action, SIGNAL(triggered()), m_view->nodeManager(), SLOT(rotateLeft90()));
}

RotateImage::~RotateImage()
{
}

void RotateImage::slotRotateImage()
{
    KisImageWSP image = m_view->image();

    if (!image) return;

    DlgRotateImage * dlgRotateImage = new DlgRotateImage(m_view->mainWindow(), "RotateImage");
    Q_CHECK_PTR(dlgRotateImage);

    dlgRotateImage->setCaption(i18n("Rotate Image"));

    if (dlgRotateImage->exec() == QDialog::Accepted) {
        double angle = dlgRotateImage->angle() * M_PI / 180;
        m_view->imageManager()->rotateCurrentImage(angle);
    }
    delete dlgRotateImage;
}

void RotateImage::slotRotateImage90()
{
    m_view->imageManager()->rotateCurrentImage(M_PI / 2);
}

void RotateImage::slotRotateImage180()
{
    m_view->imageManager()->rotateCurrentImage(M_PI);
}

void RotateImage::slotRotateImage270()
{
    m_view->imageManager()->rotateCurrentImage(- M_PI / 2 + M_PI*2);
}

void RotateImage::slotMirrorImageVertical()
{
    KisImageWSP image = m_view->image();
    if (!image) return;
    m_view->nodeManager()->mirrorNode(image->rootLayer(), kundo2_i18n("Mirror Image Vertically"), Qt::Vertical);
}

void RotateImage::slotMirrorImageHorizontal()
{
    KisImageWSP image = m_view->image();
    if (!image) return;
    m_view->nodeManager()->mirrorNode(image->rootLayer(), kundo2_i18n("Mirror Image Horizontally"), Qt::Horizontal);
}

void RotateImage::slotRotateLayer()
{
    KisImageWSP image = m_view->image();

    if (!image) return;

    DlgRotateImage * dlgRotateImage = new DlgRotateImage(m_view->mainWindow(), "RotateLayer");
    Q_CHECK_PTR(dlgRotateImage);

    dlgRotateImage->setCaption(i18n("Rotate Layer"));

    if (dlgRotateImage->exec() == QDialog::Accepted) {
        double angle = dlgRotateImage->angle() * M_PI / 180;
        m_view->nodeManager()->rotate(angle);

    }
    delete dlgRotateImage;
}

#include "rotateimage.moc"
