/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef CSSFontFace_h
#define CSSFontFace_h

#include "CSSFontFaceRule.h"
#include "CSSFontFaceSource.h"
#include "FontTraitsMask.h"
#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

class CSSSegmentedFontFace;
class FontDescription;
class SimpleFontData;

class CSSFontFace : public RefCounted<CSSFontFace> {
public:
    static PassRefPtr<CSSFontFace> create(FontTraitsMask traitsMask, PassRefPtr<CSSFontFaceRule> rule, bool isLocalFallback = false) { return adoptRef(new CSSFontFace(traitsMask, rule, isLocalFallback)); }

    FontTraitsMask traitsMask() const { return m_traitsMask; }

    struct UnicodeRange;

    void addRange(UChar32 from, UChar32 to) { m_ranges.append(UnicodeRange(from, to)); }
    const Vector<UnicodeRange>& ranges() const { return m_ranges; }

    void addedToSegmentedFontFace(CSSSegmentedFontFace*);
    void removedFromSegmentedFontFace(CSSSegmentedFontFace*);

    bool isLoaded() const;
    bool isValid() const;

    bool isLocalFallback() const { return m_isLocalFallback; }

    void addSource(PassOwnPtr<CSSFontFaceSource>);

    void fontLoaded(CSSFontFaceSource*);

    PassRefPtr<SimpleFontData> getFontData(const FontDescription&, bool syntheticBold, bool syntheticItalic);

    struct UnicodeRange {
        UnicodeRange(UChar32 from, UChar32 to)
            : m_from(from)
            , m_to(to)
        {
        }

        UChar32 from() const { return m_from; }
        UChar32 to() const { return m_to; }

    private:
        UChar32 m_from;
        UChar32 m_to;
    };

#if ENABLE(SVG_FONTS)
    bool hasSVGFontFaceSource() const;
#endif

#if ENABLE(FONT_LOAD_EVENTS)
    enum LoadState { NotLoaded, Loading, Loaded, Error };
    LoadState loadState() const { return m_loadState; }
#endif

private:
    CSSFontFace(FontTraitsMask traitsMask, PassRefPtr<CSSFontFaceRule> rule, bool isLocalFallback)
        : m_traitsMask(traitsMask)
        , m_activeSource(0)
        , m_isLocalFallback(isLocalFallback)
#if ENABLE(FONT_LOAD_EVENTS)
        , m_loadState(isLocalFallback ? Loaded : NotLoaded)
        , m_rule(rule)
#endif
    {
        UNUSED_PARAM(rule);
    }

    FontTraitsMask m_traitsMask;
    Vector<UnicodeRange> m_ranges;
    HashSet<CSSSegmentedFontFace*> m_segmentedFontFaces;
    Vector<OwnPtr<CSSFontFaceSource> > m_sources;
    CSSFontFaceSource* m_activeSource;
    bool m_isLocalFallback;
#if ENABLE(FONT_LOAD_EVENTS)
    LoadState m_loadState;
    RefPtr<CSSFontFaceRule> m_rule;
    void notifyFontLoader(LoadState);
    void notifyLoadingDone();
#endif
};

}

#endif
