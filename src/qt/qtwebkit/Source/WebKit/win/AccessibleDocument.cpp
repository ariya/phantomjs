/*
 * Copyright (C) 2008, 2013 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Serotek Corporation. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "AccessibleDocument.h"

#include <WebCore/AXObjectCache.h>
#include <WebCore/Document.h>
#include <WebCore/RenderObject.h>

using namespace WebCore;

// AccessibleDocument
AccessibleDocument::AccessibleDocument(Document* doc, HWND window)
    : AccessibleBase(doc->axObjectCache()->rootObject(), window)
{
}

long AccessibleDocument::role() const
{
    return ROLE_SYSTEM_DOCUMENT;
}

Document* AccessibleDocument::document() const
{
    if (!m_object)
        return 0;
    return m_object->document();
}

long AccessibleDocument::state() const
{
    long state = AccessibleBase::state();

    // The document is considered focusable on Windows.
    state |= STATE_SYSTEM_FOCUSABLE;

    // The document must expose itself as focused if no element has focus.
    if (m_object->focusedUIElement() == m_object)
        state |= STATE_SYSTEM_FOCUSED;

    return state;
}
