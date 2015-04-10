/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef FontGenericFamilies_h
#define FontGenericFamilies_h

#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

// UScriptCode uses -1 and 0 for UScriptInvalidCode and UScriptCommon.
// We need to use -2 and -3 for empty value and deleted value.
struct UScriptCodeHashTraits : WTF::GenericHashTraits<int> {
    static const bool emptyValueIsZero = false;
    static int emptyValue() { return -2; }
    static void constructDeletedValue(int& slot) { slot = -3; }
    static bool isDeletedValue(int value) { return value == -3; }
};

typedef HashMap<int, AtomicString, DefaultHash<int>::Hash, UScriptCodeHashTraits> ScriptFontFamilyMap;

class FontGenericFamilies : public RefCounted<FontGenericFamilies> {
public:
    static PassRefPtr<FontGenericFamilies> create() { return adoptRef(new FontGenericFamilies); }

    const AtomicString& standardFontFamily(UScriptCode = USCRIPT_COMMON) const;
    const AtomicString& fixedFontFamily(UScriptCode = USCRIPT_COMMON) const;
    const AtomicString& serifFontFamily(UScriptCode = USCRIPT_COMMON) const;
    const AtomicString& sansSerifFontFamily(UScriptCode = USCRIPT_COMMON) const;
    const AtomicString& cursiveFontFamily(UScriptCode = USCRIPT_COMMON) const;
    const AtomicString& fantasyFontFamily(UScriptCode = USCRIPT_COMMON) const;
    const AtomicString& pictographFontFamily(UScriptCode = USCRIPT_COMMON) const;

    bool setStandardFontFamily(const AtomicString&, UScriptCode);
    bool setFixedFontFamily(const AtomicString&, UScriptCode);
    bool setSerifFontFamily(const AtomicString&, UScriptCode);
    bool setSansSerifFontFamily(const AtomicString&, UScriptCode);
    bool setCursiveFontFamily(const AtomicString&, UScriptCode);
    bool setFantasyFontFamily(const AtomicString&, UScriptCode);
    bool setPictographFontFamily(const AtomicString&, UScriptCode);

private:
    FontGenericFamilies();

    ScriptFontFamilyMap m_standardFontFamilyMap;
    ScriptFontFamilyMap m_serifFontFamilyMap;
    ScriptFontFamilyMap m_fixedFontFamilyMap;
    ScriptFontFamilyMap m_sansSerifFontFamilyMap;
    ScriptFontFamilyMap m_cursiveFontFamilyMap;
    ScriptFontFamilyMap m_fantasyFontFamilyMap;
    ScriptFontFamilyMap m_pictographFontFamilyMap;
};

}

#endif
