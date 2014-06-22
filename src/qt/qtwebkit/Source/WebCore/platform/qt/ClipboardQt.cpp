/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 Sencha, Inc. All rights reserved.
 * Copyright (C) 2013 Cisco Systems, Inc. All rights reserved.
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
#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "Frame.h"
#include "Image.h"
#include "Pasteboard.h"

namespace WebCore {

DragImageRef Clipboard::createDragImage(IntPoint& location) const
{
    location = m_dragLoc;

    if (m_dragImage)
        return createDragImageFromImage(m_dragImage->image());

    if (m_dragImageElement) {
        Document* document = m_dragImageElement->document();
        if (Frame* frame = document->frame())
            return frame->nodeImage(m_dragImageElement.get());
    }

    return 0; // We do not have enough information to create a drag image, use the default icon.
}

void Clipboard::declareAndWriteDragImage(Element* element, const KURL& url, const String& label, Frame* frame)
{
    m_pasteboard->writeImage(element->toNode(), url, label);
}

}
