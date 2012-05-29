/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
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
#include "RenderPart.h"

#include "Frame.h"
#include "FrameView.h"
#include "HTMLFrameElementBase.h"
#include "PluginViewBase.h"
#include "RenderView.h"

namespace WebCore {

RenderPart::RenderPart(Element* node)
    : RenderWidget(node)
{
    setInline(false);
}

RenderPart::~RenderPart()
{
    clearWidget();
}

void RenderPart::setWidget(PassRefPtr<Widget> widget)
{
    if (widget == this->widget())
        return;

    RenderWidget::setWidget(widget);

    // make sure the scrollbars are set correctly for restore
    // ### find better fix
    viewCleared();
}

void RenderPart::viewCleared()
{
}

#if USE(ACCELERATED_COMPOSITING)
bool RenderPart::requiresLayer() const
{
    if (RenderWidget::requiresLayer())
        return true;
    
    return requiresAcceleratedCompositing();
}

bool RenderPart::requiresAcceleratedCompositing() const
{
    // There are two general cases in which we can return true. First, if this is a plugin 
    // renderer and the plugin has a layer, then we need a layer. Second, if this is 
    // a renderer with a contentDocument and that document needs a layer, then we need
    // a layer.
    if (widget() && widget()->isPluginViewBase() && static_cast<PluginViewBase*>(widget())->platformLayer())
        return true;

    if (!node() || !node()->isFrameOwnerElement())
        return false;

    HTMLFrameOwnerElement* element = static_cast<HTMLFrameOwnerElement*>(node());
    if (Document* contentDocument = element->contentDocument()) {
        if (RenderView* view = contentDocument->renderView())
            return view->usesCompositing();
    }

    return false;
}
#endif

}
