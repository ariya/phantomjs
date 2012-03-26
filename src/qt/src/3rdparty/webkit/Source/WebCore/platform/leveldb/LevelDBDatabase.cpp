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
#include "LevelDBDatabase.h"

#if ENABLE(LEVELDB)

#include "LevelDBComparator.h"
#include "LevelDBIterator.h"
#include "LevelDBSlice.h"
#include <leveldb/comparator.h>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <string>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static leveldb::Slice makeSlice(const Vector<char>& value)
{
    return leveldb::Slice(value.data(), value.size());
}

static leveldb::Slice makeSlice(const LevelDBSlice& s)
{
    return leveldb::Slice(s.begin(), s.end() - s.begin());
}

static LevelDBSlice makeLevelDBSlice(const leveldb::Slice& s)
{
    return LevelDBSlice(s.data(), s.data() + s.size());
}

static Vector<char> makeVector(const std::string& s)
{
    Vector<char> res;
    res.append(s.c_str(), s.length());
    return res;
}

class ComparatorAdapter : public leveldb::Comparator {
public:
    ComparatorAdapter(const LevelDBComparator* comparator)
        : m_comparator(comparator)
    {
    }

    virtual int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const
    {
        return m_comparator->compare(makeLevelDBSlice(a), makeLevelDBSlice(b));
    }

    virtual const char* Name() const { return m_comparator->name(); }

    // FIXME: Support the methods below in the future.
    virtual void FindShortestSeparator(std::string* start, const leveldb::Slice& limit) const { }
    virtual void FindShortSuccessor(std::string* key) const { }

private:
    const LevelDBComparator* m_comparator;
};

LevelDBDatabase::LevelDBDatabase()
{
}

LevelDBDatabase::~LevelDBDatabase()
{
}

PassOwnPtr<LevelDBDatabase> LevelDBDatabase::open(const String& fileName, const LevelDBComparator* comparator)
{
    OwnPtr<ComparatorAdapter> comparatorAdapter = adoptPtr(new ComparatorAdapter(comparator));

    OwnPtr<LevelDBDatabase> result = adoptPtr(new LevelDBDatabase);

    leveldb::Options options;
    options.comparator = comparatorAdapter.get();
    options.create_if_missing = true;
    leveldb::DB* db;
    leveldb::Status s = leveldb::DB::Open(options, fileName.utf8().data(), &db);

    if (!s.ok())
        return nullptr;

    result->m_db = adoptPtr(db);
    result->m_comparatorAdapter = comparatorAdapter.release();

    return result.release();
}


bool LevelDBDatabase::put(const LevelDBSlice& key, const Vector<char>& value)
{
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = false;

    return m_db->Put(writeOptions, makeSlice(key), makeSlice(value)).ok();
}

bool LevelDBDatabase::remove(const LevelDBSlice& key)
{
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = false;

    return m_db->Delete(writeOptions, makeSlice(key)).ok();
}

bool LevelDBDatabase::get(const LevelDBSlice& key, Vector<char>& value)
{
    std::string result;
    if (!m_db->Get(leveldb::ReadOptions(), makeSlice(key), &result).ok())
        return false;

    value = makeVector(result);
    return true;
}

PassOwnPtr<LevelDBIterator> LevelDBDatabase::createIterator()
{
    OwnPtr<leveldb::Iterator> i = adoptPtr(m_db->NewIterator(leveldb::ReadOptions()));
    if (!i) // FIXME: Double check if we actually need to check this.
        return nullptr;
    return adoptPtr(new LevelDBIterator(i.release()));
}

}

#endif
