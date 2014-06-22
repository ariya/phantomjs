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

#include "config.h"
#include "DatabaseAuthorizer.h"

#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

PassRefPtr<DatabaseAuthorizer> DatabaseAuthorizer::create(const String& databaseInfoTableName)
{
    return adoptRef(new DatabaseAuthorizer(databaseInfoTableName));
}

DatabaseAuthorizer::DatabaseAuthorizer(const String& databaseInfoTableName)
    : m_securityEnabled(false)
    , m_databaseInfoTableName(databaseInfoTableName)
{
    reset();
    addWhitelistedFunctions();
}

void DatabaseAuthorizer::reset()
{
    m_lastActionWasInsert = false;
    m_lastActionChangedDatabase = false;
    m_permissions = ReadWriteMask;
}

void DatabaseAuthorizer::resetDeletes()
{
    m_hadDeletes = false;
}

void DatabaseAuthorizer::addWhitelistedFunctions()
{
    // SQLite functions used to help implement some operations
    // ALTER TABLE helpers
    m_whitelistedFunctions.add("sqlite_rename_table");
    m_whitelistedFunctions.add("sqlite_rename_trigger");
    // GLOB helpers
    m_whitelistedFunctions.add("glob");

    // SQLite core functions
    m_whitelistedFunctions.add("abs");
    m_whitelistedFunctions.add("changes");
    m_whitelistedFunctions.add("coalesce");
    m_whitelistedFunctions.add("glob");
    m_whitelistedFunctions.add("ifnull");
    m_whitelistedFunctions.add("hex");
    m_whitelistedFunctions.add("last_insert_rowid");
    m_whitelistedFunctions.add("length");
    m_whitelistedFunctions.add("like");
    m_whitelistedFunctions.add("lower");
    m_whitelistedFunctions.add("ltrim");
    m_whitelistedFunctions.add("max");
    m_whitelistedFunctions.add("min");
    m_whitelistedFunctions.add("nullif");
    m_whitelistedFunctions.add("quote");
    m_whitelistedFunctions.add("replace");
    m_whitelistedFunctions.add("round");
    m_whitelistedFunctions.add("rtrim");
    m_whitelistedFunctions.add("soundex");
    m_whitelistedFunctions.add("sqlite_source_id");
    m_whitelistedFunctions.add("sqlite_version");
    m_whitelistedFunctions.add("substr");
    m_whitelistedFunctions.add("total_changes");
    m_whitelistedFunctions.add("trim");
    m_whitelistedFunctions.add("typeof");
    m_whitelistedFunctions.add("upper");
    m_whitelistedFunctions.add("zeroblob");

    // SQLite date and time functions
    m_whitelistedFunctions.add("date");
    m_whitelistedFunctions.add("time");
    m_whitelistedFunctions.add("datetime");
    m_whitelistedFunctions.add("julianday");
    m_whitelistedFunctions.add("strftime");

    // SQLite aggregate functions
    // max() and min() are already in the list
    m_whitelistedFunctions.add("avg");
    m_whitelistedFunctions.add("count");
    m_whitelistedFunctions.add("group_concat");
    m_whitelistedFunctions.add("sum");
    m_whitelistedFunctions.add("total");

    // SQLite FTS functions
    m_whitelistedFunctions.add("match");
    m_whitelistedFunctions.add("snippet");
    m_whitelistedFunctions.add("offsets");
    m_whitelistedFunctions.add("optimize");

    // SQLite ICU functions
    // like(), lower() and upper() are already in the list
    m_whitelistedFunctions.add("regexp");
}

int DatabaseAuthorizer::createTable(const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    m_lastActionChangedDatabase = true;
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::createTempTable(const String& tableName)
{
    // SQLITE_CREATE_TEMP_TABLE results in a UPDATE operation, which is not
    // allowed in read-only transactions or private browsing, so we might as
    // well disallow SQLITE_CREATE_TEMP_TABLE in these cases
    if (!allowWrite())
        return SQLAuthDeny;

    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::dropTable(const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::dropTempTable(const String& tableName)
{
    // SQLITE_DROP_TEMP_TABLE results in a DELETE operation, which is not
    // allowed in read-only transactions or private browsing, so we might as
    // well disallow SQLITE_DROP_TEMP_TABLE in these cases
    if (!allowWrite())
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::allowAlterTable(const String&, const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    m_lastActionChangedDatabase = true;
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::createIndex(const String&, const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    m_lastActionChangedDatabase = true;
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::createTempIndex(const String&, const String& tableName)
{
    // SQLITE_CREATE_TEMP_INDEX should result in a UPDATE or INSERT operation,
    // which is not allowed in read-only transactions or private browsing,
    // so we might as well disallow SQLITE_CREATE_TEMP_INDEX in these cases
    if (!allowWrite())
        return SQLAuthDeny;

    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::dropIndex(const String&, const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::dropTempIndex(const String&, const String& tableName)
{
    // SQLITE_DROP_TEMP_INDEX should result in a DELETE operation, which is
    // not allowed in read-only transactions or private browsing, so we might
    // as well disallow SQLITE_DROP_TEMP_INDEX in these cases
    if (!allowWrite())
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::createTrigger(const String&, const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    m_lastActionChangedDatabase = true;
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::createTempTrigger(const String&, const String& tableName)
{
    // SQLITE_CREATE_TEMP_TRIGGER results in a INSERT operation, which is not
    // allowed in read-only transactions or private browsing, so we might as
    // well disallow SQLITE_CREATE_TEMP_TRIGGER in these cases
    if (!allowWrite())
        return SQLAuthDeny;

    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::dropTrigger(const String&, const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::dropTempTrigger(const String&, const String& tableName)
{
    // SQLITE_DROP_TEMP_TRIGGER results in a DELETE operation, which is not
    // allowed in read-only transactions or private browsing, so we might as
    // well disallow SQLITE_DROP_TEMP_TRIGGER in these cases
    if (!allowWrite())
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::createView(const String&)
{
    return (!allowWrite() ? SQLAuthDeny : SQLAuthAllow);
}

int DatabaseAuthorizer::createTempView(const String&)
{
    // SQLITE_CREATE_TEMP_VIEW results in a UPDATE operation, which is not
    // allowed in read-only transactions or private browsing, so we might as
    // well disallow SQLITE_CREATE_TEMP_VIEW in these cases
    return (!allowWrite() ? SQLAuthDeny : SQLAuthAllow);
}

int DatabaseAuthorizer::dropView(const String&)
{
    if (!allowWrite())
        return SQLAuthDeny;

    m_hadDeletes = true;
    return SQLAuthAllow;
}

int DatabaseAuthorizer::dropTempView(const String&)
{
    // SQLITE_DROP_TEMP_VIEW results in a DELETE operation, which is not
    // allowed in read-only transactions or private browsing, so we might as
    // well disallow SQLITE_DROP_TEMP_VIEW in these cases
    if (!allowWrite())
        return SQLAuthDeny;

    m_hadDeletes = true;
    return SQLAuthAllow;
}

int DatabaseAuthorizer::createVTable(const String& tableName, const String& moduleName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    // Allow only the FTS3 extension
    if (!equalIgnoringCase(moduleName, "fts3"))
        return SQLAuthDeny;

    m_lastActionChangedDatabase = true;
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::dropVTable(const String& tableName, const String& moduleName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    // Allow only the FTS3 extension
    if (!equalIgnoringCase(moduleName, "fts3"))
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::allowDelete(const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    return updateDeletesBasedOnTableName(tableName);
}

int DatabaseAuthorizer::allowInsert(const String& tableName)
{
    if (!allowWrite())
        return SQLAuthDeny;

    m_lastActionChangedDatabase = true;
    m_lastActionWasInsert = true;
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::allowUpdate(const String& tableName, const String&)
{
    if (!allowWrite())
        return SQLAuthDeny;

    m_lastActionChangedDatabase = true;
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::allowTransaction()
{
    return m_securityEnabled ? SQLAuthDeny : SQLAuthAllow;
}

int DatabaseAuthorizer::allowRead(const String& tableName, const String&)
{
    if (m_permissions & NoAccessMask && m_securityEnabled)
        return SQLAuthDeny;
    
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::allowReindex(const String&)
{
    return (!allowWrite() ? SQLAuthDeny : SQLAuthAllow);
}

int DatabaseAuthorizer::allowAnalyze(const String& tableName)
{
    return denyBasedOnTableName(tableName);
}

int DatabaseAuthorizer::allowPragma(const String&, const String&)
{
    return m_securityEnabled ? SQLAuthDeny : SQLAuthAllow;
}

int DatabaseAuthorizer::allowAttach(const String&)
{
    return m_securityEnabled ? SQLAuthDeny : SQLAuthAllow;
}

int DatabaseAuthorizer::allowDetach(const String&)
{
    return m_securityEnabled ? SQLAuthDeny : SQLAuthAllow;
}

int DatabaseAuthorizer::allowFunction(const String& functionName)
{
    if (m_securityEnabled && !m_whitelistedFunctions.contains(functionName))
        return SQLAuthDeny;

    return SQLAuthAllow;
}

void DatabaseAuthorizer::disable()
{
    m_securityEnabled = false;
}

void DatabaseAuthorizer::enable()
{
    m_securityEnabled = true;
}

bool DatabaseAuthorizer::allowWrite()
{
    return !(m_securityEnabled && (m_permissions & ReadOnlyMask || m_permissions & NoAccessMask));
}

void DatabaseAuthorizer::setReadOnly()
{
    m_permissions |= ReadOnlyMask;
}
   
void DatabaseAuthorizer::setPermissions(int permissions)
{
    m_permissions = permissions;
}

int DatabaseAuthorizer::denyBasedOnTableName(const String& tableName) const
{
    if (!m_securityEnabled)
        return SQLAuthAllow;

    // Sadly, normal creates and drops end up affecting sqlite_master in an authorizer callback, so
    // it will be tough to enforce all of the following policies
    //if (equalIgnoringCase(tableName, "sqlite_master") || equalIgnoringCase(tableName, "sqlite_temp_master") ||
    //    equalIgnoringCase(tableName, "sqlite_sequence") || equalIgnoringCase(tableName, Database::databaseInfoTableName()))
    //        return SQLAuthDeny;

    if (equalIgnoringCase(tableName, m_databaseInfoTableName))
        return SQLAuthDeny;

    return SQLAuthAllow;
}

int DatabaseAuthorizer::updateDeletesBasedOnTableName(const String& tableName)
{
    int allow = denyBasedOnTableName(tableName);
    if (allow)
        m_hadDeletes = true;
    return allow;
}

} // namespace WebCore
