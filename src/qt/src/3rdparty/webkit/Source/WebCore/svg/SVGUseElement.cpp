/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2011 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "CSSStyleSelector.h"
#include "Document.h"
#include "Event.h"
#include "EventListener.h"
#include "HTMLNames.h"
#include "NodeRenderStyle.h"
#include "RegisteredEventListener.h"
#include "RenderSVGResource.h"
#include "RenderSVGShadowTreeRootContainer.h"
#include "SVGElementInstance.h"
#include "SVGElementInstanceList.h"
#include "SVGGElement.h"
#include "SVGNames.h"
#include "SVGSMILElement.h"
#include "SVGSVGElement.h"
#include "SVGShadowTreeElements.h"
#include "SVGSymbolElement.h"
#include "XLinkNames.h"
#include "XMLDocumentParser.h"
#include "XMLSerializer.h"

#include <wtf/text/StringConcatenate.h>

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

inline SVGUseElement::SVGUseElement(const QualifiedName& tagName, Document* document)
    : SVGStyledTransformableElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
    , m_updatesBlocked(false)
    , m_needsShadowTreeRecreation(false)
{
}

PassRefPtr<SVGUseElement> SVGUseElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGUseElement(tagName, document));
}

SVGElementInstance* SVGUseElement::instanceRoot() const
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
 
void SVGUseElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
        if (widthBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for use attribute <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
        if (heightBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for use attribute <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        SVGStyledTransformableElement::parseMappedAttribute(attr);
    }
}

static inline bool isWellFormedDocument(Document* document)
{
    if (document->isSVGDocument() || document->isXHTMLDocument())
        return static_cast<XMLDocumentParser*>(document->parser())->wellFormed();
    return true;
}

void SVGUseElement::insertedIntoDocument()
{
    // This functions exists to assure assumptions made in the code regarding SVGElementInstance creation/destruction are satisfied.
    SVGStyledTransformableElement::insertedIntoDocument();
    ASSERT(!m_targetElementInstance || !isWellFormedDocument(document()));
    ASSERT(!hasPendingResources() || !isWellFormedDocument(document()));
}

void SVGUseElement::removedFromDocument()
{
    SVGStyledTransformableElement::removedFromDocument();
    detachInstance();
}

void SVGUseElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::svgAttributeChanged(attrName);

    bool isXYAttribute = attrName == SVGNames::xAttr || attrName == SVGNames::yAttr;
    bool isWidthHeightAttribute = attrName == SVGNames::widthAttr || attrName == SVGNames::heightAttr;

    if (isXYAttribute || isWidthHeightAttribute)
        updateRelativeLengthsInformation();

    if (SVGTests::handleAttributeChange(this, attrName))
        return;

    RenderObject* object = renderer();
    if (!object)
        return;

    if (SVGURIReference::isKnownAttribute(attrName)) {
        if (hasPendingResources()) {
            OwnPtr<SVGDocumentExtensions::SVGPendingElements> clients(document()->accessSVGExtensions()->removePendingResource(m_resourceId));
            ASSERT(!clients->isEmpty());

            const SVGDocumentExtensions::SVGPendingElements::const_iterator end = clients->end();
            for (SVGDocumentExtensions::SVGPendingElements::const_iterator it = clients->begin(); it != end; ++it) {
                ASSERT((*it)->hasPendingResources());
                (*it)->clearHasPendingResourcesIfPossible();
            }

            m_resourceId = String();
            clearHasPendingResourcesIfPossible();
        }

        invalidateShadowTree();
        return;
    }

    if (isXYAttribute) {
        updateContainerOffsets();
        return;
    }

    if (isWidthHeightAttribute) {
        updateContainerSizes();
        return;
    }

    // Be very careful here, if svgAttributeChanged() has been called because a SVG CSS property changed, we do NOT want to reclone the tree!
    if (SVGStyledElement::isKnownAttribute(attrName)) {
        setNeedsStyleRecalc();
        return;
    }

    if (SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName))
        invalidateShadowTree();
}

void SVGUseElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledTransformableElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeExternalResourcesRequired();
        synchronizeHref();
        SVGTests::synchronizeProperties(this, attrName);
        return;
    }

    if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::widthAttr)
        synchronizeWidth();
    else if (attrName == SVGNames::heightAttr)
        synchronizeHeight();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGURIReference::isKnownAttribute(attrName))
        synchronizeHref();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

AttributeToPropertyTypeMap& SVGUseElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGUseElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();

    SVGStyledTransformableElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::widthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::heightAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(XLinkNames::hrefAttr, AnimatedString);
}

static void updateContainerSize(SVGElementInstance* targetInstance)
{
    // Depth-first used to write the method in early exit style, no particular other reason.
    for (SVGElementInstance* instance = targetInstance->firstChild(); instance; instance = instance->nextSibling())
        updateContainerSize(instance);

    SVGUseElement* useElement = targetInstance->directUseElement();
    if (!useElement)
        return;

    SVGElement* correspondingElement = targetInstance->correspondingElement();
    ASSERT(correspondingElement);

    bool isSymbolTag = correspondingElement->hasTagName(SVGNames::symbolTag);
    if (!correspondingElement->hasTagName(SVGNames::svgTag) && !isSymbolTag)
        return;

    SVGElement* shadowTreeElement = targetInstance->shadowTreeElement();
    ASSERT(shadowTreeElement);
    ASSERT(shadowTreeElement->hasTagName(SVGNames::svgTag));

    // Spec (<use> on <symbol>): This generated 'svg' will always have explicit values for attributes width and height.
    // If attributes width and/or height are provided on the 'use' element, then these attributes
    // will be transferred to the generated 'svg'. If attributes width and/or height are not specified,
    // the generated 'svg' element will use values of 100% for these attributes.
    
    // Spec (<use> on <svg>): If attributes width and/or height are provided on the 'use' element, then these
    // values will override the corresponding attributes on the 'svg' in the generated tree.

    if (useElement->hasAttribute(SVGNames::widthAttr))
        shadowTreeElement->setAttribute(SVGNames::widthAttr, useElement->getAttribute(SVGNames::widthAttr));
    else if (isSymbolTag && shadowTreeElement->hasAttribute(SVGNames::widthAttr))
        shadowTreeElement->setAttribute(SVGNames::widthAttr, "100%");

    if (useElement->hasAttribute(SVGNames::heightAttr))
        shadowTreeElement->setAttribute(SVGNames::heightAttr, useElement->getAttribute(SVGNames::heightAttr));
    else if (isSymbolTag && shadowTreeElement->hasAttribute(SVGNames::heightAttr))
        shadowTreeElement->setAttribute(SVGNames::heightAttr, "100%");
}   

void SVGUseElement::updateContainerSizes()
{
    if (!m_targetElementInstance)
        return;

    // Update whole subtree, scanning for shadow container elements, that correspond to <svg>/<symbol> tags
    ASSERT(m_targetElementInstance->directUseElement() == this);
    updateContainerSize(m_targetElementInstance.get());

    if (RenderObject* object = renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(object);
}

static void updateContainerOffset(SVGElementInstance* targetInstance)
{
    // Depth-first used to write the method in early exit style, no particular other reason.
    for (SVGElementInstance* instance = targetInstance->firstChild(); instance; instance = instance->nextSibling())
        updateContainerOffset(instance);

    SVGElement* correspondingElement = targetInstance->correspondingElement();
    ASSERT(correspondingElement);

    if (!correspondingElement->hasTagName(SVGNames::useTag))
        return;

    SVGElement* shadowTreeElement = targetInstance->shadowTreeElement();
    ASSERT(shadowTreeElement);
    ASSERT(shadowTreeElement->hasTagName(SVGNames::gTag));

    if (!static_cast<SVGGElement*>(shadowTreeElement)->isShadowTreeContainerElement())
        return;

    // Spec: An additional transformation translate(x,y) is appended to the end
    // (i.e., right-side) of the transform attribute on the generated 'g', where x
    // and y represent the values of the x and y attributes on the 'use' element. 
    SVGUseElement* useElement = static_cast<SVGUseElement*>(correspondingElement);
    SVGShadowTreeContainerElement* containerElement = static_cast<SVGShadowTreeContainerElement*>(shadowTreeElement);
    containerElement->setContainerOffset(useElement->x(), useElement->y());
}

void SVGUseElement::updateContainerOffsets()
{
    if (!m_targetElementInstance)
        return;

    // Update root container offset (not reachable through instance tree)
    SVGElement* shadowRoot = m_targetElementInstance->shadowTreeElement();
    ASSERT(shadowRoot);

    ContainerNode* parentNode = shadowRoot->parentNode();
    ASSERT(parentNode);
    ASSERT(parentNode->isSVGElement());
    ASSERT(parentNode->hasTagName(SVGNames::gTag));
    ASSERT(static_cast<SVGGElement*>(parentNode)->isShadowTreeContainerElement());

    SVGShadowTreeContainerElement* containerElement = static_cast<SVGShadowTreeContainerElement*>(parentNode);
    containerElement->setContainerOffset(x(), y());

    // Update whole subtree, scanning for shadow container elements, marking a cloned use subtree
    updateContainerOffset(m_targetElementInstance.get());

    if (RenderObject* object = renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(object);
}

void SVGUseElement::recalcStyle(StyleChange change)
{
    // Eventually mark shadow root element needing style recalc
    if ((change >= Inherit || needsStyleRecalc() || childNeedsStyleRecalc()) && m_targetElementInstance && !m_updatesBlocked) {
        if (SVGElement* shadowRoot = m_targetElementInstance->shadowTreeElement())
            shadowRoot->setNeedsStyleRecalc();
    }

    SVGStyledTransformableElement::recalcStyle(change);

    // Assure that the shadow tree has not been marked for recreation, while we're building it.
    if (m_updatesBlocked)
        ASSERT(!m_needsShadowTreeRecreation);

    RenderSVGShadowTreeRootContainer* shadowRoot = static_cast<RenderSVGShadowTreeRootContainer*>(renderer());
    if (!shadowRoot)
        return;
    
    bool needsStyleUpdate = !m_needsShadowTreeRecreation;
    if (m_needsShadowTreeRecreation) {
        shadowRoot->markShadowTreeForRecreation();
        m_needsShadowTreeRecreation = false;
    }

    shadowRoot->updateFromElement();

    if (!needsStyleUpdate)
        return;

    shadowRoot->updateStyle(change);
}

#ifdef DUMP_INSTANCE_TREE
void dumpInstanceTree(unsigned int& depth, String& text, SVGElementInstance* targetInstance)
{
    SVGElement* element = targetInstance->correspondingElement();
    ASSERT(element);

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
    text += makeString("Corresponding element is associated with ", String::number(elementInstances.size()), " instance(s):\n");

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

static bool isDisallowedElement(Node* element)
{
#if ENABLE(SVG_FOREIGN_OBJECT)
    // <foreignObject> should never be contained in a <use> tree. Too dangerous side effects possible.
    if (element->hasTagName(SVGNames::foreignObjectTag))
        return true;
#endif
#if ENABLE(SVG_ANIMATION)
    if (SVGSMILElement::isSMILElement(element))
        return true;
#endif

    return false;
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

void SVGUseElement::buildPendingResource()
{
    // If we're called the first time (during shadow tree root creation from RenderSVGShadowTreeRootContainer)
    // we either determine that our target is available or not - then we add ourselves to the pending resource list
    // Once the pending resource appears, it will call buildPendingResource(), so we're called a second time.
    String id = SVGURIReference::getTarget(href());
    Element* targetElement = treeScope()->getElementById(id);
    ASSERT(!m_targetElementInstance);

    if (!targetElement) {
        if (hasPendingResources() || id.isEmpty())
            return;

        m_resourceId = id;
        ASSERT(!hasPendingResources());
        document()->accessSVGExtensions()->addPendingResource(id, this);
        ASSERT(hasPendingResources());
        return;
    }


    if (hasPendingResources()) {
        ASSERT(!m_targetElementInstance);
        m_resourceId = String();
        invalidateShadowTree();
    }
}

void SVGUseElement::buildShadowAndInstanceTree(SVGShadowTreeRootElement* shadowRoot)
{
    struct ShadowTreeUpdateBlocker {
        ShadowTreeUpdateBlocker(SVGUseElement* currentUseElement)
            : useElement(currentUseElement)
        {
            useElement->setUpdatesBlocked(true);
        }

        ~ShadowTreeUpdateBlocker()
        {
            useElement->setUpdatesBlocked(false);
        }

        SVGUseElement* useElement;
    };

    // When cloning the target nodes, they may decide to synchronize style and/or animated SVG attributes.
    // That causes calls to SVGElementInstance::updateAllInstancesOfElement(), which mark the shadow tree for recreation.
    // Solution: block any updates to the shadow tree while we're building it.
    ShadowTreeUpdateBlocker blocker(this);

    String id = SVGURIReference::getTarget(href());
    Element* targetElement = treeScope()->getElementById(id);
    if (!targetElement) {
        // The only time we should get here is when the use element has not been
        // given a resource to target.
        ASSERT(m_resourceId.isEmpty());
        return;
    }

    // Do not build the shadow/instance tree for <use> elements living in a shadow tree.
    // The will be expanded soon anyway - see expandUseElementsInShadowTree().
    ContainerNode* parent = parentNode();
    while (parent) {
        if (parent->isSVGShadowRoot())
            return;

        parent = parent->parentNodeGuaranteedHostFree();
    }

    SVGElement* target = 0;
    if (targetElement && targetElement->isSVGElement())
        target = static_cast<SVGElement*>(targetElement);

    detachInstance();

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
    buildInstanceTree(target, m_targetElementInstance.get(), foundProblem);

    // SVG specification does not say a word about <use> & cycles. My view on this is: just ignore it!
    // Non-appearing <use> content is easier to debug, then half-appearing content.
    if (foundProblem) {
        detachInstance();
        return;
    }

    // Assure instance tree building was successfull
    ASSERT(m_targetElementInstance);
    ASSERT(!m_targetElementInstance->shadowTreeElement());
    ASSERT(m_targetElementInstance->correspondingUseElement() == this);
    ASSERT(m_targetElementInstance->directUseElement() == this);
    ASSERT(m_targetElementInstance->correspondingElement() == target);

    // Build shadow tree from instance tree
    // This also handles the special cases: <use> on <symbol>, <use> on <svg>.
    buildShadowTree(shadowRoot, target, m_targetElementInstance.get());

#if ENABLE(SVG) && ENABLE(SVG_USE)
    // Expand all <use> elements in the shadow tree.
    // Expand means: replace the actual <use> element by what it references.
    expandUseElementsInShadowTree(shadowRoot);

    // Expand all <symbol> elements in the shadow tree.
    // Expand means: replace the actual <symbol> element by the <svg> element.
    expandSymbolElementsInShadowTree(shadowRoot);
#endif

    // Now that the shadow tree is completly expanded, we can associate
    // shadow tree elements <-> instances in the instance tree.
    associateInstancesWithShadowTreeElements(shadowRoot->firstChild(), m_targetElementInstance.get());

    // If no shadow tree element is present, this means that the reference root
    // element was removed, as it is disallowed (ie. <use> on <foreignObject>)
    // Do NOT leave an inconsistent instance tree around, instead destruct it.
    if (!m_targetElementInstance->shadowTreeElement()) {
        shadowRoot->removeAllChildren();
        detachInstance();
        return;
    }

    // Consistency checks - this is assumed in updateContainerOffset().
    ASSERT(m_targetElementInstance->shadowTreeElement()->parentNode() == shadowRoot);

    // Transfer event listeners assigned to the referenced element to our shadow tree elements.
    transferEventListenersToShadowTree(m_targetElementInstance.get());

    // Update container offset/size
    updateContainerOffsets();
    updateContainerSizes();

    // Update relative length information
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
    ExceptionCode ec = 0;

    RefPtr<XMLSerializer> serializer = XMLSerializer::create();

    String markup = serializer->serializeToString(shadowRoot, ec);
    ASSERT(!ec);

    fprintf(stderr, "Dumping <use> shadow tree markup:\n%s\n", markup.latin1().data());
#endif
}

void SVGUseElement::detachInstance()
{
    if (!m_targetElementInstance)
        return;
    m_targetElementInstance->clearUseElements();
    m_targetElementInstance->clearChildren();
    m_targetElementInstance = 0;
}

RenderObject* SVGUseElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGShadowTreeRootContainer(this);
}

static void updateFromElementCallback(Node* node)
{
    if (RenderObject* renderer = node->renderer())
        renderer->updateFromElement();
}

void SVGUseElement::attach()
{
    SVGStyledTransformableElement::attach();

    if (renderer())
        queuePostAttachCallback(updateFromElementCallback, this);
}

void SVGUseElement::detach()
{
    SVGStyledTransformableElement::detach();
    detachInstance();
}

static bool isDirectReference(Node* node)
{
    return node->hasTagName(SVGNames::pathTag)
           || node->hasTagName(SVGNames::rectTag)
           || node->hasTagName(SVGNames::circleTag)
           || node->hasTagName(SVGNames::ellipseTag)
           || node->hasTagName(SVGNames::polygonTag)
           || node->hasTagName(SVGNames::polylineTag)
           || node->hasTagName(SVGNames::textTag);
}

void SVGUseElement::toClipPath(Path& path) const
{
    ASSERT(path.isEmpty());

    Node* n = m_targetElementInstance ? m_targetElementInstance->shadowTreeElement() : 0;
    if (!n)
        return;

    if (n->isSVGElement() && static_cast<SVGElement*>(n)->isStyledTransformable()) {
        if (!isDirectReference(n))
            // Spec: Indirect references are an error (14.3.5)
            document()->accessSVGExtensions()->reportError("Not allowed to use indirect reference in <clip-path>");
        else {
            static_cast<SVGStyledTransformableElement*>(n)->toClipPath(path);
            path.translate(FloatSize(x().value(this), y().value(this)));
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
        return static_cast<SVGElement*>(n)->renderer();

    return 0;
}

void SVGUseElement::buildInstanceTree(SVGElement* target, SVGElementInstance* targetInstance, bool& foundProblem)
{
    ASSERT(target);
    ASSERT(targetInstance);

    // Spec: If the referenced object is itself a 'use', or if there are 'use' subelements within the referenced
    // object, the instance tree will contain recursive expansion of the indirect references to form a complete tree.
    bool targetHasUseTag = target->hasTagName(SVGNames::useTag);
    SVGElement* newTarget = 0;
    if (targetHasUseTag) {
        foundProblem = hasCycleUseReferencing(static_cast<SVGUseElement*>(target), targetInstance, newTarget);
        if (foundProblem)
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
            element = static_cast<SVGElement*>(node);

        // Skip any non-svg nodes or any disallowed element.
        if (!element || isDisallowedElement(element))
            continue;

        // Create SVGElementInstance object, for both container/non-container nodes.
        RefPtr<SVGElementInstance> instance = SVGElementInstance::create(this, 0, element);
        SVGElementInstance* instancePtr = instance.get();
        targetInstance->appendChild(instance.release());

        // Enter recursion, appending new instance tree nodes to the "instance" object.
        buildInstanceTree(element, instancePtr, foundProblem);
        if (foundProblem)
            return;
    }

    if (!targetHasUseTag || !newTarget)
        return;

    RefPtr<SVGElementInstance> newInstance = SVGElementInstance::create(this, static_cast<SVGUseElement*>(target), newTarget);
    SVGElementInstance* newInstancePtr = newInstance.get();
    targetInstance->appendChild(newInstance.release());
    buildInstanceTree(newTarget, newInstancePtr, foundProblem);
}

bool SVGUseElement::hasCycleUseReferencing(SVGUseElement* use, SVGElementInstance* targetInstance, SVGElement*& newTarget)
{
    String id = SVGURIReference::getTarget(use->href());
    Element* targetElement = treeScope()->getElementById(id); 
    newTarget = 0;
    if (targetElement && targetElement->isSVGElement())
        newTarget = static_cast<SVGElement*>(targetElement);

    if (!newTarget)
        return false;

    // Shortcut for self-references
    if (newTarget == this)
        return true;

    SVGElementInstance* instance = targetInstance->parentNode();
    while (instance) {
        SVGElement* element = instance->correspondingElement();

        // FIXME: This should probably be using getIdAttribute instead of idForStyleResolution.
        if (element->hasID() && element->idForStyleResolution() == id)
            return true;
    
        instance = instance->parentNode();
    }
    return false;
}

void SVGUseElement::removeDisallowedElementsFromSubtree(Node* subtree)
{
    ASSERT(!subtree->inDocument());
    ExceptionCode ec;
    Node* node = subtree->firstChild();
    while (node) {
        if (isDisallowedElement(node)) {
            Node* next = node->traverseNextSibling(subtree);
            // The subtree is not in document so this won't generate events that could mutate the tree.
            node->parentNode()->removeChild(node, ec);
            node = next;
        } else
            node = node->traverseNextNode(subtree);
    }
}

void SVGUseElement::buildShadowTree(SVGShadowTreeRootElement* shadowRoot, SVGElement* target, SVGElementInstance* targetInstance)
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

    SVGElement* newChildPtr = 0;
    if (newChild->isSVGElement())
        newChildPtr = static_cast<SVGElement*>(newChild.get());
    ASSERT_UNUSED(newChildPtr, newChildPtr);

    ExceptionCode ec = 0;
    shadowRoot->appendChild(newChild.release(), ec);
    ASSERT(!ec);
}

#if ENABLE(SVG) && ENABLE(SVG_USE)
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
        SVGUseElement* use = static_cast<SVGUseElement*>(element);

        String id = SVGURIReference::getTarget(use->href());
        Element* targetElement = treeScope()->getElementById(id); 
        SVGElement* target = 0;
        if (targetElement && targetElement->isSVGElement())
            target = static_cast<SVGElement*>(targetElement);

        // Don't ASSERT(target) here, it may be "pending", too.
        // Setup sub-shadow tree root node
        RefPtr<SVGShadowTreeContainerElement> cloneParent = SVGShadowTreeContainerElement::create(document());
        use->cloneChildNodes(cloneParent.get());

        // Spec: In the generated content, the 'use' will be replaced by 'g', where all attributes from the
        // 'use' element except for x, y, width, height and xlink:href are transferred to the generated 'g' element.
        transferUseAttributesToReplacedElement(use, cloneParent.get());

        ExceptionCode ec = 0;
        if (target && !isDisallowedElement(target)) {
            RefPtr<Element> newChild = target->cloneElementWithChildren();

            SVGElement* newChildPtr = 0;
            if (newChild->isSVGElement())
                newChildPtr = static_cast<SVGElement*>(newChild.get());
            ASSERT_UNUSED(newChildPtr, newChildPtr);

            cloneParent->appendChild(newChild.release(), ec);
            ASSERT(!ec);
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
        use->parentNode()->replaceChild(cloneParent.release(), use, ec);
        ASSERT(!ec);

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
        RefPtr<SVGSVGElement> svgElement = SVGSVGElement::create(SVGNames::svgTag, document());

        // Transfer all attributes from <symbol> to the new <svg> element
        svgElement->attributes()->setAttributes(*element->attributes());

        // Only clone symbol children, and add them to the new <svg> element    
        ExceptionCode ec = 0;
        for (Node* child = element->firstChild(); child; child = child->nextSibling()) {
            RefPtr<Node> newChild = child->cloneNode(true);
            svgElement->appendChild(newChild.release(), ec);
            ASSERT(!ec);
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
        ASSERT(element->parentNode()); 
        element->parentNode()->replaceChild(svgElement.release(), element, ec);
        ASSERT(!ec);

        // Expand the siblings because the *element* is replaced and we will
        // lose the sibling chain when we are back from recursion.
        element = replacingElement.get();
        for (RefPtr<Node> sibling = element->nextSibling(); sibling; sibling = sibling->nextSibling())
            expandSymbolElementsInShadowTree(sibling.get());
    }

    for (RefPtr<Node> child = element->firstChild(); child; child = child->nextSibling())
        expandSymbolElementsInShadowTree(child.get());
}

#endif

void SVGUseElement::transferEventListenersToShadowTree(SVGElementInstance* target)
{
    if (!target)
        return;

    SVGElement* originalElement = target->correspondingElement();
    ASSERT(originalElement);

    if (SVGElement* shadowTreeElement = target->shadowTreeElement()) {
        if (EventTargetData* d = originalElement->eventTargetData()) {
            EventListenerMap& map = d->eventListenerMap;
            EventListenerMap::iterator end = map.end();
            for (EventListenerMap::iterator it = map.begin(); it != end; ++it) {
                EventListenerVector& entry = *it->second;
                for (size_t i = 0; i < entry.size(); ++i) {
                    // Event listeners created from markup have already been transfered to the shadow tree during cloning.
                    if (entry[i].listener->wasCreatedFromMarkup())
                        continue;
                    shadowTreeElement->addEventListener(it->first, entry[i].listener, entry[i].useCapture);
                }
            }
        }
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
#if ENABLE(SVG) && ENABLE(SVG_USE)
        // <use> gets replaced by <g>
        ASSERT(target->nodeName() == SVGNames::gTag);
#else 
        ASSERT(target->nodeName() == SVGNames::gTag || target->nodeName() == SVGNames::useTag);
#endif
    } else if (originalElement->hasTagName(SVGNames::symbolTag)) {
        // <symbol> gets replaced by <svg>
#if ENABLE(SVG) && ENABLE(SVG_USE) && ENABLE(SVG_FOREIGN_OBJECT)
        ASSERT(target->nodeName() == SVGNames::svgTag);
#endif
    } else
        ASSERT(target->nodeName() == originalElement->nodeName());

    SVGElement* element = 0;
    if (target->isSVGElement())
        element = static_cast<SVGElement*>(target);

    ASSERT(!targetInstance->shadowTreeElement());
    targetInstance->setShadowTreeElement(element);

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
    // Don't mutate the shadow tree while we're building it.
    if (m_updatesBlocked)
        return;

    m_needsShadowTreeRecreation = true;
    setNeedsStyleRecalc();
}

void SVGUseElement::transferUseAttributesToReplacedElement(SVGElement* from, SVGElement* to) const
{
    ASSERT(from);
    ASSERT(to);

    to->attributes()->setAttributes(*from->attributes());

    ExceptionCode ec = 0;

    to->removeAttribute(SVGNames::xAttr, ec);
    ASSERT(!ec);

    to->removeAttribute(SVGNames::yAttr, ec);
    ASSERT(!ec);

    to->removeAttribute(SVGNames::widthAttr, ec);
    ASSERT(!ec);

    to->removeAttribute(SVGNames::heightAttr, ec);
    ASSERT(!ec);

    to->removeAttribute(XLinkNames::hrefAttr, ec);
    ASSERT(!ec);
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
    if (!element || !element->isStyled())
        return false;

    return static_cast<SVGStyledElement*>(element)->hasRelativeLengths();
}

}

#endif // ENABLE(SVG)
