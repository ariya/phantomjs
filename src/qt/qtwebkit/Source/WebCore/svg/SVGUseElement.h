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
#include "CachedResourceHandle.h"
#include "CachedSVGDocumentClient.h"
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedLength.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGGraphicsElement.h"
#include "SVGNames.h"
#include "SVGURIReference.h"

namespace WebCore {

class CachedSVGDocument;
class SVGElementInstance;

class SVGUseElement FINAL : public SVGGraphicsElement,
                            public SVGExternalResourcesRequired,
                            public SVGURIReference,
                            public CachedSVGDocumentClient {
public:
    static PassRefPtr<SVGUseElement> create(const QualifiedName&, Document*, bool wasInsertedByParser);
    virtual ~SVGUseElement();

    SVGElementInstance* instanceRoot();
    SVGElementInstance* animatedInstanceRoot() const;
    SVGElementInstance* instanceForShadowTreeElement(Node*) const;
    void invalidateShadowTree();
    void invalidateDependentShadowTrees();

    RenderObject* rendererClipChild() const;

private:
    SVGUseElement(const QualifiedName&, Document*, bool wasInsertedByParser);

    virtual bool isValid() const { return SVGTests::isValid(); }
    virtual bool supportsFocus() const OVERRIDE { return true; }

    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual void buildPendingResource();

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);

    virtual bool willRecalcStyle(StyleChange);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void toClipPath(Path&);

    void clearResourceReferences();
    void buildShadowAndInstanceTree(SVGElement* target);
    void detachInstance();

    virtual bool haveLoadedRequiredResources() { return SVGExternalResourcesRequired::haveLoadedRequiredResources(); }

    virtual void finishParsingChildren();
    virtual bool selfHasRelativeLengths() const;

    // Instance tree handling
    void buildInstanceTree(SVGElement* target, SVGElementInstance* targetInstance, bool& foundCycle, bool foundUse);
    bool hasCycleUseReferencing(SVGUseElement*, SVGElementInstance* targetInstance, SVGElement*& newTarget);

    // Shadow tree handling
    void buildShadowTree(SVGElement* target, SVGElementInstance* targetInstance);

    void expandUseElementsInShadowTree(Node* element);
    void expandSymbolElementsInShadowTree(Node* element);

    // "Tree connector" 
    void associateInstancesWithShadowTreeElements(Node* target, SVGElementInstance* targetInstance);
    SVGElementInstance* instanceForShadowTreeElement(Node* element, SVGElementInstance* instance) const;

    void transferUseAttributesToReplacedElement(SVGElement* from, SVGElement* to) const;
    void transferEventListenersToShadowTree(SVGElementInstance* target);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGUseElement)
        DECLARE_ANIMATED_LENGTH(X, x)
        DECLARE_ANIMATED_LENGTH(Y, y)
        DECLARE_ANIMATED_LENGTH(Width, width)
        DECLARE_ANIMATED_LENGTH(Height, height)
        DECLARE_ANIMATED_STRING(Href, href)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    bool cachedDocumentIsStillLoading();
    Document* externalDocument() const;
    bool instanceTreeIsLoading(SVGElementInstance*);
    virtual void notifyFinished(CachedResource*);
    Document* referencedDocument() const;
    void setCachedDocument(CachedResourceHandle<CachedSVGDocument>);

    // SVGExternalResourcesRequired
    virtual void setHaveFiredLoadEvent(bool haveFiredLoadEvent) { m_haveFiredLoadEvent = haveFiredLoadEvent; }
    virtual bool isParserInserted() const { return m_wasInsertedByParser; }
    virtual bool haveFiredLoadEvent() const { return m_haveFiredLoadEvent; }
    virtual Timer<SVGElement>* svgLoadEventTimer() OVERRIDE { return &m_svgLoadEventTimer; }

    bool m_wasInsertedByParser;
    bool m_haveFiredLoadEvent;
    bool m_needsShadowTreeRecreation;
    RefPtr<SVGElementInstance> m_targetElementInstance;
    CachedResourceHandle<CachedSVGDocument> m_cachedDocument;
    Timer<SVGElement> m_svgLoadEventTimer;
};

inline SVGUseElement* toSVGUseElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->hasTagName(SVGNames::useTag));
    return static_cast<SVGUseElement*>(node);
}

}

#endif
#endif
