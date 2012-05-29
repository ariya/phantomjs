/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 Sencha, Inc. All rights reserved.
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
#include "ClipboardQt.h"

#include "CachedImage.h"
#include "DataTransferItemsQt.h"
#include "Document.h"
#include "DragData.h"
#include "Element.h"
#include "FileList.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "Image.h"
#include "IntPoint.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include "Range.h"
#include "RenderImage.h"
#include "markup.h"
#include <wtf/text/StringHash.h>

#include <QApplication>
#include <QClipboard>
#include <QList>
#include <QMimeData>
#include <QStringList>
#include <QTextCodec>
#include <QUrl>
#include <qdebug.h>

#define methodDebug() qDebug("ClipboardQt: %s", __FUNCTION__)

namespace WebCore {

static bool isTextMimeType(const String& type)
{
    return type == "text/plain" || type.startsWith("text/plain;");
}

static bool isHtmlMimeType(const String& type)
{
    return type == "text/html" || type.startsWith("text/html;");
}

PassRefPtr<Clipboard> Clipboard::create(ClipboardAccessPolicy policy, DragData* dragData, Frame* frame)
{
    return ClipboardQt::create(policy, dragData->platformData(), frame);
}

ClipboardQt::ClipboardQt(ClipboardAccessPolicy policy, const QMimeData* readableClipboard, Frame* frame)
    : Clipboard(policy, DragAndDrop)
    , m_readableData(readableClipboard)
    , m_writableData(0)
    , m_frame(frame)
{
    Q_ASSERT(policy == ClipboardReadable || policy == ClipboardTypesReadable);
}

ClipboardQt::ClipboardQt(ClipboardAccessPolicy policy, ClipboardType clipboardType, Frame* frame)
    : Clipboard(policy, clipboardType)
    , m_readableData(0)
    , m_writableData(0)
    , m_frame(frame)
{
    Q_ASSERT(policy == ClipboardReadable || policy == ClipboardWritable || policy == ClipboardNumb);

#ifndef QT_NO_CLIPBOARD
    if (policy != ClipboardWritable) {
        Q_ASSERT(isForCopyAndPaste());
        m_readableData = QApplication::clipboard()->mimeData();
    }
#endif
}

ClipboardQt::~ClipboardQt()
{
    if (m_writableData && isForCopyAndPaste())
        m_writableData = 0;
    else
        delete m_writableData;
    m_readableData = 0;
}

void ClipboardQt::clearData(const String& type)
{
    if (policy() != ClipboardWritable)
        return;

    if (m_writableData) {
        m_writableData->removeFormat(type);
        if (m_writableData->formats().isEmpty()) {
            if (isForDragAndDrop())
                delete m_writableData;
            m_writableData = 0;
        }
    }
#ifndef QT_NO_CLIPBOARD
    if (isForCopyAndPaste())
        QApplication::clipboard()->setMimeData(m_writableData);
#endif
}

void ClipboardQt::clearAllData()
{
    if (policy() != ClipboardWritable)
        return;

#ifndef QT_NO_CLIPBOARD
    if (isForCopyAndPaste())
        QApplication::clipboard()->setMimeData(0);
    else
#endif
        delete m_writableData;
    m_writableData = 0;
}

String ClipboardQt::getData(const String& type, bool& success) const
{

    if (policy() != ClipboardReadable) {
        success = false;
        return String();
    }

    if (isHtmlMimeType(type) && m_readableData->hasHtml()) {
        success = true;
        return m_readableData->html();
    }

    if (isTextMimeType(type) && m_readableData->hasText()) {
        success = true;
        return m_readableData->text();
    }

    ASSERT(m_readableData);
    QByteArray rawData = m_readableData->data(type);
    QString data = QTextCodec::codecForName("UTF-16")->toUnicode(rawData);
    success = !data.isEmpty();
    return data;
}

bool ClipboardQt::setData(const String& type, const String& data)
{
    if (policy() != ClipboardWritable)
        return false;

    if (!m_writableData)
        m_writableData = new QMimeData;

    if (isTextMimeType(type))
        m_writableData->setText(QString(data));
    else if (isHtmlMimeType(type))
        m_writableData->setHtml(QString(data));
    else {
        QByteArray array(reinterpret_cast<const char*>(data.characters()), data.length() * 2);
        m_writableData->setData(QString(type), array);
    }

#ifndef QT_NO_CLIPBOARD
    if (isForCopyAndPaste())
        QApplication::clipboard()->setMimeData(m_writableData);
#endif
    return true;
}

// extensions beyond IE's API
HashSet<String> ClipboardQt::types() const
{
    if (policy() != ClipboardReadable && policy() != ClipboardTypesReadable)
        return HashSet<String>();

    ASSERT(m_readableData);
    HashSet<String> result;
    QStringList formats = m_readableData->formats();
    for (int i = 0; i < formats.count(); ++i)
        result.add(formats.at(i));
    return result;
}

PassRefPtr<FileList> ClipboardQt::files() const
{
    if (policy() != ClipboardReadable || !m_readableData->hasUrls())
        return FileList::create();

    RefPtr<FileList> fileList = FileList::create();
    QList<QUrl> urls = m_readableData->urls();

    for (int i = 0; i < urls.size(); i++) {
        QUrl url = urls[i];
        if (url.scheme() != QLatin1String("file"))
            continue;
        fileList->append(File::create(url.toLocalFile()));
    }

    return fileList.release();
}

void ClipboardQt::setDragImage(CachedImage* image, const IntPoint& point)
{
    setDragImage(image, 0, point);
}

void ClipboardQt::setDragImageElement(Node* node, const IntPoint& point)
{
    setDragImage(0, node, point);
}

void ClipboardQt::setDragImage(CachedImage* image, Node *node, const IntPoint &loc)
{
    if (policy() != ClipboardImageWritable && policy() != ClipboardWritable)
        return;

    if (m_dragImage)
        m_dragImage->removeClient(this);
    m_dragImage = image;
    if (m_dragImage)
        m_dragImage->addClient(this);

    m_dragLoc = loc;
    m_dragImageElement = node;
}

DragImageRef ClipboardQt::createDragImage(IntPoint& dragLoc) const
{
    if (!m_dragImage)
        return 0;
    dragLoc = m_dragLoc;
    return m_dragImage->image()->nativeImageForCurrentFrame();
}


static CachedImage* getCachedImage(Element* element)
{
    // Attempt to pull CachedImage from element
    ASSERT(element);
    RenderObject* renderer = element->renderer();
    if (!renderer || !renderer->isImage())
        return 0;

    RenderImage* image = toRenderImage(renderer);
    if (image->cachedImage() && !image->cachedImage()->errorOccurred())
        return image->cachedImage();

    return 0;
}

void ClipboardQt::declareAndWriteDragImage(Element* element, const KURL& url, const String& title, Frame* frame)
{
    ASSERT(frame);

    // WebCore::writeURL(m_writableDataObject.get(), url, title, true, false);
    if (!m_writableData)
        m_writableData = new QMimeData;

    CachedImage* cachedImage = getCachedImage(element);
    if (!cachedImage || !cachedImage->image() || !cachedImage->isLoaded())
        return;
    QPixmap* pixmap = cachedImage->image()->nativeImageForCurrentFrame();
    if (pixmap)
        m_writableData->setImageData(*pixmap);

    AtomicString imageURL = element->getAttribute(HTMLNames::srcAttr);
    if (imageURL.isEmpty())
        return;

    KURL fullURL = frame->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(imageURL));
    if (fullURL.isEmpty())
        return;

    QList<QUrl> urls;
    urls.append(url);

    m_writableData->setText(title);
    m_writableData->setUrls(urls);
    m_writableData->setHtml(imageToMarkup(fullURL, element));
#ifndef QT_NO_CLIPBOARD
    if (isForCopyAndPaste())
        QApplication::clipboard()->setMimeData(m_writableData);
#endif
}

void ClipboardQt::writeURL(const KURL& url, const String& title, Frame* frame)
{
    ASSERT(frame);

    QList<QUrl> urls;
    urls.append(frame->document()->completeURL(url.string()));
    if (!m_writableData)
        m_writableData = new QMimeData;
    m_writableData->setUrls(urls);
    m_writableData->setText(title);
#ifndef QT_NO_CLIPBOARD
    if (isForCopyAndPaste())
        QApplication::clipboard()->setMimeData(m_writableData);
#endif
}

void ClipboardQt::writeRange(Range* range, Frame* frame)
{
    ASSERT(range);
    ASSERT(frame);

    if (!m_writableData)
        m_writableData = new QMimeData;
    QString text = frame->editor()->selectedText();
    text.replace(QChar(0xa0), QLatin1Char(' '));
    m_writableData->setText(text);
    m_writableData->setHtml(createMarkup(range, 0, AnnotateForInterchange, false, AbsoluteURLs));
#ifndef QT_NO_CLIPBOARD
    if (isForCopyAndPaste())
        QApplication::clipboard()->setMimeData(m_writableData);
#endif
}

void ClipboardQt::writePlainText(const String& str)
{
    if (!m_writableData)
        m_writableData = new QMimeData;
    QString text = str;
    text.replace(QChar(0xa0), QLatin1Char(' '));
    m_writableData->setText(text);
#ifndef QT_NO_CLIPBOARD
    if (isForCopyAndPaste())
        QApplication::clipboard()->setMimeData(m_writableData);
#endif
}

bool ClipboardQt::hasData()
{
    const QMimeData *data = m_readableData ? m_readableData : m_writableData;
    if (!data)
        return false;
    return data->formats().count() > 0;
}

#if ENABLE(DATA_TRANSFER_ITEMS)
PassRefPtr<DataTransferItems> ClipboardQt::items()
{

    if (!m_frame && !m_frame->document())
        return 0;

    RefPtr<DataTransferItemsQt> items = DataTransferItemsQt::create(this, m_frame->document()->scriptExecutionContext());

    if (!m_readableData)
        return items;

    if (isForCopyAndPaste() && policy() == ClipboardReadable) {
        const QStringList types = m_readableData->formats();
        for (int i = 0; i < types.count(); ++i)
            items->addPasteboardItem(types.at(i));
    }
    return items;
}
#endif

}
