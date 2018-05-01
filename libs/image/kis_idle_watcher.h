/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_IDLE_WATCHER_H
#define __KIS_IDLE_WATCHER_H

#include "kritaimage_export.h"

#include <QScopedPointer>
#include <QObject>
#include <QString>

#include "kis_types.h"


class KRITAIMAGE_EXPORT KisIdleWatcher : public QObject
{
    Q_OBJECT
public:
    KisIdleWatcher(int delay, QObject* parent = 0);
    ~KisIdleWatcher();

    bool isIdle() const;

    void setTrackedImages(const QVector<KisImageSP> &images);
    void setTrackedImage(KisImageSP image);

    //Force to image modified state and start countdown to event
    void startCountdown(void) { slotImageModified(); }

Q_SIGNALS:
    void startedIdleMode();

private Q_SLOTS:
    void slotImageModified();
    void slotIdleCheckTick();

    void startIdleCheck();
    void stopIdleCheck();

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_IDLE_WATCHER_H */
