/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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

#import "config.h"

#import "AccessibilityTextMarker.h"
#import "DumpRenderTree.h"

// MARK: AccessibilityTextMarker

AccessibilityTextMarker::AccessibilityTextMarker(PlatformTextMarker marker)
    : m_textMarker(marker)
{
}

AccessibilityTextMarker::AccessibilityTextMarker(const AccessibilityTextMarker& marker)
    : m_textMarker(marker.platformTextMarker())
{
}

AccessibilityTextMarker::~AccessibilityTextMarker()
{
}

bool AccessibilityTextMarker::isEqual(AccessibilityTextMarker* other)
{
    return [(id)platformTextMarker() isEqual:(id)other->platformTextMarker()];
}

PlatformTextMarker AccessibilityTextMarker::platformTextMarker() const 
{ 
    return m_textMarker.get();
}

// MARK: AccessibilityTextMarkerRange

AccessibilityTextMarkerRange::AccessibilityTextMarkerRange(PlatformTextMarkerRange markerRange)
    : m_textMarkerRange(markerRange)
{
}

AccessibilityTextMarkerRange::AccessibilityTextMarkerRange(const AccessibilityTextMarkerRange& markerRange)
    : m_textMarkerRange(markerRange.platformTextMarkerRange())
{
}

AccessibilityTextMarkerRange::~AccessibilityTextMarkerRange()
{
}

bool AccessibilityTextMarkerRange::isEqual(AccessibilityTextMarkerRange* other)
{
    return [(id)platformTextMarkerRange() isEqual:(id)other->platformTextMarkerRange()];
}

PlatformTextMarkerRange AccessibilityTextMarkerRange::platformTextMarkerRange() const
{
    return m_textMarkerRange.get();
}
