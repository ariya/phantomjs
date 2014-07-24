/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promo te products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PageSerializer_h
#define PageSerializer_h

#include "KURL.h"
#include "KURLHash.h"
#include "SharedBuffer.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/Vector.h>

namespace WebCore {

class CachedImage;
class CSSStyleSheet;
class Document;
class Frame;
class Page;
class RenderObject;
class StylePropertySet;
class StyleRule;

// This class is used to serialize a page contents back to text (typically HTML).
// It serializes all the page frames and retrieves resources such as images and CSS stylesheets.
class PageSerializer {
public:
    struct Resource {
        KURL url;
        String mimeType;
        RefPtr<SharedBuffer> data;
        Resource();
        Resource(const KURL&, const String& mimeType, PassRefPtr<SharedBuffer> data);
    };

    explicit PageSerializer(Vector<Resource>*);

    // Initiates the serialization of the frame's page. All serialized content and retrieved
    // resources are added to the Vector passed to the constructor. The first resource in that
    // vector is the top frame serialized content.
    void serialize(Page*);

    KURL urlForBlankFrame(Frame*);

private:
    void serializeFrame(Frame*);

    // Serializes the stylesheet back to text and adds it to the resources if URL is not-empty.
    // It also adds any resources included in that stylesheet (including any imported stylesheets and their own resources).
    void serializeCSSStyleSheet(CSSStyleSheet*, const KURL&);

    void addImageToResources(CachedImage*, RenderObject*, const KURL&);
    void retrieveResourcesForProperties(const StylePropertySet*, Document*);
    void retrieveResourcesForRule(StyleRule*, Document*);

    Vector<Resource>* m_resources;
    ListHashSet<KURL> m_resourceURLs;
    HashMap<Frame*, KURL> m_blankFrameURLs;
    unsigned m_blankFrameCounter;
};

}

#endif
