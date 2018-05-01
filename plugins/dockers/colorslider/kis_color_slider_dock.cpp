/*
 *  Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2014 Wolthera van Hövell <griffinvalley@gmail.com>
 *  Copyright (c) 2015 Moritz Molch <kde@moritzmolch.de>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
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

#include "kis_color_slider_dock.h"

//#include <QWidget>
#include <QVBoxLayout>
#include <QBitArray>

#include <ksharedconfig.h>
#include <klocalizedstring.h>
#include <kconfig.h>
#include "kis_config_notifier.h"
#include <kconfiggroup.h>

#include <kis_layer.h>
#include <KisViewManager.h>
#include <canvas/kis_canvas2.h>
#include <kis_canvas_resource_provider.h>
#include <kis_image.h>
#include <kis_display_color_converter.h>

#include "kis_color_slider_widget.h"

ColorSliderDock::ColorSliderDock()
    : QDockWidget(i18n("Color Sliders"))
    , m_canvas(0)
    , m_view(0)
    , m_colorSliders(0)
{
}

void ColorSliderDock::setCanvas(KoCanvasBase * canvas)
{
    setEnabled(canvas != 0);

    if (m_canvas) {
        m_canvas->disconnectCanvasObserver(this);
    }
    if (m_view) {
        m_canvasConnections.clear();
    }

    KisCanvas2* kisCanvas = dynamic_cast<KisCanvas2*>(canvas);
    if (kisCanvas) {
        KisViewManager* view = kisCanvas->viewManager();

        if (m_colorSliders) {
            m_colorSliders->disconnect(); // explicit disconnect in case Qt gets confused.
            delete m_colorSliders;
        }
        QWidget *m_sliderdockerwidget = new QWidget;
        QVBoxLayout *m_layout = new QVBoxLayout(m_sliderdockerwidget);
        m_layout->setContentsMargins(4,4,4,0);
        m_layout->setSpacing(1);
        setWidget(m_sliderdockerwidget);
        //m_updateAllowed = true;

        //settings//
        QBitArray m_SlidersConfigArray(12);

        KConfigGroup cfg =  KSharedConfig::openConfig()->group("hsxColorSlider");

        m_SlidersConfigArray[0] =cfg.readEntry("hsvH", false);
        m_SlidersConfigArray[1] =cfg.readEntry("hsvS", false);
        m_SlidersConfigArray[2] =cfg.readEntry("hsvV", false);

        m_SlidersConfigArray[3] =cfg.readEntry("hslH", true);
        m_SlidersConfigArray[4] =cfg.readEntry("hslS", true);
        m_SlidersConfigArray[5] =cfg.readEntry("hslL", true);

        m_SlidersConfigArray[6] =cfg.readEntry("hsiH", false);
        m_SlidersConfigArray[7] =cfg.readEntry("hsiS", false);
        m_SlidersConfigArray[8] =cfg.readEntry("hsiI", false);

        m_SlidersConfigArray[9] =cfg.readEntry("hsyH", false);
        m_SlidersConfigArray[10]=cfg.readEntry("hsyS", false);
        m_SlidersConfigArray[11]=cfg.readEntry("hsyY", false);


        m_colorSliders = new KisColorSliderWidget(kisCanvas->displayColorConverter()->displayRendererInterface(), this, kisCanvas, m_SlidersConfigArray);
        m_layout->addWidget(m_colorSliders);

        m_canvasConnections.addConnection(m_colorSliders, SIGNAL(colorChanged(const KoColor&)), view->resourceProvider(), SLOT(slotSetFGColor(const KoColor&)));
        m_canvasConnections.addConnection(view->resourceProvider(), SIGNAL(sigFGColorChanged(const KoColor&)), m_colorSliders, SLOT(setColor(const KoColor&)));
        m_canvasConnections.addConnection(kisCanvas->displayColorConverter(), SIGNAL(displayConfigurationChanged()), this, SLOT(udpateSliders()));
        m_canvasConnections.addConnection(KisConfigNotifier::instance(), SIGNAL(configChanged()), m_colorSliders, SLOT(slotConfigChanged()));

        m_canvas = kisCanvas;
        m_view = view;
    }
}


void ColorSliderDock::unsetCanvas()
{
    setEnabled(false);
    m_canvas = 0;
    m_view = 0;

    delete m_colorSliders;
    m_colorSliders = 0;
}

void ColorSliderDock::udpateSliders()
{
    if (!m_colorSliders) return;
    m_colorSliders->setColor(m_view->resourceProvider()->fgColor());
}


#include "moc_kis_color_slider_dock.cpp"



