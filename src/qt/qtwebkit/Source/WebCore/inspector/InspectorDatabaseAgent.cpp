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

#include "config.h"

#if ENABLE(INSPECTOR) && ENABLE(SQL_DATABASE)

#include "InspectorDatabaseAgent.h"

#include "Database.h"
#include "ExceptionCode.h"
#include "ExceptionCodePlaceholder.h"
#include "InspectorDatabaseResource.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "SQLError.h"
#include "SQLResultSet.h"
#include "SQLResultSetRowList.h"
#include "SQLStatementCallback.h"
#include "SQLStatementErrorCallback.h"
#include "SQLTransaction.h"
#include "SQLTransactionCallback.h"
#include "SQLTransactionErrorCallback.h"
#include "SQLValue.h"
#include "VoidCallback.h"

#include <wtf/Vector.h>

typedef WebCore::InspectorBackendDispatcher::DatabaseCommandHandler::ExecuteSQLCallback ExecuteSQLCallback;

namespace WebCore {

namespace DatabaseAgentState {
static const char databaseAgentEnabled[] = "databaseAgentEnabled";
};

namespace {

void reportTransactionFailed(ExecuteSQLCallback* requestCallback, SQLError* error)
{
    RefPtr<TypeBuilder::Database::Error> errorObject = TypeBuilder::Database::Error::create()
        .setMessage(error->message())
        .setCode(error->code());
    requestCallback->sendSuccess(0, 0, errorObject.release());
}

class StatementCallback : public SQLStatementCallback {
public:
    static PassRefPtr<StatementCallback> create(PassRefPtr<ExecuteSQLCallback> requestCallback)
    {
        return adoptRef(new StatementCallback(requestCallback));
    }

    virtual ~StatementCallback() { }

    virtual bool handleEvent(SQLTransaction*, SQLResultSet* resultSet)
    {
        SQLResultSetRowList* rowList = resultSet->rows();

        RefPtr<TypeBuilder::Array<String> > columnNames = TypeBuilder::Array<String>::create();
        const Vector<String>& columns = rowList->columnNames();
        for (size_t i = 0; i < columns.size(); ++i)
            columnNames->addItem(columns[i]);

        RefPtr<TypeBuilder::Array<InspectorValue> > values = TypeBuilder::Array<InspectorValue>::create();
        const Vector<SQLValue>& data = rowList->values();
        for (size_t i = 0; i < data.size(); ++i) {
            const SQLValue& value = rowList->values()[i];
            switch (value.type()) {
            case SQLValue::StringValue: values->addItem(InspectorString::create(value.string())); break;
            case SQLValue::NumberValue: values->addItem(InspectorBasicValue::create(value.number())); break;
            case SQLValue::NullValue: values->addItem(InspectorValue::null()); break;
            }
        }
        m_requestCallback->sendSuccess(columnNames.release(), values.release(), 0);
        return true;
    }

private:
    StatementCallback(PassRefPtr<ExecuteSQLCallback> requestCallback)
        : m_requestCallback(requestCallback) { }
    RefPtr<ExecuteSQLCallback> m_requestCallback;
};

class StatementErrorCallback : public SQLStatementErrorCallback {
public:
    static PassRefPtr<StatementErrorCallback> create(PassRefPtr<ExecuteSQLCallback> requestCallback)
    {
        return adoptRef(new StatementErrorCallback(requestCallback));
    }

    virtual ~StatementErrorCallback() { }

    virtual bool handleEvent(SQLTransaction*, SQLError* error)
    {
        reportTransactionFailed(m_requestCallback.get(), error);
        return true;  
    }

private:
    StatementErrorCallback(PassRefPtr<ExecuteSQLCallback> requestCallback)
        : m_requestCallback(requestCallback) { }
    RefPtr<ExecuteSQLCallback> m_requestCallback;
};

class TransactionCallback : public SQLTransactionCallback {
public:
    static PassRefPtr<TransactionCallback> create(const String& sqlStatement, PassRefPtr<ExecuteSQLCallback> requestCallback)
    {
        return adoptRef(new TransactionCallback(sqlStatement, requestCallback));
    }

    virtual ~TransactionCallback() { }

    virtual bool handleEvent(SQLTransaction* transaction)
    {
        if (!m_requestCallback->isActive())
            return true;

        Vector<SQLValue> sqlValues;
        RefPtr<SQLStatementCallback> callback(StatementCallback::create(m_requestCallback.get()));
        RefPtr<SQLStatementErrorCallback> errorCallback(StatementErrorCallback::create(m_requestCallback.get()));
        transaction->executeSQL(m_sqlStatement, sqlValues, callback.release(), errorCallback.release(), IGNORE_EXCEPTION);
        return true;
    }
private:
    TransactionCallback(const String& sqlStatement, PassRefPtr<ExecuteSQLCallback> requestCallback)
        : m_sqlStatement(sqlStatement)
        , m_requestCallback(requestCallback) { }
    String m_sqlStatement;
    RefPtr<ExecuteSQLCallback> m_requestCallback;
};

class TransactionErrorCallback : public SQLTransactionErrorCallback {
public:
    static PassRefPtr<TransactionErrorCallback> create(PassRefPtr<ExecuteSQLCallback> requestCallback)
    {
        return adoptRef(new TransactionErrorCallback(requestCallback));
    }

    virtual ~TransactionErrorCallback() { }

    virtual bool handleEvent(SQLError* error)
    {
        reportTransactionFailed(m_requestCallback.get(), error);
        return true;
    }
private:
    TransactionErrorCallback(PassRefPtr<ExecuteSQLCallback> requestCallback)
        : m_requestCallback(requestCallback) { }
    RefPtr<ExecuteSQLCallback> m_requestCallback;
};

class TransactionSuccessCallback : public VoidCallback {
public:
    static PassRefPtr<TransactionSuccessCallback> create()
    {
        return adoptRef(new TransactionSuccessCallback());
    }

    virtual ~TransactionSuccessCallback() { }

    virtual bool handleEvent() { return false; }

private:
    TransactionSuccessCallback() { }
};

} // namespace

void InspectorDatabaseAgent::didOpenDatabase(PassRefPtr<Database> database, const String& domain, const String& name, const String& version)
{
    if (InspectorDatabaseResource* resource = findByFileName(database->fileName())) {
        resource->setDatabase(database);
        return;
    }

    RefPtr<InspectorDatabaseResource> resource = InspectorDatabaseResource::create(database, domain, name, version);
    m_resources.set(resource->id(), resource);
    // Resources are only bound while visible.
    if (m_frontend && m_enabled)
        resource->bind(m_frontend);
}

void InspectorDatabaseAgent::clearResources()
{
    m_resources.clear();
}

InspectorDatabaseAgent::InspectorDatabaseAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state)
    : InspectorBaseAgent<InspectorDatabaseAgent>("Database", instrumentingAgents, state)
    , m_enabled(false)
{
    m_instrumentingAgents->setInspectorDatabaseAgent(this);
}

InspectorDatabaseAgent::~InspectorDatabaseAgent()
{
    m_instrumentingAgents->setInspectorDatabaseAgent(0);
}

void InspectorDatabaseAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->database();
}

void InspectorDatabaseAgent::clearFrontend()
{
    m_frontend = 0;
    disable(0);
}

void InspectorDatabaseAgent::enable(ErrorString*)
{
    if (m_enabled)
        return;
    m_enabled = true;
    m_state->setBoolean(DatabaseAgentState::databaseAgentEnabled, m_enabled);

    DatabaseResourcesMap::iterator databasesEnd = m_resources.end();
    for (DatabaseResourcesMap::iterator it = m_resources.begin(); it != databasesEnd; ++it)
        it->value->bind(m_frontend);
}

void InspectorDatabaseAgent::disable(ErrorString*)
{
    if (!m_enabled)
        return;
    m_enabled = false;
    m_state->setBoolean(DatabaseAgentState::databaseAgentEnabled, m_enabled);
}

void InspectorDatabaseAgent::restore()
{
    m_enabled = m_state->getBoolean(DatabaseAgentState::databaseAgentEnabled);
}

void InspectorDatabaseAgent::getDatabaseTableNames(ErrorString* error, const String& databaseId, RefPtr<TypeBuilder::Array<String> >& names)
{
    if (!m_enabled) {
        *error = "Database agent is not enabled";
        return;
    }

    names = TypeBuilder::Array<String>::create();

    Database* database = databaseForId(databaseId);
    if (database) {
        Vector<String> tableNames = database->tableNames();
        unsigned length = tableNames.size();
        for (unsigned i = 0; i < length; ++i)
            names->addItem(tableNames[i]);
    }
}

void InspectorDatabaseAgent::executeSQL(ErrorString*, const String& databaseId, const String& query, PassRefPtr<ExecuteSQLCallback> prpRequestCallback)
{
    RefPtr<ExecuteSQLCallback> requestCallback = prpRequestCallback;

    if (!m_enabled) {
        requestCallback->sendFailure("Database agent is not enabled");
        return;
    }

    Database* database = databaseForId(databaseId);
    if (!database) {
        requestCallback->sendFailure("Database not found");
        return;
    }

    RefPtr<SQLTransactionCallback> callback(TransactionCallback::create(query, requestCallback.get()));
    RefPtr<SQLTransactionErrorCallback> errorCallback(TransactionErrorCallback::create(requestCallback.get()));
    RefPtr<VoidCallback> successCallback(TransactionSuccessCallback::create());
    database->transaction(callback.release(), errorCallback.release(), successCallback.release());
}

String InspectorDatabaseAgent::databaseId(Database* database)
{
    for (DatabaseResourcesMap::iterator it = m_resources.begin(); it != m_resources.end(); ++it) {
        if (it->value->database() == database)
            return it->key;
    }
    return String();
}

InspectorDatabaseResource* InspectorDatabaseAgent::findByFileName(const String& fileName)
{
    for (DatabaseResourcesMap::iterator it = m_resources.begin(); it != m_resources.end(); ++it) {
        if (it->value->database()->fileName() == fileName)
            return it->value.get();
    }
    return 0;
}

Database* InspectorDatabaseAgent::databaseForId(const String& databaseId)
{
    DatabaseResourcesMap::iterator it = m_resources.find(databaseId);
    if (it == m_resources.end())
        return 0;
    return it->value->database();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(SQL_DATABASE)
