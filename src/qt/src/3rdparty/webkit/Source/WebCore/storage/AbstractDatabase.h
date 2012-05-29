/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AbstractDatabase_h
#define AbstractDatabase_h

#if ENABLE(DATABASE)

#include "ExceptionCode.h"
#include "PlatformString.h"
#include "SQLiteDatabase.h"
#include <wtf/Forward.h>
#include <wtf/ThreadSafeRefCounted.h>
#ifndef NDEBUG
#include "SecurityOrigin.h"
#endif

namespace WebCore {

class DatabaseAuthorizer;
class ScriptExecutionContext;
class SecurityOrigin;

class AbstractDatabase : public ThreadSafeRefCounted<AbstractDatabase> {
public:
    static bool isAvailable();
    static void setIsAvailable(bool available);

    virtual ~AbstractDatabase();

    virtual String version() const;

    bool opened() const { return m_opened; }
    bool isNew() const { return m_new; }

    virtual ScriptExecutionContext* scriptExecutionContext() const;
    virtual SecurityOrigin* securityOrigin() const;
    virtual String stringIdentifier() const;
    virtual String displayName() const;
    virtual unsigned long estimatedSize() const;
    virtual String fileName() const;
    SQLiteDatabase& sqliteDatabase() { return m_sqliteDatabase; }

    unsigned long long maximumSize() const;
    void incrementalVacuumIfNeeded();
    void interrupt();
    bool isInterrupted();

    // FIXME: move all version-related methods to a DatabaseVersionTracker class
    bool versionMatchesExpected() const;
    void setExpectedVersion(const String& version);
    bool getVersionFromDatabase(String& version);
    bool setVersionInDatabase(const String& version);

    void disableAuthorizer();
    void enableAuthorizer();
    void setAuthorizerReadOnly();
    void setAuthorizerPermissions(int permissions);
    bool lastActionChangedDatabase();
    bool lastActionWasInsert();
    void resetDeletes();
    bool hadDeletes();
    void resetAuthorizer();

    virtual void markAsDeletedAndClose() = 0;
    virtual void closeImmediately() = 0;

protected:
    AbstractDatabase(ScriptExecutionContext*, const String& name, const String& expectedVersion,
                     const String& displayName, unsigned long estimatedSize);

    void closeDatabase();

    virtual bool performOpenAndVerify(bool shouldSetVersionInNewDatabase, ExceptionCode& ec);

    static const String& databaseInfoTableName();

    RefPtr<ScriptExecutionContext> m_scriptExecutionContext;
    RefPtr<SecurityOrigin> m_contextThreadSecurityOrigin;

    String m_name;
    String m_expectedVersion;
    String m_displayName;
    unsigned long m_estimatedSize;
    String m_filename;

#ifndef NDEBUG
    String databaseDebugName() const { return m_contextThreadSecurityOrigin->toString() + "::" + m_name; }
#endif

private:
    static const String& databaseVersionKey();

    int m_guid;
    bool m_opened;
    bool m_new;

    SQLiteDatabase m_sqliteDatabase;

    RefPtr<DatabaseAuthorizer> m_databaseAuthorizer;
};

} // namespace WebCore

#endif // ENABLE(DATABASE)

#endif // AbstractDatabase_h
