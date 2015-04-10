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

#ifndef DatabaseThreadWinCE_h
#define DatabaseThreadWinCE_h

#include <wtf/Deque.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    class Database;
    class DatabaseTask;

    class DatabaseThread: public WTF::RefCounted<DatabaseThread> {

    public:
        static PassRefPtr<DatabaseThread> create() { return adoptRef(new DatabaseThread); }
        ~DatabaseThread();

        bool start() { return true; }
        void requestTermination();
        bool terminationRequested() const;

        void scheduleTask(PassRefPtr<DatabaseTask>);
        void scheduleImmediateTask(PassRefPtr<DatabaseTask>);
        void unscheduleDatabaseTasks(Database*);
        void recordDatabaseOpen(Database*);
        void recordDatabaseClosed(Database*);
#ifndef NDEBUG
        ThreadIdentifier getThreadID() const { return currentThread(); }
#endif

    private:
        DatabaseThread();

        void timerFired(Timer<DatabaseThread>*);

        Deque<RefPtr<DatabaseTask> > m_queue;
        Timer<DatabaseThread> m_timer;
    };

} // namespace WebCore

#endif // DatabaseThreadWinCE_h
