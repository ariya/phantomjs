/*
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 */

#include "config.h"

#if ENABLE(XHTMLMP)
#include "HTMLNoScriptElement.h"

#include "CSSStyleSelector.h"
#include "HTMLNames.h"
#include "RenderObject.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLNoScriptElement::HTMLNoScriptElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(noscriptTag));
}

PassRefPtr<HTMLNoScriptElement> HTMLNoScriptElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLNoScriptElement(tagName, document));
}

void HTMLNoScriptElement::attach()
{
    HTMLElement::attach();

    // If no need to process <noscript>, we hide it by setting display:none temporarily
    if (!document()->shouldProcessNoscriptElement()) {
        if (renderer() && renderer()->style())
            renderer()->style()->setDisplay(NONE);
        setNeedsStyleRecalc();
    }
}

void HTMLNoScriptElement::recalcStyle(StyleChange change)
{
    if (!document()->shouldProcessNoscriptElement() || !renderer() || !renderer()->style())
        return;

    // If <noscript> needs processing, we make it visiable here, including its visible children
    RefPtr<RenderStyle> style = renderer()->style();
    if (style->display() == NONE) {
        style->setDisplay(INLINE);

        // Create renderers for its children
        if (hasChildNodes()) {
            for (Node* n = firstChild(); n; n = n->traverseNextNode(this))
                if (!n->renderer())
                    n->createRendererIfNeeded();
        }
    }
}

bool HTMLNoScriptElement::childShouldCreateRenderer(Node*) const
{
    return document()->shouldProcessNoscriptElement();
}

}

#endif
