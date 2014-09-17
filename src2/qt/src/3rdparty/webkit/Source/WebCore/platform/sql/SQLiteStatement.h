/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef SQLiteStatement_h
#define SQLiteStatement_h

#include "SQLiteDatabase.h"

struct sqlite3_stmt;

namespace WebCore {

class SQLValue;

class SQLiteStatement {
    WTF_MAKE_NONCOPYABLE(SQLiteStatement); WTF_MAKE_FAST_ALLOCATED;
public:
    SQLiteStatement(SQLiteDatabase&, const String&);
    ~SQLiteStatement();
    
    int prepare();
    int bindBlob(int index, const void* blob, int size);
    int bindBlob(int index, const String&);
    int bindText(int index, const String&);
    int bindInt(int index, int);
    int bindInt64(int index, int64_t);
    int bindDouble(int index, double);
    int bindNull(int index);
    int bindValue(int index, const SQLValue&);
    unsigned bindParameterCount() const;

    int step();
    int finalize();
    int reset();
    
    int prepareAndStep() { if (int error = prepare()) return error; return step(); }
    
    // prepares, steps, and finalizes the query.
    // returns true if all 3 steps succeed with step() returning SQLITE_DONE
    // returns false otherwise  
    bool executeCommand();
    
    // prepares, steps, and finalizes.  
    // returns true is step() returns SQLITE_ROW
    // returns false otherwise
    bool returnsAtLeastOneResult();

    bool isExpired();

    // Returns -1 on last-step failing.  Otherwise, returns number of rows
    // returned in the last step()
    int columnCount();
    
    bool isColumnNull(int col);
    String getColumnName(int col);
    SQLValue getColumnValue(int col);
    String getColumnText(int col);
    double getColumnDouble(int col);
    int getColumnInt(int col);
    int64_t getColumnInt64(int col);
    const void* getColumnBlob(int col, int& size);
    String getColumnBlobAsString(int col);
    void getColumnBlobAsVector(int col, Vector<char>&);

    bool returnTextResults(int col, Vector<String>&);
    bool returnIntResults(int col, Vector<int>&);
    bool returnInt64Results(int col, Vector<int64_t>&);
    bool returnDoubleResults(int col, Vector<double>&);

    SQLiteDatabase* database() { return &m_database; }
    
    const String& query() const { return m_query; }
    
private:
    SQLiteDatabase& m_database;
    String m_query;
    sqlite3_stmt* m_statement;
#ifndef NDEBUG
    bool m_isPrepared;
#endif
};

} // namespace WebCore

#endif // SQLiteStatement_h
