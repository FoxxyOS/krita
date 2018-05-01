/*
 *  Copyright (c) 2010 Adam Celarek <kdedev at xibo dot at>
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


#ifndef KIS_COLOR_SELECTOR_NG_DOCKER_WIDGET_H
#define KIS_COLOR_SELECTOR_NG_DOCKER_WIDGET_H

#include <QWidget>
#include <QPointer>
#include <kis_canvas2.h>

class QAction;

class KisCommonColors;
class KisColorHistory;
class KisColorSelectorContainer;

class QVBoxLayout;
class QHBoxLayout;

class KisColorSelectorNgDockerWidget : public QWidget
{
Q_OBJECT
public:
    explicit KisColorSelectorNgDockerWidget(QWidget *parent = 0);
    void setCanvas(KisCanvas2* canvas);
    void unsetCanvas();
public Q_SLOTS:
    void openSettings();

Q_SIGNALS:
    void settingsChanged();

protected Q_SLOTS:
    void updateLayout();
    void reactOnLayerChange();

private:
    KisColorSelectorContainer* m_colorSelectorContainer;
    KisColorHistory* m_colorHistoryWidget;
    KisCommonColors* m_commonColorsWidget;

    QAction * m_colorHistoryAction;
    QAction * m_commonColorsAction;

    QHBoxLayout* m_verticalColorPatchesLayout; // vertical color patches should be added here
    QVBoxLayout* m_horizontalColorPatchesLayout;//horizontal ----------"----------------------

    QPointer<KisCanvas2> m_canvas;

};

#endif
