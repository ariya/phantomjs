/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "TestInvocation.h"

#include "PlatformWebView.h"
#include "TestController.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QtGui/QPainter>
#include <WebKit2/WKImageQt.h>
#include <stdio.h>
#include <wtf/Assertions.h>

namespace WTR {

static void dumpImage(const QImage& image)
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();
    const QByteArray& data = buffer.data();

    printf("Content-Type: %s\n", "image/png");
    printf("Content-Length: %lu\n", static_cast<unsigned long>(data.length()));

    const quint32 bytesToWriteInOneChunk = 1 << 15;
    quint32 dataRemainingToWrite = data.length();
    const char* ptr = data.data();
    while (dataRemainingToWrite) {
        quint32 bytesToWriteInThisChunk = qMin(dataRemainingToWrite, bytesToWriteInOneChunk);
        quint32 bytesWritten = fwrite(ptr, 1, bytesToWriteInThisChunk, stdout);
        if (bytesWritten != bytesToWriteInThisChunk)
            break;
        dataRemainingToWrite -= bytesWritten;
        ptr += bytesWritten;
    }

    fflush(stdout);
}

void TestInvocation::forceRepaintDoneCallback(WKErrorRef, void *context)
{
    static_cast<TestInvocation*>(context)->m_gotRepaint = true;
    TestController::shared().notifyDone();
}

void TestInvocation::dumpPixelsAndCompareWithExpected(WKImageRef imageRef, WKArrayRef repaintRects)
{
    QImage image;
    if (PlatformWebView::windowShapshotEnabled()) {
        WKPageRef page = TestController::shared().mainWebView()->page();
        WKPageForceRepaint(page, this, &forceRepaintDoneCallback);

        TestController::shared().runUntil(m_gotRepaint, TestController::ShortTimeout);

        if (m_gotRepaint)
            image = WKImageCreateQImage(TestController::shared().mainWebView()->windowSnapshotImage().get());
        else {
            m_error = true;
            m_errorMessage = "Timed out waiting for repaint\n";
            m_webProcessIsUnresponsive = true;
            return;
        }
    } else
        image = WKImageCreateQImage(imageRef);

    if (repaintRects) {
        QImage mask(image.size(), image.format());
        mask.fill(QColor(0, 0, 0, 0.66 * 255));

        QPainter maskPainter(&mask);
        maskPainter.setCompositionMode(QPainter::CompositionMode_Source);
        size_t count = WKArrayGetSize(repaintRects);
        for (size_t i = 0; i < count; ++i) {
            WKRect wkRect = WKRectGetValue(static_cast<WKRectRef>(WKArrayGetItemAtIndex(repaintRects, i)));
            QRectF rect(wkRect.origin.x, wkRect.origin.y, wkRect.size.width, wkRect.size.height);
            maskPainter.fillRect(rect, Qt::transparent);
        }

        QPainter painter(&image);
        painter.drawImage(image.rect(), mask);
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    for (unsigned row = 0; row < image.height(); ++row)
        hash.addData(reinterpret_cast<const char*>(image.constScanLine(row)), image.bytesPerLine());

    QByteArray actualHash = hash.result().toHex();
    ASSERT(actualHash.size() == 32);
    if (!compareActualHashToExpectedAndDumpResults(actualHash)) {
        image.setText("checksum", actualHash);
        dumpImage(image);
    }
}

} // namespace WTR
