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

#ifndef CSSFontSelector_h
#define CSSFontSelector_h

#include "FontSelector.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CSSFontFace;
class CSSFontFaceRule;
class CSSSegmentedFontFace;
class Document;
class CachedResourceLoader;
class FontDescription;

class CSSFontSelector : public FontSelector {
public:
    static PassRefPtr<CSSFontSelector> create(Document* document)
    {
        return adoptRef(new CSSFontSelector(document));
    }
    virtual ~CSSFontSelector();

    virtual FontData* getFontData(const FontDescription& fontDescription, const AtomicString& familyName);
    
    void clearDocument() { m_document = 0; }

    void addFontFaceRule(const CSSFontFaceRule*);

    void fontLoaded();
    virtual void fontCacheInvalidated();

    bool isEmpty() const;

    CachedResourceLoader* cachedResourceLoader() const;

    virtual void registerForInvalidationCallbacks(FontSelectorClient*);
    virtual void unregisterForInvalidationCallbacks(FontSelectorClient*);

private:
    CSSFontSelector(Document*);

    void dispatchInvalidationCallbacks();

    Document* m_document;
    HashMap<String, Vector<RefPtr<CSSFontFace> >*, CaseFoldingHash> m_fontFaces;
    HashMap<String, Vector<RefPtr<CSSFontFace> >*, CaseFoldingHash> m_locallyInstalledFontFaces;
    HashMap<String, HashMap<unsigned, RefPtr<CSSSegmentedFontFace> >*, CaseFoldingHash> m_fonts;
    HashSet<FontSelectorClient*> m_clients;
};

} // namespace WebCore

#endif // CSSFontSelector_h
