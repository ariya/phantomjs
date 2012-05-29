/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "IDBSQLiteBackingStore.h"

#if ENABLE(INDEXED_DATABASE)

#include "FileSystem.h"
#include "IDBFactoryBackendImpl.h"
#include "IDBKey.h"
#include "IDBKeyRange.h"
#include "SQLiteDatabase.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include "SecurityOrigin.h"

namespace WebCore {

IDBSQLiteBackingStore::IDBSQLiteBackingStore(String identifier, IDBFactoryBackendImpl* factory)
    : m_identifier(identifier)
    , m_factory(factory)
{
    m_factory->addIDBBackingStore(identifier, this);
}

IDBSQLiteBackingStore::~IDBSQLiteBackingStore()
{
    m_factory->removeIDBBackingStore(m_identifier);
}

static bool runCommands(SQLiteDatabase& sqliteDatabase, const char** commands, size_t numberOfCommands)
{
    SQLiteTransaction transaction(sqliteDatabase, false);
    transaction.begin();
    for (size_t i = 0; i < numberOfCommands; ++i) {
        if (!sqliteDatabase.executeCommand(commands[i])) {
            LOG_ERROR("Failed to run the following command for IndexedDB: %s", commands[i]);
            return false;
        }
    }
    transaction.commit();
    return true;
}

static bool createTables(SQLiteDatabase& sqliteDatabase)
{
    if (sqliteDatabase.tableExists("Databases"))
        return true;
    static const char* commands[] = {
        "CREATE TABLE Databases (id INTEGER PRIMARY KEY, name TEXT NOT NULL, description TEXT NOT NULL, version TEXT NOT NULL)",
        "CREATE UNIQUE INDEX Databases_name ON Databases(name)",

        "CREATE TABLE ObjectStores (id INTEGER PRIMARY KEY, name TEXT NOT NULL, keyPath TEXT, doAutoIncrement INTEGER NOT NULL, databaseId INTEGER NOT NULL REFERENCES Databases(id))",
        "CREATE UNIQUE INDEX ObjectStores_composit ON ObjectStores(databaseId, name)",

        "CREATE TABLE Indexes (id INTEGER PRIMARY KEY, objectStoreId INTEGER NOT NULL REFERENCES ObjectStore(id), name TEXT NOT NULL, keyPath TEXT, isUnique INTEGER NOT NULL)",
        "CREATE UNIQUE INDEX Indexes_composit ON Indexes(objectStoreId, name)",

        "CREATE TABLE ObjectStoreData (id INTEGER PRIMARY KEY, objectStoreId INTEGER NOT NULL REFERENCES ObjectStore(id), keyString TEXT, keyDate INTEGER, keyNumber INTEGER, value TEXT NOT NULL)",
        "CREATE UNIQUE INDEX ObjectStoreData_composit ON ObjectStoreData(keyString, keyDate, keyNumber, objectStoreId)",

        "CREATE TABLE IndexData (id INTEGER PRIMARY KEY, indexId INTEGER NOT NULL REFERENCES Indexes(id), keyString TEXT, keyDate INTEGER, keyNumber INTEGER, objectStoreDataId INTEGER NOT NULL REFERENCES ObjectStoreData(id))",
        "CREATE INDEX IndexData_composit ON IndexData(keyString, keyDate, keyNumber, indexId)",
        "CREATE INDEX IndexData_objectStoreDataId ON IndexData(objectStoreDataId)",
        "CREATE INDEX IndexData_indexId ON IndexData(indexId)",
        };

    return runCommands(sqliteDatabase, commands, sizeof(commands) / sizeof(commands[0]));
}

static bool createMetaDataTable(SQLiteDatabase& sqliteDatabase)
{
    static const char* commands[] = {
        "CREATE TABLE MetaData (name TEXT PRIMARY KEY, value NONE)",
        "INSERT INTO MetaData VALUES ('version', 1)",
    };

    return runCommands(sqliteDatabase, commands, sizeof(commands) / sizeof(commands[0]));
}

static bool getDatabaseSchemaVersion(SQLiteDatabase& sqliteDatabase, int* databaseVersion)
{
    SQLiteStatement query(sqliteDatabase, "SELECT value FROM MetaData WHERE name = 'version'");
    if (query.prepare() != SQLResultOk || query.step() != SQLResultRow)
        return false;

    *databaseVersion = query.getColumnInt(0);
    return query.finalize() == SQLResultOk;
}

static bool migrateDatabase(SQLiteDatabase& sqliteDatabase)
{
    if (!sqliteDatabase.tableExists("MetaData")) {
        if (!createMetaDataTable(sqliteDatabase))
            return false;
    }

    int databaseVersion;
    if (!getDatabaseSchemaVersion(sqliteDatabase, &databaseVersion))
        return false;

    if (databaseVersion == 1) {
        static const char* commands[] = {
            "DROP TABLE IF EXISTS ObjectStoreData2",
            "CREATE TABLE ObjectStoreData2 (id INTEGER PRIMARY KEY, objectStoreId INTEGER NOT NULL REFERENCES ObjectStore(id), keyString TEXT, keyDate REAL, keyNumber REAL, value TEXT NOT NULL)",
            "INSERT INTO ObjectStoreData2 SELECT * FROM ObjectStoreData",
            "DROP TABLE ObjectStoreData", // This depends on SQLite not enforcing referential consistency.
            "ALTER TABLE ObjectStoreData2 RENAME TO ObjectStoreData",
            "CREATE UNIQUE INDEX ObjectStoreData_composit ON ObjectStoreData(keyString, keyDate, keyNumber, objectStoreId)",
            "DROP TABLE IF EXISTS IndexData2", // This depends on SQLite not enforcing referential consistency.
            "CREATE TABLE IndexData2 (id INTEGER PRIMARY KEY, indexId INTEGER NOT NULL REFERENCES Indexes(id), keyString TEXT, keyDate REAL, keyNumber REAL, objectStoreDataId INTEGER NOT NULL REFERENCES ObjectStoreData(id))",
            "INSERT INTO IndexData2 SELECT * FROM IndexData",
            "DROP TABLE IndexData",
            "ALTER TABLE IndexData2 RENAME TO IndexData",
            "CREATE INDEX IndexData_composit ON IndexData(keyString, keyDate, keyNumber, indexId)",
            "CREATE INDEX IndexData_objectStoreDataId ON IndexData(objectStoreDataId)",
            "CREATE INDEX IndexData_indexId ON IndexData(indexId)",
            "UPDATE MetaData SET value = 2 WHERE name = 'version'",
        };

        if (!runCommands(sqliteDatabase, commands, sizeof(commands) / sizeof(commands[0])))
            return false;

        databaseVersion = 2;
    }

    if (databaseVersion == 2) {
        // We need to make the ObjectStoreData.value be a BLOB instead of TEXT.
        static const char* commands[] = {
            "DROP TABLE IF EXISTS ObjectStoreData", // This drops associated indices.
            "CREATE TABLE ObjectStoreData (id INTEGER PRIMARY KEY, objectStoreId INTEGER NOT NULL REFERENCES ObjectStore(id), keyString TEXT, keyDate REAL, keyNumber REAL, value BLOB NOT NULL)",
            "CREATE UNIQUE INDEX ObjectStoreData_composit ON ObjectStoreData(keyString, keyDate, keyNumber, objectStoreId)",
            "UPDATE MetaData SET value = 3 WHERE name = 'version'",
        };

        if (!runCommands(sqliteDatabase, commands, sizeof(commands) / sizeof(commands[0])))
            return false;

        databaseVersion = 3;
    }

    return true;
}

PassRefPtr<IDBBackingStore> IDBSQLiteBackingStore::open(SecurityOrigin* securityOrigin, const String& pathBase, int64_t maximumSize, const String& fileIdentifier, IDBFactoryBackendImpl* factory)
{
    RefPtr<IDBSQLiteBackingStore> backingStore(adoptRef(new IDBSQLiteBackingStore(fileIdentifier, factory)));

    String path = ":memory:";
    if (!pathBase.isEmpty()) {
        if (!makeAllDirectories(pathBase)) {
            // FIXME: Is there any other thing we could possibly do to recover at this point? If so, do it rather than just erroring out.
            LOG_ERROR("Unable to create Indexed DB database path %s", pathBase.utf8().data());
            return 0;
        }
        path = pathByAppendingComponent(pathBase, securityOrigin->databaseIdentifier() + ".indexeddb");
    }

    if (!backingStore->m_db.open(path)) {
        // FIXME: Is there any other thing we could possibly do to recover at this point? If so, do it rather than just erroring out.
        LOG_ERROR("Failed to open database file %s for IndexedDB", path.utf8().data());
        return 0;
    }

    // FIXME: Error checking?
    backingStore->m_db.setMaximumSize(maximumSize);
    backingStore->m_db.turnOnIncrementalAutoVacuum();

    if (!createTables(backingStore->m_db))
        return 0;
    if (!migrateDatabase(backingStore->m_db))
        return 0;

    return backingStore.release();
}

bool IDBSQLiteBackingStore::extractIDBDatabaseMetaData(const String& name, String& foundVersion, int64_t& foundId)
{
    SQLiteStatement databaseQuery(m_db, "SELECT id, version FROM Databases WHERE name = ?");
    if (databaseQuery.prepare() != SQLResultOk) {
        ASSERT_NOT_REACHED();
        return false;
    }
    databaseQuery.bindText(1, name);
    if (databaseQuery.step() != SQLResultRow)
        return false;

    foundId = databaseQuery.getColumnInt64(0);
    foundVersion = databaseQuery.getColumnText(1);

    if (databaseQuery.step() == SQLResultRow)
        ASSERT_NOT_REACHED();
    return true;
}

bool IDBSQLiteBackingStore::setIDBDatabaseMetaData(const String& name, const String& version, int64_t& rowId, bool invalidRowId)
{
    ASSERT(!name.isNull());
    ASSERT(!version.isNull());

    String sql = invalidRowId ? "INSERT INTO Databases (name, description, version) VALUES (?, '', ?)" : "UPDATE Databases SET name = ?, version = ? WHERE id = ?";
    SQLiteStatement query(m_db, sql);
    if (query.prepare() != SQLResultOk) {
        ASSERT_NOT_REACHED();
        return false;
    }

    query.bindText(1, name);
    query.bindText(2, version);
    if (!invalidRowId)
        query.bindInt64(3, rowId);

    if (query.step() != SQLResultDone)
        return false;

    if (invalidRowId)
        rowId = m_db.lastInsertRowID();

    return true;
}

void IDBSQLiteBackingStore::getObjectStores(int64_t databaseId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundAutoIncrementFlags)
{
    SQLiteStatement query(m_db, "SELECT id, name, keyPath, doAutoIncrement FROM ObjectStores WHERE databaseId = ?");
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    ASSERT(foundIds.isEmpty());
    ASSERT(foundNames.isEmpty());
    ASSERT(foundKeyPaths.isEmpty());
    ASSERT(foundAutoIncrementFlags.isEmpty());

    query.bindInt64(1, databaseId);

    while (query.step() == SQLResultRow) {
        foundIds.append(query.getColumnInt64(0));
        foundNames.append(query.getColumnText(1));
        foundKeyPaths.append(query.getColumnText(2));
        foundAutoIncrementFlags.append(!!query.getColumnInt(3));
    }
}

bool IDBSQLiteBackingStore::createObjectStore(int64_t databaseId, const String& name, const String& keyPath, bool autoIncrement, int64_t& assignedObjectStoreId)
{
    SQLiteStatement query(m_db, "INSERT INTO ObjectStores (name, keyPath, doAutoIncrement, databaseId) VALUES (?, ?, ?, ?)");
    if (query.prepare() != SQLResultOk)
        return false;

    query.bindText(1, name);
    query.bindText(2, keyPath);
    query.bindInt(3, static_cast<int>(autoIncrement));
    query.bindInt64(4, databaseId);

    if (query.step() != SQLResultDone)
        return false;

    assignedObjectStoreId = m_db.lastInsertRowID();
    return true;
}

static void doDelete(SQLiteDatabase& db, const char* sql, int64_t id)
{
    SQLiteStatement deleteQuery(db, sql);
    bool ok = deleteQuery.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling.
    deleteQuery.bindInt64(1, id);
    ok = deleteQuery.step() == SQLResultDone;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling.
}

void IDBSQLiteBackingStore::deleteObjectStore(int64_t, int64_t objectStoreId)
{
    doDelete(m_db, "DELETE FROM ObjectStores WHERE id = ?", objectStoreId);
    doDelete(m_db, "DELETE FROM ObjectStoreData WHERE objectStoreId = ?", objectStoreId);
    doDelete(m_db, "DELETE FROM IndexData WHERE indexId IN (SELECT id FROM Indexes WHERE objectStoreId = ?)", objectStoreId);
    doDelete(m_db, "DELETE FROM Indexes WHERE objectStoreId = ?", objectStoreId);
}

namespace {
class SQLiteRecordIdentifier : public IDBBackingStore::ObjectStoreRecordIdentifier {
public:
    static PassRefPtr<SQLiteRecordIdentifier> create() { return adoptRef(new SQLiteRecordIdentifier()); }
    static PassRefPtr<SQLiteRecordIdentifier> create(int64_t id) { return adoptRef(new SQLiteRecordIdentifier(id)); }
    virtual bool isValid() const { return m_id != -1; }
    int64_t id() const { return m_id; }
    void setId(int64_t id) { m_id = id; }
private:
    SQLiteRecordIdentifier() : m_id(-1) { }
    SQLiteRecordIdentifier(int64_t id) : m_id(id) { }
    int64_t m_id;
};
}

PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> IDBSQLiteBackingStore::createInvalidRecordIdentifier()
{
    return SQLiteRecordIdentifier::create();
}

static String whereSyntaxForKey(const IDBKey& key, String qualifiedTableName = "")
{
    switch (key.type()) {
    case IDBKey::StringType:
        return qualifiedTableName + "keyString = ?  AND  " + qualifiedTableName + "keyDate IS NULL  AND  " + qualifiedTableName + "keyNumber IS NULL  ";
    case IDBKey::NumberType:
        return qualifiedTableName + "keyString IS NULL  AND  " + qualifiedTableName + "keyDate IS NULL  AND  " + qualifiedTableName + "keyNumber = ?  ";
    case IDBKey::DateType:
        return qualifiedTableName + "keyString IS NULL  AND  " + qualifiedTableName + "keyDate = ?  AND  " + qualifiedTableName + "keyNumber IS NULL  ";
    case IDBKey::NullType:
        return qualifiedTableName + "keyString IS NULL  AND  " + qualifiedTableName + "keyDate IS NULL  AND  " + qualifiedTableName + "keyNumber IS NULL  ";
    }

    ASSERT_NOT_REACHED();
    return "";
}

// Returns the number of items bound.
static int bindKeyToQuery(SQLiteStatement& query, int column, const IDBKey& key)
{
    switch (key.type()) {
    case IDBKey::StringType:
        query.bindText(column, key.string());
        return 1;
    case IDBKey::DateType:
        query.bindDouble(column, key.date());
        return 1;
    case IDBKey::NumberType:
        query.bindDouble(column, key.number());
        return 1;
    case IDBKey::NullType:
        return 0;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

static String lowerCursorWhereFragment(const IDBKey& key, String comparisonOperator, String qualifiedTableName = "")
{
    switch (key.type()) {
    case IDBKey::StringType:
        return "? " + comparisonOperator + " " + qualifiedTableName + "keyString  AND  ";
    case IDBKey::DateType:
        return "(? " + comparisonOperator + " " + qualifiedTableName + "keyDate  OR NOT " + qualifiedTableName + "keyString IS NULL)  AND  ";
    case IDBKey::NumberType:
        return "(? " + comparisonOperator + " " + qualifiedTableName + "keyNumber  OR  NOT " + qualifiedTableName + "keyString IS NULL  OR  NOT " + qualifiedTableName + "keyDate IS NULL)  AND  ";
    case IDBKey::NullType:
        if (comparisonOperator == "<")
            return "NOT(" + qualifiedTableName + "keyString IS NULL  AND  " + qualifiedTableName + "keyDate IS NULL  AND  " + qualifiedTableName + "keyNumber IS NULL)  AND  ";
        return ""; // If it's =, the upper bound half will do the constraining. If it's <=, then that's a no-op.
    }
    ASSERT_NOT_REACHED();
    return "";
}

static String upperCursorWhereFragment(const IDBKey& key, String comparisonOperator, String qualifiedTableName = "")
{
    switch (key.type()) {
    case IDBKey::StringType:
        return "(" + qualifiedTableName + "keyString " + comparisonOperator + " ?  OR  " + qualifiedTableName + "keyString IS NULL)  AND  ";
    case IDBKey::DateType:
        return "(" + qualifiedTableName + "keyDate " + comparisonOperator + " ? OR " + qualifiedTableName + "keyDate IS NULL)  AND  " + qualifiedTableName + "keyString IS NULL  AND  ";
    case IDBKey::NumberType:
        return "(" + qualifiedTableName + "keyNumber " + comparisonOperator + " ? OR " + qualifiedTableName + "keyNumber IS NULL)  AND  " + qualifiedTableName + "keyString IS NULL  AND  " + qualifiedTableName + "keyDate IS NULL  AND  ";
    case IDBKey::NullType:
        if (comparisonOperator == "<")
            return "0 != 0  AND  ";
        return qualifiedTableName + "keyString IS NULL  AND  " + qualifiedTableName + "keyDate IS NULL  AND  " + qualifiedTableName + "keyNumber IS NULL  AND  ";
    }
    ASSERT_NOT_REACHED();
    return "";
}

String IDBSQLiteBackingStore::getObjectStoreRecord(int64_t, int64_t objectStoreId, const IDBKey& key)
{
    SQLiteStatement query(m_db, "SELECT keyString, keyDate, keyNumber, value FROM ObjectStoreData WHERE objectStoreId = ? AND " + whereSyntaxForKey(key));
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    query.bindInt64(1, objectStoreId);
    bindKeyToQuery(query, 2, key);
    if (query.step() != SQLResultRow)
        return String(); // Null String means record not found.

    ASSERT((key.type() == IDBKey::StringType) != query.isColumnNull(0));
    ASSERT((key.type() == IDBKey::DateType) != query.isColumnNull(1));
    ASSERT((key.type() == IDBKey::NumberType) != query.isColumnNull(2));

    String record = query.getColumnBlobAsString(3);
    ASSERT(query.step() != SQLResultRow);

    return record;
}

static void bindKeyToQueryWithNulls(SQLiteStatement& query, int baseColumn, const IDBKey& key)
{
    switch (key.type()) {
    case IDBKey::StringType:
        query.bindText(baseColumn + 0, key.string());
        query.bindNull(baseColumn + 1);
        query.bindNull(baseColumn + 2);
        break;
    case IDBKey::DateType:
        query.bindNull(baseColumn + 0);
        query.bindDouble(baseColumn + 1, key.date());
        query.bindNull(baseColumn + 2);
        break;
    case IDBKey::NumberType:
        query.bindNull(baseColumn + 0);
        query.bindNull(baseColumn + 1);
        query.bindDouble(baseColumn + 2, key.number());
        break;
    case IDBKey::NullType:
        query.bindNull(baseColumn + 0);
        query.bindNull(baseColumn + 1);
        query.bindNull(baseColumn + 2);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

bool IDBSQLiteBackingStore::putObjectStoreRecord(int64_t, int64_t objectStoreId, const IDBKey& key, const String& value, ObjectStoreRecordIdentifier* recordIdentifier)
{
    SQLiteRecordIdentifier* sqliteRecordIdentifier = static_cast<SQLiteRecordIdentifier*>(recordIdentifier);

    String sql = sqliteRecordIdentifier->isValid() ? "UPDATE ObjectStoreData SET keyString = ?, keyDate = ?, keyNumber = ?, value = ? WHERE id = ?"
                                                   : "INSERT INTO ObjectStoreData (keyString, keyDate, keyNumber, value, objectStoreId) VALUES (?, ?, ?, ?, ?)";
    SQLiteStatement query(m_db, sql);
    if (query.prepare() != SQLResultOk)
        return false;

    bindKeyToQueryWithNulls(query, 1, key);
    query.bindBlob(4, value);
    if (sqliteRecordIdentifier->isValid())
        query.bindInt64(5, sqliteRecordIdentifier->id());
    else
        query.bindInt64(5, objectStoreId);

    if (query.step() != SQLResultDone)
        return false;

    if (!sqliteRecordIdentifier->isValid())
        sqliteRecordIdentifier->setId(m_db.lastInsertRowID());

    return true;
}

void IDBSQLiteBackingStore::clearObjectStore(int64_t, int64_t objectStoreId)
{
    doDelete(m_db, "DELETE FROM IndexData WHERE objectStoreDataId IN (SELECT id FROM ObjectStoreData WHERE objectStoreId = ?)", objectStoreId);
    doDelete(m_db, "DELETE FROM ObjectStoreData WHERE objectStoreId = ?", objectStoreId);
}

void IDBSQLiteBackingStore::deleteObjectStoreRecord(int64_t, int64_t objectStoreId, const ObjectStoreRecordIdentifier* recordIdentifier)
{
    const SQLiteRecordIdentifier* sqliteRecordIdentifier = static_cast<const SQLiteRecordIdentifier*>(recordIdentifier);
    ASSERT(sqliteRecordIdentifier->isValid());

    SQLiteStatement osQuery(m_db, "DELETE FROM ObjectStoreData WHERE id = ?");
    bool ok = osQuery.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    osQuery.bindInt64(1, sqliteRecordIdentifier->id());

    ok = osQuery.step() == SQLResultDone;
    ASSERT_UNUSED(ok, ok);
}

double IDBSQLiteBackingStore::nextAutoIncrementNumber(int64_t, int64_t objectStoreId)
{
    SQLiteStatement query(m_db, "SELECT max(keyNumber) + 1 FROM ObjectStoreData WHERE objectStoreId = ? AND keyString IS NULL AND keyDate IS NULL");
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok);

    query.bindInt64(1, objectStoreId);

    if (query.step() != SQLResultRow || query.isColumnNull(0))
        return 1;

    return query.getColumnDouble(0);
}

bool IDBSQLiteBackingStore::keyExistsInObjectStore(int64_t, int64_t objectStoreId, const IDBKey& key, ObjectStoreRecordIdentifier* foundRecordIdentifier)
{
    SQLiteRecordIdentifier* sqliteRecordIdentifier = static_cast<SQLiteRecordIdentifier*>(foundRecordIdentifier);

    String sql = String("SELECT id FROM ObjectStoreData WHERE objectStoreId = ? AND ") + whereSyntaxForKey(key);
    SQLiteStatement query(m_db, sql);
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    query.bindInt64(1, objectStoreId);
    bindKeyToQuery(query, 2, key);

    if (query.step() != SQLResultRow)
        return false;

    sqliteRecordIdentifier->setId(query.getColumnInt64(0));
    return true;
}

bool IDBSQLiteBackingStore::forEachObjectStoreRecord(int64_t, int64_t objectStoreId, ObjectStoreRecordCallback& callback)
{
    SQLiteStatement query(m_db, "SELECT id, value FROM ObjectStoreData WHERE objectStoreId = ?");
    if (query.prepare() != SQLResultOk)
        return false;

    query.bindInt64(1, objectStoreId);

    while (query.step() == SQLResultRow) {
        int64_t objectStoreDataId = query.getColumnInt64(0);
        String value = query.getColumnBlobAsString(1);
        RefPtr<SQLiteRecordIdentifier> recordIdentifier = SQLiteRecordIdentifier::create(objectStoreDataId);
        if (!callback.callback(recordIdentifier.get(), value))
            return false;
    }

    return true;
}

void IDBSQLiteBackingStore::getIndexes(int64_t, int64_t objectStoreId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundUniqueFlags)
{
    SQLiteStatement query(m_db, "SELECT id, name, keyPath, isUnique FROM Indexes WHERE objectStoreId = ?");
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    ASSERT(foundIds.isEmpty());
    ASSERT(foundNames.isEmpty());
    ASSERT(foundKeyPaths.isEmpty());
    ASSERT(foundUniqueFlags.isEmpty());

    query.bindInt64(1, objectStoreId);

    while (query.step() == SQLResultRow) {
        foundIds.append(query.getColumnInt64(0));
        foundNames.append(query.getColumnText(1));
        foundKeyPaths.append(query.getColumnText(2));
        foundUniqueFlags.append(!!query.getColumnInt(3));
    }
}

bool IDBSQLiteBackingStore::createIndex(int64_t, int64_t objectStoreId, const String& name, const String& keyPath, bool isUnique, int64_t& indexId)
{
    SQLiteStatement query(m_db, "INSERT INTO Indexes (objectStoreId, name, keyPath, isUnique) VALUES (?, ?, ?, ?)");
    if (query.prepare() != SQLResultOk)
        return false;

    query.bindInt64(1, objectStoreId);
    query.bindText(2, name);
    query.bindText(3, keyPath);
    query.bindInt(4, static_cast<int>(isUnique));

    if (query.step() != SQLResultDone)
        return false;

    indexId = m_db.lastInsertRowID();
    return true;
}

void IDBSQLiteBackingStore::deleteIndex(int64_t, int64_t, int64_t indexId)
{
    doDelete(m_db, "DELETE FROM Indexes WHERE id = ?", indexId);
    doDelete(m_db, "DELETE FROM IndexData WHERE indexId = ?", indexId);
}

bool IDBSQLiteBackingStore::putIndexDataForRecord(int64_t, int64_t, int64_t indexId, const IDBKey& key, const ObjectStoreRecordIdentifier* recordIdentifier)
{
    const SQLiteRecordIdentifier* sqliteRecordIdentifier = static_cast<const SQLiteRecordIdentifier*>(recordIdentifier);

    SQLiteStatement query(m_db, "INSERT INTO IndexData (keyString, keyDate, keyNumber, indexId, objectStoreDataId) VALUES (?, ?, ?, ?, ?)");
    if (query.prepare() != SQLResultOk)
        return false;

    bindKeyToQueryWithNulls(query, 1, key);
    query.bindInt64(4, indexId);
    query.bindInt64(5, sqliteRecordIdentifier->id());

    return query.step() == SQLResultDone;
}

bool IDBSQLiteBackingStore::deleteIndexDataForRecord(int64_t, int64_t, int64_t indexId, const ObjectStoreRecordIdentifier* recordIdentifier)
{
    const SQLiteRecordIdentifier* sqliteRecordIdentifier = static_cast<const SQLiteRecordIdentifier*>(recordIdentifier);

    SQLiteStatement query(m_db, "DELETE FROM IndexData WHERE objectStoreDataId = ? AND indexId = ?");
    if (query.prepare() != SQLResultOk)
        return false;

    query.bindInt64(1, sqliteRecordIdentifier->id());
    query.bindInt64(2, indexId);
    return query.step() == SQLResultDone;
}

String IDBSQLiteBackingStore::getObjectViaIndex(int64_t, int64_t, int64_t indexId, const IDBKey& key)
{
    String sql = String("SELECT ")
                 + "ObjectStoreData.value "
                 + "FROM IndexData INNER JOIN ObjectStoreData ON IndexData.objectStoreDataId = ObjectStoreData.id "
                 + "WHERE IndexData.indexId = ?  AND  " + whereSyntaxForKey(key, "IndexData.")
                 + "ORDER BY IndexData.id LIMIT 1"; // Order by insertion order when all else fails.
    SQLiteStatement query(m_db, sql);
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    query.bindInt64(1, indexId);
    bindKeyToQuery(query, 2, key);

    if (query.step() != SQLResultRow)
        return String();

    String foundValue = query.getColumnBlobAsString(0);
    ASSERT(query.step() != SQLResultRow);
    return foundValue;
}

static PassRefPtr<IDBKey> keyFromQuery(SQLiteStatement& query, int baseColumn)
{
    if (query.columnCount() <= baseColumn)
        return 0;

    if (!query.isColumnNull(baseColumn))
        return IDBKey::createString(query.getColumnText(baseColumn));

    if (!query.isColumnNull(baseColumn + 1))
        return IDBKey::createDate(query.getColumnDouble(baseColumn + 1));

    if (!query.isColumnNull(baseColumn + 2))
        return IDBKey::createNumber(query.getColumnDouble(baseColumn + 2));

    return IDBKey::createNull();
}

PassRefPtr<IDBKey> IDBSQLiteBackingStore::getPrimaryKeyViaIndex(int64_t, int64_t, int64_t indexId, const IDBKey& key)
{
    String sql = String("SELECT ")
                 + "ObjectStoreData.keyString, ObjectStoreData.keyDate, ObjectStoreData.keyNumber "
                 + "FROM IndexData INNER JOIN ObjectStoreData ON IndexData.objectStoreDataId = ObjectStoreData.id "
                 + "WHERE IndexData.indexId = ?  AND  " + whereSyntaxForKey(key, "IndexData.")
                 + "ORDER BY IndexData.id LIMIT 1"; // Order by insertion order when all else fails.
    SQLiteStatement query(m_db, sql);
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    query.bindInt64(1, indexId);
    bindKeyToQuery(query, 2, key);

    if (query.step() != SQLResultRow)
        return 0;

    RefPtr<IDBKey> foundKey = keyFromQuery(query, 0);
    ASSERT(query.step() != SQLResultRow);
    return foundKey.release();
}

bool IDBSQLiteBackingStore::keyExistsInIndex(int64_t, int64_t, int64_t indexId, const IDBKey& key)
{
    String sql = String("SELECT id FROM IndexData WHERE indexId = ? AND ") + whereSyntaxForKey(key);
    SQLiteStatement query(m_db, sql);
    bool ok = query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    query.bindInt64(1, indexId);
    bindKeyToQuery(query, 2, key);

    return query.step() == SQLResultRow;
}

namespace {

class CursorImplCommon : public IDBSQLiteBackingStore::Cursor {
public:
    CursorImplCommon(SQLiteDatabase& sqliteDatabase, String query, bool uniquenessConstraint, bool iterateForward)
        : m_query(sqliteDatabase, query)
        , m_db(sqliteDatabase)
        , m_uniquenessConstraint(uniquenessConstraint)
        , m_iterateForward(iterateForward)
    {
    }
    virtual ~CursorImplCommon() {}

    // IDBBackingStore::Cursor
    virtual bool continueFunction(const IDBKey*);
    virtual PassRefPtr<IDBKey> key() { return m_currentKey; }
    virtual PassRefPtr<IDBKey> primaryKey() { return m_currentKey; }
    virtual String value() = 0;
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() = 0;
    virtual int64_t indexDataId() = 0;

    virtual void loadCurrentRow() = 0;
    virtual bool currentRowExists() = 0;

    SQLiteStatement m_query;

protected:
    SQLiteDatabase& m_db;
    bool m_uniquenessConstraint;
    bool m_iterateForward;
    int64_t m_currentId;
    RefPtr<IDBKey> m_currentKey;
};

bool CursorImplCommon::continueFunction(const IDBKey* key)
{
    while (true) {
        if (m_query.step() != SQLResultRow)
            return false;

        RefPtr<IDBKey> oldKey = m_currentKey;
        loadCurrentRow();

        // Skip if this entry has been deleted from the object store.
        if (!currentRowExists())
            continue;

        // If a key was supplied, we must loop until we find a key greater than or equal to it (or hit the end).
        if (key) {
            if (m_iterateForward) {
                if (m_currentKey->isLessThan(key))
                    continue;
            } else {
                if (key->isLessThan(m_currentKey.get()))
                    continue;
            }
        }

        // If we don't have a uniqueness constraint, we can stop now.
        if (!m_uniquenessConstraint)
            break;
        if (!m_currentKey->isEqual(oldKey.get()))
            break;
    }

    return true;
}

class ObjectStoreCursorImpl : public CursorImplCommon {
public:
    ObjectStoreCursorImpl(SQLiteDatabase& sqliteDatabase, String query, bool uniquenessConstraint, bool iterateForward)
        : CursorImplCommon(sqliteDatabase, query, uniquenessConstraint, iterateForward)
    {
    }

    // CursorImplCommon.
    virtual String value() { return m_currentValue; }
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() { return SQLiteRecordIdentifier::create(m_currentId); }
    virtual int64_t indexDataId() { ASSERT_NOT_REACHED(); return 0; }
    virtual void loadCurrentRow();
    virtual bool currentRowExists();

private:
    String m_currentValue;
};

void ObjectStoreCursorImpl::loadCurrentRow()
{
    m_currentId = m_query.getColumnInt64(0);
    m_currentKey = keyFromQuery(m_query, 1);
    m_currentValue = m_query.getColumnBlobAsString(4);
}

bool ObjectStoreCursorImpl::currentRowExists()
{
    String sql = "SELECT id FROM ObjectStoreData WHERE id = ?";
    SQLiteStatement statement(m_db, sql);

    bool ok = statement.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok);

    statement.bindInt64(1, m_currentId);
    return statement.step() == SQLResultRow;
}

class IndexKeyCursorImpl : public CursorImplCommon {
public:
    IndexKeyCursorImpl(SQLiteDatabase& sqliteDatabase, String query, bool uniquenessConstraint, bool iterateForward)
        : CursorImplCommon(sqliteDatabase, query, uniquenessConstraint, iterateForward)
    {
    }

    // CursorImplCommon
    virtual PassRefPtr<IDBKey> primaryKey() { return m_currentPrimaryKey; }
    virtual String value() { ASSERT_NOT_REACHED(); return String(); }
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() { ASSERT_NOT_REACHED(); return 0; }
    virtual int64_t indexDataId() { return m_currentId; }
    virtual void loadCurrentRow();
    virtual bool currentRowExists();

private:
    RefPtr<IDBKey> m_currentPrimaryKey;
};

void IndexKeyCursorImpl::loadCurrentRow()
{
    m_currentId = m_query.getColumnInt64(0);
    m_currentKey = keyFromQuery(m_query, 1);
    m_currentPrimaryKey = keyFromQuery(m_query, 4);
}

bool IndexKeyCursorImpl::currentRowExists()
{
    String sql = "SELECT id FROM IndexData WHERE id = ?";
    SQLiteStatement statement(m_db, sql);

    bool ok = statement.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok);

    statement.bindInt64(1, m_currentId);
    return statement.step() == SQLResultRow;
}

class IndexCursorImpl : public CursorImplCommon {
public:
    IndexCursorImpl(SQLiteDatabase& sqliteDatabase, String query, bool uniquenessConstraint, bool iterateForward)
        : CursorImplCommon(sqliteDatabase, query, uniquenessConstraint, iterateForward)
    {
    }

    // CursorImplCommon
    virtual PassRefPtr<IDBKey> primaryKey() { return m_currentPrimaryKey; }
    virtual String value() { return m_currentValue; }
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() { ASSERT_NOT_REACHED(); return 0; }
    virtual int64_t indexDataId() { return m_currentId; }
    virtual void loadCurrentRow();
    virtual bool currentRowExists();

private:
    RefPtr<IDBKey> m_currentPrimaryKey;
    String m_currentValue;
};

void IndexCursorImpl::loadCurrentRow()
{
    m_currentId = m_query.getColumnInt64(0);
    m_currentKey = keyFromQuery(m_query, 1);
    m_currentValue = m_query.getColumnBlobAsString(4);
    m_currentPrimaryKey = keyFromQuery(m_query, 5);
}

bool IndexCursorImpl::currentRowExists()
{
    String sql = "SELECT id FROM IndexData WHERE id = ?";
    SQLiteStatement statement(m_db, sql);

    bool ok = statement.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok);

    statement.bindInt64(1, m_currentId);
    return statement.step() == SQLResultRow;
}

} // namespace

PassRefPtr<IDBBackingStore::Cursor> IDBSQLiteBackingStore::openObjectStoreCursor(int64_t, int64_t objectStoreId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();

    String sql = "SELECT id, keyString, keyDate, keyNumber, value FROM ObjectStoreData WHERE ";
    if (lowerBound)
        sql += lowerCursorWhereFragment(*range->lower(), range->lowerOpen() ? "<" : "<=");
    if (upperBound)
        sql += upperCursorWhereFragment(*range->upper(), range->upperOpen() ? "<" : "<=");
    sql += "objectStoreId = ? ORDER BY ";

    if (direction == IDBCursor::NEXT || direction == IDBCursor::NEXT_NO_DUPLICATE)
        sql += "keyString, keyDate, keyNumber";
    else
        sql += "keyString DESC, keyDate DESC, keyNumber DESC";

    RefPtr<ObjectStoreCursorImpl> cursor = adoptRef(new ObjectStoreCursorImpl(m_db, sql, direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::PREV_NO_DUPLICATE,
                                                                              direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT));

    bool ok = cursor->m_query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    int currentColumn = 1;
    if (lowerBound)
        currentColumn += bindKeyToQuery(cursor->m_query, currentColumn, *range->lower());
    if (upperBound)
        currentColumn += bindKeyToQuery(cursor->m_query, currentColumn, *range->upper());
    cursor->m_query.bindInt64(currentColumn, objectStoreId);

    if (cursor->m_query.step() != SQLResultRow)
        return 0;

    cursor->loadCurrentRow();
    return cursor.release();
}

PassRefPtr<IDBBackingStore::Cursor> IDBSQLiteBackingStore::openIndexKeyCursor(int64_t, int64_t, int64_t indexId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    String sql = String("SELECT IndexData.id, IndexData.keyString, IndexData.keyDate, IndexData.keyNumber, ")
                 + ("ObjectStoreData.keyString, ObjectStoreData.keyDate, ObjectStoreData.keyNumber ")
                 + "FROM IndexData INNER JOIN ObjectStoreData ON IndexData.objectStoreDataId = ObjectStoreData.id WHERE ";

    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();

    if (lowerBound)
        sql += lowerCursorWhereFragment(*range->lower(), range->lowerOpen() ? "<" : "<=", "IndexData.");
    if (upperBound)
        sql += upperCursorWhereFragment(*range->upper(), range->upperOpen() ? "<" : "<=", "IndexData.");
    sql += "IndexData.indexId = ? ORDER BY ";

    if (direction == IDBCursor::NEXT || direction == IDBCursor::NEXT_NO_DUPLICATE)
        sql += "IndexData.keyString, IndexData.keyDate, IndexData.keyNumber, IndexData.id";
    else
        sql += "IndexData.keyString DESC, IndexData.keyDate DESC, IndexData.keyNumber DESC, IndexData.id DESC";

    RefPtr<IndexKeyCursorImpl> cursor = adoptRef(new IndexKeyCursorImpl(m_db, sql, direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::PREV_NO_DUPLICATE,
                                                                        direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT));

    bool ok = cursor->m_query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    int indexColumn = 1;
    if (lowerBound)
        indexColumn += bindKeyToQuery(cursor->m_query, indexColumn, *range->lower());
    if (upperBound)
        indexColumn += bindKeyToQuery(cursor->m_query, indexColumn, *range->upper());
    cursor->m_query.bindInt64(indexColumn, indexId);

    if (cursor->m_query.step() != SQLResultRow)
        return 0;

    cursor->loadCurrentRow();
    return cursor.release();
}

PassRefPtr<IDBBackingStore::Cursor> IDBSQLiteBackingStore::openIndexCursor(int64_t, int64_t, int64_t indexId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    String sql = String("SELECT IndexData.id, IndexData.keyString, IndexData.keyDate, IndexData.keyNumber, ")
                 + ("ObjectStoreData.value, ObjectStoreData.keyString, ObjectStoreData.keyDate, ObjectStoreData.keyNumber ")
                 + "FROM IndexData INNER JOIN ObjectStoreData ON IndexData.objectStoreDataId = ObjectStoreData.id WHERE ";

    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();

    if (lowerBound)
        sql += lowerCursorWhereFragment(*range->lower(), range->lowerOpen() ? "<" : "<=", "IndexData.");
    if (upperBound)
        sql += upperCursorWhereFragment(*range->upper(), range->upperOpen() ? "<" : "<=", "IndexData.");
    sql += "IndexData.indexId = ? ORDER BY ";

    if (direction == IDBCursor::NEXT || direction == IDBCursor::NEXT_NO_DUPLICATE)
        sql += "IndexData.keyString, IndexData.keyDate, IndexData.keyNumber, IndexData.id";
    else
        sql += "IndexData.keyString DESC, IndexData.keyDate DESC, IndexData.keyNumber DESC, IndexData.id DESC";

    RefPtr<IndexCursorImpl> cursor = adoptRef(new IndexCursorImpl(m_db, sql, direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::PREV_NO_DUPLICATE,
                                                                  direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT));

    bool ok = cursor->m_query.prepare() == SQLResultOk;
    ASSERT_UNUSED(ok, ok); // FIXME: Better error handling?

    int indexColumn = 1;
    if (lowerBound)
        indexColumn += bindKeyToQuery(cursor->m_query, indexColumn, *range->lower());
    if (upperBound)
        indexColumn += bindKeyToQuery(cursor->m_query, indexColumn, *range->upper());
    cursor->m_query.bindInt64(indexColumn, indexId);

    if (cursor->m_query.step() != SQLResultRow)
        return 0;

    cursor->loadCurrentRow();
    return cursor.release();
}

namespace {

class TransactionImpl : public IDBBackingStore::Transaction {
public:
    TransactionImpl(SQLiteDatabase& db)
        : m_transaction(db)
    {
    }

    // IDBBackingStore::Transaction
    virtual void begin() { m_transaction.begin(); }
    virtual void commit() { m_transaction.commit(); }
    virtual void rollback() { m_transaction.rollback(); }

private:
    SQLiteTransaction m_transaction;
};

} // namespace

PassRefPtr<IDBBackingStore::Transaction> IDBSQLiteBackingStore::createTransaction()
{
    return adoptRef(new TransactionImpl(m_db));
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
