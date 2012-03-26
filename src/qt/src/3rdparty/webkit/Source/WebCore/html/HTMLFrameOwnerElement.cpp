/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "HTMLFrameOwnerElement.h"

#include "DOMWindow.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "RenderPart.h"

#if ENABLE(SVG)
#include "ExceptionCode.h"
#include "SVGDocument.h"
#endif

namespace WebCore {

HTMLFrameOwnerElement::HTMLFrameOwnerElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_contentFrame(0)
    , m_sandboxFlags(SandboxNone)
{
}

RenderPart* HTMLFrameOwnerElement::renderPart() const
{
    // HTMLObjectElement and HTMLEmbedElement may return arbitrary renderers
    // when using fallback content.
    if (!renderer() || !renderer()->isRenderPart())
        return 0;
    return toRenderPart(renderer());
}

void HTMLFrameOwnerElement::willRemove()
{
    // FIXME: It is unclear why this can't be moved to removedFromDocument()
    // this is the only implementation of willRemove in WebCore!
    if (Frame* frame = contentFrame()) {
        RefPtr<Frame> protect(frame);
        frame->loader()->frameDetached();
        frame->disconnectOwnerElement();
    }

    HTMLElement::willRemove();
}

HTMLFrameOwnerElement::~HTMLFrameOwnerElement()
{
    if (m_contentFrame)
        m_contentFrame->disconnectOwnerElement();
}

Document* HTMLFrameOwnerElement::contentDocument() const
{
    return m_contentFrame ? m_contentFrame->document() : 0;
}

DOMWindow* HTMLFrameOwnerElement::contentWindow() const
{
    return m_contentFrame ? m_contentFrame->domWindow() : 0;
}

void HTMLFrameOwnerElement::setSandboxFlags(SandboxFlags flags)
{
    if (m_sandboxFlags == flags)
        return;

    m_sandboxFlags = flags;

    if (Frame* frame = contentFrame())
        frame->loader()->ownerElementSandboxFlagsChanged();
}

bool HTMLFrameOwnerElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    return m_contentFrame && HTMLElement::isKeyboardFocusable(event);
}

#if ENABLE(SVG)
SVGDocument* HTMLFrameOwnerElement::getSVGDocument(ExceptionCode& ec) const
{
    Document* doc = contentDocument();
    if (doc && doc->isSVGDocument())
        return static_cast<SVGDocument*>(doc);
    // Spec: http://www.w3.org/TR/SVG/struct.html#InterfaceGetSVGDocument
    ec = NOT_SUPPORTED_ERR;
    return 0;
}
#endif

} // namespace WebCore
