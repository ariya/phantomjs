/*
 * Copyright (C) 2008, 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef CSSImageGeneratorValue_h
#define CSSImageGeneratorValue_h

#include "CSSValue.h"
#include "GeneratorGeneratedImage.h"
#include "IntSizeHash.h"
#include "Timer.h"
#include <wtf/HashCountedSet.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CachedResourceLoader;
class GeneratorGeneratedImage;
class RenderObject;
class StyleResolver;

class CSSImageGeneratorValue : public CSSValue {
public:
    ~CSSImageGeneratorValue();

    void addClient(RenderObject*);
    void removeClient(RenderObject*);

    PassRefPtr<Image> image(RenderObject*, const IntSize&);

    bool isFixedSize() const;
    IntSize fixedSize(const RenderObject*);

    bool isPending() const;
    bool knownToBeOpaque(const RenderObject*) const;

    void loadSubimages(CachedResourceLoader*);

protected:
    CSSImageGeneratorValue(ClassType);

    GeneratorGeneratedImage* cachedImageForSize(IntSize);
    void saveCachedImageForSize(IntSize, PassRefPtr<GeneratorGeneratedImage>);
    const HashCountedSet<RenderObject*>& clients() const { return m_clients; }

private:
    class CachedGeneratedImage {
    public:
        CachedGeneratedImage(CSSImageGeneratorValue&, IntSize, PassRefPtr<GeneratorGeneratedImage>);
        GeneratorGeneratedImage* image() { return m_image.get(); }
        void puntEvictionTimer() { m_evictionTimer.restart(); }

    private:
        void evictionTimerFired(DeferrableOneShotTimer<CachedGeneratedImage>*);

        CSSImageGeneratorValue& m_owner;
        IntSize m_size;
        RefPtr<GeneratorGeneratedImage> m_image;
        DeferrableOneShotTimer<CachedGeneratedImage> m_evictionTimer;
    };

    friend class CachedGeneratedImage;
    void evictCachedGeneratedImage(IntSize);

    HashCountedSet<RenderObject*> m_clients;
    HashMap<IntSize, OwnPtr<CachedGeneratedImage> > m_images;
};

} // namespace WebCore

#endif // CSSImageGeneratorValue_h
