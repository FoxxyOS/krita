/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_update_scheduler.h"

#include "klocalizedstring.h"
#include "kis_image_config.h"
#include "kis_merge_walker.h"
#include "kis_full_refresh_walker.h"

#include "kis_updater_context.h"
#include "kis_simple_update_queue.h"
#include "kis_strokes_queue.h"

#include "kis_queues_progress_updater.h"

#include <QReadWriteLock>
#include "kis_lazy_wait_condition.h"

//#define DEBUG_BALANCING

#ifdef DEBUG_BALANCING
#define DEBUG_BALANCING_METRICS(decidedFirst, excl)                     \
    dbgKrita << "Balance decision:" << decidedFirst                     \
    << "(" << excl << ")"                                               \
    << "updates:" << m_d->updatesQueue.sizeMetric()                    \
    << "strokes:" << m_d->strokesQueue.sizeMetric()
#else
#define DEBUG_BALANCING_METRICS(decidedFirst, excl)
#endif


struct Q_DECL_HIDDEN KisUpdateScheduler::Private {
    Private(KisUpdateScheduler *_q, KisProjectionUpdateListener *p)
        : q(_q)
        , projectionUpdateListener(p)
    {}

    KisUpdateScheduler *q;

    KisSimpleUpdateQueue updatesQueue;
    KisStrokesQueue strokesQueue;
    KisUpdaterContext updaterContext;
    bool processingBlocked = false;
    qreal balancingRatio = 1.0; // updates-queue-size/strokes-queue-size
    KisProjectionUpdateListener *projectionUpdateListener;
    KisQueuesProgressUpdater *progressUpdater = 0;

    QAtomicInt updatesLockCounter;
    QReadWriteLock updatesStartLock;
    KisLazyWaitCondition updatesFinishedCondition;
};

KisUpdateScheduler::KisUpdateScheduler(KisProjectionUpdateListener *projectionUpdateListener)
    : m_d(new Private(this, projectionUpdateListener))
{
    updateSettings();
    connectSignals();
}

KisUpdateScheduler::KisUpdateScheduler()
    : m_d(new Private(this, 0))
{
}

KisUpdateScheduler::~KisUpdateScheduler()
{
    delete m_d->progressUpdater;
    delete m_d;
}

void KisUpdateScheduler::connectSignals()
{
    connect(&m_d->updaterContext, SIGNAL(sigContinueUpdate(const QRect&)),
            SLOT(continueUpdate(const QRect&)),
            Qt::DirectConnection);

    connect(&m_d->updaterContext, SIGNAL(sigDoSomeUsefulWork()),
            SLOT(doSomeUsefulWork()), Qt::DirectConnection);

    connect(&m_d->updaterContext, SIGNAL(sigSpareThreadAppeared()),
            SLOT(spareThreadAppeared()), Qt::DirectConnection);
}

void KisUpdateScheduler::setProgressProxy(KoProgressProxy *progressProxy)
{
    delete m_d->progressUpdater;
    m_d->progressUpdater = progressProxy ?
        new KisQueuesProgressUpdater(progressProxy) : 0;
}

void KisUpdateScheduler::progressUpdate()
{
    if (!m_d->progressUpdater) return;

    if(!m_d->strokesQueue.hasOpenedStrokes()) {
        QString jobName = m_d->strokesQueue.currentStrokeName().toString();
        if(jobName.isEmpty()) {
            jobName = i18n("Updating...");
        }

        int sizeMetric = m_d->strokesQueue.sizeMetric();
        if (!sizeMetric) {
            sizeMetric = m_d->updatesQueue.sizeMetric();
        }

        m_d->progressUpdater->updateProgress(sizeMetric, jobName);
    }
    else {
        m_d->progressUpdater->hide();
    }
}

void KisUpdateScheduler::updateProjection(KisNodeSP node, const QRect& rc, const QRect &cropRect)
{
    m_d->updatesQueue.addUpdateJob(node, rc, cropRect, currentLevelOfDetail());
    processQueues();
}

void KisUpdateScheduler::updateProjectionNoFilthy(KisNodeSP node, const QRect& rc, const QRect &cropRect)
{
    m_d->updatesQueue.addUpdateNoFilthyJob(node, rc, cropRect, currentLevelOfDetail());
    processQueues();
}

void KisUpdateScheduler::fullRefreshAsync(KisNodeSP root, const QRect& rc, const QRect &cropRect)
{
    m_d->updatesQueue.addFullRefreshJob(root, rc, cropRect, currentLevelOfDetail());
    processQueues();
}

void KisUpdateScheduler::fullRefresh(KisNodeSP root, const QRect& rc, const QRect &cropRect)
{
    KisBaseRectsWalkerSP walker = new KisFullRefreshWalker(cropRect);
    walker->collectRects(root, rc);

    bool needLock = true;

    if(m_d->processingBlocked) {
        warnImage << "WARNING: Calling synchronous fullRefresh under a scheduler lock held";
        warnImage << "We will not assert for now, but please port caller's to strokes";
        warnImage << "to avoid this warning";
        needLock = false;
    }

    if(needLock) lock();
    m_d->updaterContext.lock();

    Q_ASSERT(m_d->updaterContext.isJobAllowed(walker));
    m_d->updaterContext.addMergeJob(walker);
    m_d->updaterContext.waitForDone();

    m_d->updaterContext.unlock();
    if(needLock) unlock(true);
}

void KisUpdateScheduler::addSpontaneousJob(KisSpontaneousJob *spontaneousJob)
{
    m_d->updatesQueue.addSpontaneousJob(spontaneousJob);
    processQueues();
}

KisStrokeId KisUpdateScheduler::startStroke(KisStrokeStrategy *strokeStrategy)
{
    KisStrokeId id  = m_d->strokesQueue.startStroke(strokeStrategy);
    processQueues();
    return id;
}

void KisUpdateScheduler::addJob(KisStrokeId id, KisStrokeJobData *data)
{
    m_d->strokesQueue.addJob(id, data);
    processQueues();
}

void KisUpdateScheduler::endStroke(KisStrokeId id)
{
    m_d->strokesQueue.endStroke(id);
    processQueues();
}

bool KisUpdateScheduler::cancelStroke(KisStrokeId id)
{
    bool result = m_d->strokesQueue.cancelStroke(id);
    processQueues();
    return result;
}

bool KisUpdateScheduler::tryCancelCurrentStrokeAsync()
{
    return m_d->strokesQueue.tryCancelCurrentStrokeAsync();
}

UndoResult KisUpdateScheduler::tryUndoLastStrokeAsync()
{
    return m_d->strokesQueue.tryUndoLastStrokeAsync();
}

bool KisUpdateScheduler::wrapAroundModeSupported() const
{
    return m_d->strokesQueue.wrapAroundModeSupported();
}

void KisUpdateScheduler::setDesiredLevelOfDetail(int lod)
{
    m_d->strokesQueue.setDesiredLevelOfDetail(lod);

    /**
     * The queue might have started an internal stroke for
     * cache synchronization. Process the queues to execute
     * it if needed.
     */
    processQueues();
}

void KisUpdateScheduler::explicitRegenerateLevelOfDetail()
{
    m_d->strokesQueue.explicitRegenerateLevelOfDetail();

    // \see a comment in setDesiredLevelOfDetail()
    processQueues();
}

int KisUpdateScheduler::currentLevelOfDetail() const
{
    int levelOfDetail = -1;

    if (levelOfDetail < 0) {
        levelOfDetail = m_d->updaterContext.currentLevelOfDetail();
    }

    if (levelOfDetail < 0) {
        levelOfDetail = m_d->updatesQueue.overrideLevelOfDetail();
    }

    if (levelOfDetail < 0) {
        levelOfDetail = 0;
    }

    return levelOfDetail;
}

void KisUpdateScheduler::setLod0ToNStrokeStrategyFactory(const KisLodSyncStrokeStrategyFactory &factory)
{
    m_d->strokesQueue.setLod0ToNStrokeStrategyFactory(factory);
}

void KisUpdateScheduler::setSuspendUpdatesStrokeStrategyFactory(const KisSuspendResumeStrategyFactory &factory)
{
    m_d->strokesQueue.setSuspendUpdatesStrokeStrategyFactory(factory);
}

void KisUpdateScheduler::setResumeUpdatesStrokeStrategyFactory(const KisSuspendResumeStrategyFactory &factory)
{
    m_d->strokesQueue.setResumeUpdatesStrokeStrategyFactory(factory);
}

KisPostExecutionUndoAdapter *KisUpdateScheduler::lodNPostExecutionUndoAdapter() const
{
    return m_d->strokesQueue.lodNPostExecutionUndoAdapter();
}

void KisUpdateScheduler::updateSettings()
{
    m_d->updatesQueue.updateSettings();

    KisImageConfig config;
    m_d->balancingRatio = config.schedulerBalancingRatio();
}

void KisUpdateScheduler::lock()
{
    m_d->processingBlocked = true;
    m_d->updaterContext.waitForDone();
}

void KisUpdateScheduler::unlock(bool resetLodLevels)
{
    if (resetLodLevels) {
        /**
         * Legacy strokes may have changed the image while we didn't
         * control it. Notify the queue to take it into account.
         */
        m_d->strokesQueue.notifyUFOChangedImage();
    }

    m_d->processingBlocked = false;
    processQueues();
}

bool KisUpdateScheduler::isIdle()
{
    bool result = false;

    if (tryBarrierLock()) {
        result = true;
        unlock(false);
    }

    return result;
}

void KisUpdateScheduler::waitForDone()
{
    do {
        processQueues();
        m_d->updaterContext.waitForDone();
    } while(!m_d->updatesQueue.isEmpty() || !m_d->strokesQueue.isEmpty());
}

bool KisUpdateScheduler::tryBarrierLock()
{
    if(!m_d->updatesQueue.isEmpty() || !m_d->strokesQueue.isEmpty()) {
        return false;
    }

    m_d->processingBlocked = true;
    m_d->updaterContext.waitForDone();
    if(!m_d->updatesQueue.isEmpty() || !m_d->strokesQueue.isEmpty()) {
        m_d->processingBlocked = false;
        processQueues();
        return false;
    }

    return true;
}

void KisUpdateScheduler::barrierLock()
{
    do {
        m_d->processingBlocked = false;
        processQueues();
        m_d->processingBlocked = true;
        m_d->updaterContext.waitForDone();
    } while(!m_d->updatesQueue.isEmpty() || !m_d->strokesQueue.isEmpty());
}

void KisUpdateScheduler::processQueues()
{
    wakeUpWaitingThreads();

    if(m_d->processingBlocked) return;

    if(m_d->strokesQueue.needsExclusiveAccess()) {
        DEBUG_BALANCING_METRICS("STROKES", "X");
        m_d->strokesQueue.processQueue(m_d->updaterContext,
                                        !m_d->updatesQueue.isEmpty());

        if(!m_d->strokesQueue.needsExclusiveAccess()) {
            tryProcessUpdatesQueue();
        }
    }
    else if(m_d->balancingRatio * m_d->strokesQueue.sizeMetric() > m_d->updatesQueue.sizeMetric()) {
        DEBUG_BALANCING_METRICS("STROKES", "N");
        m_d->strokesQueue.processQueue(m_d->updaterContext,
                                        !m_d->updatesQueue.isEmpty());
        tryProcessUpdatesQueue();
    }
    else {
        DEBUG_BALANCING_METRICS("UPDATES", "N");
        tryProcessUpdatesQueue();
        m_d->strokesQueue.processQueue(m_d->updaterContext,
                                        !m_d->updatesQueue.isEmpty());

    }

    progressUpdate();
}

void KisUpdateScheduler::blockUpdates()
{
    m_d->updatesFinishedCondition.initWaiting();

    m_d->updatesLockCounter.ref();
    while(haveUpdatesRunning()) {
        m_d->updatesFinishedCondition.wait();
    }

    m_d->updatesFinishedCondition.endWaiting();
}

void KisUpdateScheduler::unblockUpdates()
{
    m_d->updatesLockCounter.deref();
    processQueues();
}

void KisUpdateScheduler::wakeUpWaitingThreads()
{
    if(m_d->updatesLockCounter && !haveUpdatesRunning()) {
        m_d->updatesFinishedCondition.wakeAll();
    }
}

void KisUpdateScheduler::tryProcessUpdatesQueue()
{
    QReadLocker locker(&m_d->updatesStartLock);
    if(m_d->updatesLockCounter) return;

    m_d->updatesQueue.processQueue(m_d->updaterContext);
}

bool KisUpdateScheduler::haveUpdatesRunning()
{
    QWriteLocker locker(&m_d->updatesStartLock);

    qint32 numMergeJobs, numStrokeJobs;
    m_d->updaterContext.getJobsSnapshot(numMergeJobs, numStrokeJobs);

    return numMergeJobs;
}

void KisUpdateScheduler::continueUpdate(const QRect &rect)
{
    Q_ASSERT(m_d->projectionUpdateListener);
    m_d->projectionUpdateListener->notifyProjectionUpdated(rect);
}

void KisUpdateScheduler::doSomeUsefulWork()
{
    m_d->updatesQueue.optimize();
}

void KisUpdateScheduler::spareThreadAppeared()
{
    processQueues();
}

KisTestableUpdateScheduler::KisTestableUpdateScheduler(KisProjectionUpdateListener *projectionUpdateListener,
                                                       qint32 threadCount)
{
    Q_UNUSED(threadCount);
    updateSettings();
    m_d->projectionUpdateListener = projectionUpdateListener;

    // The queue will update settings in a constructor itself
    // m_d->updatesQueue = new KisTestableSimpleUpdateQueue();
    // m_d->strokesQueue = new KisStrokesQueue();
    // m_d->updaterContext = new KisTestableUpdaterContext(threadCount);

    connectSignals();
}

KisTestableUpdaterContext* KisTestableUpdateScheduler::updaterContext()
{
    return dynamic_cast<KisTestableUpdaterContext*>(&m_d->updaterContext);
}

KisTestableSimpleUpdateQueue* KisTestableUpdateScheduler::updateQueue()
{
    return dynamic_cast<KisTestableSimpleUpdateQueue*>(&m_d->updatesQueue);
}
