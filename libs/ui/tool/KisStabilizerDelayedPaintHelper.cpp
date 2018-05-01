/*
 *  Copyright (c) 2016 Alvin Wong <alvinhochun-at-gmail-com>
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

#include "KisStabilizerDelayedPaintHelper.h"

KisStabilizerDelayedPaintHelper::TimedPaintInfo::TimedPaintInfo(int elapsedTime, KisPaintInformation paintInfo)
    : elapsedTime(elapsedTime)
    , paintInfo(paintInfo)
{
}

KisStabilizerDelayedPaintHelper::KisStabilizerDelayedPaintHelper()
{
    connect(&m_paintTimer, SIGNAL(timeout()), SLOT(stabilizerDelayedPaint()));
}

void KisStabilizerDelayedPaintHelper::start(int paintInterval, const KisPaintInformation &firstPaintInfo) {
    if (running()) {
        cancel();
    }
    m_paintTimer.setInterval(paintInterval);
    m_paintTimer.start();
    m_elapsedTimer.start();
    m_lastPendingTime = m_elapsedTimer.elapsed();
    m_paintQueue.enqueue(TimedPaintInfo(m_lastPendingTime, firstPaintInfo));
}

void KisStabilizerDelayedPaintHelper::update(const QVector<KisPaintInformation> &newPaintInfos) {
    int now = m_elapsedTimer.elapsed();
    int delayedPaintInterval = m_elapsedTimer.elapsed() - m_lastPendingTime;
    for (int i = 0; i < newPaintInfos.size(); i++) {
        // TODO: Risk of overflowing?
        int offsetTime = (delayedPaintInterval * i) / newPaintInfos.size();
        m_paintQueue.enqueue(TimedPaintInfo(now + offsetTime, newPaintInfos[i]));
    }
    m_lastPendingTime = now;
}

void KisStabilizerDelayedPaintHelper::end() {
    stabilizerDelayedPaint(true);
    m_paintTimer.stop();
}

void KisStabilizerDelayedPaintHelper::cancel() {
    m_paintTimer.stop();
    m_paintQueue.clear();
}

void KisStabilizerDelayedPaintHelper::stabilizerDelayedPaint(bool isEndStroke) {
    if (m_paintQueue.isEmpty()) {
        return;
    }
    if (isEndStroke) {
        TimedPaintInfo dequeued = m_paintQueue.dequeue();
        while (!m_paintQueue.isEmpty()) {
            const TimedPaintInfo dequeued2 = m_paintQueue.dequeue();
            m_paintLine(dequeued.paintInfo, dequeued2.paintInfo);
            dequeued = dequeued2;
        }
        m_requestUpdateOutline();
        return;
    }
    int now = m_elapsedTimer.elapsed();
    // Always keep one in the queue since painting requires two points
    while (m_paintQueue.size() > 1 && m_paintQueue.head().elapsedTime <= now) {
        const TimedPaintInfo dequeued = m_paintQueue.dequeue();
        m_paintLine(dequeued.paintInfo, m_paintQueue.head().paintInfo);
    }
    m_requestUpdateOutline();
}
