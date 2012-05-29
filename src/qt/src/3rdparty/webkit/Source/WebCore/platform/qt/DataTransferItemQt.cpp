/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "DataTransferItemQt.h"

#if ENABLE(DATA_TRANSFER_ITEMS)

#include "Blob.h"
#include "Clipboard.h"
#include "NotImplemented.h"
#include "StringCallback.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QTextCodec>

namespace WebCore {

PassRefPtr<DataTransferItem> DataTransferItem::create(PassRefPtr<Clipboard> owner,
                                                      ScriptExecutionContext* context,
                                                      const String& data,
                                                      const String& type)
{
    return DataTransferItemQt::create(owner, context, type, data);
}

PassRefPtr<DataTransferItemQt> DataTransferItemQt::createFromPasteboard(PassRefPtr<Clipboard> owner,
                                                                        ScriptExecutionContext* context,
                                                                        const String& type)
{
    if (type == "text/plain" || type == "text/html")
        return adoptRef(new DataTransferItemQt(owner, context, PasteboardSource, DataTransferItem::kindString, type, ""));

    return adoptRef(new DataTransferItemQt(owner, context, PasteboardSource, DataTransferItem::kindFile, type, ""));
}

PassRefPtr<DataTransferItemQt> DataTransferItemQt::create(PassRefPtr<Clipboard> owner,
                                                          ScriptExecutionContext* context,
                                                          const String& type,
                                                          const String& data)
{
    return adoptRef(new DataTransferItemQt(owner, context, InternalSource, DataTransferItem::kindString, type, data));
}

DataTransferItemQt::DataTransferItemQt(PassRefPtr<Clipboard> owner,
                                       ScriptExecutionContext* context,
                                       DataSource source,
                                       const String& kind, const String& type,
                                       const String& data)
    : DataTransferItem(owner, kind, type)
    , m_context(context)
    , m_dataSource(source)
    , m_data(data)
{
}

void DataTransferItemQt::getAsString(PassRefPtr<StringCallback> callback)
{
    if ((owner()->policy() != ClipboardReadable && owner()->policy() != ClipboardWritable)
        || kind() != kindString)
        return;

    if (m_dataSource == InternalSource) {
        callback->scheduleCallback(m_context, m_data);
        return;
    }

    const QMimeData* mimeData = QApplication::clipboard()->mimeData(QClipboard::Clipboard);
    if (!mimeData)
        return;

    QString data;
    if (type() == "text/plain")
        data = mimeData->text();
    else if (type() == "text/html")
        data = mimeData->html();
    else {
        QByteArray rawData = mimeData->data(type());
        data = QTextCodec::codecForName("UTF-16")->toUnicode(rawData);
    }

    callback->scheduleCallback(m_context, data);
}

PassRefPtr<Blob> DataTransferItemQt::getAsFile()
{
    notImplemented();
    return 0;
}

}

#endif // ENABLE(DATA_TRANSFER_ITEMS)
