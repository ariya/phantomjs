/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebSocketDeflater_h
#define WebSocketDeflater_h

#if ENABLE(WEB_SOCKETS)

#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

struct z_stream_s;
typedef z_stream_s z_stream;

namespace WebCore {

class WebSocketDeflater {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum ContextTakeOverMode {
        DoNotTakeOverContext,
        TakeOverContext
    };
    static PassOwnPtr<WebSocketDeflater> create(int windowBits, ContextTakeOverMode = TakeOverContext);

    ~WebSocketDeflater();

    bool initialize();
    bool addBytes(const char*, size_t);
    bool finish();
    const char* data() { return m_buffer.data(); }
    size_t size() const { return m_buffer.size(); }
    void reset();

private:
    WebSocketDeflater(int windowBits, ContextTakeOverMode);

    int m_windowBits;
    ContextTakeOverMode m_contextTakeOverMode;
    Vector<char> m_buffer;
    OwnPtr<z_stream> m_stream;
};

class WebSocketInflater {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<WebSocketInflater> create(int windowBits = 15);

    ~WebSocketInflater();

    bool initialize();
    bool addBytes(const char*, size_t);
    bool finish();
    const char* data() { return m_buffer.data(); }
    size_t size() const { return m_buffer.size(); }
    void reset();

private:
    explicit WebSocketInflater(int windowBits);

    int m_windowBits;
    Vector<char> m_buffer;
    OwnPtr<z_stream> m_stream;
};

}

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketDeflater_h
