/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "DragData.h"

#include "Document.h"
#include "DocumentFragment.h"
#include "Frame.h"
#include "markup.h"

#include <QColor>
#include <QList>
#include <QMimeData>
#include <QUrl>

namespace WebCore {

bool DragData::canSmartReplace() const
{
    return false;
}

bool DragData::containsColor() const
{
    if (!m_platformDragData)
        return false;
    return m_platformDragData->hasColor();
}

bool DragData::containsFiles() const
{
    if (!m_platformDragData)
        return false;
    QList<QUrl> urls = m_platformDragData->urls();
    foreach (const QUrl &url, urls) {
        if (!url.toLocalFile().isEmpty())
            return true;
    }
    return false;
}

void DragData::asFilenames(Vector<String>& result) const
{
    if (!m_platformDragData)
        return;
    QList<QUrl> urls = m_platformDragData->urls();
    foreach (const QUrl &url, urls) {
        QString file = url.toLocalFile();
        if (!file.isEmpty())
            result.append(file);
    }
}

bool DragData::containsPlainText() const
{
    if (!m_platformDragData)
        return false;
    return m_platformDragData->hasText() || m_platformDragData->hasUrls();
}

String DragData::asPlainText(Frame* frame) const
{
    if (!m_platformDragData)
        return String();
    String text = m_platformDragData->text();
    if (!text.isEmpty())
        return text;

    // FIXME: Should handle rich text here
    return asURL(frame, DoNotConvertFilenames, 0);
}

Color DragData::asColor() const
{
    if (!m_platformDragData)
        return Color();
    return qvariant_cast<QColor>(m_platformDragData->colorData());
}

bool DragData::containsCompatibleContent() const
{
    if (!m_platformDragData)
        return false;
    return containsColor() || containsURL(0) || m_platformDragData->hasHtml() || m_platformDragData->hasText();
}

bool DragData::containsURL(Frame*, FilenameConversionPolicy filenamePolicy) const
{
    // FIXME: Use filenamePolicy.
    if (!m_platformDragData)
        return false;
    return m_platformDragData->hasUrls();
}

String DragData::asURL(Frame*, FilenameConversionPolicy filenamePolicy, String*) const
{
    // FIXME: Use filenamePolicy.
    if (!m_platformDragData)
        return String();
    QList<QUrl> urls = m_platformDragData->urls();

    if (urls.isEmpty())
        return String();

    QByteArray encodedUrl = urls.first().toEncoded();
    return String(encodedUrl.constData(), encodedUrl.length());
}

PassRefPtr<DocumentFragment> DragData::asFragment(Frame* frame, PassRefPtr<Range>, bool, bool&) const
{
    if (m_platformDragData && m_platformDragData->hasHtml())
        return createFragmentFromMarkup(frame->document(), m_platformDragData->html(), "", FragmentScriptingNotAllowed);

    return 0;
}

}

