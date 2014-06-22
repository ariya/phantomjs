/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2011 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (C) 2012 University of Szeged
 * Copyright (C) 2012 Renata Hodovan <reni@webkit.org>
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

#if ENABLE(SVG)
#include "SVGUseElement.h"

#include "Attribute.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CachedSVGDocument.h"
#include "Document.h"
#include "ElementShadow.h"
#include "Event.h"
#include "EventListener.h"
#include "HTMLNames.h"
#include "NodeRenderStyle.h"
#include "NodeTraversal.h"
#include "RegisteredEventListener.h"
#include "RenderSVGResource.h"
#include "RenderSVGTransformableContainer.h"
#include "ShadowRoot.h"
#include "SVGElementInstance.h"
#include "SVGElementRareData.h"
#include "SVGElementInstanceList.h"
#include "SVGGElement.h"
#include "SVGLengthContext.h"
#include "SVGNames.h"
#include "SVGSMILElement.h"
#include "SVGSVGElement.h"
#include "SVGSymbolElement.h"
#include "StyleResolver.h"
#include "XLinkNames.h"
#include "XMLDocumentParser.h"
#include "XMLSerializer.h"

// Dump SVGElementInstance object tree - useful to debug instanceRoot problems
// #define DUMP_INSTANCE_TREE

// Dump the deep-expanded shadow tree (where the renderers are built from)
// #define DUMP_SHADOW_TREE

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGUseElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGUseElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGUseElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGUseElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_STRING(SVGUseElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGUseElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGUseElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y)
    REGISTER_LOCAL_ANIMATED_PROPERTY(width)
    REGISTER_LOCAL_ANIMATED_PROPERTY(height)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGUseElement::SVGUseElement(const QualifiedName& tagName, Document* document, bool wasInsertedByParser)
    : SVGGraphicsElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_wasInsertedByParser(wasInsertedByParser)
    , m_haveFiredLoadEvent(false)
    , m_needsShadowTreeRecreation(false)
    , m_svgLoadEventTimer(this, &SVGElement::svgLoadEventTimerFired)
{
    ASSERT(hasCustomStyleCallbacks());
    ASSERT(hasTagName(SVGNames::useTag));
    registerAnimatedPropertiesForSVGUseElement();
}

PassRefPtr<SVGUseElement> SVGUseElement::create(const QualifiedName& tagName, Document* document, bool wasInsertedByParser)
{
    // Always build a #shadow-root for SVGUseElement.
    RefPtr<SVGUseElement> use = adoptRef(new SVGUseElement(tagName, document, wasInsertedByParser));
    use->ensureUserAgentShadowRoot();
    return use.release();
}

SVGUseElement::~SVGUseElement()
{
    setCachedDocument(0);

    clearResourceReferences();
}

SVGElementInstance* SVGUseElement::instanceRoot()
{
    // If there is no element instance tree, force immediate SVGElementInstance tree
    // creation by asking the document to invoke our recalcStyle function - as we can't
    // wait for the lazy creation to happen if e.g. JS wants to access the instanceRoot
    // object right after creating the element on-the-fly
    if (!m_targetElementInstance)
        document()->updateLayoutIgnorePendingStylesheets();

    return m_targetElementInstance.get();
}

SVGElementInstance* SVGUseElement::animatedInstanceRoot() const
{
    // FIXME: Implement me.
    return 0;
}

bool SVGUseElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::xAttr);
        supportedAttributes.add(SVGNames::yAttr);
        supportedAttributes.add(SVGNames::widthAttr);
        supportedAttributes.add(SVGNames::heightAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGUseElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(name))
        SVGGraphicsElement::parseAttribute(name, value);
    else if (name == SVGNames::xAttr)
        setXBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::yAttr)
        setYBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::widthAttr)
        setWidthBaseValue(SVGLength::construct(LengthModeWidth, value, parseError, ForbidNegativeLengths));
    else if (name == SVGNames::heightAttr)
        setHeightBaseValue(SVGLength::construct(LengthModeHeight, value, parseError, ForbidNegativeLengths));
    else if (SVGLangSpace::parseAttribute(name, value)
             || SVGExternalResourcesRequired::parseAttribute(name, value)
             || SVGURIReference::parseAttribute(name, value)) {
    } else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, name, value);
}

static inline bool isWellFormedDocument(Document* document)
{
    if (document->isSVGDocument() || document->isXHTMLDocument())
        return static_cast<XMLDocumentParser*>(document->parser())->wellFormed();
    return true;
}

Node::InsertionNotificationRequest SVGUseElement::insertedInto(ContainerNode* rootParent)
{
    // This functions exists to assure assumptions made in the code regarding SVGElementInstance creation/destruction are satisfied.
    SVGGraphicsElement::insertedInto(rootParent);
    if (!rootParent->inDocument())
        return InsertionDone;
    ASSERT(!m_targetElementInstance || !isWellFormedDocument(document()));
    ASSERT(!hasPendingResources() || !isWellFormedDocument(document()));
    if (!m_wasInsertedByParser)
        buildPendingResource();
    SVGExternalResourcesRequired::insertedIntoDocument(this);
    return InsertionDone;
}

void SVGUseElement::removedFrom(ContainerNode* rootParent)
{
    SVGGraphicsElement::removedFrom(rootParent);
    if (rootParent->inDocument())
        clearResourceReferences();
}

Document* SVGUseElement::referencedDocument() const
{
    if (!isExternalURIReference(href(), document()))
        return document();
    return externalDocument();
}

Document* SVGUseElement::externalDocument() const
{
    if (m_cachedDocument && m_cachedDocument->isLoaded()) {
        // Gracefully handle error condition.
        if (m_cachedDocument->errorOccurred())
            return 0;
        ASSERT(m_cachedDocument->document());
        return m_cachedDocument->document();
    }
    return 0;
}

void SVGUseElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    RenderObject* renderer = this->renderer();
    if (attrName == SVGNames::xAttr
        || attrName == SVGNames::yAttr
        || attrName == SVGNames::widthAttr
        || attrName == SVGNames::heightAttr) {
        updateRelativeLengthsInformation();
        if (renderer)
            RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    if (SVGExternalResourcesRequired::handleAttributeChange(this, attrName))
        return;

    if (SVGURIReference::isKnownAttribute(attrName)) {
        bool isExternalReference = isExternalURIReference(href(), document());
        if (isExternalReference) {
            KURL url = document()->completeURL(href());
            if (url.hasFragmentIdentifier()) {
                CachedResourceRequest request(ResourceRequest(url.string()));
                request.setInitiator(this);
                setCachedDocument(document()->cachedResourceLoader()->requestSVGDocument(request));
            }
        } else
            setCachedDocument(0);

        if (!m_wasInsertedByParser)
            buildPendingResource();

        return;
    }

    if (!renderer)
        return;

    if (SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName)) {
        invalidateShadowTree();
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGUseElement::willRecalcStyle(StyleChange)
{
    if (!m_wasInsertedByParser && m_needsShadowTreeRecreation && renderer() && needsStyleRecalc())
        buildPendingResource();
    return true;
}

#ifdef DUMP_INSTANCE_TREE
static void dumpInstanceTree(unsigned int& depth, String& text, SVGElementInstance* targetInstance)
{
    SVGElement* element = targetInstance->correspondingElement();
    ASSERT(element);

    if (element->hasTagName(SVGNames::useTag)) {
        if (toSVGUseElement(element)->cachedDocumentIsStillLoading())
            return;
    }

    SVGElement* shadowTreeElement = targetInstance->shadowTreeElement();
    ASSERT(shadowTreeElement);

    SVGUseElement* directUseElement = targetInstance->directUseElement();
    String directUseElementName = directUseElement ? directUseElement->nodeName() : "null";

    String elementId = element->getIdAttribute();
    String elementNodeName = element->nodeName();
    String shadowTreeElementNodeName = shadowTreeElement->nodeName();
    String parentNodeName = element->parentNode() ? element->parentNode()->nodeName() : "null";
    String firstChildNodeName = element->firstChild() ? element->firstChild()->nodeName() : "null";

    for (unsigned int i = 0; i < depth; ++i)
        text += "  ";

    text += String::format("SVGElementInstance this=%p, (parentNode=%s (%p), firstChild=%s (%p), correspondingElement=%s (%p), directUseElement=%s (%p), shadowTreeElement=%s (%p), id=%s)\n",
                           targetInstance, parentNodeName.latin1().data(), element->parentNode(), firstChildNodeName.latin1().data(), element->firstChild(),
                           elementNodeName.latin1().data(), element, directUseElementName.latin1().data(), directUseElement, shadowTreeElementNodeName.latin1().data(), shadowTreeElement, elementId.latin1().data());

    for (unsigned int i = 0; i < depth; ++i)
        text += "  ";

    const HashSet<SVGElementInstance*>& elementInstances = element->instancesForElement();
    text += "Corresponding element is associated with " + String::number(elementInstances.size()) + " instance(s):\n";

    const HashSet<SVGElementInstance*>::const_iterator end = elementInstances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = elementInstances.begin(); it != end; ++it) {
        for (unsigned int i = 0; i < depth; ++i)
            text += "  ";

        text += String::format(" -> SVGElementInstance this=%p, (refCount: %i, shadowTreeElement in document? %i)\n",
                               *it, (*it)->refCount(), (*it)->shadowTreeElement()->inDocument());
    }

    ++depth;

    for (SVGElementInstance* instance = targetInstance->firstChild(); instance; instance = instance->nextSibling())
        dumpInstanceTree(depth, text, instance);

    --depth;
}
#endif

static bool isDisallowedElement(Node* node)
{
    // Spec: "Any 'svg', 'symbol', 'g', graphics element or other 'use' is potentially a template object that can be re-used
    // (i.e., "instanced") in the SVG document via a 'use' element."
    // "Graphics Element" is defined as 'circle', 'ellipse', 'image', 'line', 'path', 'polygon', 'polyline', 'rect', 'text'
    // Excluded are anything that is used by reference or that only make sense to appear once in a document.
    // We must also allow the shadow roots of other use elements.
    if (node->isShadowRoot() || node->isTextNode())
        return false;

    if (!node->isSVGElement())
        return true;

    Element* element = toElement(node);

    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, allowedElementTags, ());
    if (allowedElementTags.isEmpty()) {
        allowedElementTags.add(SVGNames::aTag);
        allowedElementTags.add(SVGNames::circleTag);
        allowedElementTags.add(SVGNames::descTag);
        allowedElementTags.add(SVGNames::ellipseTag);
        allowedElementTags.add(SVGNames::gTag);
        allowedElementTags.add(SVGNames::imageTag);
        allowedElementTags.add(SVGNames::lineTag);
        allowedElementTags.add(SVGNames::metadataTag);
        allowedElementTags.add(SVGNames::pathTag);
        allowedElementTags.add(SVGNames::polygonTag);
        allowedElementTags.add(SVGNames::polylineTag);
        allowedElementTags.add(SVGNames::rectTag);
        allowedElementTags.add(SVGNames::svgTag);
        allowedElementTags.add(SVGNames::switchTag);
        allowedElementTags.add(SVGNames::symbolTag);
        allowedElementTags.add(SVGNames::textTag);
        allowedElementTags.add(SVGNames::textPathTag);
        allowedElementTags.add(SVGNames::titleTag);
        allowedElementTags.add(SVGNames::trefTag);
        allowedElementTags.add(SVGNames::tspanTag);
        allowedElementTags.add(SVGNames::useTag);
    }
    return !allowedElementTags.contains<SVGAttributeHashTranslator>(element->tagQName());
}

static bool subtreeContainsDisallowedElement(Node* start)
{
    if (isDisallowedElement(start))
        return true;

    for (Node* cur = start->firstChild(); cur; cur = cur->nextSibling()) {
        if (subtreeContainsDisallowedElement(cur))
            return true;
    }

    return false;
}

void SVGUseElement::clearResourceReferences()
{
    // FIXME: We should try to optimize this, to at least allow partial reclones.
    if (ShadowRoot* shadowTreeRootElement = shadow()->shadowRoot())
        shadowTreeRootElement->removeChildren();

    if (m_targetElementInstance) {
        m_targetElementInstance->detach();
        m_targetElementInstance = 0;
    }

    m_needsShadowTreeRecreation = false;

    ASSERT(document());
    document()->accessSVGExtensions()->removeAllTargetReferencesForElement(this);
}

void SVGUseElement::buildPendingResource()
{
    if (!referencedDocument() || isInShadowTree())
        return;
    clearResourceReferences();
    if (!inDocument())
        return;

    String id;
    Element* target = SVGURIReference::targetElementFromIRIString(href(), document(), &id, externalDocument());
    if (!target || !target->inDocument()) {
        // If we can't find the target of an external element, just give up.
        // We can't observe if the target somewhen enters the external document, nor should we do it.
        if (externalDocument())
            return;
        if (id.isEmpty())
            return;

        referencedDocument()->accessSVGExtensions()->addPendingResource(id, this);
        ASSERT(hasPendingResources());
        return;
    }

    if (target->isSVGElement()) {
        buildShadowAndInstanceTree(toSVGElement(target));
        invalidateDependentShadowTrees();
    }

    ASSERT(!m_needsShadowTreeRecreation);
}

void SVGUseElement::buildShadowAndInstanceTree(SVGElement* target)
{
    ASSERT(!m_targetElementInstance);

    // Do not build the shadow/instance tree for <use> elements living in a shadow tree.
    // The will be expanded soon anyway - see expandUseElementsInShadowTree().
    if (isInShadowTree())
        return;

    // Do not allow self-referencing.
    // 'target' may be null, if it's a non SVG namespaced element.
    if (!target || target == this)
        return;

    // Why a seperated instance/shadow tree? SVG demands it:
    // The instance tree is accesable from JavaScript, and has to
    // expose a 1:1 copy of the referenced tree, whereas internally we need
    // to alter the tree for correct "use-on-symbol", "use-on-svg" support.

    // Build instance tree. Create root SVGElementInstance object for the first sub-tree node.
    //
    // Spec: If the 'use' element references a simple graphics element such as a 'rect', then there is only a
    // single SVGElementInstance object, and the correspondingElement attribute on this SVGElementInstance object
    // is the SVGRectElement that corresponds to the referenced 'rect' element.
    m_targetElementInstance = SVGElementInstance::create(this, this, target);

    // Eventually enter recursion to build SVGElementInstance objects for the sub-tree children
    bool foundProblem = false;
    buildInstanceTree(target, m_targetElementInstance.get(), foundProblem, false);

    if (instanceTreeIsLoading(m_targetElementInstance.get()))
        return;

    // SVG specification does not say a word about <use> & cycles. My view on this is: just ignore it!
    // Non-appearing <use> content is easier to debug, then half-appearing content.
    if (foundProblem) {
        clearResourceReferences();
        return;
    }

    // Assure instance tree building was successfull
    ASSERT(m_targetElementInstance);
    ASSERT(!m_targetElementInstance->shadowTreeElement());
    ASSERT(m_targetElementInstance->correspondingUseElement() == this);
    ASSERT(m_targetElementInstance->directUseElement() == this);
    ASSERT(m_targetElementInstance->correspondingElement() == target);

    ShadowRoot* shadowTreeRootElement = shadow()->shadowRoot();
    ASSERT(shadowTreeRootElement);

    // Build shadow tree from instance tree
    // This also handles the special cases: <use> on <symbol>, <use> on <svg>.
    buildShadowTree(target, m_targetElementInstance.get());

    // Expand all <use> elements in the shadow tree.
    // Expand means: replace the actual <use> element by what it references.
    expandUseElementsInShadowTree(shadowTreeRootElement);

    // Expand all <symbol> elements in the shadow tree.
    // Expand means: replace the actual <symbol> element by the <svg> element.
    expandSymbolElementsInShadowTree(shadowTreeRootElement);

    // Now that the shadow tree is completly expanded, we can associate
    // shadow tree elements <-> instances in the instance tree.
    associateInstancesWithShadowTreeElements(shadowTreeRootElement->firstChild(), m_targetElementInstance.get());

    // If no shadow tree element is present, this means that the reference root
    // element was removed, as it is disallowed (ie. <use> on <foreignObject>)
    // Do NOT leave an inconsistent instance tree around, instead destruct it.
    if (!m_targetElementInstance->shadowTreeElement()) {
        clearResourceReferences();
        return;
    }

    ASSERT(m_targetElementInstance->shadowTreeElement()->parentNode() == shadowTreeRootElement);

    // Transfer event listeners assigned to the referenced element to our shadow tree elements.
    transferEventListenersToShadowTree(m_targetElementInstance.get());

    // Update relative length information.
    updateRelativeLengthsInformation();

    // Eventually dump instance tree
#ifdef DUMP_INSTANCE_TREE
    String text;
    unsigned int depth = 0;

    dumpInstanceTree(depth, text, m_targetElementInstance.get());
    fprintf(stderr, "\nDumping <use> instance tree:\n%s\n", text.latin1().data());
#endif

    // Eventually dump shadow tree
#ifdef DUMP_SHADOW_TREE
    RefPtr<XMLSerializer> serializer = XMLSerializer::create();
    String markup = serializer->serializeToString(shadowTreeRootElement, ASSERT_NO_EXCEPTION);
    fprintf(stderr, "Dumping <use> shadow tree markup:\n%s\n", markup.latin1().data());
#endif
}

RenderObject* SVGUseElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGTransformableContainer(this);
}

static bool isDirectReference(const Node* node)
{
    return node->hasTagName(SVGNames::pathTag)
           || node->hasTagName(SVGNames::rectTag)
           || node->hasTagName(SVGNames::circleTag)
           || node->hasTagName(SVGNames::ellipseTag)
           || node->hasTagName(SVGNames::polygonTag)
           || node->hasTagName(SVGNames::polylineTag)
           || node->hasTagName(SVGNames::textTag);
}

void SVGUseElement::toClipPath(Path& path)
{
    ASSERT(path.isEmpty());

    Node* n = m_targetElementInstance ? m_targetElementInstance->shadowTreeElement() : 0;
    if (!n)
        return;

    if (n->isSVGElement() && toSVGElement(n)->isSVGGraphicsElement()) {
        if (!isDirectReference(n))
            // Spec: Indirect references are an error (14.3.5)
            document()->accessSVGExtensions()->reportError("Not allowed to use indirect reference in <clip-path>");
        else {
            toSVGGraphicsElement(n)->toClipPath(path);
            // FIXME: Avoid manual resolution of x/y here. Its potentially harmful.
            SVGLengthContext lengthContext(this);
            path.translate(FloatSize(x().value(lengthContext), y().value(lengthContext)));
            path.transform(animatedLocalTransform());
        }
    }
}

RenderObject* SVGUseElement::rendererClipChild() const
{
    Node* n = m_targetElementInstance ? m_targetElementInstance->shadowTreeElement() : 0;
    if (!n)
        return 0;

    if (n->isSVGElement() && isDirectReference(n))
        return toSVGElement(n)->renderer();

    return 0;
}

void SVGUseElement::buildInstanceTree(SVGElement* target, SVGElementInstance* targetInstance, bool& foundProblem, bool foundUse)
{
    ASSERT(target);
    ASSERT(targetInstance);

    // Spec: If the referenced object is itself a 'use', or if there are 'use' subelements within the referenced
    // object, the instance tree will contain recursive expansion of the indirect references to form a complete tree.
    bool targetHasUseTag = target->hasTagName(SVGNames::useTag);
    SVGElement* newTarget = 0;
    if (targetHasUseTag) {
        foundProblem = hasCycleUseReferencing(toSVGUseElement(target), targetInstance, newTarget);
        if (foundProblem)
            return;

        // We only need to track first degree <use> dependencies. Indirect references are handled
        // as the invalidation bubbles up the dependency chain.
        if (!foundUse) {
            ASSERT(document());
            document()->accessSVGExtensions()->addElementReferencingTarget(this, target);
            foundUse = true;
        }
    } else if (isDisallowedElement(target)) {
        foundProblem = true;
        return;
    }

    // A general description from the SVG spec, describing what buildInstanceTree() actually does.
    //
    // Spec: If the 'use' element references a 'g' which contains two 'rect' elements, then the instance tree
    // contains three SVGElementInstance objects, a root SVGElementInstance object whose correspondingElement
    // is the SVGGElement object for the 'g', and then two child SVGElementInstance objects, each of which has
    // its correspondingElement that is an SVGRectElement object.

    for (Node* node = target->firstChild(); node; node = node->nextSibling()) {
        SVGElement* element = 0;
        if (node->isSVGElement())
            element = toSVGElement(node);

        // Skip any non-svg nodes or any disallowed element.
        if (!element || isDisallowedElement(element))
            continue;

        // Create SVGElementInstance object, for both container/non-container nodes.
        RefPtr<SVGElementInstance> instance = SVGElementInstance::create(this, 0, element);
        SVGElementInstance* instancePtr = instance.get();
        targetInstance->appendChild(instance.release());

        // Enter recursion, appending new instance tree nodes to the "instance" object.
        buildInstanceTree(element, instancePtr, foundProblem, foundUse);
        if (foundProblem)
            return;
    }

    if (!targetHasUseTag || !newTarget)
        return;

    RefPtr<SVGElementInstance> newInstance = SVGElementInstance::create(this, toSVGUseElement(target), newTarget);
    SVGElementInstance* newInstancePtr = newInstance.get();
    targetInstance->appendChild(newInstance.release());
    buildInstanceTree(newTarget, newInstancePtr, foundProblem, foundUse);
}

bool SVGUseElement::hasCycleUseReferencing(SVGUseElement* use, SVGElementInstance* targetInstance, SVGElement*& newTarget)
{
    Element* targetElement = SVGURIReference::targetElementFromIRIString(use->href(), referencedDocument());
    newTarget = 0;
    if (targetElement && targetElement->isSVGElement())
        newTarget = toSVGElement(targetElement);

    if (!newTarget)
        return false;

    // Shortcut for self-references
    if (newTarget == this)
        return true;

    AtomicString targetId = newTarget->getIdAttribute();
    SVGElementInstance* instance = targetInstance->parentNode();
    while (instance) {
        SVGElement* element = instance->correspondingElement();

        if (element->hasID() && element->getIdAttribute() == targetId && element->document() == newTarget->document())
            return true;

        instance = instance->parentNode();
    }
    return false;
}

static inline void removeDisallowedElementsFromSubtree(Element* subtree)
{
    ASSERT(!subtree->inDocument());
    Element* element = ElementTraversal::firstWithin(subtree);
    while (element) {
        if (isDisallowedElement(element)) {
            Element* next = ElementTraversal::nextSkippingChildren(element, subtree);
            // The subtree is not in document so this won't generate events that could mutate the tree.
            element->parentNode()->removeChild(element);
            element = next;
        } else
            element = ElementTraversal::next(element, subtree);
    }
}

void SVGUseElement::buildShadowTree(SVGElement* target, SVGElementInstance* targetInstance)
{
    // For instance <use> on <foreignObject> (direct case).
    if (isDisallowedElement(target))
        return;

    RefPtr<Element> newChild = targetInstance->correspondingElement()->cloneElementWithChildren();

    // We don't walk the target tree element-by-element, and clone each element,
    // but instead use cloneElementWithChildren(). This is an optimization for the common
    // case where <use> doesn't contain disallowed elements (ie. <foreignObject>).
    // Though if there are disallowed elements in the subtree, we have to remove them.
    // For instance: <use> on <g> containing <foreignObject> (indirect case).
    if (subtreeContainsDisallowedElement(newChild.get()))
        removeDisallowedElementsFromSubtree(newChild.get());

    shadow()->shadowRoot()->appendChild(newChild.release());
}

void SVGUseElement::expandUseElementsInShadowTree(Node* element)
{
    // Why expand the <use> elements in the shadow tree here, and not just
    // do this directly in buildShadowTree, if we encounter a <use> element?
    //
    // Short answer: Because we may miss to expand some elements. Ie. if a <symbol>
    // contains <use> tags, we'd miss them. So once we're done with settin' up the
    // actual shadow tree (after the special case modification for svg/symbol) we have
    // to walk it completely and expand all <use> elements.
    if (element->hasTagName(SVGNames::useTag)) {
        SVGUseElement* use = toSVGUseElement(element);
        ASSERT(!use->cachedDocumentIsStillLoading());

        Element* targetElement = SVGURIReference::targetElementFromIRIString(use->href(), referencedDocument());
        SVGElement* target = 0;
        if (targetElement && targetElement->isSVGElement())
            target = toSVGElement(targetElement);

        // Don't ASSERT(target) here, it may be "pending", too.
        // Setup sub-shadow tree root node
        RefPtr<SVGGElement> cloneParent = SVGGElement::create(SVGNames::gTag, referencedDocument());
        use->cloneChildNodes(cloneParent.get());

        // Spec: In the generated content, the 'use' will be replaced by 'g', where all attributes from the
        // 'use' element except for x, y, width, height and xlink:href are transferred to the generated 'g' element.
        transferUseAttributesToReplacedElement(use, cloneParent.get());

        if (target && !isDisallowedElement(target)) {
            RefPtr<Element> newChild = target->cloneElementWithChildren();
            ASSERT(newChild->isSVGElement());
            cloneParent->appendChild(newChild.release());
        }

        // We don't walk the target tree element-by-element, and clone each element,
        // but instead use cloneElementWithChildren(). This is an optimization for the common
        // case where <use> doesn't contain disallowed elements (ie. <foreignObject>).
        // Though if there are disallowed elements in the subtree, we have to remove them.
        // For instance: <use> on <g> containing <foreignObject> (indirect case).
        if (subtreeContainsDisallowedElement(cloneParent.get()))
            removeDisallowedElementsFromSubtree(cloneParent.get());

        RefPtr<Node> replacingElement(cloneParent.get());

        // Replace <use> with referenced content.
        ASSERT(use->parentNode());
        use->parentNode()->replaceChild(cloneParent.release(), use);

        // Expand the siblings because the *element* is replaced and we will
        // lose the sibling chain when we are back from recursion.
        element = replacingElement.get();
        for (RefPtr<Node> sibling = element->nextSibling(); sibling; sibling = sibling->nextSibling())
            expandUseElementsInShadowTree(sibling.get());
    }

    for (RefPtr<Node> child = element->firstChild(); child; child = child->nextSibling())
        expandUseElementsInShadowTree(child.get());
}

void SVGUseElement::expandSymbolElementsInShadowTree(Node* element)
{
    if (element->hasTagName(SVGNames::symbolTag)) {
        // Spec: The referenced 'symbol' and its contents are deep-cloned into the generated tree,
        // with the exception that the 'symbol' is replaced by an 'svg'. This generated 'svg' will
        // always have explicit values for attributes width and height. If attributes width and/or
        // height are provided on the 'use' element, then these attributes will be transferred to
        // the generated 'svg'. If attributes width and/or height are not specified, the generated
        // 'svg' element will use values of 100% for these attributes.
        RefPtr<SVGSVGElement> svgElement = SVGSVGElement::create(SVGNames::svgTag, referencedDocument());

        // Transfer all data (attributes, etc.) from <symbol> to the new <svg> element.
        svgElement->cloneDataFromElement(*toElement(element));

        // Only clone symbol children, and add them to the new <svg> element
        for (Node* child = element->firstChild(); child; child = child->nextSibling()) {
            RefPtr<Node> newChild = child->cloneNode(true);
            svgElement->appendChild(newChild.release());
        }

        // We don't walk the target tree element-by-element, and clone each element,
        // but instead use cloneNode(deep=true). This is an optimization for the common
        // case where <use> doesn't contain disallowed elements (ie. <foreignObject>).
        // Though if there are disallowed elements in the subtree, we have to remove them.
        // For instance: <use> on <g> containing <foreignObject> (indirect case).
        if (subtreeContainsDisallowedElement(svgElement.get()))
            removeDisallowedElementsFromSubtree(svgElement.get());

        RefPtr<Node> replacingElement(svgElement.get());

        // Replace <symbol> with <svg>.
        element->parentNode()->replaceChild(svgElement.release(), element);

        // Expand the siblings because the *element* is replaced and we will
        // lose the sibling chain when we are back from recursion.
        element = replacingElement.get();
        for (RefPtr<Node> sibling = element->nextSibling(); sibling; sibling = sibling->nextSibling())
            expandSymbolElementsInShadowTree(sibling.get());
    }

    for (RefPtr<Node> child = element->firstChild(); child; child = child->nextSibling())
        expandSymbolElementsInShadowTree(child.get());
}

void SVGUseElement::transferEventListenersToShadowTree(SVGElementInstance* target)
{
    if (!target)
        return;

    SVGElement* originalElement = target->correspondingElement();
    ASSERT(originalElement);

    if (SVGElement* shadowTreeElement = target->shadowTreeElement()) {
        if (EventTargetData* data = originalElement->eventTargetData())
            data->eventListenerMap.copyEventListenersNotCreatedFromMarkupToTarget(shadowTreeElement);
    }

    for (SVGElementInstance* instance = target->firstChild(); instance; instance = instance->nextSibling())
        transferEventListenersToShadowTree(instance);
}

void SVGUseElement::associateInstancesWithShadowTreeElements(Node* target, SVGElementInstance* targetInstance)
{
    if (!target || !targetInstance)
        return;

    SVGElement* originalElement = targetInstance->correspondingElement();

    if (originalElement->hasTagName(SVGNames::useTag)) {
        // <use> gets replaced by <g>
        ASSERT(target->nodeName() == SVGNames::gTag);
    } else if (originalElement->hasTagName(SVGNames::symbolTag)) {
        // <symbol> gets replaced by <svg>
        ASSERT(target->nodeName() == SVGNames::svgTag);
    } else
        ASSERT(target->nodeName() == originalElement->nodeName());

    SVGElement* element = 0;
    if (target->isSVGElement())
        element = toSVGElement(target);

    ASSERT(!targetInstance->shadowTreeElement());
    targetInstance->setShadowTreeElement(element);
    element->setCorrespondingElement(originalElement);

    Node* node = target->firstChild();
    for (SVGElementInstance* instance = targetInstance->firstChild(); node && instance; instance = instance->nextSibling()) {
        // Skip any non-svg elements in shadow tree
        while (node && !node->isSVGElement())
           node = node->nextSibling();

        if (!node)
            break;

        associateInstancesWithShadowTreeElements(node, instance);
        node = node->nextSibling();
    }
}

SVGElementInstance* SVGUseElement::instanceForShadowTreeElement(Node* element) const
{
    if (!m_targetElementInstance) {
        ASSERT(!inDocument());
        return 0;
    }

    return instanceForShadowTreeElement(element, m_targetElementInstance.get());
}

SVGElementInstance* SVGUseElement::instanceForShadowTreeElement(Node* element, SVGElementInstance* instance) const
{
    ASSERT(element);
    ASSERT(instance);

    // We're dispatching a mutation event during shadow tree construction
    // this instance hasn't yet been associated to a shadowTree element.
    if (!instance->shadowTreeElement())
        return 0;

    if (element == instance->shadowTreeElement())
        return instance;

    for (SVGElementInstance* current = instance->firstChild(); current; current = current->nextSibling()) {
        if (SVGElementInstance* search = instanceForShadowTreeElement(element, current))
            return search;
    }

    return 0;
}

void SVGUseElement::invalidateShadowTree()
{
    if (!renderer() || m_needsShadowTreeRecreation)
        return;
    m_needsShadowTreeRecreation = true;
    setNeedsStyleRecalc();
    invalidateDependentShadowTrees();
}

void SVGUseElement::invalidateDependentShadowTrees()
{
    // Recursively invalidate dependent <use> shadow trees
    const HashSet<SVGElementInstance*>& instances = instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        if (SVGUseElement* element = (*it)->correspondingUseElement()) {
            ASSERT(element->inDocument());
            element->invalidateShadowTree();
        }
    }
}

void SVGUseElement::transferUseAttributesToReplacedElement(SVGElement* from, SVGElement* to) const
{
    ASSERT(from);
    ASSERT(to);

    to->cloneDataFromElement(*from);

    to->removeAttribute(SVGNames::xAttr);
    to->removeAttribute(SVGNames::yAttr);
    to->removeAttribute(SVGNames::widthAttr);
    to->removeAttribute(SVGNames::heightAttr);
    to->removeAttribute(XLinkNames::hrefAttr);
}

bool SVGUseElement::selfHasRelativeLengths() const
{
    if (x().isRelative()
     || y().isRelative()
     || width().isRelative()
     || height().isRelative())
        return true;

    if (!m_targetElementInstance)
        return false;

    SVGElement* element = m_targetElementInstance->correspondingElement();
    if (!element || !element->isSVGStyledElement())
        return false;

    return toSVGStyledElement(element)->hasRelativeLengths();
}

void SVGUseElement::notifyFinished(CachedResource* resource)
{
    if (!inDocument())
        return;

    invalidateShadowTree();
    if (resource->errorOccurred())
        dispatchEvent(Event::create(eventNames().errorEvent, false, false));
    else if (!resource->wasCanceled())
        SVGExternalResourcesRequired::dispatchLoadEvent(this);
}

bool SVGUseElement::cachedDocumentIsStillLoading()
{
    if (m_cachedDocument && m_cachedDocument->isLoading())
        return true;
    return false;
}

bool SVGUseElement::instanceTreeIsLoading(SVGElementInstance* targetElementInstance)
{
    for (SVGElementInstance* instance = targetElementInstance->firstChild(); instance; instance = instance->nextSibling()) {
        if (SVGUseElement* use = instance->correspondingUseElement()) {
             if (use->cachedDocumentIsStillLoading())
                 return true;
        }
        if (instance->hasChildNodes())
            instanceTreeIsLoading(instance);
    }
    return false;
}

void SVGUseElement::finishParsingChildren()
{
    SVGGraphicsElement::finishParsingChildren();
    SVGExternalResourcesRequired::finishParsingChildren();
    if (m_wasInsertedByParser) {
        buildPendingResource();
        m_wasInsertedByParser = false;
    }
}

void SVGUseElement::setCachedDocument(CachedResourceHandle<CachedSVGDocument> cachedDocument)
{
    if (m_cachedDocument == cachedDocument)
        return;

    if (m_cachedDocument)
        m_cachedDocument->removeClient(this);

    m_cachedDocument = cachedDocument;
    if (m_cachedDocument)
        m_cachedDocument->addClient(this);
}

}

#endif // ENABLE(SVG)
