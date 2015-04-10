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

#ifndef AccessibilityTextMarker_h
#define AccessibilityTextMarker_h

#include <JavaScriptCore/JSObjectRef.h>

#if PLATFORM(MAC)
#define SUPPORTS_AX_TEXTMARKERS 1
#else
#define SUPPORTS_AX_TEXTMARKERS 0
#endif

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
typedef CFTypeRef PlatformTextMarker;
typedef CFTypeRef PlatformTextMarkerRange;
#else
typedef void* PlatformTextMarker;
typedef void* PlatformTextMarkerRange;
#endif

class AccessibilityUIElement;

class AccessibilityTextMarker {
public:
    AccessibilityTextMarker(PlatformTextMarker);
    AccessibilityTextMarker(const AccessibilityTextMarker&);
    ~AccessibilityTextMarker();
    
    PlatformTextMarker platformTextMarker() const;
    
    static JSObjectRef makeJSAccessibilityTextMarker(JSContextRef, const AccessibilityTextMarker&);
    bool isEqual(AccessibilityTextMarker*);
    
private:
    static JSClassRef getJSClass();
#if PLATFORM(MAC)
    RetainPtr<PlatformTextMarker> m_textMarker;
#else
    PlatformTextMarker m_textMarker;
#endif
};

class AccessibilityTextMarkerRange {
public:
    AccessibilityTextMarkerRange(PlatformTextMarkerRange);
    AccessibilityTextMarkerRange(const AccessibilityTextMarkerRange&);
    ~AccessibilityTextMarkerRange();
    
    PlatformTextMarkerRange platformTextMarkerRange() const;
    
    static JSObjectRef makeJSAccessibilityTextMarkerRange(JSContextRef, const AccessibilityTextMarkerRange&);
    bool isEqual(AccessibilityTextMarkerRange*);
    
private:
    static JSClassRef getJSClass();
#if PLATFORM(MAC)
    RetainPtr<PlatformTextMarkerRange> m_textMarkerRange;
#else
    PlatformTextMarkerRange m_textMarkerRange;
#endif
};

AccessibilityTextMarker* toTextMarker(JSObjectRef object);
AccessibilityTextMarkerRange* toTextMarkerRange(JSObjectRef object);

#if !SUPPORTS_AX_TEXTMARKERS
inline AccessibilityTextMarker::AccessibilityTextMarker(PlatformTextMarker) { }
inline AccessibilityTextMarker::AccessibilityTextMarker(const AccessibilityTextMarker&) { }
inline AccessibilityTextMarker::~AccessibilityTextMarker() { }
inline bool AccessibilityTextMarker::isEqual(AccessibilityTextMarker*) { return false; }
inline PlatformTextMarker AccessibilityTextMarker::platformTextMarker() const { return m_textMarker; }

inline AccessibilityTextMarkerRange::AccessibilityTextMarkerRange(PlatformTextMarkerRange) { }
inline AccessibilityTextMarkerRange::AccessibilityTextMarkerRange(const AccessibilityTextMarkerRange&) { }
inline AccessibilityTextMarkerRange::~AccessibilityTextMarkerRange() { }
inline bool AccessibilityTextMarkerRange::isEqual(AccessibilityTextMarkerRange*) { return false; }
inline PlatformTextMarkerRange AccessibilityTextMarkerRange::platformTextMarkerRange() const { return m_textMarkerRange; }
#endif

#endif // AccessibilityUIElement_h
