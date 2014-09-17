/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGUseElement_h
#define SVGUseElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedLength.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"
#include "SVGURIReference.h"

namespace WebCore {

class SVGElementInstance;
class SVGShadowTreeRootElement;

class SVGUseElement : public SVGStyledTransformableElement,
                      public SVGTests,
                      public SVGLangSpace,
                      public SVGExternalResourcesRequired,
                      public SVGURIReference {
public:
    static PassRefPtr<SVGUseElement> create(const QualifiedName&, Document*);

    SVGElementInstance* instanceRoot() const;
    SVGElementInstance* animatedInstanceRoot() const;
    SVGElementInstance* instanceForShadowTreeElement(Node*) const;
    void invalidateShadowTree();

    RenderObject* rendererClipChild() const;

private:
    SVGUseElement(const QualifiedName&, Document*);

    virtual bool isValid() const { return SVGTests::isValid(); }

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void buildPendingResource();

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();

    virtual void recalcStyle(StyleChange = NoChange);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void attach();
    virtual void detach();

    virtual void toClipPath(Path&) const;

    static void removeDisallowedElementsFromSubtree(Node* element);

    void setUpdatesBlocked(bool blocked) { m_updatesBlocked = blocked; }

    friend class RenderSVGShadowTreeRootContainer;
    void buildShadowAndInstanceTree(SVGShadowTreeRootElement*);
    void detachInstance();

    virtual bool selfHasRelativeLengths() const;

    // Instance tree handling
    void buildInstanceTree(SVGElement* target, SVGElementInstance* targetInstance, bool& foundCycle);
    bool hasCycleUseReferencing(SVGUseElement*, SVGElementInstance* targetInstance, SVGElement*& newTarget);

    // Shadow tree handling
    void buildShadowTree(SVGShadowTreeRootElement*, SVGElement* target, SVGElementInstance* targetInstance);

#if ENABLE(SVG) && ENABLE(SVG_USE)
    void expandUseElementsInShadowTree(Node* element);
    void expandSymbolElementsInShadowTree(Node* element);
#endif

    // "Tree connector" 
    void associateInstancesWithShadowTreeElements(Node* target, SVGElementInstance* targetInstance);
    SVGElementInstance* instanceForShadowTreeElement(Node* element, SVGElementInstance* instance) const;

    void transferUseAttributesToReplacedElement(SVGElement* from, SVGElement* to) const;
    void transferEventListenersToShadowTree(SVGElementInstance* target);

    void updateContainerOffsets();
    void updateContainerSizes();

    // Animated property declarations
    DECLARE_ANIMATED_LENGTH(X, x)
    DECLARE_ANIMATED_LENGTH(Y, y)
    DECLARE_ANIMATED_LENGTH(Width, width)
    DECLARE_ANIMATED_LENGTH(Height, height)

    // SVGURIReference
    DECLARE_ANIMATED_STRING(Href, href)

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)

    bool m_updatesBlocked;
    bool m_needsShadowTreeRecreation;
    String m_resourceId;
    RefPtr<SVGElementInstance> m_targetElementInstance;
};

}

#endif
#endif
