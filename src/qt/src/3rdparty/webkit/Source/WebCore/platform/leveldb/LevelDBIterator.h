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

#ifndef LevelDBIterator_h
#define LevelDBIterator_h

#if ENABLE(LEVELDB)

#include "LevelDBSlice.h"
#include "PlatformString.h"
#include <OwnPtr.h>
#include <Vector.h>

namespace leveldb {
class Iterator;
}

namespace WebCore {

class LevelDBIterator {
public:
    ~LevelDBIterator();

    bool isValid() const;
    void seekToLast();
    void seek(const Vector<char>& target);
    void next();
    void prev();
    LevelDBSlice key() const;
    LevelDBSlice value() const;

private:
    LevelDBIterator(PassOwnPtr<leveldb::Iterator>);
    friend class LevelDBDatabase;

    OwnPtr<leveldb::Iterator> m_iterator;
};


} // namespace WebCore

#endif // ENABLE(LEVELDB)
#endif // LevelDBIterator_h
