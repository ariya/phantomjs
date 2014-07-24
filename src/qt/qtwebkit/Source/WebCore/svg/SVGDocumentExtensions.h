/*
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGDocumentExtensions_h
#define SVGDocumentExtensions_h

#if ENABLE(SVG)
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class Document;
class RenderSVGResourceContainer;
class SVGElement;
#if ENABLE(SVG_FONTS)
class SVGFontFaceElement;
#endif
class SVGResourcesCache;
class SVGSMILElement;
class SVGSVGElement;
class Element;

class SVGDocumentExtensions {
    WTF_MAKE_NONCOPYABLE(SVGDocumentExtensions); WTF_MAKE_FAST_ALLOCATED;
public:
    typedef HashSet<Element*> SVGPendingElements;
    SVGDocumentExtensions(Document*);
    ~SVGDocumentExtensions();
    
    void addTimeContainer(SVGSVGElement*);
    void removeTimeContainer(SVGSVGElement*);

    void addResource(const AtomicString& id, RenderSVGResourceContainer*);
    void removeResource(const AtomicString& id);
    RenderSVGResourceContainer* resourceById(const AtomicString& id) const;

    void startAnimations();
    void pauseAnimations();
    void unpauseAnimations();
    void dispatchSVGLoadEventToOutermostSVGElements();

    void reportWarning(const String&);
    void reportError(const String&);

    SVGResourcesCache* resourcesCache() const { return m_resourcesCache.get(); }

    HashSet<SVGElement*>* setOfElementsReferencingTarget(SVGElement* referencedElement) const;
    void addElementReferencingTarget(SVGElement* referencingElement, SVGElement* referencedElement);
    void removeAllTargetReferencesForElement(SVGElement*);
    void rebuildAllElementReferencesForTarget(SVGElement*);
    void removeAllElementReferencesForTarget(SVGElement*);

#if ENABLE(SVG_FONTS)
    const HashSet<SVGFontFaceElement*>& svgFontFaceElements() const { return m_svgFontFaceElements; }
    void registerSVGFontFaceElement(SVGFontFaceElement*);
    void unregisterSVGFontFaceElement(SVGFontFaceElement*);
#endif

private:
    Document* m_document; // weak reference
    HashSet<SVGSVGElement*> m_timeContainers; // For SVG 1.2 support this will need to be made more general.
#if ENABLE(SVG_FONTS)
    HashSet<SVGFontFaceElement*> m_svgFontFaceElements;
#endif
    HashMap<AtomicString, RenderSVGResourceContainer*> m_resources;
    HashMap<AtomicString, OwnPtr<SVGPendingElements> > m_pendingResources; // Resources that are pending.
    HashMap<AtomicString, OwnPtr<SVGPendingElements> > m_pendingResourcesForRemoval; // Resources that are pending and scheduled for removal.
    HashMap<SVGElement*, OwnPtr<HashSet<SVGElement*> > > m_elementDependencies;
    OwnPtr<SVGResourcesCache> m_resourcesCache;

public:
    // This HashMap contains a list of pending resources. Pending resources, are such
    // which are referenced by any object in the SVG document, but do NOT exist yet.
    // For instance, dynamically build gradients / patterns / clippers...
    void addPendingResource(const AtomicString& id, Element*);
    bool hasPendingResource(const AtomicString& id) const;
    bool isElementPendingResources(Element*) const;
    bool isElementPendingResource(Element*, const AtomicString& id) const;
    void clearHasPendingResourcesIfPossible(Element*);
    void removeElementFromPendingResources(Element*);
    PassOwnPtr<SVGPendingElements> removePendingResource(const AtomicString& id);

    // The following two functions are used for scheduling a pending resource to be removed.
    void markPendingResourcesForRemoval(const AtomicString&);
    Element* removeElementFromPendingResourcesForRemoval(const AtomicString&);

private:
    PassOwnPtr<SVGPendingElements> removePendingResourceForRemoval(const AtomicString&);
};

}

#endif
#endif
