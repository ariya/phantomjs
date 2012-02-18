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
#include "SQLResultSet.h"

#if ENABLE(DATABASE)

namespace WebCore {

static unsigned const MaxErrorCode = 2;

SQLResultSet::SQLResultSet()
    : m_rows(SQLResultSetRowList::create())
    , m_insertId(0)
    , m_insertIdSet(false)
    , m_rowsAffected(0)
{
}

int64_t SQLResultSet::insertId(ExceptionCode& e) const
{
    // 4.11.4 - Return the id of the last row inserted as a result of the query
    // If the query didn't result in any rows being added, raise an INVALID_ACCESS_ERR exception
    if (m_insertIdSet)
        return m_insertId;

    e = INVALID_ACCESS_ERR;
    return -1;
}

int SQLResultSet::rowsAffected() const
{
    return m_rowsAffected;
}

SQLResultSetRowList* SQLResultSet::rows() const
{
    return m_rows.get();
}

void SQLResultSet::setInsertId(int64_t id)
{
    ASSERT(!m_insertIdSet);

    m_insertId = id;
    m_insertIdSet = true;
}

void SQLResultSet::setRowsAffected(int count)
{
    m_rowsAffected = count;
}

}

#endif
