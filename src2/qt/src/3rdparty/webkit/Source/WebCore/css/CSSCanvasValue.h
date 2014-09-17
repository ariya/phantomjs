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

#ifndef CSSCanvasValue_h
#define CSSCanvasValue_h

#include "CSSImageGeneratorValue.h"
#include "HTMLCanvasElement.h"

namespace WebCore {

class Document;

class CSSCanvasValue : public CSSImageGeneratorValue, private CanvasObserver {
public:
    static PassRefPtr<CSSCanvasValue> create() { return adoptRef(new CSSCanvasValue); }
    virtual ~CSSCanvasValue();

    virtual String cssText() const;

    virtual PassRefPtr<Image> image(RenderObject*, const IntSize&);
    virtual bool isFixedSize() const { return true; }
    virtual IntSize fixedSize(const RenderObject*);

    void setName(const String& name) { m_name = name; }

private:
    CSSCanvasValue()
        : m_element(0)
    {
    }

    virtual void canvasChanged(HTMLCanvasElement*, const FloatRect& changedRect);
    virtual void canvasResized(HTMLCanvasElement*);
    virtual void canvasDestroyed(HTMLCanvasElement*);

    HTMLCanvasElement* element(Document*);
     
    // The name of the canvas.
    String m_name;
    // The document supplies the element and owns it.
    HTMLCanvasElement* m_element;
};

} // namespace WebCore

#endif // CSSCanvasValue_h
