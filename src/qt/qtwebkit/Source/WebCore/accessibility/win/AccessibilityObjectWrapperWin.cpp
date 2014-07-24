/*
 * Copyright (C) 2013 Apple Inc. All Rights Reserved.
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
#include "AccessibilityObjectWrapperWin.h"

#if HAVE(ACCESSIBILITY)

#include "AXObjectCache.h"
#include "AccessibilityObject.h"
#include "BString.h"
#include "HTMLNames.h"
#include "QualifiedName.h"

namespace WebCore {

void AccessibilityObjectWrapper::accessibilityAttributeValue(const AtomicString& attributeName, VARIANT* result)
{
    // FIXME: This should be fleshed out to match the Mac version

    // Not a real concept on Windows, but used heavily in WebKit accessibility testing.
    if (attributeName == "AXTitleUIElementAttribute") {
        if (!m_object->exposesTitleUIElement())
            return;

        AccessibilityObject* obj = m_object->titleUIElement();
        if (obj) {
            ASSERT(V_VT(result) == VT_EMPTY);
            V_VT(result) = VT_UNKNOWN;
            AccessibilityObjectWrapper* wrapper = obj->wrapper();
            V_UNKNOWN(result) = wrapper;
            if (wrapper)
                wrapper->AddRef();
        }
        return;
    }

    // Used by DRT to find an accessible node by its element id.
    if (attributeName == "AXDRTElementIdAttribute") {
        ASSERT(V_VT(result) == VT_EMPTY);

        V_VT(result) = VT_BSTR;
        V_BSTR(result) = WebCore::BString(m_object->getAttribute(WebCore::HTMLNames::idAttr)).release();
        return;
    }
}


} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
