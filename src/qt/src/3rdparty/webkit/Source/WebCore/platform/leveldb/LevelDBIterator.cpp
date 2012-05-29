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
#include "LevelDBIterator.h"

#if ENABLE(LEVELDB)

#include <leveldb/iterator.h>
#include <leveldb/slice.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

LevelDBIterator::~LevelDBIterator()
{
}

LevelDBIterator::LevelDBIterator(PassOwnPtr<leveldb::Iterator> it)
    : m_iterator(it)
{
}

static leveldb::Slice makeSlice(const Vector<char>& value)
{
    return leveldb::Slice(value.data(), value.size());
}

static LevelDBSlice makeLevelDBSlice(leveldb::Slice s)
{
    return LevelDBSlice(s.data(), s.data() + s.size());
}

bool LevelDBIterator::isValid() const
{
    return m_iterator->Valid();
}

void LevelDBIterator::seekToLast()
{
    m_iterator->SeekToLast();
}

void LevelDBIterator::seek(const Vector<char>& target)
{
    m_iterator->Seek(makeSlice(target));
}

void LevelDBIterator::next()
{
    m_iterator->Next();
}

void LevelDBIterator::prev()
{
    m_iterator->Prev();
}

LevelDBSlice LevelDBIterator::key() const
{
    return makeLevelDBSlice(m_iterator->key());
}

LevelDBSlice LevelDBIterator::value() const
{
    return makeLevelDBSlice(m_iterator->value());
}

} // namespace WebCore

#endif // ENABLE(LEVELDB)
