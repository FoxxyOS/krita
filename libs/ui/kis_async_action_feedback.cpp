/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_async_action_feedback.h"

#include <QtConcurrent>
#include <QProgressDialog>


struct KisAsyncActionFeedback::Private
{
    QScopedPointer<QProgressDialog> progress;
};

KisAsyncActionFeedback::KisAsyncActionFeedback(const QString &message, QWidget *parent)
    : m_d(new Private)
{
    m_d->progress.reset(new QProgressDialog(message, "", 0, 0, parent));
    m_d->progress->setWindowModality(Qt::ApplicationModal);
    m_d->progress->setCancelButton(0);
    m_d->progress->setMinimumDuration(1000);
    m_d->progress->setValue(0);
}

KisAsyncActionFeedback::~KisAsyncActionFeedback()
{
}

bool KisAsyncActionFeedback::runAction(std::function<bool()> func)
{
    QFuture<bool> result = QtConcurrent::run(func);
    QFutureWatcher<bool> watcher;
    watcher.setFuture(result);

    while (watcher.isRunning()) {
        qApp->processEvents();
    }

    watcher.waitForFinished();
    return watcher.result();
}
