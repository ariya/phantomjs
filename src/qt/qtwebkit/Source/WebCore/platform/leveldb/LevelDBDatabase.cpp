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

#if USE(LEVELDB)

#include "HistogramSupport.h"
#include "LevelDBComparator.h"
#include "LevelDBIterator.h"
#include "LevelDBSlice.h"
#include "LevelDBWriteBatch.h"
#include "Logging.h"
#include "NotImplemented.h"
#include <leveldb/comparator.h>
#include <leveldb/db.h>
#include <leveldb/env.h>
#include <helpers/memenv/memenv.h>
#include <leveldb/slice.h>
#include <string>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace leveldb {

static Env* IDBEnv()
{
    return leveldb::Env::Default();
}

}

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
    virtual void FindShortestSeparator(std::string* /* start */, const leveldb::Slice& /* limit */) const { }
    virtual void FindShortSuccessor(std::string* /* key */) const { }

private:
    const LevelDBComparator* m_comparator;
};

LevelDBSnapshot::LevelDBSnapshot(LevelDBDatabase* db)
    : m_db(db->m_db.get())
    , m_snapshot(m_db->GetSnapshot())
{
}

LevelDBSnapshot::~LevelDBSnapshot()
{
    m_db->ReleaseSnapshot(m_snapshot);
}

LevelDBDatabase::LevelDBDatabase()
{
}

LevelDBDatabase::~LevelDBDatabase()
{
    // m_db's destructor uses m_comparatorAdapter; order of deletion is important.
    m_db.clear();
    m_comparatorAdapter.clear();
    m_env.clear();
}

static leveldb::Status openDB(leveldb::Comparator* comparator, leveldb::Env* env, const String& path, leveldb::DB** db)
{
    leveldb::Options options;
    options.comparator = comparator;
    options.create_if_missing = true;
    options.paranoid_checks = true;
    // 20 max_open_files is the minimum LevelDB allows.
    options.max_open_files = 20;
    options.env = env;

    return leveldb::DB::Open(options, path.utf8().data(), db);
}

bool LevelDBDatabase::destroy(const String& fileName)
{
    leveldb::Options options;
    options.env = leveldb::IDBEnv();
    const leveldb::Status s = leveldb::DestroyDB(fileName.utf8().data(), options);
    return s.ok();
}

static void histogramLevelDBError(const char* histogramName, const leveldb::Status& s)
{
    ASSERT(!s.ok());
    enum {
        LevelDBNotFound,
        LevelDBCorruption,
        LevelDBIOError,
        LevelDBOther,
        LevelDBMaxError
    };
    int levelDBError = LevelDBOther;
    if (s.IsNotFound())
        levelDBError = LevelDBNotFound;
    else if (s.IsCorruption())
        levelDBError = LevelDBCorruption;
    else if (s.IsIOError())
        levelDBError = LevelDBIOError;
    HistogramSupport::histogramEnumeration(histogramName, levelDBError, LevelDBMaxError);
}

PassOwnPtr<LevelDBDatabase> LevelDBDatabase::open(const String& fileName, const LevelDBComparator* comparator)
{
    OwnPtr<ComparatorAdapter> comparatorAdapter = adoptPtr(new ComparatorAdapter(comparator));

    leveldb::DB* db;
    const leveldb::Status s = openDB(comparatorAdapter.get(), leveldb::IDBEnv(), fileName, &db);

    if (!s.ok()) {
        histogramLevelDBError("WebCore.IndexedDB.LevelDBOpenErrors", s);

        LOG_ERROR("Failed to open LevelDB database from %s: %s", fileName.ascii().data(), s.ToString().c_str());
        return nullptr;
    }

    OwnPtr<LevelDBDatabase> result = adoptPtr(new LevelDBDatabase);
    result->m_db = adoptPtr(db);
    result->m_comparatorAdapter = comparatorAdapter.release();
    result->m_comparator = comparator;

    return result.release();
}

PassOwnPtr<LevelDBDatabase> LevelDBDatabase::openInMemory(const LevelDBComparator* comparator)
{
    OwnPtr<ComparatorAdapter> comparatorAdapter = adoptPtr(new ComparatorAdapter(comparator));
    OwnPtr<leveldb::Env> inMemoryEnv = adoptPtr(leveldb::NewMemEnv(leveldb::IDBEnv()));

    leveldb::DB* db;
    const leveldb::Status s = openDB(comparatorAdapter.get(), inMemoryEnv.get(), String(), &db);

    if (!s.ok()) {
        LOG_ERROR("Failed to open in-memory LevelDB database: %s", s.ToString().c_str());
        return nullptr;
    }

    OwnPtr<LevelDBDatabase> result = adoptPtr(new LevelDBDatabase);
    result->m_env = inMemoryEnv.release();
    result->m_db = adoptPtr(db);
    result->m_comparatorAdapter = comparatorAdapter.release();
    result->m_comparator = comparator;

    return result.release();
}

bool LevelDBDatabase::put(const LevelDBSlice& key, const Vector<char>& value)
{
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;

    const leveldb::Status s = m_db->Put(writeOptions, makeSlice(key), makeSlice(value));
    if (s.ok())
        return true;
    LOG_ERROR("LevelDB put failed: %s", s.ToString().c_str());
    return false;
}

bool LevelDBDatabase::remove(const LevelDBSlice& key)
{
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;

    const leveldb::Status s = m_db->Delete(writeOptions, makeSlice(key));
    if (s.ok())
        return true;
    if (s.IsNotFound())
        return false;
    LOG_ERROR("LevelDB remove failed: %s", s.ToString().c_str());
    return false;
}

bool LevelDBDatabase::safeGet(const LevelDBSlice& key, Vector<char>& value, bool& found, const LevelDBSnapshot* snapshot)
{
    found = false;
    std::string result;
    leveldb::ReadOptions readOptions;
    readOptions.verify_checksums = true; // FIXME: Disable this if the performance impact is too great.
    readOptions.snapshot = snapshot ? snapshot->m_snapshot : 0;

    const leveldb::Status s = m_db->Get(readOptions, makeSlice(key), &result);
    if (s.ok()) {
        found = true;
        value.clear();
        value.append(result.c_str(), result.length());
        return true;
    }
    if (s.IsNotFound())
        return true;
    LOG_ERROR("LevelDB get failed: %s", s.ToString().c_str());
    return false;
}

bool LevelDBDatabase::write(LevelDBWriteBatch& writeBatch)
{
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;

    const leveldb::Status s = m_db->Write(writeOptions, writeBatch.m_writeBatch.get());
    if (s.ok())
        return true;
    histogramLevelDBError("WebCore.IndexedDB.LevelDBWriteErrors", s);
    LOG_ERROR("LevelDB write failed: %s", s.ToString().c_str());
    return false;
}

namespace {
class IteratorImpl : public LevelDBIterator {
public:
    ~IteratorImpl() { };

    virtual bool isValid() const;
    virtual void seekToLast();
    virtual void seek(const LevelDBSlice& target);
    virtual void next();
    virtual void prev();
    virtual LevelDBSlice key() const;
    virtual LevelDBSlice value() const;

private:
    friend class WebCore::LevelDBDatabase;
    IteratorImpl(PassOwnPtr<leveldb::Iterator>);
    void checkStatus();

    OwnPtr<leveldb::Iterator> m_iterator;
};
}

IteratorImpl::IteratorImpl(PassOwnPtr<leveldb::Iterator> it)
    : m_iterator(it)
{
}

void IteratorImpl::checkStatus()
{
    const leveldb::Status s = m_iterator->status();
    if (!s.ok())
        LOG_ERROR("LevelDB iterator error: %s", s.ToString().c_str());
}

bool IteratorImpl::isValid() const
{
    return m_iterator->Valid();
}

void IteratorImpl::seekToLast()
{
    m_iterator->SeekToLast();
    checkStatus();
}

void IteratorImpl::seek(const LevelDBSlice& target)
{
    m_iterator->Seek(makeSlice(target));
    checkStatus();
}

void IteratorImpl::next()
{
    ASSERT(isValid());
    m_iterator->Next();
    checkStatus();
}

void IteratorImpl::prev()
{
    ASSERT(isValid());
    m_iterator->Prev();
    checkStatus();
}

LevelDBSlice IteratorImpl::key() const
{
    ASSERT(isValid());
    return makeLevelDBSlice(m_iterator->key());
}

LevelDBSlice IteratorImpl::value() const
{
    ASSERT(isValid());
    return makeLevelDBSlice(m_iterator->value());
}

PassOwnPtr<LevelDBIterator> LevelDBDatabase::createIterator(const LevelDBSnapshot* snapshot)
{
    leveldb::ReadOptions readOptions;
    readOptions.verify_checksums = true; // FIXME: Disable this if the performance impact is too great.
    readOptions.snapshot = snapshot ? snapshot->m_snapshot : 0;
    OwnPtr<leveldb::Iterator> i = adoptPtr(m_db->NewIterator(readOptions));
    if (!i) // FIXME: Double check if we actually need to check this.
        return nullptr;
    return adoptPtr(new IteratorImpl(i.release()));
}

const LevelDBComparator* LevelDBDatabase::comparator() const
{
    return m_comparator;
}

}

#endif
