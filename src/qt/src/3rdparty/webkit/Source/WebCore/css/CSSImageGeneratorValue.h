/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "IntSizeHash.h"
#include <wtf/HashMap.h>
#include <wtf/HashCountedSet.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Image;
class StyleGeneratedImage;
class RenderObject;

class CSSImageGeneratorValue : public CSSValue {
public:
    virtual ~CSSImageGeneratorValue();

    void addClient(RenderObject*, const IntSize&);
    void removeClient(RenderObject*);
    virtual PassRefPtr<Image> image(RenderObject*, const IntSize&) = 0;

    StyleGeneratedImage* generatedImage();
    
    virtual bool isFixedSize() const { return false; }
    virtual IntSize fixedSize(const RenderObject*) { return IntSize(); }
    
protected:
    CSSImageGeneratorValue();
    
    Image* getImage(RenderObject*, const IntSize&);
    void putImage(const IntSize&, PassRefPtr<Image>);

    typedef pair<IntSize, int> SizeCountPair;
    typedef HashMap<RenderObject*, SizeCountPair> RenderObjectSizeCountMap;

    HashCountedSet<IntSize> m_sizes; // A count of how many times a given image size is in use.
    RenderObjectSizeCountMap m_clients; // A map from RenderObjects (with entry count) to image sizes.
    HashMap<IntSize, RefPtr<Image> > m_images; // A cache of Image objects by image size.
    
    RefPtr<StyleGeneratedImage> m_image;
    bool m_accessedImage;

private:
    virtual bool isImageGeneratorValue() const { return true; }
};

} // namespace WebCore

#endif // CSSImageGeneratorValue_h
