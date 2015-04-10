/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "PageClientEfl.h"

#include "ewk_view_private.h"

namespace WebCore {
class IntRect;
}

PageClientEfl::PageClientEfl(Evas_Object* view)
    : m_view(view)
{
    ASSERT(m_view);
}

PageClientEfl::~PageClientEfl()
{
}

#if USE(ACCELERATED_COMPOSITING)
bool PageClientEfl::createEvasObjectForAcceleratedCompositing(Evas_Native_Surface* nativeSurface, const WebCore::IntRect& rect)
{
    return ewk_view_accelerated_compositing_object_create(m_view, nativeSurface, rect);
}

WebCore::GraphicsContext3D* PageClientEfl::acceleratedCompositingContext()
{
    return ewk_view_accelerated_compositing_context_get(m_view);
}
#endif
