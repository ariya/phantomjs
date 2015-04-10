/*
 * Copyright (C) 2009 Torch Mobile, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  This library is distributed in the hope that i will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "StorageThread.h"

#include "StorageTask.h"
#include "StorageAreaSync.h"

namespace WebCore {

StorageThread::StorageThread()
: m_timer(this, &StorageThread::timerFired)
{
}

StorageThread::~StorageThread()
{
}

bool StorageThread::start()
{
    return true;
}

void StorageThread::timerFired(Timer<StorageThread>*)
{
    if (!m_queue.isEmpty()) {
        RefPtr<StorageTask> task = m_queue.first();
        task->performTask();
        m_queue.removeFirst();
        if (!m_queue.isEmpty())
            m_timer.startOneShot(0);
    }
}

void StorageThread::scheduleImport(PassRefPtr<StorageAreaSync> area)
{
    m_queue.append(StorageTask::createImport(area));
    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

void StorageThread::scheduleSync(PassRefPtr<StorageAreaSync> area)
{
    m_queue.append(StorageTask::createSync(area));
    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

void StorageThread::terminate()
{
    m_queue.clear();
    m_timer.stop();
}

void StorageThread::performTerminate()
{
    m_queue.clear();
    m_timer.stop();
}

} // namespace WebCore
