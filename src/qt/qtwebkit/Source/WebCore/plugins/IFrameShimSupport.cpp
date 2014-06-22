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
 * contributors may be used to endorse or promote products derived from
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

#include "config.h"
#include "IFrameShimSupport.h"

#include "Element.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLNames.h"
#include "RenderBox.h"
#include "RenderObject.h"
#include "Widget.h"

#include <wtf/HashSet.h>

// This file provides plugin-related utility functions for iframe shims and is shared by platforms that inherit
// from PluginView (e.g. Qt) and those that do not (e.g. Chromium).

namespace WebCore {

static void getObjectStack(const RenderObject* ro, Vector<const RenderObject*>* roStack)
{
    roStack->clear();
    while (ro) {
        roStack->append(ro);
        ro = ro->parent();
    }
}

// Returns true if stack1 is at or above stack2
static bool iframeIsAbovePlugin(const Vector<const RenderObject*>& iframeZstack, const Vector<const RenderObject*>& pluginZstack)
{
    for (size_t i = 0; i < iframeZstack.size() && i < pluginZstack.size(); i++) {
        // The root is at the end of these stacks.  We want to iterate
        // root-downwards so we index backwards from the end.
        const RenderObject* ro1 = iframeZstack[iframeZstack.size() - 1 - i];
        const RenderObject* ro2 = pluginZstack[pluginZstack.size() - 1 - i];

        if (ro1 != ro2) {
            // When we find nodes in the stack that are not the same, then
            // we've found the nodes just below the lowest comment ancestor.
            // Determine which should be on top.

            // See if z-index determines an order.
            if (ro1->style() && ro2->style()) {
                int z1 = ro1->style()->zIndex();
                int z2 = ro2->style()->zIndex();
                if (z1 > z2)
                    return true;
                if (z1 < z2)
                    return false;
            }

            // If the plugin does not have an explicit z-index it stacks behind the iframe.
            // This is for maintaining compatibility with IE.
            if (ro2->style()->position() == StaticPosition) {
                // The 0'th elements of these RenderObject arrays represent the plugin node and
                // the iframe.
                const RenderObject* pluginRenderObject = pluginZstack[0];
                const RenderObject* iframeRenderObject = iframeZstack[0];

                if (pluginRenderObject->style() && iframeRenderObject->style()) {
                    if (pluginRenderObject->style()->zIndex() > iframeRenderObject->style()->zIndex())
                        return false;
                }
                return true;
            }

            // Inspect the document order.  Later order means higher stacking.
            const RenderObject* parent = ro1->parent();
            if (!parent)
                return false;
            ASSERT(parent == ro2->parent());

            for (const RenderObject* ro = parent->firstChild(); ro; ro = ro->nextSibling()) {
                if (ro == ro1)
                    return false;
                if (ro == ro2)
                    return true;
            }
            ASSERT(false); // We should have seen ro1 and ro2 by now.
            return false;
        }
    }
    return true;
}

// Return a set of rectangles that should not be overdrawn by the
// plugin ("cutouts").  This helps implement the "iframe shim"
// technique of overlaying a windowed plugin with content from the
// page.  In a nutshell, iframe elements should occlude plugins when
// they occur higher in the stacking order.
void getPluginOcclusions(Element* element, Widget* parentWidget, const IntRect& frameRect, Vector<IntRect>& occlusions)
{
    RenderObject* pluginNode = element->renderer();
    ASSERT(pluginNode);
    if (!pluginNode->style())
        return;
    Vector<const RenderObject*> pluginZstack;
    Vector<const RenderObject*> iframeZstack;
    getObjectStack(pluginNode, &pluginZstack);

    if (!parentWidget->isFrameView())
        return;

    FrameView* parentFrameView = toFrameView(parentWidget);

    const HashSet<RefPtr<Widget> >* children = parentFrameView->children();
    for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != children->end(); ++it) {
        // We only care about FrameView's because iframes show up as FrameViews.
        if (!(*it)->isFrameView())
            continue;

        const FrameView* frameView = toFrameView((*it).get());
        // Check to make sure we can get both the element and the RenderObject
        // for this FrameView, if we can't just move on to the next object.
        if (!frameView->frame() || !frameView->frame()->ownerElement()
            || !frameView->frame()->ownerElement()->renderer())
            continue;

        HTMLElement* element = frameView->frame()->ownerElement();
        RenderObject* iframeRenderer = element->renderer();

        if (element->hasTagName(HTMLNames::iframeTag)
            && iframeRenderer->absoluteBoundingBoxRectIgnoringTransforms().intersects(frameRect)
            && (!iframeRenderer->style() || iframeRenderer->style()->visibility() == VISIBLE)) {
            getObjectStack(iframeRenderer, &iframeZstack);
            if (iframeIsAbovePlugin(iframeZstack, pluginZstack)) {
                IntPoint point = roundedIntPoint(iframeRenderer->localToAbsolute());
                RenderBox* rbox = toRenderBox(iframeRenderer);
                IntSize size(rbox->width(), rbox->height());
                occlusions.append(IntRect(point, size));
            }
        }
    }
}

} // namespace WebCore
