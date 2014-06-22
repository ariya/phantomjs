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

#ifndef LevelDBSlice_h
#define LevelDBSlice_h

#if USE(LEVELDB)

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class LevelDBSlice {
public:
    LevelDBSlice(const char* begin, const char* end)
        : m_begin(begin)
        , m_end(end)
    {
        ASSERT(m_end >= m_begin);
    }

    LevelDBSlice(const Vector<char>& v)
        : m_begin(v.data())
        , m_end(m_begin + v.size())
    {
        ASSERT(m_end >= m_begin);
    }

    ~LevelDBSlice()
    {
    }

    const char* begin() const { return m_begin; }
    const char* end() const { return m_end; }

private:
    const char* m_begin;
    const char* m_end;
};

} // namespace WebCore

#endif // USE(LEVELDB)

#endif // LevelDBSlice_h
