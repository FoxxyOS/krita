/*
 *  Copyright (c) 2007-2008 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_filter_selector_widget.h"

#include <QHeaderView>
#include <QTreeView>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QLayout>

#include "ui_wdgfilterselector.h"

#include <kis_layer.h>
#include <kis_paint_device.h>
#include <filter/kis_filter.h>
#include <kis_config_widget.h>
#include <filter/kis_filter_configuration.h>
#include "kis_default_bounds.h"

// From krita/ui
#include "kis_bookmarked_configurations_editor.h"
#include "kis_bookmarked_filter_configurations_model.h"
#include "kis_filters_model.h"
#include "kis_config.h"

class ThumbnailBounds : public KisDefaultBounds {
public:
    ThumbnailBounds() : KisDefaultBounds() {}
    ~ThumbnailBounds() override {}

    QRect bounds() const override
    {
        return QRect(0, 0, 100, 100);
    }
private:
    Q_DISABLE_COPY(ThumbnailBounds)
};


struct KisFilterSelectorWidget::Private {
    QWidget* currentCentralWidget;
    KisConfigWidget* currentFilterConfigurationWidget;
    KisFilterSP currentFilter;
    KisPaintDeviceSP paintDevice;
    Ui_FilterSelector uiFilterSelector;
    KisPaintDeviceSP thumb;
    KisBookmarkedFilterConfigurationsModel* currentBookmarkedFilterConfigurationsModel;
    KisFiltersModel* filtersModel;
    QGridLayout *widgetLayout;
    KisViewManager *view;
    bool showFilterGallery;
};

KisFilterSelectorWidget::KisFilterSelectorWidget(QWidget* parent)
    : d(new Private)
{
    Q_UNUSED(parent);
    setObjectName("KisFilterSelectorWidget");
    d->currentCentralWidget = 0;
    d->currentFilterConfigurationWidget = 0;
    d->currentBookmarkedFilterConfigurationsModel = 0;
    d->currentFilter = 0;
    d->filtersModel = 0;
    d->view = 0;
    d->showFilterGallery = true;
    d->uiFilterSelector.setupUi(this);

    d->widgetLayout = new QGridLayout(d->uiFilterSelector.centralWidgetHolder);
    d->widgetLayout->setContentsMargins(0,0,0,0);
    d->widgetLayout->setHorizontalSpacing(0);

    showFilterGallery(false);

    connect(d->uiFilterSelector.filtersSelector, SIGNAL(clicked(const QModelIndex&)), SLOT(setFilterIndex(const QModelIndex &)));
    connect(d->uiFilterSelector.filtersSelector, SIGNAL(activated(const QModelIndex&)), SLOT(setFilterIndex(const QModelIndex &)));

    connect(d->uiFilterSelector.comboBoxPresets, SIGNAL(activated(int)),
            SLOT(slotBookmarkedFilterConfigurationSelected(int)));
    connect(d->uiFilterSelector.pushButtonEditPressets, SIGNAL(pressed()), SLOT(editConfigurations()));
}

KisFilterSelectorWidget::~KisFilterSelectorWidget()
{
    delete d->filtersModel;
    delete d->currentBookmarkedFilterConfigurationsModel;
    delete d->currentCentralWidget;
    delete d->widgetLayout;
    delete d;
}

void KisFilterSelectorWidget::setView(KisViewManager *view)
{
    d->view = view;
}

void KisFilterSelectorWidget::setPaintDevice(bool showAll, KisPaintDeviceSP _paintDevice)
{
    if (!_paintDevice) return;

    if (d->filtersModel) delete d->filtersModel;

    d->paintDevice = _paintDevice;
    d->thumb = d->paintDevice->createThumbnailDevice(100, 100);
    d->thumb->setDefaultBounds(new ThumbnailBounds());
    d->filtersModel = new KisFiltersModel(showAll, d->thumb);

    d->uiFilterSelector.filtersSelector->setFilterModel(d->filtersModel);
    d->uiFilterSelector.filtersSelector->header()->setVisible(false);

    KisConfig cfg;
    QModelIndex idx = d->filtersModel->indexForFilter(cfg.readEntry<QString>("FilterSelector/LastUsedFilter", "levels"));

    if (!idx.isValid()) {
        idx = d->filtersModel->indexForFilter("levels");
    }

    if (isFilterGalleryVisible()) {
        d->uiFilterSelector.filtersSelector->activateFilter(idx);
    }

}

void KisFilterSelectorWidget::showFilterGallery(bool visible)
{
    if (d->showFilterGallery == visible) {
        return;
    }

    d->showFilterGallery = visible;
    update();
    emit sigFilterGalleryToggled(visible);
    emit sigSizeChanged();
}

bool KisFilterSelectorWidget::isFilterGalleryVisible() const
{
    return d->showFilterGallery;
}

KisFilterSP KisFilterSelectorWidget::currentFilter() const
{
    return d->currentFilter;
}

void KisFilterSelectorWidget::setFilter(KisFilterSP f)
{
    Q_ASSERT(f);
    Q_ASSERT(d->filtersModel);
    setWindowTitle(f->name());
    dbgKrita << "setFilter: " << f;
    d->currentFilter = f;
    delete d->currentCentralWidget;

    {
        bool v = d->uiFilterSelector.filtersSelector->blockSignals(true);
        d->uiFilterSelector.filtersSelector->setCurrentIndex(d->filtersModel->indexForFilter(f->id()));
        d->uiFilterSelector.filtersSelector->blockSignals(v);
    }

    KisConfigWidget* widget =
        d->currentFilter->createConfigurationWidget(d->uiFilterSelector.centralWidgetHolder, d->paintDevice);

    if (!widget) { // No widget, so display a label instead
        d->uiFilterSelector.comboBoxPresets->setEnabled(false);
        d->uiFilterSelector.pushButtonEditPressets->setEnabled(false);

        d->currentFilterConfigurationWidget = 0;
        d->currentCentralWidget = new QLabel(i18n("No configuration options"),
                                             d->uiFilterSelector.centralWidgetHolder);
        d->uiFilterSelector.scrollArea->setMinimumSize(d->currentCentralWidget->sizeHint());
        qobject_cast<QLabel*>(d->currentCentralWidget)->setAlignment(Qt::AlignCenter);
    } else {
        d->uiFilterSelector.comboBoxPresets->setEnabled(true);
        d->uiFilterSelector.pushButtonEditPressets->setEnabled(true);

        d->currentFilterConfigurationWidget = widget;
        d->currentCentralWidget = widget;
        widget->layout()->setContentsMargins(0,0,0,0);
        d->currentFilterConfigurationWidget->setView(d->view);
        d->currentFilterConfigurationWidget->blockSignals(true);
        d->currentFilterConfigurationWidget->setConfiguration(d->currentFilter->defaultConfiguration());
        d->currentFilterConfigurationWidget->blockSignals(false);
        d->uiFilterSelector.scrollArea->setContentsMargins(0,0,0,0);
        d->uiFilterSelector.scrollArea->setMinimumWidth(widget->sizeHint().width() + 18);
        connect(d->currentFilterConfigurationWidget, SIGNAL(sigConfigurationUpdated()), this, SIGNAL(configurationChanged()));
    }

    // Change the list of presets
    delete d->currentBookmarkedFilterConfigurationsModel;
    d->currentBookmarkedFilterConfigurationsModel = new KisBookmarkedFilterConfigurationsModel(d->thumb, f);
    d->uiFilterSelector.comboBoxPresets->setModel(d->currentBookmarkedFilterConfigurationsModel);

    // Add the widget to the layout
    d->currentCentralWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    d->widgetLayout->addWidget(d->currentCentralWidget, 0 , 0);

    update();
}

void KisFilterSelectorWidget::setFilterIndex(const QModelIndex& idx)
{
    if (!idx.isValid()) return;

    Q_ASSERT(d->filtersModel);
    KisFilter* filter = const_cast<KisFilter*>(d->filtersModel->indexToFilter(idx));
    if (filter) {
        setFilter(filter);
    }
    else {
        if (d->currentFilter) {
            bool v = d->uiFilterSelector.filtersSelector->blockSignals(true);
            QModelIndex idx = d->filtersModel->indexForFilter(d->currentFilter->id());
            d->uiFilterSelector.filtersSelector->setCurrentIndex(idx);
            d->uiFilterSelector.filtersSelector->scrollTo(idx);
            d->uiFilterSelector.filtersSelector->blockSignals(v);
        }
    }

    KisConfig cfg;
    cfg.writeEntry<QString>("FilterSelector/LastUsedFilter", d->currentFilter->id());
    emit(configurationChanged());
}

void KisFilterSelectorWidget::slotBookmarkedFilterConfigurationSelected(int index)
{
    if (d->currentFilterConfigurationWidget) {
        QModelIndex modelIndex = d->currentBookmarkedFilterConfigurationsModel->index(index, 0);
        KisFilterConfigurationSP config  = d->currentBookmarkedFilterConfigurationsModel->configuration(modelIndex);
        d->currentFilterConfigurationWidget->setConfiguration(config);
    }
}

void KisFilterSelectorWidget::editConfigurations()
{
    KisSerializableConfigurationSP config =
        d->currentFilterConfigurationWidget ? d->currentFilterConfigurationWidget->configuration() : 0;
    KisBookmarkedConfigurationsEditor editor(this, d->currentBookmarkedFilterConfigurationsModel, config);
    editor.exec();
}

void KisFilterSelectorWidget::update()
{
    d->uiFilterSelector.filtersSelector->setVisible(d->showFilterGallery);
    if (d->showFilterGallery) {
        setMinimumWidth(qMax(sizeHint().width(), 700));
        d->uiFilterSelector.scrollArea->setMinimumHeight(400);
        setMinimumHeight(d->uiFilterSelector.widget->sizeHint().height());
        if (d->currentFilter) {
            bool v = d->uiFilterSelector.filtersSelector->blockSignals(true);
            d->uiFilterSelector.filtersSelector->setCurrentIndex(d->filtersModel->indexForFilter(d->currentFilter->id()));
            d->uiFilterSelector.filtersSelector->blockSignals(v);
        }
    }
    else {
        if (d->currentCentralWidget) {
            d->uiFilterSelector.scrollArea->setMinimumHeight(qMin(400, d->currentCentralWidget->sizeHint().height()));
        }
        setMinimumSize(d->uiFilterSelector.widget->sizeHint());
    }
}

KisFilterConfigurationSP KisFilterSelectorWidget::configuration()
{
    if (d->currentFilterConfigurationWidget) {
        KisFilterConfigurationSP config = dynamic_cast<KisFilterConfiguration*>(d->currentFilterConfigurationWidget->configuration().data());
        if (config) {
            return config;
        }
    } else if (d->currentFilter) {
        return d->currentFilter->defaultConfiguration();
    }
    return 0;

}

void KisFilterTree::setFilterModel(QAbstractItemModel *model)
{
    m_model = model;

}

void KisFilterTree::activateFilter(QModelIndex idx)
{
    setModel(m_model);
    selectionModel()->select(idx, QItemSelectionModel::SelectCurrent);
    expand(idx);
    scrollTo(idx);
    emit activated(idx);
}

void KisFilterSelectorWidget::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible) {
        update();
    }
}

