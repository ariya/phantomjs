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

#include "config.h"
#include "FontGenericFamilies.h"

namespace WebCore {

static bool setGenericFontFamilyForScript(ScriptFontFamilyMap& fontMap, const AtomicString& family, UScriptCode script)
{
    if (family.isEmpty()) {
        ScriptFontFamilyMap::iterator it = fontMap.find(static_cast<int>(script));
        if (it == fontMap.end())
            return false;
        fontMap.remove(it);
        return true;
    }
    ScriptFontFamilyMap::AddResult addResult = fontMap.add(static_cast<int>(script), family);
    if (addResult.isNewEntry)
        return true;
    if (addResult.iterator->value == family)
        return false;
    addResult.iterator->value = family;
    return true;
}

static const AtomicString& genericFontFamilyForScript(const ScriptFontFamilyMap& fontMap, UScriptCode script)
{
    ScriptFontFamilyMap::const_iterator it = fontMap.find(static_cast<int>(script));
    if (it != fontMap.end())
        return it->value;
    if (script != USCRIPT_COMMON)
        return genericFontFamilyForScript(fontMap, USCRIPT_COMMON);
    return emptyAtom;
}

FontGenericFamilies::FontGenericFamilies()
{
}

const AtomicString& FontGenericFamilies::standardFontFamily(UScriptCode script) const
{
    return genericFontFamilyForScript(m_standardFontFamilyMap, script);
}

const AtomicString& FontGenericFamilies::fixedFontFamily(UScriptCode script) const
{
    return genericFontFamilyForScript(m_fixedFontFamilyMap, script);
}

const AtomicString& FontGenericFamilies::serifFontFamily(UScriptCode script) const
{
    return genericFontFamilyForScript(m_serifFontFamilyMap, script);
}

const AtomicString& FontGenericFamilies::sansSerifFontFamily(UScriptCode script) const
{
    return genericFontFamilyForScript(m_sansSerifFontFamilyMap, script);
}

const AtomicString& FontGenericFamilies::cursiveFontFamily(UScriptCode script) const
{
    return genericFontFamilyForScript(m_cursiveFontFamilyMap, script);
}

const AtomicString& FontGenericFamilies::fantasyFontFamily(UScriptCode script) const
{
    return genericFontFamilyForScript(m_fantasyFontFamilyMap, script);
}

const AtomicString& FontGenericFamilies::pictographFontFamily(UScriptCode script) const
{
    return genericFontFamilyForScript(m_pictographFontFamilyMap, script);
}

bool FontGenericFamilies::setStandardFontFamily(const AtomicString& family, UScriptCode script)
{
    return setGenericFontFamilyForScript(m_standardFontFamilyMap, family, script);
}

bool FontGenericFamilies::setFixedFontFamily(const AtomicString& family, UScriptCode script)
{
    return setGenericFontFamilyForScript(m_fixedFontFamilyMap, family, script);
}

bool FontGenericFamilies::setSerifFontFamily(const AtomicString& family, UScriptCode script)
{
    return setGenericFontFamilyForScript(m_serifFontFamilyMap, family, script);
}

bool FontGenericFamilies::setSansSerifFontFamily(const AtomicString& family, UScriptCode script)
{
    return setGenericFontFamilyForScript(m_sansSerifFontFamilyMap, family, script);
}

bool FontGenericFamilies::setCursiveFontFamily(const AtomicString& family, UScriptCode script)
{
    return setGenericFontFamilyForScript(m_cursiveFontFamilyMap, family, script);
}

bool FontGenericFamilies::setFantasyFontFamily(const AtomicString& family, UScriptCode script)
{
    return setGenericFontFamilyForScript(m_fantasyFontFamilyMap, family, script);
}

bool FontGenericFamilies::setPictographFontFamily(const AtomicString& family, UScriptCode script)
{
    return setGenericFontFamilyForScript(m_pictographFontFamilyMap, family, script);
}

}
