/*
 *  Copyright (c) 2011 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_selection_tool_config_widget_helper.h"

#include <QKeyEvent>
#include "kis_selection_options.h"
#include "kis_canvas2.h"
#include "KisViewManager.h"
#include "kis_canvas_resource_provider.h"

KisSelectionToolConfigWidgetHelper::KisSelectionToolConfigWidgetHelper(const QString &windowTitle)
    : m_optionsWidget(0),
      m_windowTitle(windowTitle)
{
}

void KisSelectionToolConfigWidgetHelper::createOptionWidget(KisCanvas2 *canvas, const QString &toolId)
{
    m_optionsWidget = new KisSelectionOptions(canvas);
    Q_CHECK_PTR(m_optionsWidget);

    // slotCanvasResourceChanged... yuck
    m_resourceProvider = canvas->viewManager()->resourceProvider();
    // connect(m_resourceProvider->resourceManager(), &KisCanvasResourceManager::canvasResourceChanged,
    //         this, KisSelectionToolConfigWidgetHelper::slotCanvasResourceChanged);

    m_optionsWidget->setObjectName(toolId + "option widget");
    m_optionsWidget->setWindowTitle(m_windowTitle);
    m_optionsWidget->setAction(selectionAction());
    m_optionsWidget->setMode(selectionMode());

    // See https://bugs.kde.org/show_bug.cgi?id=316896
    QWidget *specialSpacer = new QWidget(m_optionsWidget);
    specialSpacer->setObjectName("SpecialSpacer");
    specialSpacer->setFixedSize(0, 0);
    m_optionsWidget->layout()->addWidget(specialSpacer);

    connect(m_optionsWidget, &KisSelectionOptions::actionChanged,
            this, &KisSelectionToolConfigWidgetHelper::slotWidgetActionChanged);
    connect(m_optionsWidget, &KisSelectionOptions::modeChanged,
            this, &KisSelectionToolConfigWidgetHelper::slotWidgetModeChanged);
    connect(m_resourceProvider, &KisCanvasResourceProvider::sigSelectionActionChanged,
            this, &KisSelectionToolConfigWidgetHelper::slotGlobalActionChanged);
    connect(m_resourceProvider, &KisCanvasResourceProvider::sigSelectionModeChanged,
            this, &KisSelectionToolConfigWidgetHelper::slotGlobalModeChanged);

    m_optionsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_optionsWidget->adjustSize();
}

KisSelectionOptions* KisSelectionToolConfigWidgetHelper::optionWidget() const
{
    return m_optionsWidget;
}

SelectionMode KisSelectionToolConfigWidgetHelper::selectionMode() const
{
    return (SelectionMode)m_resourceProvider->selectionMode();
}

SelectionAction KisSelectionToolConfigWidgetHelper::selectionAction() const
{
    return (SelectionAction)m_resourceProvider->selectionAction();
}

void KisSelectionToolConfigWidgetHelper::slotWidgetActionChanged(int action)
{
    if (action >= SELECTION_REPLACE && action <= SELECTION_INTERSECT) {
        m_optionsWidget->setAction(action);
        m_resourceProvider->setSelectionAction(action);
        emit selectionActionChanged(action);
    }
}

void KisSelectionToolConfigWidgetHelper::slotWidgetModeChanged(int mode)
{
    m_optionsWidget->setMode(mode);
    m_resourceProvider->setSelectionMode(mode);
    emit selectionModeChanged(mode);
}

void KisSelectionToolConfigWidgetHelper::slotGlobalActionChanged(int action)
{
    m_optionsWidget->setAction(action);
}

void KisSelectionToolConfigWidgetHelper::slotGlobalModeChanged(int mode)
{
    m_optionsWidget->setMode(mode);
}



bool KisSelectionToolConfigWidgetHelper::processKeyPressEvent(QKeyEvent *event)
{
    event->accept();

    switch(event->key()) {
    case Qt::Key_A:
        slotWidgetActionChanged(SELECTION_ADD);
        break;
    case Qt::Key_S:
        slotWidgetActionChanged(SELECTION_SUBTRACT);
        break;
    case Qt::Key_R:
        slotWidgetActionChanged(SELECTION_REPLACE);
        break;
    case Qt::Key_T:
        slotWidgetActionChanged(SELECTION_INTERSECT);
        break;
    default:
        event->ignore();
    }

    return event->isAccepted();
}
