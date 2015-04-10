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

#ifndef LevelDBDatabase_h
#define LevelDBDatabase_h

#if USE(LEVELDB)

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace leveldb {
class Comparator;
class DB;
class Env;
class Snapshot;
}

namespace WebCore {

class LevelDBComparator;
class LevelDBDatabase;
class LevelDBIterator;
class LevelDBSlice;
class LevelDBWriteBatch;

class LevelDBSnapshot {
private:
    friend class LevelDBDatabase;
    friend class LevelDBTransaction;

    explicit LevelDBSnapshot(LevelDBDatabase*);
    ~LevelDBSnapshot();

    leveldb::DB* m_db;
    const leveldb::Snapshot* m_snapshot;
};

class LevelDBDatabase {
public:
    static PassOwnPtr<LevelDBDatabase> open(const String& fileName, const LevelDBComparator*);
    static PassOwnPtr<LevelDBDatabase> openInMemory(const LevelDBComparator*);
    static bool destroy(const String& fileName);
    virtual ~LevelDBDatabase();

    bool put(const LevelDBSlice& key, const Vector<char>& value);
    bool remove(const LevelDBSlice& key);
    virtual bool safeGet(const LevelDBSlice& key, Vector<char>& value, bool& found, const LevelDBSnapshot* = 0);
    bool write(LevelDBWriteBatch&);
    PassOwnPtr<LevelDBIterator> createIterator(const LevelDBSnapshot* = 0);
    const LevelDBComparator* comparator() const;

protected:
    LevelDBDatabase();

private:
    friend class LevelDBSnapshot;

    OwnPtr<leveldb::Env> m_env;
    OwnPtr<leveldb::Comparator> m_comparatorAdapter;
    OwnPtr<leveldb::DB> m_db;
    const LevelDBComparator* m_comparator;
};

}

#endif
#endif
