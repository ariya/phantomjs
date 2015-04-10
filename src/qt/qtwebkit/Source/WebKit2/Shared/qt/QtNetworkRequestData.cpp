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

#include "config.h"
#include "QtNetworkRequestData.h"

#include "ArgumentCodersQt.h"
#include "WebCoreArgumentCoders.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUuid>
#include <wtf/text/WTFString.h>

namespace WebKit {

QtRefCountedNetworkRequestData::QtRefCountedNetworkRequestData(const QtNetworkRequestData& data)
    : m_data(data)
{ }

QtNetworkRequestData::QtNetworkRequestData()
{ }

QtNetworkRequestData::QtNetworkRequestData(const QNetworkRequest& request, QNetworkReply* reply)
{
    m_scheme = request.url().scheme();
    m_urlString = request.url().toString();
    m_replyUuid = QUuid::createUuid().toString();
}

void QtNetworkRequestData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_scheme;
    encoder << m_urlString;
    encoder << m_replyUuid;
}

bool QtNetworkRequestData::decode(CoreIPC::ArgumentDecoder& decoder, QtNetworkRequestData& destination)
{
    if (!decoder.decode(destination.m_scheme))
        return false;
    if (!decoder.decode(destination.m_urlString))
        return false;
    if (!decoder.decode(destination.m_replyUuid))
        return false;
    return true;
}

} // namespace WebKit

