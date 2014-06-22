/*
 * Copyright (C) 2011 Zeno Albisser <zeno@webkit.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef QtNetworkReplyData_h
#define QtNetworkReplyData_h

#include "SharedMemory.h"
#include <QNetworkAccessManager>
#include <wtf/Noncopyable.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
class ArgumentEncoder;
class ArgumentDecoder;
};

namespace WebKit {

struct QtNetworkReplyData {
    QtNetworkReplyData();

    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, QtNetworkReplyData&);

    WTF::String m_urlString;

    WTF::String m_contentType;
    uint64_t m_contentLength;
    String m_replyUuid;

    SharedMemory::Handle m_dataHandle;
};

struct QtRefCountedNetworkReplyData : public WTF::RefCounted<QtRefCountedNetworkReplyData> {
    QtNetworkReplyData& data() { return m_data; }
private:
    QtNetworkReplyData m_data;
};


} // namespace WebKit

#endif // QtNetworkReplyData_h
