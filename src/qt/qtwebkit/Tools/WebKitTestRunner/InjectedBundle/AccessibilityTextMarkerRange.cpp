/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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
#include "AccessibilityTextMarker.h"

#include "AccessibilityUIElement.h"
#include "JSAccessibilityTextMarkerRange.h"

#include <JavaScriptCore/JSRetainPtr.h>

namespace WTR {
    
PassRefPtr<AccessibilityTextMarkerRange> AccessibilityTextMarkerRange::create(PlatformTextMarkerRange markerRange)
{
    return adoptRef(new AccessibilityTextMarkerRange(markerRange));
}

PassRefPtr<AccessibilityTextMarkerRange> AccessibilityTextMarkerRange::create(const AccessibilityTextMarkerRange& markerRange)
{
    return adoptRef(new AccessibilityTextMarkerRange(markerRange));
}

AccessibilityTextMarkerRange::AccessibilityTextMarkerRange(PlatformTextMarkerRange markerRange)
    : m_textMarkerRange(markerRange)
{
}

AccessibilityTextMarkerRange::AccessibilityTextMarkerRange(const AccessibilityTextMarkerRange& markerRange)
    : JSWrappable()
    , m_textMarkerRange(markerRange.platformTextMarkerRange())
{
}

AccessibilityTextMarkerRange::~AccessibilityTextMarkerRange()
{
}

PlatformTextMarkerRange AccessibilityTextMarkerRange::platformTextMarkerRange() const
{
#if PLATFORM(MAC)
    return m_textMarkerRange.get();
#else
    return m_textMarkerRange;
#endif
}
    
JSClassRef AccessibilityTextMarkerRange::wrapperClass()
{
    return JSAccessibilityTextMarkerRange::accessibilityTextMarkerRangeClass();
}
    
} // namespace WTR

