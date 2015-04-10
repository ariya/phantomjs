/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "SQLStatementSync.h"

#if ENABLE(SQL_DATABASE)

#include "DatabaseSync.h"
#include "SQLException.h"
#include "SQLResultSet.h"
#include "SQLValue.h"
#include "SQLiteDatabase.h"
#include "SQLiteStatement.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

SQLStatementSync::SQLStatementSync(const String& statement, const Vector<SQLValue>& arguments, int permissions)
    : m_statement(statement)
    , m_arguments(arguments)
    , m_permissions(permissions)
{
    ASSERT(!m_statement.isEmpty());
}

PassRefPtr<SQLResultSet> SQLStatementSync::execute(DatabaseSync* db, ExceptionCode& ec)
{
    db->setAuthorizerPermissions(m_permissions);

    SQLiteDatabase* database = &db->sqliteDatabase();

    SQLiteStatement statement(*database, m_statement);
    int result = statement.prepare();
    if (result != SQLResultOk) {
        ec = (result == SQLResultInterrupt ? SQLException::DATABASE_ERR : SQLException::SYNTAX_ERR);
        db->setLastErrorMessage("could not prepare statement", result, database->lastErrorMsg());
        return 0;
    }

    if (statement.bindParameterCount() != m_arguments.size()) {
        ec = (db->isInterrupted()? SQLException::DATABASE_ERR : SQLException::SYNTAX_ERR);
        db->setLastErrorMessage("number of '?'s in statement string does not match argument count");
        return 0;
    }

    for (unsigned i = 0; i < m_arguments.size(); ++i) {
        result = statement.bindValue(i + 1, m_arguments[i]);
        if (result == SQLResultFull) {
            ec = SQLException::QUOTA_ERR;
            db->setLastErrorMessage("there was not enough remaining storage space");
            return 0;
        }

        if (result != SQLResultOk) {
            ec = SQLException::DATABASE_ERR;
            db->setLastErrorMessage("could not bind value", result, database->lastErrorMsg());
            return 0;
        }
    }

    RefPtr<SQLResultSet> resultSet = SQLResultSet::create();

    // Step so we can fetch the column names.
    result = statement.step();
    if (result == SQLResultRow) {
        int columnCount = statement.columnCount();
        SQLResultSetRowList* rows = resultSet->rows();

        for (int i = 0; i < columnCount; i++)
            rows->addColumn(statement.getColumnName(i));

        do {
            for (int i = 0; i < columnCount; i++)
                rows->addResult(statement.getColumnValue(i));

            result = statement.step();
        } while (result == SQLResultRow);

        if (result != SQLResultDone) {
            ec = SQLException::DATABASE_ERR;
            db->setLastErrorMessage("could not iterate results", result, database->lastErrorMsg());
            return 0;
        }
    } else if (result == SQLResultDone) {
        // Didn't find anything, or was an insert.
        if (db->lastActionWasInsert())
            resultSet->setInsertId(database->lastInsertRowID());
    } else if (result == SQLResultFull) {
        // Quota error, the delegate will be asked for more space and this statement might be re-run.
        ec = SQLException::QUOTA_ERR;
        db->setLastErrorMessage("there was not enough remaining storage space");
        return 0;
    } else if (result == SQLResultConstraint) {
        ec = SQLException::CONSTRAINT_ERR;
        db->setLastErrorMessage("statement failed due to a constraint failure");
        return 0;
    } else {
        ec = SQLException::DATABASE_ERR;
        db->setLastErrorMessage("could not execute statement", result, database->lastErrorMsg());
        return 0;
    }

    resultSet->setRowsAffected(database->lastChanges());
    return resultSet.release();
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
