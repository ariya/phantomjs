/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#ifndef DatabaseAuthorizer_h
#define DatabaseAuthorizer_h

#include "PlatformString.h"
#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

extern const int SQLAuthAllow;
extern const int SQLAuthIgnore;
extern const int SQLAuthDeny;

class DatabaseAuthorizer : public ThreadSafeRefCounted<DatabaseAuthorizer> {
public:

    enum Permissions {
        ReadWriteMask = 0,
        ReadOnlyMask = 1 << 1,
        NoAccessMask = 1 << 2
    };

    static PassRefPtr<DatabaseAuthorizer> create(const String& databaseInfoTableName);

    int createTable(const String& tableName);
    int createTempTable(const String& tableName);
    int dropTable(const String& tableName);
    int dropTempTable(const String& tableName);
    int allowAlterTable(const String& databaseName, const String& tableName);

    int createIndex(const String& indexName, const String& tableName);
    int createTempIndex(const String& indexName, const String& tableName);
    int dropIndex(const String& indexName, const String& tableName);
    int dropTempIndex(const String& indexName, const String& tableName);

    int createTrigger(const String& triggerName, const String& tableName);
    int createTempTrigger(const String& triggerName, const String& tableName);
    int dropTrigger(const String& triggerName, const String& tableName);
    int dropTempTrigger(const String& triggerName, const String& tableName);

    int createView(const String& viewName);
    int createTempView(const String& viewName);
    int dropView(const String& viewName);
    int dropTempView(const String& viewName);

    int createVTable(const String& tableName, const String& moduleName);
    int dropVTable(const String& tableName, const String& moduleName);

    int allowDelete(const String& tableName);
    int allowInsert(const String& tableName);
    int allowUpdate(const String& tableName, const String& columnName);
    int allowTransaction();

    int allowSelect() { return SQLAuthAllow; }
    int allowRead(const String& tableName, const String& columnName);

    int allowReindex(const String& indexName);
    int allowAnalyze(const String& tableName);
    int allowFunction(const String& functionName);
    int allowPragma(const String& pragmaName, const String& firstArgument);

    int allowAttach(const String& filename);
    int allowDetach(const String& databaseName);

    void disable();
    void enable();
    void setReadOnly();
    void setPermissions(int permissions);

    void reset();
    void resetDeletes();

    bool lastActionWasInsert() const { return m_lastActionWasInsert; }
    bool lastActionChangedDatabase() const { return m_lastActionChangedDatabase; }
    bool hadDeletes() const { return m_hadDeletes; }

private:
    DatabaseAuthorizer(const String& databaseInfoTableName);
    void addWhitelistedFunctions();
    int denyBasedOnTableName(const String&) const;
    int updateDeletesBasedOnTableName(const String&);
    bool allowWrite();

    int m_permissions;
    bool m_securityEnabled : 1;
    bool m_lastActionWasInsert : 1;
    bool m_lastActionChangedDatabase : 1;
    bool m_hadDeletes : 1;

    const String m_databaseInfoTableName;

    HashSet<String, CaseFoldingHash> m_whitelistedFunctions;
};

} // namespace WebCore

#endif // DatabaseAuthorizer_h
