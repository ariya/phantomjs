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

#ifndef LevelDBWriteBatch_h
#define LevelDBWriteBatch_h

#if USE(LEVELDB)

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace leveldb {
class WriteBatch;
}

namespace WebCore {

class LevelDBSlice;

// Wrapper around leveldb::WriteBatch.
// This class holds a collection of updates to apply atomically to a database.
class LevelDBWriteBatch {
public:
    static PassOwnPtr<LevelDBWriteBatch> create();
    ~LevelDBWriteBatch();

    void put(const LevelDBSlice& key, const LevelDBSlice& value);
    void remove(const LevelDBSlice& key); // Add remove operation to the batch.
    void clear();

private:
    friend class LevelDBDatabase;
    LevelDBWriteBatch();

    OwnPtr<leveldb::WriteBatch> m_writeBatch;
};

}

#endif // USE(LEVELDB)

#endif // LevelDBWriteBatch_h
