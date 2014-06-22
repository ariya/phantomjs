/*
 * Copyright (C) 2007, 2013 Apple Inc. All rights reserved.
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
#ifndef SQLStatementBackend_h
#define SQLStatementBackend_h

#if ENABLE(SQL_DATABASE)

#include "AbstractSQLStatementBackend.h"
#include "SQLValue.h"
#include <wtf/Forward.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AbstractSQLStatement;
class DatabaseBackend;
class SQLError;
class SQLTransactionBackend;

class SQLStatementBackend : public AbstractSQLStatementBackend {
public:
    static PassRefPtr<SQLStatementBackend> create(PassOwnPtr<AbstractSQLStatement>,
        const String& sqlStatement, const Vector<SQLValue>& arguments, int permissions);

    bool execute(DatabaseBackend*);
    bool lastExecutionFailedDueToQuota() const;

    bool hasStatementCallback() const { return m_hasCallback; }
    bool hasStatementErrorCallback() const { return m_hasErrorCallback; }

    void setDatabaseDeletedError();
    void setVersionMismatchedError();

    AbstractSQLStatement* frontend();
    virtual PassRefPtr<SQLError> sqlError() const;
    virtual PassRefPtr<SQLResultSet> sqlResultSet() const;

private:
    SQLStatementBackend(PassOwnPtr<AbstractSQLStatement>, const String& statement,
        const Vector<SQLValue>& arguments, int permissions);

    void setFailureDueToQuota();
    void clearFailureDueToQuota();

    OwnPtr<AbstractSQLStatement> m_frontend;
    String m_statement;
    Vector<SQLValue> m_arguments;
    bool m_hasCallback;
    bool m_hasErrorCallback;

    RefPtr<SQLError> m_error;
    RefPtr<SQLResultSet> m_resultSet;

    int m_permissions;
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // SQLStatementBackend_h
