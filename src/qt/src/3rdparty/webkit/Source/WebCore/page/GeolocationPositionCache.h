/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GeolocationPositionCache_h
#define GeolocationPositionCache_h

#include "PlatformString.h"
#include "ScriptExecutionContext.h"

#include <wtf/Forward.h>
#include <wtf/MessageQueue.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Geoposition;

// Maintains a cached position for Geolocation. Takes care of writing and
// reading the position to a database on a background thread. The Geolocation
// spec does not require a cached position to be maintained, so we take a
// best-effort approach where we do not wait for database reads or writes.
class GeolocationPositionCache {
public:
    static GeolocationPositionCache* instance();

    void addUser();
    void removeUser();

    void setDatabasePath(const String&);
    void setCachedPosition(Geoposition*);
    Geoposition* cachedPosition();

private:
    GeolocationPositionCache();

    void startBackgroundThread();
    static void* threadEntryPoint(void* object);
    void threadEntryPointImpl();

    void triggerReadFromDatabase();
    static void readFromDatabase(ScriptExecutionContext*, GeolocationPositionCache*);
    void readFromDatabaseImpl();
    void triggerWriteToDatabase();
    static void writeToDatabase(ScriptExecutionContext*, GeolocationPositionCache*);
    void writeToDatabaseImpl();

    RefPtr<Geoposition> m_cachedPosition;
    Mutex m_cachedPositionMutex;
    ThreadIdentifier m_threadId;
    MessageQueue<ScriptExecutionContext::Task> m_queue;
    String m_databaseFile;
    Mutex m_databaseFileMutex;
};

} // namespace WebCore

#endif // GeolocationPositionCache_h
