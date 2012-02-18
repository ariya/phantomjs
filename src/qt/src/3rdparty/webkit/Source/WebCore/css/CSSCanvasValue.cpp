/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "CSSCanvasValue.h"

#include "ImageBuffer.h"
#include "RenderObject.h"

namespace WebCore {

CSSCanvasValue::~CSSCanvasValue()
{
    if (m_element)
        m_element->removeObserver(this);
}

String CSSCanvasValue::cssText() const
{
    String result = "-webkit-canvas(";
    result += m_name  + ")";
    return result;
}

void CSSCanvasValue::canvasChanged(HTMLCanvasElement*, const FloatRect& changedRect)
{
    IntRect imageChangeRect = enclosingIntRect(changedRect);
    RenderObjectSizeCountMap::const_iterator end = m_clients.end();
    for (RenderObjectSizeCountMap::const_iterator curr = m_clients.begin(); curr != end; ++curr)
        curr->first->imageChanged(static_cast<WrappedImagePtr>(this), &imageChangeRect);
}

void CSSCanvasValue::canvasResized(HTMLCanvasElement*)
{
    RenderObjectSizeCountMap::const_iterator end = m_clients.end();
    for (RenderObjectSizeCountMap::const_iterator curr = m_clients.begin(); curr != end; ++curr)
        curr->first->imageChanged(static_cast<WrappedImagePtr>(this));
}

void CSSCanvasValue::canvasDestroyed(HTMLCanvasElement* element)
{
    ASSERT_UNUSED(element, element == m_element);
    m_element = 0;
}

IntSize CSSCanvasValue::fixedSize(const RenderObject* renderer)
{
    if (HTMLCanvasElement* elt = element(renderer->document()))
        return IntSize(elt->width(), elt->height());
    return IntSize();
}

HTMLCanvasElement* CSSCanvasValue::element(Document* document)
{
     if (!m_element) {
        m_element = document->getCSSCanvasElement(m_name);
        if (!m_element)
            return 0;
        m_element->addObserver(this);
    }
    return m_element;
}

PassRefPtr<Image> CSSCanvasValue::image(RenderObject* renderer, const IntSize& /*size*/)
{
    ASSERT(m_clients.contains(renderer));
    HTMLCanvasElement* elt = element(renderer->document());
    if (!elt || !elt->buffer())
        return 0;
    return elt->copiedImage();
}

} // namespace WebCore
