/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2006, 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010, 2012 Google Inc. All rights reserved.
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

#ifndef RenderLayerModelObject_h
#define RenderLayerModelObject_h

#include "RenderObject.h"

namespace WebCore {

class RenderLayer;

class RenderLayerModelObject : public RenderObject {
public:
    explicit RenderLayerModelObject(ContainerNode*);
    virtual ~RenderLayerModelObject();

    // Called by RenderObject::willBeDestroyed() and is the only way layers should ever be destroyed
    void destroyLayer();

    bool hasSelfPaintingLayer() const;
    RenderLayer* layer() const { return m_layer; }

    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle) OVERRIDE;
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE;
    virtual void updateFromStyle() { }

    virtual bool requiresLayer() const = 0;

    // Returns true if the background is painted opaque in the given rect.
    // The query rect is given in local coordinate system.
    virtual bool backgroundIsKnownToBeOpaqueInRect(const LayoutRect&) const { return false; }

    // This is null for anonymous renderers.
    ContainerNode* node() const { return toContainerNode(RenderObject::node()); }

protected:
    void ensureLayer();

    virtual void willBeDestroyed() OVERRIDE;

private:
    virtual bool isLayerModelObject() const OVERRIDE { return true; }

    RenderLayer* m_layer;

    // Used to store state between styleWillChange and styleDidChange
    static bool s_wasFloating;
    static bool s_hadLayer;
    static bool s_hadTransform;
    static bool s_layerWasSelfPainting;
};

inline RenderLayerModelObject* toRenderLayerModelObject(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isLayerModelObject());
    return static_cast<RenderLayerModelObject*>(object);
}

inline const RenderLayerModelObject* toRenderLayerModelObject(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isLayerModelObject());
    return static_cast<const RenderLayerModelObject*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderLayerModelObject(const RenderLayerModelObject*);

} // namespace WebCore

#endif // RenderLayerModelObject_h
