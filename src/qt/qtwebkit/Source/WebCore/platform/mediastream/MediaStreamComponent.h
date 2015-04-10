/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef MediaStreamComponent_h
#define MediaStreamComponent_h

#if ENABLE(MEDIA_STREAM)

#include "MediaStreamSource.h"
#include "UUID.h"

namespace WebCore {

class MediaStreamDescriptor;

class MediaStreamComponent : public RefCounted<MediaStreamComponent> {
public:
    static PassRefPtr<MediaStreamComponent> create(PassRefPtr<MediaStreamSource> source)
    {
        return adoptRef(new MediaStreamComponent(createCanonicalUUIDString(), 0, source));
    }

    static PassRefPtr<MediaStreamComponent> create(const String& id, PassRefPtr<MediaStreamSource> source)
    {
        return adoptRef(new MediaStreamComponent(id, 0, source));
    }

    static PassRefPtr<MediaStreamComponent> create(MediaStreamDescriptor* stream, PassRefPtr<MediaStreamSource> source)
    {
        return adoptRef(new MediaStreamComponent(createCanonicalUUIDString(), stream, source));
    }

    MediaStreamDescriptor* stream() const { return m_stream; }
    void setStream(MediaStreamDescriptor* stream) { ASSERT(!m_stream && stream); m_stream = stream; }

    MediaStreamSource* source() const { return m_source.get(); }

    String id() const { return m_id; }
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    MediaStreamComponent(const String& id, MediaStreamDescriptor* stream, PassRefPtr<MediaStreamSource> source)
        : m_stream(stream)
        , m_source(source)
        , m_id(id)
        , m_enabled(true)
    {
        ASSERT(m_id.length());
    }

    MediaStreamDescriptor* m_stream;
    RefPtr<MediaStreamSource> m_source;
    String m_id;
    bool m_enabled;
};

typedef Vector<RefPtr<MediaStreamComponent> > MediaStreamComponentVector;

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStreamComponent_h
