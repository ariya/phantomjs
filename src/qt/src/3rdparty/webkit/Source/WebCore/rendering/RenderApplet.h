/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
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

#ifndef RenderApplet_h
#define RenderApplet_h

#include "RenderWidget.h"
#include <wtf/text/StringHash.h>

namespace WebCore {

class HTMLAppletElement;

class RenderApplet : public RenderWidget {
public:
    RenderApplet(HTMLAppletElement*, const HashMap<String, String>& args);
    virtual ~RenderApplet();

    void createWidgetIfNecessary();

#if USE(ACCELERATED_COMPOSITING)
    virtual bool allowsAcceleratedCompositing() const;
#endif

private:
    virtual const char* renderName() const { return "RenderApplet"; }

    virtual bool isApplet() const { return true; }

    virtual void layout();
    virtual IntSize intrinsicSize() const;

#if USE(ACCELERATED_COMPOSITING)
    virtual bool requiresLayer() const;
#endif

    HashMap<String, String> m_args;
};

inline RenderApplet* toRenderApplet(RenderObject* object)
{
    ASSERT(!object || object->isApplet());
    return static_cast<RenderApplet*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderApplet(const RenderApplet*);

} // namespace WebCore

#endif // RenderApplet_h
