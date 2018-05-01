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

#include "kis_simple_stroke_strategy.h"


/***************************************************************/
/*         private class: SimpleStrokeJobStrategy              */
/***************************************************************/

class SimpleStrokeJobStrategy : public KisStrokeJobStrategy
{
public:
    SimpleStrokeJobStrategy(KisSimpleStrokeStrategy::JobType type,
                            KisSimpleStrokeStrategy *parentStroke)
        : m_type(type),
          m_parentStroke(parentStroke)
    {
    }

    void run(KisStrokeJobData *data) override {
        switch(m_type) {
        case KisSimpleStrokeStrategy::JOB_INIT:
            Q_UNUSED(data);
            m_parentStroke->initStrokeCallback();
            break;
        case KisSimpleStrokeStrategy::JOB_FINISH:
            Q_UNUSED(data);
            m_parentStroke->finishStrokeCallback();
            break;
        case KisSimpleStrokeStrategy::JOB_CANCEL:
            Q_UNUSED(data);
            m_parentStroke->cancelStrokeCallback();
            break;
        case KisSimpleStrokeStrategy::JOB_DOSTROKE:
            m_parentStroke->doStrokeCallback(data);
            break;
        case KisSimpleStrokeStrategy::JOB_SUSPEND:
            m_parentStroke->suspendStrokeCallback();
            break;
        case KisSimpleStrokeStrategy::JOB_RESUME:
            m_parentStroke->resumeStrokeCallback();
            break;
        default:
            break;
        }
    }

private:
    KisSimpleStrokeStrategy::JobType m_type;
    KisSimpleStrokeStrategy *m_parentStroke;
};


/***************************************************************/
/*                 KisSimpleStrokeStrategy                     */
/***************************************************************/

KisSimpleStrokeStrategy::KisSimpleStrokeStrategy(QString id, const KUndo2MagicString &name)
    : KisStrokeStrategy(id, name),
      m_jobEnabled(NJOBS, false),
      m_jobSequentiality(NJOBS, KisStrokeJobData::SEQUENTIAL),
      m_jobExclusivity(NJOBS, KisStrokeJobData::NORMAL)
{
}

KisSimpleStrokeStrategy::KisSimpleStrokeStrategy(const KisSimpleStrokeStrategy &rhs)
    : KisStrokeStrategy(rhs),
      m_jobEnabled(rhs.m_jobEnabled),
      m_jobSequentiality(rhs.m_jobSequentiality),
      m_jobExclusivity(rhs.m_jobExclusivity)
{
}

void KisSimpleStrokeStrategy::enableJob(JobType type, bool enable,
                                        KisStrokeJobData::Sequentiality sequentiality,
                                        KisStrokeJobData::Exclusivity exclusivity)
{
    m_jobEnabled[(int)type] = enable;
    m_jobSequentiality[(int)type] = sequentiality;
    m_jobExclusivity[(int)type] = exclusivity;
}

KisStrokeJobStrategy*
KisSimpleStrokeStrategy::createStrategy(JobType type)
{
    KisStrokeJobStrategy *strategy = 0;

    if(m_jobEnabled[(int)type]) {
        strategy = new SimpleStrokeJobStrategy(type, this);
    }

    return strategy;
}

KisStrokeJobStrategy* KisSimpleStrokeStrategy::createInitStrategy()
{
    return createStrategy(JOB_INIT);
}

KisStrokeJobStrategy* KisSimpleStrokeStrategy::createFinishStrategy()
{
    return createStrategy(JOB_FINISH);
}

KisStrokeJobStrategy* KisSimpleStrokeStrategy::createCancelStrategy()
{
    return createStrategy(JOB_CANCEL);
}

KisStrokeJobStrategy* KisSimpleStrokeStrategy::createDabStrategy()
{
    return createStrategy(JOB_DOSTROKE);
}

KisStrokeJobStrategy* KisSimpleStrokeStrategy::createSuspendStrategy()
{
    return createStrategy(JOB_SUSPEND);
}

KisStrokeJobStrategy* KisSimpleStrokeStrategy::createResumeStrategy()
{
    return createStrategy(JOB_RESUME);
}

KisStrokeJobData* KisSimpleStrokeStrategy::createData(JobType type)
{
    KisStrokeJobData::Sequentiality sequentiality = m_jobSequentiality[(int)type];
    KisStrokeJobData::Exclusivity exclusivity = m_jobExclusivity[(int)type];

    return new KisStrokeJobData(sequentiality, exclusivity);
}

KisStrokeJobData* KisSimpleStrokeStrategy::createInitData()
{
    return createData(JOB_INIT);
}

KisStrokeJobData* KisSimpleStrokeStrategy::createFinishData()
{
    return createData(JOB_FINISH);
}

KisStrokeJobData* KisSimpleStrokeStrategy::createCancelData()
{
    return createData(JOB_CANCEL);
}

KisStrokeJobData* KisSimpleStrokeStrategy::createSuspendData()
{
    return createData(JOB_SUSPEND);
}

KisStrokeJobData* KisSimpleStrokeStrategy::createResumeData()
{
    return createData(JOB_RESUME);
}

void KisSimpleStrokeStrategy::initStrokeCallback()
{
}

void KisSimpleStrokeStrategy::finishStrokeCallback()
{
}

void KisSimpleStrokeStrategy::cancelStrokeCallback()
{
}

void KisSimpleStrokeStrategy::doStrokeCallback(KisStrokeJobData *data)
{
    Q_UNUSED(data);
}

void KisSimpleStrokeStrategy::suspendStrokeCallback()
{
}

void KisSimpleStrokeStrategy::resumeStrokeCallback()
{
}
