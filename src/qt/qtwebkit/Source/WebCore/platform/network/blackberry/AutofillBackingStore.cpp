/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "AutofillBackingStore.h"

#include "FileSystem.h"
#include "SQLiteStatement.h"
#include <BlackBerryPlatformSettings.h>

#define HANDLE_SQL_EXEC_FAILURE(statement, returnValue, ...) \
    if (statement) { \
        LOG_ERROR(__VA_ARGS__); \
        return returnValue; \
    }

namespace WebCore {

AutofillBackingStore& autofillBackingStore()
{
    DEFINE_STATIC_LOCAL(AutofillBackingStore, backingStore, ());
    if (!backingStore.m_database.isOpen())
        backingStore.open(pathByAppendingComponent(BlackBerry::Platform::Settings::instance()->applicationDataDirectory().c_str(), "/autofill.db"));
    return backingStore;
}

AutofillBackingStore::AutofillBackingStore()
    : m_addStatement(0)
    , m_updateStatement(0)
    , m_containsStatement(0)
    , m_getStatement(0)
{
}

AutofillBackingStore::~AutofillBackingStore()
{
    delete m_addStatement;
    m_addStatement = 0;
    delete m_updateStatement;
    m_updateStatement = 0;
    delete m_containsStatement;
    m_containsStatement = 0;
    delete m_getStatement;
    m_getStatement = 0;

    if (m_database.isOpen())
        m_database.close();
}

bool AutofillBackingStore::open(const String& dbPath)
{
    ASSERT(!m_database.isOpen());

    HANDLE_SQL_EXEC_FAILURE(!m_database.open(dbPath), false,
        "Failed to open database file %s for autofill database", dbPath.utf8().data());

    if (!m_database.tableExists("autofill")) {
        HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("CREATE TABLE autofill (id INTEGER PRIMARY KEY, name VARCHAR NOT NULL, value VARCHAR NOT NULL, count INTEGER DEFAULT 1)"),
            false, "Failed to create table autofill for autofill database");

        // Create index for table autofill.
        HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("CREATE INDEX autofill_name ON autofill (name)"),
            false, "Failed to create autofill_name index for table autofill");
    }

    // Prepare the statements.
    m_addStatement = new SQLiteStatement(m_database, "INSERT INTO autofill (name, value) VALUES (?, ?)");
    HANDLE_SQL_EXEC_FAILURE(m_addStatement->prepare() != SQLResultOk,
        false, "Failed to prepare add statement");

    m_updateStatement = new SQLiteStatement(m_database, "UPDATE autofill SET count = (SELECT count + 1 from autofill WHERE name = ? AND value = ?) WHERE name = ? AND value = ?");
    HANDLE_SQL_EXEC_FAILURE(m_updateStatement->prepare() != SQLResultOk,
        false, "Failed to prepare update statement");

    m_containsStatement = new SQLiteStatement(m_database, "SELECT COUNT(*) FROM autofill WHERE name = ? AND value = ?");
    HANDLE_SQL_EXEC_FAILURE(m_containsStatement->prepare() != SQLResultOk,
        false, "Failed to prepare contains statement");

    m_getStatement = new SQLiteStatement(m_database, "SELECT value FROM autofill WHERE name = ? and value like ? ORDER BY count DESC");
    HANDLE_SQL_EXEC_FAILURE(m_getStatement->prepare() != SQLResultOk,
        false, "Failed to prepare get statement");

    return true;
}

bool AutofillBackingStore::add(const String& name, const String& value)
{
    if (name.isEmpty() || value.isEmpty())
        return false;

    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("autofill"));

    if (contains(name, value))
        return update(name, value);

    if (!m_addStatement)
        return false;

    m_addStatement->bindText(1, name);
    m_addStatement->bindText(2, value);

    int result = m_addStatement->step();
    m_addStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, false,
        "Failed to add autofill item into table autofill - %i", result);

    return true;
}

bool AutofillBackingStore::update(const String& name, const String& value)
{
    if (!m_updateStatement)
        return false;

    m_updateStatement->bindText(1, name);
    m_updateStatement->bindText(2, value);
    m_updateStatement->bindText(3, name);
    m_updateStatement->bindText(4, value);

    int result = m_updateStatement->step();
    m_updateStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, false,
        "Failed to update autofill item in table autofill - %i", result);

    return true;
}

bool AutofillBackingStore::contains(const String& name, const String& value) const
{
    if (!m_containsStatement)
        return false;

    m_containsStatement->bindText(1, name);
    m_containsStatement->bindText(2, value);

    int result = m_containsStatement->step();
    int numberOfRows = m_containsStatement->getColumnInt(0);
    m_containsStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultRow, false,
        "Failed to execute select autofill item from table autofill in contains - %i", result);

    return numberOfRows;
}

Vector<String> AutofillBackingStore::get(const String& name, const String& valueHint)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("autofill"));

    Vector<String> candidates;
    if (name.isEmpty() || valueHint.isEmpty() || !m_getStatement)
        return candidates;

    String value = valueHint + "%";
    m_getStatement->bindText(1, name);
    m_getStatement->bindText(2, value);

    int result;
    while ((result = m_getStatement->step()) == SQLResultRow)
        candidates.append(m_getStatement->getColumnText(0));
    m_getStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, candidates,
        "Failed to execute select autofill item from table autofill in get - %i", result);

    return candidates;
}

bool AutofillBackingStore::clear()
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("autofill"));

    HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("DELETE FROM autofill"),
        false, "Failed to clear table autofill");

    return true;
}

} // namespace WebCore
