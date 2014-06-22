/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
#include "AccessibleImage.h"

#include <WebCore/AccessibilityRenderObject.h>
#include <WebCore/HTMLNames.h>

using namespace WebCore;
using namespace WebCore::HTMLNames;

AccessibleImage::AccessibleImage(AccessibilityObject* obj, HWND window)
    : AccessibleBase(obj, window)
{
    ASSERT_ARG(obj, obj->isImage());
    ASSERT_ARG(obj, obj->isAccessibilityRenderObject());
}

String AccessibleImage::name() const
{
    if (!m_object->isAccessibilityRenderObject())
        return AccessibleBase::name();

    AccessibilityRenderObject* obj = static_cast<AccessibilityRenderObject*>(m_object);

    String ariaLabel = obj->ariaLabeledByAttribute();
    if (!ariaLabel.isEmpty())
        return ariaLabel;

    const AtomicString& altText = obj->getAttribute(HTMLNames::altAttr);
    if (!altText.isEmpty())
        return altText;

    return AccessibleBase::name();
}
