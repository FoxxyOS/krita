/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *                2004 Adrian Page     <adrian@pagenet.plus.com>
 *                2009 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_progress_widget.h"
#include <kis_debug.h>
#include <QToolButton>
#include <QHBoxLayout>
#include <QKeyEvent>

#include <kis_icon.h>

#include <KoProgressUpdater.h>
#include <KoProgressBar.h>

#include <kis_progress_updater.h>

KisProgressWidget::KisProgressWidget(QWidget* parent)
        : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    m_cancelButton = new QToolButton(this);
    m_cancelButton->setIcon(KisIconUtils::loadIcon("process-stop"));

    QSizePolicy sizePolicy = m_cancelButton->sizePolicy();
    sizePolicy.setVerticalPolicy(QSizePolicy::Ignored);
    m_cancelButton->setSizePolicy(sizePolicy);

    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));

    m_progressBar = new KoProgressBar(this);
    connect(m_progressBar, SIGNAL(valueChanged(int)), SLOT(correctVisibility(int)));
    layout->addWidget(m_progressBar);
    layout->addWidget(m_cancelButton);
    layout->setContentsMargins(0, 0, 0, 0);

    m_progressBar->setVisible(false);
    m_cancelButton->setVisible(false);

    setMaximumWidth(225);
    setMinimumWidth(225);
}

KisProgressWidget::~KisProgressWidget()
{
}

KoProgressProxy* KisProgressWidget::progressProxy()
{
    return m_progressBar;
}

void KisProgressWidget::cancel()
{
    Q_FOREACH (KoProgressUpdater* updater, m_activeUpdaters) {
        updater->cancel();
    }
    emit sigCancellationRequested();
}

void KisProgressWidget::correctVisibility(int progressValue)
{
    bool visibility = progressValue >= m_progressBar->minimum() &&
        progressValue < m_progressBar->maximum();

    m_progressBar->setVisible(visibility);
    m_cancelButton->setVisible(visibility);
}

void KisProgressWidget::detachUpdater(KoProgressUpdater* updater)
{
    m_activeUpdaters.removeOne(updater);
}

void KisProgressWidget::attachUpdater(KoProgressUpdater* updater)
{
    m_activeUpdaters << updater;
}

