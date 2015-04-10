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

#include "config.h"
#include "QtNetworkReply.h"

#include "SharedMemory.h"
#include "WebFrameNetworkingContext.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace WebKit {

QtNetworkReply::QtNetworkReply(const QNetworkRequest& req, QtNetworkAccessManager* parent)
    : QNetworkReply(parent)
    , m_bytesAvailable(0)
    , m_sharedMemorySize(0)
{
    setRequest(req);
    setOperation(QNetworkAccessManager::GetOperation);
    setUrl(req.url());
    setOpenMode(QIODevice::ReadOnly);
}

void QtNetworkReply::setData(const SharedMemory::Handle& handle, qint64 dataSize)
{
    if (handle.isNull())
        return;
    m_sharedMemory = SharedMemory::create(handle, SharedMemory::ReadOnly);
    if (!m_sharedMemory)
        return;

    m_bytesAvailable = dataSize;
    m_sharedMemorySize = dataSize;
}

void QtNetworkReply::setReplyData(const QtNetworkReplyData& replyData)
{
    if (!replyData.m_contentType.isEmpty())
        setHeader(QNetworkRequest::ContentTypeHeader, QString(replyData.m_contentType));
    setHeader(QNetworkRequest::ContentLengthHeader, QVariant::fromValue(replyData.m_contentLength));
    setData(replyData.m_dataHandle, replyData.m_contentLength);
}

qint64 QtNetworkReply::readData(char* data, qint64 maxlen)
{
    if (!m_sharedMemory)
        return 0;

    qint64 bytesRead = maxlen < m_bytesAvailable ? maxlen : m_bytesAvailable;
    if (memcpy(data, static_cast<char*>(m_sharedMemory->data()) + m_sharedMemorySize - m_bytesAvailable, bytesRead)) {
        m_bytesAvailable -= bytesRead;
        return bytesRead;
    }
    return 0;
}

qint64 QtNetworkReply::bytesAvailable() const
{
    return m_bytesAvailable + QNetworkReply::bytesAvailable();
}

void QtNetworkReply::abort() { }
void QtNetworkReply::close() { }
void QtNetworkReply::setReadBufferSize(qint64 size) { }
bool QtNetworkReply::canReadLine () const { return true; }

void QtNetworkReply::finalize()
{
    QNetworkReply::setFinished(true);
    emit readyRead();
    emit finished();
}

} // namespace WebKit

