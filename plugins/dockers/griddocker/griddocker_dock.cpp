/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
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

#include "griddocker_dock.h"
//#include "gridwidget.h"

// #include <QLabel>
// #include <QVBoxLayout>
#include <QStatusBar>
#include <klocalizedstring.h>

#include "kis_canvas2.h"
#include <KisViewManager.h>
#include <kis_zoom_manager.h>
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_signal_compressor.h"
#include "grid_config_widget.h"
#include "kis_grid_manager.h"
#include "kis_grid_config.h"
#include "kis_guides_manager.h"
#include "kis_guides_config.h"


GridDockerDock::GridDockerDock( )
    : QDockWidget(i18n("Grid and Guides"))
    , m_canvas(0)
{
    m_configWidget = new GridConfigWidget(this);
    connect(m_configWidget, SIGNAL(gridValueChanged()), SLOT(slotGuiGridConfigChanged()));
    connect(m_configWidget, SIGNAL(guidesValueChanged()), SLOT(slotGuiGuidesConfigChanged()));
    setWidget(m_configWidget);
    setEnabled(m_canvas);
}

GridDockerDock::~GridDockerDock()
{
}

void GridDockerDock::setCanvas(KoCanvasBase * canvas)
{
    if(canvas && m_canvas == canvas)
        return;

    if (m_canvas) {
        m_canvasConnections.clear();
        m_canvas->disconnectCanvasObserver(this);
        m_canvas->image()->disconnect(this);
    }

    m_canvas = canvas ? dynamic_cast<KisCanvas2*>(canvas) : 0;
    setEnabled(m_canvas);

    if (m_canvas) {
        m_canvasConnections.addConnection(
            m_canvas->viewManager()->gridManager(),
            SIGNAL(sigRequestUpdateGridConfig(const KisGridConfig&)),
            this,
            SLOT(slotGridConfigUpdateRequested(const KisGridConfig&)));

        m_canvasConnections.addConnection(
            m_canvas->viewManager()->guidesManager(),
            SIGNAL(sigRequestUpdateGuidesConfig(const KisGuidesConfig&)),
            this,
            SLOT(slotGuidesConfigUpdateRequested(const KisGuidesConfig&)));

        QRect rc = m_canvas->image()->bounds();
        m_configWidget->setGridDivision(rc.width() / 2, rc.height() / 2);
    }
}

void GridDockerDock::unsetCanvas()
{
    setCanvas(0);
}

void GridDockerDock::slotGuiGridConfigChanged()
{
    if (!m_canvas) return;
    m_canvas->viewManager()->gridManager()->setGridConfig(m_configWidget->gridConfig());
}

void GridDockerDock::slotGridConfigUpdateRequested(const KisGridConfig &config)
{
    m_configWidget->setGridConfig(config);
}

void GridDockerDock::slotGuiGuidesConfigChanged()
{
    if (!m_canvas) return;
    m_canvas->viewManager()->guidesManager()->setGuidesConfig(m_configWidget->guidesConfig());
}

void GridDockerDock::slotGuidesConfigUpdateRequested(const KisGuidesConfig &config)
{
    m_configWidget->setGuidesConfig(config);
}
