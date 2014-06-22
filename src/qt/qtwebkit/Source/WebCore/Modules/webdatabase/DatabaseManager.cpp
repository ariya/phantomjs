/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DatabaseManager.h"

#if ENABLE(SQL_DATABASE)

#include "AbstractDatabaseServer.h"
#include "Database.h"
#include "DatabaseBackend.h"
#include "DatabaseBackendBase.h"
#include "DatabaseBackendContext.h"
#include "DatabaseBackendSync.h"
#include "DatabaseCallback.h"
#include "DatabaseContext.h"
#include "DatabaseStrategy.h"
#include "DatabaseSync.h"
#include "DatabaseTask.h"
#include "ExceptionCode.h"
#include "InspectorDatabaseInstrumentation.h"
#include "Logging.h"
#include "PlatformStrategies.h"
#include "ScriptController.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"

namespace WebCore {

DatabaseManager& DatabaseManager::manager()
{
    static DatabaseManager* dbManager = 0;
    // FIXME: The following is vulnerable to a race between threads. Need to
    // implement a thread safe on-first-use static initializer.
    if (!dbManager)
        dbManager = new DatabaseManager();

    return *dbManager;
}

DatabaseManager::DatabaseManager()
    : m_server(platformStrategies()->databaseStrategy()->getDatabaseServer())
    , m_client(0)
    , m_databaseIsAvailable(true)
#if !ASSERT_DISABLED
    , m_databaseContextRegisteredCount(0)
    , m_databaseContextInstanceCount(0)
#endif
{
    ASSERT(m_server); // We should always have a server to work with.
}

void DatabaseManager::initialize(const String& databasePath)
{
    m_server->initialize(databasePath);
}

void DatabaseManager::setClient(DatabaseManagerClient* client)
{
    m_client = client;
    m_server->setClient(client);
}

String DatabaseManager::databaseDirectoryPath() const
{
    return m_server->databaseDirectoryPath();
}

void DatabaseManager::setDatabaseDirectoryPath(const String& path)
{
    m_server->setDatabaseDirectoryPath(path);
}

bool DatabaseManager::isAvailable()
{
    return m_databaseIsAvailable;
}

void DatabaseManager::setIsAvailable(bool available)
{
    m_databaseIsAvailable = available;
}

class DatabaseCreationCallbackTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<DatabaseCreationCallbackTask> create(PassRefPtr<Database> database, PassRefPtr<DatabaseCallback> creationCallback)
    {
        return adoptPtr(new DatabaseCreationCallbackTask(database, creationCallback));
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        m_creationCallback->handleEvent(m_database.get());
    }

private:
    DatabaseCreationCallbackTask(PassRefPtr<Database> database, PassRefPtr<DatabaseCallback> callback)
        : m_database(database)
        , m_creationCallback(callback)
    {
    }

    RefPtr<Database> m_database;
    RefPtr<DatabaseCallback> m_creationCallback;
};

PassRefPtr<DatabaseContext> DatabaseManager::existingDatabaseContextFor(ScriptExecutionContext* context)
{
    MutexLocker locker(m_contextMapLock);

    ASSERT(m_databaseContextRegisteredCount >= 0);
    ASSERT(m_databaseContextInstanceCount >= 0);
    ASSERT(m_databaseContextRegisteredCount <= m_databaseContextInstanceCount);

    RefPtr<DatabaseContext> databaseContext = adoptRef(m_contextMap.get(context));
    if (databaseContext) {
        // If we're instantiating a new DatabaseContext, the new instance would
        // carry a new refCount of 1. The client expects this and will simply
        // adoptRef the databaseContext without ref'ing it.
        //     However, instead of instantiating a new instance, we're reusing
        // an existing one that corresponds to the specified ScriptExecutionContext.
        // Hence, that new refCount need to be attributed to the reused instance
        // to ensure that the refCount is accurate when the client adopts the ref.
        // We do this by ref'ing the reused databaseContext before returning it.
        databaseContext->ref();
    }
    return databaseContext.release();
}

PassRefPtr<DatabaseContext> DatabaseManager::databaseContextFor(ScriptExecutionContext* context)
{
    RefPtr<DatabaseContext> databaseContext = existingDatabaseContextFor(context);
    if (!databaseContext)
        databaseContext = adoptRef(new DatabaseContext(context));
    return databaseContext.release();
}

void DatabaseManager::registerDatabaseContext(DatabaseContext* databaseContext)
{
    MutexLocker locker(m_contextMapLock);
    ScriptExecutionContext* context = databaseContext->scriptExecutionContext();
    m_contextMap.set(context, databaseContext);
#if !ASSERT_DISABLED
    m_databaseContextRegisteredCount++;
#endif
}

void DatabaseManager::unregisterDatabaseContext(DatabaseContext* databaseContext)
{
    MutexLocker locker(m_contextMapLock);
    ScriptExecutionContext* context = databaseContext->scriptExecutionContext();
    ASSERT(m_contextMap.get(context));
#if !ASSERT_DISABLED
    m_databaseContextRegisteredCount--;
#endif
    m_contextMap.remove(context);
}

#if !ASSERT_DISABLED
void DatabaseManager::didConstructDatabaseContext()
{
    MutexLocker lock(m_contextMapLock);
    m_databaseContextInstanceCount++;
}

void DatabaseManager::didDestructDatabaseContext()
{
    MutexLocker lock(m_contextMapLock);
    m_databaseContextInstanceCount--;
    ASSERT(m_databaseContextRegisteredCount <= m_databaseContextInstanceCount);
}
#endif

ExceptionCode DatabaseManager::exceptionCodeForDatabaseError(DatabaseError error)
{
    switch (error) {
    case DatabaseError::None:
        return 0;
    case DatabaseError::DatabaseIsBeingDeleted:
    case DatabaseError::DatabaseSizeExceededQuota:
    case DatabaseError::DatabaseSizeOverflowed:
    case DatabaseError::GenericSecurityError:
        return SECURITY_ERR;
    case DatabaseError::InvalidDatabaseState:
        return INVALID_STATE_ERR;
    }
    ASSERT_NOT_REACHED();
    return 0; // Make some older compilers happy.
}

static void logOpenDatabaseError(ScriptExecutionContext* context, const String& name)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(name);
    LOG(StorageAPI, "Database %s for origin %s not allowed to be established", name.ascii().data(),
        context->securityOrigin()->toString().ascii().data());
}

PassRefPtr<DatabaseBackendBase> DatabaseManager::openDatabaseBackend(ScriptExecutionContext* context,
    DatabaseType type, const String& name, const String& expectedVersion, const String& displayName,
    unsigned long estimatedSize, bool setVersionInNewDatabase, DatabaseError& error, String& errorMessage)
{
    ASSERT(error == DatabaseError::None);

    RefPtr<DatabaseContext> databaseContext = databaseContextFor(context);
    RefPtr<DatabaseBackendContext> backendContext = databaseContext->backend();

    RefPtr<DatabaseBackendBase> backend = m_server->openDatabase(backendContext, type, name, expectedVersion,
        displayName, estimatedSize, setVersionInNewDatabase, error, errorMessage);

    if (!backend) {
        ASSERT(error != DatabaseError::None);

        switch (error) {
        case DatabaseError::DatabaseIsBeingDeleted:
        case DatabaseError::DatabaseSizeOverflowed:
        case DatabaseError::GenericSecurityError:
            logOpenDatabaseError(context, name);
            return 0;

        case DatabaseError::InvalidDatabaseState:
            logErrorMessage(context, errorMessage);
            return 0;

        case DatabaseError::DatabaseSizeExceededQuota:
            // Notify the client that we've exceeded the database quota.
            // The client may want to increase the quota, and we'll give it
            // one more try after if that is the case.
            databaseContext->databaseExceededQuota(name,
                DatabaseDetails(name.isolatedCopy(), displayName.isolatedCopy(), estimatedSize, 0));

            error = DatabaseError::None;

            backend = m_server->openDatabase(backendContext, type, name, expectedVersion,
                displayName, estimatedSize, setVersionInNewDatabase, error, errorMessage,
                AbstractDatabaseServer::RetryOpenDatabase);
            break;

        default:
            ASSERT_NOT_REACHED();
        }

        if (!backend) {
            ASSERT(error != DatabaseError::None);

            if (error == DatabaseError::InvalidDatabaseState) {
                logErrorMessage(context, errorMessage);
                return 0;
            }

            logOpenDatabaseError(context, name);
            return 0;
        }
    }

    return backend.release();
}

PassRefPtr<Database> DatabaseManager::openDatabase(ScriptExecutionContext* context,
    const String& name, const String& expectedVersion, const String& displayName,
    unsigned long estimatedSize, PassRefPtr<DatabaseCallback> creationCallback,
    DatabaseError& error)
{
    ScriptController::initializeThreading();
    ASSERT(error == DatabaseError::None);

    bool setVersionInNewDatabase = !creationCallback;
    String errorMessage;
    RefPtr<DatabaseBackendBase> backend = openDatabaseBackend(context, DatabaseType::Async, name,
        expectedVersion, displayName, estimatedSize, setVersionInNewDatabase, error, errorMessage);
    if (!backend)
        return 0;

    RefPtr<Database> database = Database::create(context, backend);

    RefPtr<DatabaseContext> databaseContext = databaseContextFor(context);
    databaseContext->setHasOpenDatabases();
    InspectorInstrumentation::didOpenDatabase(context, database, context->securityOrigin()->host(), name, expectedVersion);

    if (backend->isNew() && creationCallback.get()) {
        LOG(StorageAPI, "Scheduling DatabaseCreationCallbackTask for database %p\n", database.get());
        database->m_scriptExecutionContext->postTask(DatabaseCreationCallbackTask::create(database, creationCallback));
    }

    ASSERT(database);
    return database.release();
}

PassRefPtr<DatabaseSync> DatabaseManager::openDatabaseSync(ScriptExecutionContext* context,
    const String& name, const String& expectedVersion, const String& displayName,
    unsigned long estimatedSize, PassRefPtr<DatabaseCallback> creationCallback, DatabaseError& error)
{
    ASSERT(context->isContextThread());
    ASSERT(error == DatabaseError::None);

    bool setVersionInNewDatabase = !creationCallback;
    String errorMessage;
    RefPtr<DatabaseBackendBase> backend = openDatabaseBackend(context, DatabaseType::Sync, name,
        expectedVersion, displayName, estimatedSize, setVersionInNewDatabase, error, errorMessage);
    if (!backend)
        return 0;

    RefPtr<DatabaseSync> database = DatabaseSync::create(context, backend);

    if (backend->isNew() && creationCallback.get()) {
        LOG(StorageAPI, "Invoking the creation callback for database %p\n", database.get());
        creationCallback->handleEvent(database.get());
    }

    ASSERT(database);
    return database.release();
}

bool DatabaseManager::hasOpenDatabases(ScriptExecutionContext* context)
{
    RefPtr<DatabaseContext> databaseContext = existingDatabaseContextFor(context);
    if (!databaseContext)
        return false;
    return databaseContext->hasOpenDatabases();
}

void DatabaseManager::stopDatabases(ScriptExecutionContext* context, DatabaseTaskSynchronizer* synchronizer)
{
    RefPtr<DatabaseContext> databaseContext = existingDatabaseContextFor(context);
    if (!databaseContext || !databaseContext->stopDatabases(synchronizer))
        if (synchronizer)
            synchronizer->taskCompleted();
}

String DatabaseManager::fullPathForDatabase(SecurityOrigin* origin, const String& name, bool createIfDoesNotExist)
{
    return m_server->fullPathForDatabase(origin, name, createIfDoesNotExist);
}

bool DatabaseManager::hasEntryForOrigin(SecurityOrigin* origin)
{
    return m_server->hasEntryForOrigin(origin);
}

void DatabaseManager::origins(Vector<RefPtr<SecurityOrigin> >& result)
{
    m_server->origins(result);
}

bool DatabaseManager::databaseNamesForOrigin(SecurityOrigin* origin, Vector<String>& result)
{
    return m_server->databaseNamesForOrigin(origin, result);
}

DatabaseDetails DatabaseManager::detailsForNameAndOrigin(const String& name, SecurityOrigin* origin)
{
    return m_server->detailsForNameAndOrigin(name, origin);
}

unsigned long long DatabaseManager::usageForOrigin(SecurityOrigin* origin)
{
    return m_server->usageForOrigin(origin);
}

unsigned long long DatabaseManager::quotaForOrigin(SecurityOrigin* origin)
{
    return m_server->quotaForOrigin(origin);
}

void DatabaseManager::setQuota(SecurityOrigin* origin, unsigned long long quotaSize)
{
    m_server->setQuota(origin, quotaSize);
}

void DatabaseManager::deleteAllDatabases()
{
    m_server->deleteAllDatabases();
}

bool DatabaseManager::deleteOrigin(SecurityOrigin* origin)
{
    return m_server->deleteOrigin(origin);
}

bool DatabaseManager::deleteDatabase(SecurityOrigin* origin, const String& name)
{
    return m_server->deleteDatabase(origin, name);
}

void DatabaseManager::interruptAllDatabasesForContext(ScriptExecutionContext* context)
{
    RefPtr<DatabaseContext> databaseContext = existingDatabaseContextFor(context);
    if (databaseContext)
        m_server->interruptAllDatabasesForContext(databaseContext->backend().get());
}

void DatabaseManager::logErrorMessage(ScriptExecutionContext* context, const String& message)
{
    context->addConsoleMessage(StorageMessageSource, ErrorMessageLevel, message);
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
