/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2013 Xueqing Huang <huangxueqing@baidu.com>
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
#include "Clipboard.h"

#include "CachedImage.h"
#include "ClipboardUtilitiesWin.h"
#include "Document.h"
#include "Element.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "NotImplemented.h"
#include "Pasteboard.h"
#include "markup.h"

namespace WebCore {

DragImageRef Clipboard::createDragImage(IntPoint& dragLocation) const
{
    HBITMAP result = 0;
    if (m_dragImage) {
#if USE(CAIRO) || USE(CG)
        result = createDragImageFromImage(m_dragImage->image());        
        dragLocation = m_dragLoc;
#else
        notImplemented();
#endif
    } else if (m_dragImageElement) {
        Node* node = m_dragImageElement.get();
        result = node->document()->frame()->nodeImage(node);
        dragLocation = m_dragLoc;
    }
    return result;
}

#if ENABLE(DRAG_SUPPORT)
void Clipboard::declareAndWriteDragImage(Element* element, const KURL& url, const String& title, Frame* frame)
{
    // Order is important here for Explorer's sake
    if (!m_pasteboard->writableDataObject())
        return;
    m_pasteboard->writeURLToWritableDataObject(url, title);
    m_pasteboard->writeImageToDataObject(element, url);
    AtomicString imageURL = element->getAttribute(HTMLNames::srcAttr);
    if (imageURL.isEmpty()) 
        return;

    KURL fullURL = frame->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(imageURL));
    if (fullURL.isEmpty()) 
        return;
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    // Put img tag on the clipboard referencing the image
    Vector<char> data;
    markupToCFHTML(createMarkup(element, IncludeNode, 0, ResolveAllURLs), "", data);
    medium.hGlobal = createGlobalData(data);
    if (medium.hGlobal && FAILED(m_pasteboard->writableDataObject()->SetData(htmlFormat(), &medium, TRUE)))
        ::GlobalFree(medium.hGlobal);
}
#endif

} // namespace WebCore
