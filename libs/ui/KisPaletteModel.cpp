/*
 *  Copyright (c) 2013 Sven Langkamp <sven.langkamp@gmail.com>
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

#include "KisPaletteModel.h"

#include <QBrush>

#include <KoColorSpace.h>
#include <resources/KoColorSet.h>
#include <KoColorDisplayRendererInterface.h>

#include <kis_layer.h>
#include <kis_paint_layer.h>

KisPaletteModel::KisPaletteModel(QObject* parent)
    : QAbstractTableModel(parent),
      m_colorSet(0),
      m_displayRenderer(KoDumbColorDisplayRenderer::instance())
{
}

KisPaletteModel::~KisPaletteModel()
{
}

void KisPaletteModel::setDisplayRenderer(KoColorDisplayRendererInterface *displayRenderer)
{
    if (displayRenderer) {
        if (m_displayRenderer) {
            disconnect(m_displayRenderer, 0, this, 0);
        }
        m_displayRenderer = displayRenderer;
        connect(m_displayRenderer, SIGNAL(displayConfigurationChanged()),
                SLOT(slotDisplayConfigurationChanged()));
    } else {
        m_displayRenderer = KoDumbColorDisplayRenderer::instance();
    }
}

void KisPaletteModel::slotDisplayConfigurationChanged()
{
    reset();
}

QVariant KisPaletteModel::data(const QModelIndex& index, int role) const
{
    if (m_colorSet) {
        int i = index.row()*columnCount()+index.column();
        if (i < m_colorSet->nColors()) {
            switch (role) {
            case Qt::DisplayRole: {
                return m_colorSet->getColor(i).name;
            }
            case Qt::BackgroundRole: {
                QColor color = m_displayRenderer->toQColor(m_colorSet->getColor(i).color);
                return QBrush(color);
            }
            }
        }
    }
    return QVariant();
}

int KisPaletteModel::rowCount(const QModelIndex& /*parent*/) const
{
    if (!m_colorSet) {
        return 0;
    }
    if (m_colorSet->columnCount() > 0) {
        return m_colorSet->nColors()/m_colorSet->columnCount() + 1;
    }
    return m_colorSet->nColors()/15 + 1;
}

int KisPaletteModel::columnCount(const QModelIndex& /*parent*/) const
{
    if (m_colorSet && m_colorSet->columnCount() > 0) {
        return m_colorSet->columnCount();
    }
    return 15;
}

Qt::ItemFlags KisPaletteModel::flags(const QModelIndex& /*index*/) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    return flags;
}

QModelIndex KisPaletteModel::index(int row, int column, const QModelIndex& parent) const
{
    int index = row*columnCount()+column;
    if (m_colorSet && index < m_colorSet->nColors()) {
        return QAbstractTableModel::index(row, column, parent);
    }
    return QModelIndex();
}

void KisPaletteModel::setColorSet(KoColorSet* colorSet)
{
    m_colorSet = colorSet;
    reset();
}

KoColorSet* KisPaletteModel::colorSet() const
{
    return m_colorSet;
}

QModelIndex KisPaletteModel::indexFromId(int i) const
{
    const int width = columnCount();
    return width > 0 ? index(i / width, i & width) : QModelIndex();
}

int KisPaletteModel::idFromIndex(const QModelIndex &index) const
{
    return index.isValid() ? index.row() * columnCount() + index.column() : -1;
}
