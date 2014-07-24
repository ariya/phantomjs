/*
 * Copyright (C) 2011 Zeno Albisser <zeno@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must replyroduce the above copyright
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

#ifndef QtNetworkRequestData_h
#define QtNetworkRequestData_h

#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
class ArgumentEncoder;
class ArgumentDecoder;
};

QT_BEGIN_NAMESPACE
class QNetworkRequest;
class QNetworkReply;
QT_END_NAMESPACE

namespace WebKit {

struct QtNetworkRequestData {
    QtNetworkRequestData();
    QtNetworkRequestData(const QNetworkRequest&, QNetworkReply*);
    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, QtNetworkRequestData&);

    String m_scheme;
    String m_urlString;
    String m_replyUuid;
};

struct QtRefCountedNetworkRequestData : public WTF::RefCounted<QtRefCountedNetworkRequestData> {
    QtRefCountedNetworkRequestData(const QtNetworkRequestData&);
    QtNetworkRequestData& data() { return m_data; }
private:
    QtNetworkRequestData m_data;
};

} // namespace WebKit

#endif // QtNetworkRequestData_h
