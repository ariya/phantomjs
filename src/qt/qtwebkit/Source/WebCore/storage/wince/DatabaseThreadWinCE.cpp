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
#include "DatabaseThread.h"

#include "Database.h"
#include "DatabaseTask.h"

namespace WebCore {

DatabaseThread::DatabaseThread()
: m_timer(this, &DatabaseThread::timerFired)
{
}

DatabaseThread::~DatabaseThread()
{
}

void DatabaseThread::requestTermination()
{
    m_queue.clear();
}

bool DatabaseThread::terminationRequested() const
{
    return m_queue.isEmpty();
}

void DatabaseThread::timerFired(Timer<DatabaseThread>*)
{
    if (!m_queue.isEmpty()) {
        RefPtr<DatabaseTask> task = m_queue.first();
        task->performTask();
        m_queue.removeFirst();
        if (!m_queue.isEmpty())
            m_timer.startOneShot(0);
    }
}

void DatabaseThread::scheduleTask(PassRefPtr<DatabaseTask> task)
{
    m_queue.append(task);
    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

void DatabaseThread::scheduleImmediateTask(PassRefPtr<DatabaseTask> task)
{
    task->performTask();
}

void DatabaseThread::unscheduleDatabaseTasks(Database* database)
{
    Deque<RefPtr<DatabaseTask> > reservedTasks;
    for (Deque<RefPtr<DatabaseTask> >::const_iterator i = m_queue.begin(); i != m_queue.end(); ++i) {
        if ((*i)->database() != database)
            reservedTasks.append(*i);
    }

    m_queue.swap(reservedTasks);
}

void DatabaseThread::recordDatabaseOpen(Database* database)
{
    notImplemented();
}

void DatabaseThread::recordDatabaseClosed(Database* database)
{
    notImplemented();
}

} // namespace WebCore
