/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#include "Pasteboard.h"

#include "DocumentFragment.h"
#include "Editor.h"
#include "Frame.h"
#include "Image.h"
#include "RenderImage.h"
#include "markup.h"
#include <qapplication.h>
#include <qclipboard.h>
#include <qdebug.h>
#include <qmimedata.h>
#include <qurl.h>

#define methodDebug() qDebug() << "PasteboardQt: " << __FUNCTION__;

namespace WebCore {

Pasteboard::Pasteboard()
    : m_selectionMode(false)
{
}

Pasteboard* Pasteboard::generalPasteboard()
{
    static Pasteboard* pasteboard = 0;
    if (!pasteboard)
        pasteboard = new Pasteboard();
    return pasteboard;
}

void Pasteboard::writeSelection(Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame)
{
    QMimeData* md = new QMimeData;
    QString text = frame->editor()->selectedText();
    text.replace(QChar(0xa0), QLatin1Char(' '));
    md->setText(text);

    QString markup = createMarkup(selectedRange, 0, AnnotateForInterchange, false, AbsoluteURLs);
#ifdef Q_OS_MAC
    markup.prepend(QLatin1String("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /></head><body>"));
    markup.append(QLatin1String("</body></html>"));
    md->setData(QLatin1String("text/html"), markup.toUtf8());
#else
    md->setHtml(markup);
#endif

#ifndef QT_NO_CLIPBOARD
    QApplication::clipboard()->setMimeData(md, m_selectionMode ? QClipboard::Selection : QClipboard::Clipboard);
#endif
    if (canSmartCopyOrDelete)
        md->setData(QLatin1String("application/vnd.qtwebkit.smartpaste"), QByteArray());
}

bool Pasteboard::canSmartReplace()
{
#ifndef QT_NO_CLIPBOARD
    if (QApplication::clipboard()->mimeData()->hasFormat((QLatin1String("application/vnd.qtwebkit.smartpaste"))))
        return true;
#endif
    return false;
}

String Pasteboard::plainText(Frame*)
{
#ifndef QT_NO_CLIPBOARD
    return QApplication::clipboard()->text(m_selectionMode ? QClipboard::Selection : QClipboard::Clipboard);
#else
    return String();
#endif
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context,
                                                          bool allowPlainText, bool& chosePlainText)
{
#ifndef QT_NO_CLIPBOARD
    const QMimeData* mimeData = QApplication::clipboard()->mimeData(
            m_selectionMode ? QClipboard::Selection : QClipboard::Clipboard);

    chosePlainText = false;

    if (mimeData->hasHtml()) {
        QString html = mimeData->html();
        if (!html.isEmpty()) {
            RefPtr<DocumentFragment> fragment = createFragmentFromMarkup(frame->document(), html, "", FragmentScriptingNotAllowed);
            if (fragment)
                return fragment.release();
        }
    }

    if (allowPlainText && mimeData->hasText()) {
        chosePlainText = true;
        RefPtr<DocumentFragment> fragment = createFragmentFromText(context.get(), mimeData->text());
        if (fragment)
            return fragment.release();
    }
#endif
    return 0;
}

void Pasteboard::writePlainText(const String& text)
{
#ifndef QT_NO_CLIPBOARD
    QMimeData* md = new QMimeData;
    QString qtext = text;
    qtext.replace(QChar(0xa0), QLatin1Char(' '));
    md->setText(qtext);
    QApplication::clipboard()->setMimeData(md, m_selectionMode ? QClipboard::Selection : QClipboard::Clipboard);
#endif
}

void Pasteboard::writeURL(const KURL& url, const String&, Frame*)
{
    ASSERT(!url.isEmpty());

#ifndef QT_NO_CLIPBOARD
    QMimeData* md = new QMimeData;
    QString urlString = url.string();
    md->setText(urlString);
    md->setUrls(QList<QUrl>() << url);
    QApplication::clipboard()->setMimeData(md, m_selectionMode ? QClipboard::Selection : QClipboard::Clipboard);
#endif
}

void Pasteboard::writeImage(Node* node, const KURL&, const String&)
{
    ASSERT(node && node->renderer() && node->renderer()->isImage());

#ifndef QT_NO_CLIPBOARD
    CachedImage* cachedImage = toRenderImage(node->renderer())->cachedImage();
    if (!cachedImage || cachedImage->errorOccurred())
        return;

    Image* image = cachedImage->image();
    ASSERT(image);

    QPixmap* pixmap = image->nativeImageForCurrentFrame();
    if (!pixmap)
        return;
    QApplication::clipboard()->setPixmap(*pixmap, QClipboard::Clipboard);
#endif
}

/* This function is called from Editor::tryDHTMLCopy before actually set the clipboard
 * It introduce a race condition with klipper, which will try to grab the clipboard 
 * It's not required to clear it anyway, since QClipboard take care about replacing the clipboard
 */
void Pasteboard::clear()
{
}

bool Pasteboard::isSelectionMode() const
{
    return m_selectionMode;
}

void Pasteboard::setSelectionMode(bool selectionMode)
{
    m_selectionMode = selectionMode;
}

}
