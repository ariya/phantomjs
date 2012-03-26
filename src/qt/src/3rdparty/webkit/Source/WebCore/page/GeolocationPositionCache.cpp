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

#include "config.h"
#include "GeolocationPositionCache.h"

#if ENABLE(GEOLOCATION)

#include "CrossThreadTask.h"
#include "Geoposition.h"
#include "SQLValue.h"
#include "SQLiteDatabase.h"
#include "SQLiteFileSystem.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Threading.h>

using namespace WTF;

namespace WebCore {

static int numUsers = 0;

GeolocationPositionCache* GeolocationPositionCache::instance()
{
    DEFINE_STATIC_LOCAL(GeolocationPositionCache*, instance, (0));
    if (!instance)
        instance = new GeolocationPositionCache();
    return instance;
}

GeolocationPositionCache::GeolocationPositionCache()
    : m_threadId(0)
{
}

void GeolocationPositionCache::addUser()
{
    ASSERT(numUsers >= 0);
    MutexLocker databaseLock(m_databaseFileMutex);
    if (!numUsers && !m_threadId && !m_databaseFile.isNull()) {
        startBackgroundThread();
        MutexLocker lock(m_cachedPositionMutex);
        if (!m_cachedPosition)
            triggerReadFromDatabase();
    }
    ++numUsers;
}

void GeolocationPositionCache::removeUser()
{
    MutexLocker lock(m_cachedPositionMutex);
    --numUsers;
    ASSERT(numUsers >= 0);
    if (!numUsers && m_cachedPosition && m_threadId)
        triggerWriteToDatabase();
}

void GeolocationPositionCache::setDatabasePath(const String& path)
{
    static const char* databaseName = "CachedGeoposition.db";
    String newFile = SQLiteFileSystem::appendDatabaseFileNameToPath(path, databaseName);
    MutexLocker lock(m_databaseFileMutex);
    if (m_databaseFile != newFile) {
        m_databaseFile = newFile;
        if (numUsers && !m_threadId) {
            startBackgroundThread();
            if (!m_cachedPosition)
                triggerReadFromDatabase();
        }
    }
}

void GeolocationPositionCache::setCachedPosition(Geoposition* cachedPosition)
{
    MutexLocker lock(m_cachedPositionMutex);
    m_cachedPosition = cachedPosition;
}

Geoposition* GeolocationPositionCache::cachedPosition()
{
    MutexLocker lock(m_cachedPositionMutex);
    return m_cachedPosition.get();
}

void GeolocationPositionCache::startBackgroundThread()
{
    // FIXME: Consider sharing this thread with other background tasks.
    m_threadId = createThread(threadEntryPoint, this, "WebCore: Geolocation cache");
}

void* GeolocationPositionCache::threadEntryPoint(void* object)
{
    static_cast<GeolocationPositionCache*>(object)->threadEntryPointImpl();
    return 0;
}

void GeolocationPositionCache::threadEntryPointImpl()
{
    while (OwnPtr<ScriptExecutionContext::Task> task = m_queue.waitForMessage()) {
        // We don't need a ScriptExecutionContext in the callback, so pass 0 here.
        task->performTask(0);
    }
}

void GeolocationPositionCache::triggerReadFromDatabase()
{
    m_queue.append(createCallbackTask(&GeolocationPositionCache::readFromDatabase, AllowCrossThreadAccess(this)));
}

void GeolocationPositionCache::readFromDatabase(ScriptExecutionContext*, GeolocationPositionCache* cache)
{
    cache->readFromDatabaseImpl();
}

void GeolocationPositionCache::readFromDatabaseImpl()
{
    SQLiteDatabase database;
    {
        MutexLocker lock(m_databaseFileMutex);
        if (!database.open(m_databaseFile))
            return;
    }

    // Create the table here, such that even if we've just created the
    // DB, the commands below should succeed.
    if (!database.executeCommand("CREATE TABLE IF NOT EXISTS CachedPosition ("
            "latitude REAL NOT NULL, "
            "longitude REAL NOT NULL, "
            "altitude REAL, "
            "accuracy REAL NOT NULL, "
            "altitudeAccuracy REAL, "
            "heading REAL, "
            "speed REAL, "
            "timestamp INTEGER NOT NULL)"))
        return;

    SQLiteStatement statement(database, "SELECT * FROM CachedPosition");
    if (statement.prepare() != SQLResultOk)
        return;

    if (statement.step() != SQLResultRow)
        return;

    bool providesAltitude = statement.getColumnValue(2).type() != SQLValue::NullValue;
    bool providesAltitudeAccuracy = statement.getColumnValue(4).type() != SQLValue::NullValue;
    bool providesHeading = statement.getColumnValue(5).type() != SQLValue::NullValue;
    bool providesSpeed = statement.getColumnValue(6).type() != SQLValue::NullValue;
    RefPtr<Coordinates> coordinates = Coordinates::create(statement.getColumnDouble(0), // latitude
                                                          statement.getColumnDouble(1), // longitude
                                                          providesAltitude, statement.getColumnDouble(2), // altitude
                                                          statement.getColumnDouble(3), // accuracy
                                                          providesAltitudeAccuracy, statement.getColumnDouble(4), // altitudeAccuracy
                                                          providesHeading, statement.getColumnDouble(5), // heading
                                                          providesSpeed, statement.getColumnDouble(6)); // speed
    DOMTimeStamp timestamp = statement.getColumnInt64(7); // timestamp

    // A position may have been set since we called triggerReadFromDatabase().
    MutexLocker lock(m_cachedPositionMutex);
    if (m_cachedPosition)
        return;
    m_cachedPosition = Geoposition::create(coordinates.release(), timestamp);
}

void GeolocationPositionCache::triggerWriteToDatabase()
{
    m_queue.append(createCallbackTask(writeToDatabase, AllowCrossThreadAccess(this)));
}

void GeolocationPositionCache::writeToDatabase(ScriptExecutionContext*, GeolocationPositionCache* cache)
{
    cache->writeToDatabaseImpl();
}

void GeolocationPositionCache::writeToDatabaseImpl()
{
    SQLiteDatabase database;
    {
        MutexLocker lock(m_databaseFileMutex);
        if (!database.open(m_databaseFile))
            return;
    }

    RefPtr<Geoposition> cachedPosition;
    {
        MutexLocker lock(m_cachedPositionMutex);
        if (m_cachedPosition)
            cachedPosition = m_cachedPosition->threadSafeCopy();
    }

    SQLiteTransaction transaction(database);

    if (!database.executeCommand("DELETE FROM CachedPosition"))
        return;

    SQLiteStatement statement(database, "INSERT INTO CachedPosition ("
        "latitude, "
        "longitude, "
        "altitude, "
        "accuracy, "
        "altitudeAccuracy, "
        "heading, "
        "speed, "
        "timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    if (statement.prepare() != SQLResultOk)
        return;

    statement.bindDouble(1, cachedPosition->coords()->latitude());
    statement.bindDouble(2, cachedPosition->coords()->longitude());
    if (cachedPosition->coords()->canProvideAltitude())
        statement.bindDouble(3, cachedPosition->coords()->altitude());
    else
        statement.bindNull(3);
    statement.bindDouble(4, cachedPosition->coords()->accuracy());
    if (cachedPosition->coords()->canProvideAltitudeAccuracy())
        statement.bindDouble(5, cachedPosition->coords()->altitudeAccuracy());
    else
        statement.bindNull(5);
    if (cachedPosition->coords()->canProvideHeading())
        statement.bindDouble(6, cachedPosition->coords()->heading());
    else
        statement.bindNull(6);
    if (cachedPosition->coords()->canProvideSpeed())
        statement.bindDouble(7, cachedPosition->coords()->speed());
    else
        statement.bindNull(7);
    statement.bindInt64(8, cachedPosition->timestamp());

    if (!statement.executeCommand())
        return;

    transaction.commit();
}

} // namespace WebCore

#endif // ENABLE(GEOLOCATION)
