/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "RenderObject.h"

#include "AXObjectCache.h"
#include "AnimationController.h"
#include "ContentData.h"
#include "CursorList.h"
#include "EventHandler.h"
#include "FloatQuad.h"
#include "FlowThreadController.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLAnchorElement.h"
#include "HTMLElement.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "HitTestResult.h"
#include "LogicalSelectionOffsetCaches.h"
#include "Page.h"
#include "RenderArena.h"
#include "RenderCounter.h"
#include "RenderDeprecatedFlexibleBox.h"
#include "RenderFlexibleBox.h"
#include "RenderGeometryMap.h"
#include "RenderGrid.h"
#include "RenderImage.h"
#include "RenderImageResourceStyleImage.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "RenderListItem.h"
#include "RenderMultiColumnBlock.h"
#include "RenderNamedFlowThread.h"
#include "RenderRegion.h"
#include "RenderRuby.h"
#include "RenderRubyText.h"
#include "RenderScrollbarPart.h"
#include "RenderTableCaption.h"
#include "RenderTableCell.h"
#include "RenderTableCol.h"
#include "RenderTableRow.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "Settings.h"
#include "StyleResolver.h"
#include "TransformState.h"
#include "htmlediting.h"
#include <algorithm>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StackStats.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(SVG)
#include "RenderSVGResourceContainer.h"
#include "SVGRenderSupport.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

#ifndef NDEBUG
static void* baseOfRenderObjectBeingDeleted;

RenderObject::SetLayoutNeededForbiddenScope::SetLayoutNeededForbiddenScope(RenderObject* renderObject, bool isForbidden)
    : m_renderObject(renderObject)
    , m_preexistingForbidden(m_renderObject->isSetNeedsLayoutForbidden())
{
    m_renderObject->setNeedsLayoutIsForbidden(isForbidden);
}

RenderObject::SetLayoutNeededForbiddenScope::~SetLayoutNeededForbiddenScope()
{
    m_renderObject->setNeedsLayoutIsForbidden(m_preexistingForbidden);
}
#endif

struct SameSizeAsRenderObject {
    virtual ~SameSizeAsRenderObject() { } // Allocate vtable pointer.
    void* pointers[5];
#ifndef NDEBUG
    unsigned m_debugBitfields : 2;
#endif
    unsigned m_bitfields;
};

COMPILE_ASSERT(sizeof(RenderObject) == sizeof(SameSizeAsRenderObject), RenderObject_should_stay_small);

// On low-powered/mobile devices, preventing blitting on a scroll can cause noticeable delays
// when scrolling a page with a fixed background image. As an optimization, assuming there are
// no fixed positoned elements on the page, we can acclerate scrolling (via blitting) if we
// ignore the CSS property "background-attachment: fixed".
static bool shouldRepaintFixedBackgroundsOnScroll(FrameView* frameView)
{
#if !ENABLE(FAST_MOBILE_SCROLLING) && !PLATFORM(QT)
    UNUSED_PARAM(frameView);
#endif

    bool repaintFixedBackgroundsOnScroll = true;
#if ENABLE(FAST_MOBILE_SCROLLING)
#if PLATFORM(QT)
    if (frameView->delegatesScrolling())
        repaintFixedBackgroundsOnScroll = false;
#else
    repaintFixedBackgroundsOnScroll = false;
#endif
#endif
    return repaintFixedBackgroundsOnScroll;
}

bool RenderObject::s_affectsParentBlock = false;
bool RenderObject::s_noLongerAffectsParentBlock = false;

RenderObjectAncestorLineboxDirtySet* RenderObject::s_ancestorLineboxDirtySet = 0;

void* RenderObject::operator new(size_t sz, RenderArena* renderArena)
{
    return renderArena->allocate(sz);
}

void RenderObject::operator delete(void* ptr, size_t sz)
{
    ASSERT(baseOfRenderObjectBeingDeleted == ptr);

    // Stash size where destroy can find it.
    *(size_t *)ptr = sz;
}

RenderObject* RenderObject::createObject(Element* element, RenderStyle* style)
{
    Document* doc = element->document();
    RenderArena* arena = doc->renderArena();

    // Minimal support for content properties replacing an entire element.
    // Works only if we have exactly one piece of content and it's a URL.
    // Otherwise acts as if we didn't support this feature.
    const ContentData* contentData = style->contentData();
    if (contentData && !contentData->next() && contentData->isImage() && !element->isPseudoElement()) {
        RenderImage* image = new (arena) RenderImage(element);
        // RenderImageResourceStyleImage requires a style being present on the image but we don't want to
        // trigger a style change now as the node is not fully attached. Moving this code to style change
        // doesn't make sense as it should be run once at renderer creation.
        image->setStyleInternal(style);
        if (const StyleImage* styleImage = static_cast<const ImageContentData*>(contentData)->image()) {
            image->setImageResource(RenderImageResourceStyleImage::create(const_cast<StyleImage*>(styleImage)));
            image->setIsGeneratedContent();
        } else
            image->setImageResource(RenderImageResource::create());
        image->setStyleInternal(0);
        return image;
    }

    if (element->hasTagName(rubyTag)) {
        if (style->display() == INLINE)
            return new (arena) RenderRubyAsInline(element);
        else if (style->display() == BLOCK)
            return new (arena) RenderRubyAsBlock(element);
    }
    // treat <rt> as ruby text ONLY if it still has its default treatment of block
    if (element->hasTagName(rtTag) && style->display() == BLOCK)
        return new (arena) RenderRubyText(element);
    if (doc->cssRegionsEnabled() && style->isDisplayRegionType() && !style->regionThread().isEmpty() && doc->renderView())
        return new (arena) RenderRegion(element, 0);
    switch (style->display()) {
    case NONE:
        return 0;
    case INLINE:
        return new (arena) RenderInline(element);
    case BLOCK:
    case INLINE_BLOCK:
    case RUN_IN:
    case COMPACT:
        if ((!style->hasAutoColumnCount() || !style->hasAutoColumnWidth()) && doc->regionBasedColumnsEnabled())
            return new (arena) RenderMultiColumnBlock(element);
        return new (arena) RenderBlock(element);
    case LIST_ITEM:
        return new (arena) RenderListItem(element);
    case TABLE:
    case INLINE_TABLE:
        return new (arena) RenderTable(element);
    case TABLE_ROW_GROUP:
    case TABLE_HEADER_GROUP:
    case TABLE_FOOTER_GROUP:
        return new (arena) RenderTableSection(element);
    case TABLE_ROW:
        return new (arena) RenderTableRow(element);
    case TABLE_COLUMN_GROUP:
    case TABLE_COLUMN:
        return new (arena) RenderTableCol(element);
    case TABLE_CELL:
        return new (arena) RenderTableCell(element);
    case TABLE_CAPTION:
        return new (arena) RenderTableCaption(element);
    case BOX:
    case INLINE_BOX:
        return new (arena) RenderDeprecatedFlexibleBox(element);
    case FLEX:
    case INLINE_FLEX:
        return new (arena) RenderFlexibleBox(element);
    case GRID:
    case INLINE_GRID:
        return new (arena) RenderGrid(element);
    }

    return 0;
}

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, renderObjectCounter, ("RenderObject"));

RenderObject::RenderObject(Node* node)
    : CachedImageClient()
    , m_style(0)
    , m_node(node)
    , m_parent(0)
    , m_previous(0)
    , m_next(0)
#ifndef NDEBUG
    , m_hasAXObject(false)
    , m_setNeedsLayoutForbidden(false)
#endif
    , m_bitfields(node)
{
#ifndef NDEBUG
    renderObjectCounter.increment();
#endif
}

RenderObject::~RenderObject()
{
#ifndef NDEBUG
    ASSERT(!m_hasAXObject);
    renderObjectCounter.decrement();
#endif
}

RenderTheme* RenderObject::theme() const
{
    ASSERT(document()->page());

    return document()->page()->theme();
}

bool RenderObject::isDescendantOf(const RenderObject* obj) const
{
    for (const RenderObject* r = this; r; r = r->m_parent) {
        if (r == obj)
            return true;
    }
    return false;
}

bool RenderObject::isBody() const
{
    return node() && node()->hasTagName(bodyTag);
}

bool RenderObject::isHR() const
{
    return node() && node()->hasTagName(hrTag);
}

bool RenderObject::isLegend() const
{
    return node() && node()->hasTagName(legendTag);
}

bool RenderObject::isHTMLMarquee() const
{
    return node() && node()->renderer() == this && node()->hasTagName(marqueeTag);
}

void RenderObject::setFlowThreadStateIncludingDescendants(FlowThreadState state)
{
    setFlowThreadState(state);

    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        // If the child is a fragmentation context it already updated the descendants flag accordingly.
        if (child->isRenderFlowThread())
            continue;
        ASSERT(state != child->flowThreadState());
        child->setFlowThreadStateIncludingDescendants(state);
    }
}

void RenderObject::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    RenderObjectChildList* children = virtualChildren();
    ASSERT(children);
    if (!children)
        return;

    bool needsTable = false;

    if (newChild->isRenderTableCol()) {
        RenderTableCol* newTableColumn = toRenderTableCol(newChild);
        bool isColumnInColumnGroup = newTableColumn->isTableColumn() && isRenderTableCol();
        needsTable = !isTable() && !isColumnInColumnGroup;
    } else if (newChild->isTableCaption())
        needsTable = !isTable();
    else if (newChild->isTableSection())
        needsTable = !isTable();
    else if (newChild->isTableRow())
        needsTable = !isTableSection();
    else if (newChild->isTableCell())
        needsTable = !isTableRow();

    if (needsTable) {
        RenderTable* table;
        RenderObject* afterChild = beforeChild ? beforeChild->previousSibling() : children->lastChild();
        if (afterChild && afterChild->isAnonymous() && afterChild->isTable() && !afterChild->isBeforeContent())
            table = toRenderTable(afterChild);
        else {
            table = RenderTable::createAnonymousWithParentRenderer(this);
            addChild(table, beforeChild);
        }
        table->addChild(newChild);
    } else
        children->insertChildNode(this, newChild, beforeChild);

    if (newChild->isText() && newChild->style()->textTransform() == CAPITALIZE)
        toRenderText(newChild)->transformText();

    // SVG creates renderers for <g display="none">, as SVG requires children of hidden
    // <g>s to have renderers - at least that's how our implementation works. Consider:
    // <g display="none"><foreignObject><body style="position: relative">FOO...
    // - requiresLayer() would return true for the <body>, creating a new RenderLayer
    // - when the document is painted, both layers are painted. The <body> layer doesn't
    //   know that it's inside a "hidden SVG subtree", and thus paints, even if it shouldn't.
    // To avoid the problem alltogether, detect early if we're inside a hidden SVG subtree
    // and stop creating layers at all for these cases - they're not used anyways.
    if (newChild->hasLayer() && !layerCreationAllowedForSubtree())
        toRenderLayerModelObject(newChild)->layer()->removeOnlyThisLayer();

#if ENABLE(SVG)
    SVGRenderSupport::childAdded(this, newChild);
#endif
}

void RenderObject::removeChild(RenderObject* oldChild)
{
    RenderObjectChildList* children = virtualChildren();
    ASSERT(children);
    if (!children)
        return;

    children->removeChildNode(this, oldChild);
}

RenderObject* RenderObject::nextInPreOrder() const
{
    if (RenderObject* o = firstChild())
        return o;

    return nextInPreOrderAfterChildren();
}

RenderObject* RenderObject::nextInPreOrderAfterChildren() const
{
    RenderObject* o;
    if (!(o = nextSibling())) {
        o = parent();
        while (o && !o->nextSibling())
            o = o->parent();
        if (o)
            o = o->nextSibling();
    }

    return o;
}

RenderObject* RenderObject::nextInPreOrder(const RenderObject* stayWithin) const
{
    if (RenderObject* o = firstChild())
        return o;

    return nextInPreOrderAfterChildren(stayWithin);
}

RenderObject* RenderObject::nextInPreOrderAfterChildren(const RenderObject* stayWithin) const
{
    if (this == stayWithin)
        return 0;

    const RenderObject* current = this;
    RenderObject* next;
    while (!(next = current->nextSibling())) {
        current = current->parent();
        if (!current || current == stayWithin)
            return 0;
    }
    return next;
}

RenderObject* RenderObject::previousInPreOrder() const
{
    if (RenderObject* o = previousSibling()) {
        while (o->lastChild())
            o = o->lastChild();
        return o;
    }

    return parent();
}

RenderObject* RenderObject::previousInPreOrder(const RenderObject* stayWithin) const
{
    if (this == stayWithin)
        return 0;

    return previousInPreOrder();
}

RenderObject* RenderObject::childAt(unsigned index) const
{
    RenderObject* child = firstChild();
    for (unsigned i = 0; child && i < index; i++)
        child = child->nextSibling();
    return child;
}

RenderObject* RenderObject::firstLeafChild() const
{
    RenderObject* r = firstChild();
    while (r) {
        RenderObject* n = 0;
        n = r->firstChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

RenderObject* RenderObject::lastLeafChild() const
{
    RenderObject* r = lastChild();
    while (r) {
        RenderObject* n = 0;
        n = r->lastChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

static void addLayers(RenderObject* obj, RenderLayer* parentLayer, RenderObject*& newObject,
                      RenderLayer*& beforeChild)
{
    if (obj->hasLayer()) {
        if (!beforeChild && newObject) {
            // We need to figure out the layer that follows newObject. We only do
            // this the first time we find a child layer, and then we update the
            // pointer values for newObject and beforeChild used by everyone else.
            beforeChild = newObject->parent()->findNextLayer(parentLayer, newObject);
            newObject = 0;
        }
        parentLayer->addChild(toRenderLayerModelObject(obj)->layer(), beforeChild);
        return;
    }

    for (RenderObject* curr = obj->firstChild(); curr; curr = curr->nextSibling())
        addLayers(curr, parentLayer, newObject, beforeChild);
}

void RenderObject::addLayers(RenderLayer* parentLayer)
{
    if (!parentLayer)
        return;

    RenderObject* object = this;
    RenderLayer* beforeChild = 0;
    WebCore::addLayers(this, parentLayer, object, beforeChild);
}

void RenderObject::removeLayers(RenderLayer* parentLayer)
{
    if (!parentLayer)
        return;

    if (hasLayer()) {
        parentLayer->removeChild(toRenderLayerModelObject(this)->layer());
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->removeLayers(parentLayer);
}

void RenderObject::moveLayers(RenderLayer* oldParent, RenderLayer* newParent)
{
    if (!newParent)
        return;

    if (hasLayer()) {
        RenderLayer* layer = toRenderLayerModelObject(this)->layer();
        ASSERT(oldParent == layer->parent());
        if (oldParent)
            oldParent->removeChild(layer);
        newParent->addChild(layer);
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->moveLayers(oldParent, newParent);
}

RenderLayer* RenderObject::findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint,
                                         bool checkParent)
{
    // Error check the parent layer passed in. If it's null, we can't find anything.
    if (!parentLayer)
        return 0;

    // Step 1: If our layer is a child of the desired parent, then return our layer.
    RenderLayer* ourLayer = hasLayer() ? toRenderLayerModelObject(this)->layer() : 0;
    if (ourLayer && ourLayer->parent() == parentLayer)
        return ourLayer;

    // Step 2: If we don't have a layer, or our layer is the desired parent, then descend
    // into our siblings trying to find the next layer whose parent is the desired parent.
    if (!ourLayer || ourLayer == parentLayer) {
        for (RenderObject* curr = startPoint ? startPoint->nextSibling() : firstChild();
             curr; curr = curr->nextSibling()) {
            RenderLayer* nextLayer = curr->findNextLayer(parentLayer, 0, false);
            if (nextLayer)
                return nextLayer;
        }
    }

    // Step 3: If our layer is the desired parent layer, then we're finished. We didn't
    // find anything.
    if (parentLayer == ourLayer)
        return 0;

    // Step 4: If |checkParent| is set, climb up to our parent and check its siblings that
    // follow us to see if we can locate a layer.
    if (checkParent && parent())
        return parent()->findNextLayer(parentLayer, this, true);

    return 0;
}

RenderLayer* RenderObject::enclosingLayer() const
{
    const RenderObject* curr = this;
    while (curr) {
        RenderLayer* layer = curr->hasLayer() ? toRenderLayerModelObject(curr)->layer() : 0;
        if (layer)
            return layer;
        curr = curr->parent();
    }
    return 0;
}

bool RenderObject::scrollRectToVisible(const LayoutRect& rect, const ScrollAlignment& alignX, const ScrollAlignment& alignY)
{
    RenderLayer* enclosingLayer = this->enclosingLayer();
    if (!enclosingLayer)
        return false;

    enclosingLayer->scrollRectToVisible(rect, alignX, alignY);
    return true;
}

RenderBox* RenderObject::enclosingBox() const
{
    RenderObject* curr = const_cast<RenderObject*>(this);
    while (curr) {
        if (curr->isBox())
            return toRenderBox(curr);
        curr = curr->parent();
    }
    
    ASSERT_NOT_REACHED();
    return 0;
}

RenderBoxModelObject* RenderObject::enclosingBoxModelObject() const
{
    RenderObject* curr = const_cast<RenderObject*>(this);
    while (curr) {
        if (curr->isBoxModelObject())
            return toRenderBoxModelObject(curr);
        curr = curr->parent();
    }

    ASSERT_NOT_REACHED();
    return 0;
}

RenderFlowThread* RenderObject::locateFlowThreadContainingBlock() const
{
    ASSERT(flowThreadState() != NotInsideFlowThread);

    // See if we have the thread cached because we're in the middle of layout.
    RenderFlowThread* flowThread = view()->flowThreadController()->currentRenderFlowThread();
    if (flowThread)
        return flowThread;
    
    // Not in the middle of layout so have to find the thread the slow way.
    RenderObject* curr = const_cast<RenderObject*>(this);
    while (curr) {
        if (curr->isRenderFlowThread())
            return toRenderFlowThread(curr);
        curr = curr->containingBlock();
    }
    return 0;
}

RenderNamedFlowThread* RenderObject::renderNamedFlowThreadWrapper() const
{
    RenderObject* object = const_cast<RenderObject*>(this);
    while (object && object->isAnonymousBlock() && !object->isRenderNamedFlowThread())
        object = object->parent();

    return object && object->isRenderNamedFlowThread() ? toRenderNamedFlowThread(object) : 0;
}

RenderBlock* RenderObject::firstLineBlock() const
{
    return 0;
}

static inline bool objectIsRelayoutBoundary(const RenderObject* object)
{
    // FIXME: In future it may be possible to broaden these conditions in order to improve performance.
    if (object->isTextControl())
        return true;

#if ENABLE(SVG)
    if (object->isSVGRoot())
        return true;
#endif

    if (!object->hasOverflowClip())
        return false;

    if (object->style()->width().isIntrinsicOrAuto() || object->style()->height().isIntrinsicOrAuto() || object->style()->height().isPercent())
        return false;

    // Table parts can't be relayout roots since the table is responsible for layouting all the parts.
    if (object->isTablePart())
        return false;

    return true;
}

void RenderObject::markContainingBlocksForLayout(bool scheduleRelayout, RenderObject* newRoot)
{
    ASSERT(!scheduleRelayout || !newRoot);
    ASSERT(!isSetNeedsLayoutForbidden());

    RenderObject* object = container();
    RenderObject* last = this;

    bool simplifiedNormalFlowLayout = needsSimplifiedNormalFlowLayout() && !selfNeedsLayout() && !normalChildNeedsLayout();

    while (object) {
#ifndef NDEBUG
        // FIXME: Remove this once we remove the special cases for counters, quotes and mathml
        // calling setNeedsLayout during preferred width computation.
        SetLayoutNeededForbiddenScope layoutForbiddenScope(object, isSetNeedsLayoutForbidden());
#endif
        // Don't mark the outermost object of an unrooted subtree. That object will be
        // marked when the subtree is added to the document.
        RenderObject* container = object->container();
        if (!container && !object->isRenderView())
            return;
        if (!last->isText() && last->style()->hasOutOfFlowPosition()) {
            bool willSkipRelativelyPositionedInlines = !object->isRenderBlock() || object->isAnonymousBlock();
            // Skip relatively positioned inlines and anonymous blocks to get to the enclosing RenderBlock.
            while (object && (!object->isRenderBlock() || object->isAnonymousBlock()))
                object = object->container();
            if (!object || object->posChildNeedsLayout())
                return;
            if (willSkipRelativelyPositionedInlines)
                container = object->container();
            object->setPosChildNeedsLayout(true);
            simplifiedNormalFlowLayout = true;
            ASSERT(!object->isSetNeedsLayoutForbidden());
        } else if (simplifiedNormalFlowLayout) {
            if (object->needsSimplifiedNormalFlowLayout())
                return;
            object->setNeedsSimplifiedNormalFlowLayout(true);
            ASSERT(!object->isSetNeedsLayoutForbidden());
        } else {
            if (object->normalChildNeedsLayout())
                return;
            object->setNormalChildNeedsLayout(true);
            ASSERT(!object->isSetNeedsLayoutForbidden());
        }

        if (object == newRoot)
            return;

        last = object;
        if (scheduleRelayout && objectIsRelayoutBoundary(last))
            break;
        object = container;
    }

    if (scheduleRelayout)
        last->scheduleRelayout();
}

#ifndef NDEBUG
void RenderObject::checkBlockPositionedObjectsNeedLayout()
{
    ASSERT(!needsLayout());

    if (isRenderBlock())
        toRenderBlock(this)->checkPositionedObjectsNeedLayout();
}
#endif

void RenderObject::setPreferredLogicalWidthsDirty(bool shouldBeDirty, MarkingBehavior markParents)
{
    bool alreadyDirty = preferredLogicalWidthsDirty();
    m_bitfields.setPreferredLogicalWidthsDirty(shouldBeDirty);
    if (shouldBeDirty && !alreadyDirty && markParents == MarkContainingBlockChain && (isText() || !style()->hasOutOfFlowPosition()))
        invalidateContainerPreferredLogicalWidths();
}

void RenderObject::invalidateContainerPreferredLogicalWidths()
{
    // In order to avoid pathological behavior when inlines are deeply nested, we do include them
    // in the chain that we mark dirty (even though they're kind of irrelevant).
    RenderObject* o = isTableCell() ? containingBlock() : container();
    while (o && !o->preferredLogicalWidthsDirty()) {
        // Don't invalidate the outermost object of an unrooted subtree. That object will be 
        // invalidated when the subtree is added to the document.
        RenderObject* container = o->isTableCell() ? o->containingBlock() : o->container();
        if (!container && !o->isRenderView())
            break;

        o->m_bitfields.setPreferredLogicalWidthsDirty(true);
        if (o->style()->hasOutOfFlowPosition())
            // A positioned object has no effect on the min/max width of its containing block ever.
            // We can optimize this case and not go up any further.
            break;
        o = container;
    }
}

void RenderObject::setLayerNeedsFullRepaint()
{
    ASSERT(hasLayer());
    toRenderLayerModelObject(this)->layer()->setRepaintStatus(NeedsFullRepaint);
}

void RenderObject::setLayerNeedsFullRepaintForPositionedMovementLayout()
{
    ASSERT(hasLayer());
    toRenderLayerModelObject(this)->layer()->setRepaintStatus(NeedsFullRepaintForPositionedMovementLayout);
}

RenderBlock* RenderObject::containingBlock() const
{
    RenderObject* o = parent();
    if (!o && isRenderScrollbarPart())
        o = toRenderScrollbarPart(this)->rendererOwningScrollbar();

    if (!isText() && m_style->position() == FixedPosition)
        o = containingBlockForFixedPosition(o);
    else if (!isText() && m_style->position() == AbsolutePosition)
        o = containingBlockForAbsolutePosition(o);
    else
        o = containingBlockForObjectInFlow(o);

    if (!o || !o->isRenderBlock())
        return 0; // This can still happen in case of an orphaned tree

    return toRenderBlock(o);
}

static bool mustRepaintFillLayers(const RenderObject* renderer, const FillLayer* layer)
{
    // Nobody will use multiple layers without wanting fancy positioning.
    if (layer->next())
        return true;

    // Make sure we have a valid image.
    StyleImage* img = layer->image();
    if (!img || !img->canRender(renderer, renderer->style()->effectiveZoom()))
        return false;

    if (!layer->xPosition().isZero() || !layer->yPosition().isZero())
        return true;

    EFillSizeType sizeType = layer->sizeType();

    if (sizeType == Contain || sizeType == Cover)
        return true;
    
    if (sizeType == SizeLength) {
        LengthSize size = layer->sizeLength();
        if (size.width().isPercent() || size.height().isPercent())
            return true;
        // If the image has neither an intrinsic width nor an intrinsic height, its size is determined as for 'contain'.
        if ((size.width().isAuto() || size.height().isAuto()) && img->isGeneratedImage())
            return true;
    } else if (img->usesImageContainerSize())
        return true;

    return false;
}

bool RenderObject::borderImageIsLoadedAndCanBeRendered() const
{
    ASSERT(style()->hasBorder());

    StyleImage* borderImage = style()->borderImage().image();
    return borderImage && borderImage->canRender(this, style()->effectiveZoom()) && borderImage->isLoaded();
}

bool RenderObject::mustRepaintBackgroundOrBorder() const
{
    if (hasMask() && mustRepaintFillLayers(this, style()->maskLayers()))
        return true;

    // If we don't have a background/border/mask, then nothing to do.
    if (!hasBoxDecorations())
        return false;

    if (mustRepaintFillLayers(this, style()->backgroundLayers()))
        return true;
     
    // Our fill layers are ok.  Let's check border.
    if (style()->hasBorder() && borderImageIsLoadedAndCanBeRendered())
        return true;

    return false;
}

void RenderObject::drawLineForBoxSide(GraphicsContext* graphicsContext, int x1, int y1, int x2, int y2,
                                      BoxSide side, Color color, EBorderStyle style,
                                      int adjacentWidth1, int adjacentWidth2, bool antialias)
{
    int thickness;
    int length;
    if (side == BSTop || side == BSBottom) {
        thickness = y2 - y1;
        length = x2 - x1;
    } else {
        thickness = x2 - x1;
        length = y2 - y1;
    }

    // FIXME: We really would like this check to be an ASSERT as we don't want to draw empty borders. However
    // nothing guarantees that the following recursive calls to drawLineForBoxSide will have non-null dimensions.
    if (!thickness || !length)
        return;

    if (style == DOUBLE && thickness < 3)
        style = SOLID;

    switch (style) {
        case BNONE:
        case BHIDDEN:
            return;
        case DOTTED:
        case DASHED: {
            if (thickness > 0) {
                bool wasAntialiased = graphicsContext->shouldAntialias();
                StrokeStyle oldStrokeStyle = graphicsContext->strokeStyle();
                graphicsContext->setShouldAntialias(antialias);
                graphicsContext->setStrokeColor(color, m_style->colorSpace());
                graphicsContext->setStrokeThickness(thickness);
                graphicsContext->setStrokeStyle(style == DASHED ? DashedStroke : DottedStroke);

                switch (side) {
                    case BSBottom:
                    case BSTop:
                        graphicsContext->drawLine(IntPoint(x1, (y1 + y2) / 2), IntPoint(x2, (y1 + y2) / 2));
                        break;
                    case BSRight:
                    case BSLeft:
                        graphicsContext->drawLine(IntPoint((x1 + x2) / 2, y1), IntPoint((x1 + x2) / 2, y2));
                        break;
                }
                graphicsContext->setShouldAntialias(wasAntialiased);
                graphicsContext->setStrokeStyle(oldStrokeStyle);
            }
            break;
        }
        case DOUBLE: {
            int thirdOfThickness = (thickness + 1) / 3;
            ASSERT(thirdOfThickness);

            if (adjacentWidth1 == 0 && adjacentWidth2 == 0) {
                StrokeStyle oldStrokeStyle = graphicsContext->strokeStyle();
                graphicsContext->setStrokeStyle(NoStroke);
                graphicsContext->setFillColor(color, m_style->colorSpace());
                
                bool wasAntialiased = graphicsContext->shouldAntialias();
                graphicsContext->setShouldAntialias(antialias);

                switch (side) {
                    case BSTop:
                    case BSBottom:
                        graphicsContext->drawRect(IntRect(x1, y1, length, thirdOfThickness));
                        graphicsContext->drawRect(IntRect(x1, y2 - thirdOfThickness, length, thirdOfThickness));
                        break;
                    case BSLeft:
                    case BSRight:
                        // FIXME: Why do we offset the border by 1 in this case but not the other one?
                        if (length > 1) {
                            graphicsContext->drawRect(IntRect(x1, y1 + 1, thirdOfThickness, length - 1));
                            graphicsContext->drawRect(IntRect(x2 - thirdOfThickness, y1 + 1, thirdOfThickness, length - 1));
                        }
                        break;
                }

                graphicsContext->setShouldAntialias(wasAntialiased);
                graphicsContext->setStrokeStyle(oldStrokeStyle);
            } else {
                int adjacent1BigThird = ((adjacentWidth1 > 0) ? adjacentWidth1 + 1 : adjacentWidth1 - 1) / 3;
                int adjacent2BigThird = ((adjacentWidth2 > 0) ? adjacentWidth2 + 1 : adjacentWidth2 - 1) / 3;

                switch (side) {
                    case BSTop:
                        drawLineForBoxSide(graphicsContext, x1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
                                   y1, x2 - max((-adjacentWidth2 * 2 + 1) / 3, 0), y1 + thirdOfThickness,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   y2 - thirdOfThickness, x2 - max((adjacentWidth2 * 2 + 1) / 3, 0), y2,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        break;
                    case BSLeft:
                        drawLineForBoxSide(graphicsContext, x1, y1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
                                   x1 + thirdOfThickness, y2 - max((-adjacentWidth2 * 2 + 1) / 3, 0),
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x2 - thirdOfThickness, y1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   x2, y2 - max((adjacentWidth2 * 2 + 1) / 3, 0),
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        break;
                    case BSBottom:
                        drawLineForBoxSide(graphicsContext, x1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   y1, x2 - max((adjacentWidth2 * 2 + 1) / 3, 0), y1 + thirdOfThickness,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
                                   y2 - thirdOfThickness, x2 - max((-adjacentWidth2 * 2 + 1) / 3, 0), y2,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        break;
                    case BSRight:
                        drawLineForBoxSide(graphicsContext, x1, y1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   x1 + thirdOfThickness, y2 - max((adjacentWidth2 * 2 + 1) / 3, 0),
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x2 - thirdOfThickness, y1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
                                   x2, y2 - max((-adjacentWidth2 * 2 + 1) / 3, 0),
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case RIDGE:
        case GROOVE: {
            EBorderStyle s1;
            EBorderStyle s2;
            if (style == GROOVE) {
                s1 = INSET;
                s2 = OUTSET;
            } else {
                s1 = OUTSET;
                s2 = INSET;
            }

            int adjacent1BigHalf = ((adjacentWidth1 > 0) ? adjacentWidth1 + 1 : adjacentWidth1 - 1) / 2;
            int adjacent2BigHalf = ((adjacentWidth2 > 0) ? adjacentWidth2 + 1 : adjacentWidth2 - 1) / 2;

            switch (side) {
                case BSTop:
                    drawLineForBoxSide(graphicsContext, x1 + max(-adjacentWidth1, 0) / 2, y1, x2 - max(-adjacentWidth2, 0) / 2, (y1 + y2 + 1) / 2,
                               side, color, s1, adjacent1BigHalf, adjacent2BigHalf, antialias);
                    drawLineForBoxSide(graphicsContext, x1 + max(adjacentWidth1 + 1, 0) / 2, (y1 + y2 + 1) / 2, x2 - max(adjacentWidth2 + 1, 0) / 2, y2,
                               side, color, s2, adjacentWidth1 / 2, adjacentWidth2 / 2, antialias);
                    break;
                case BSLeft:
                    drawLineForBoxSide(graphicsContext, x1, y1 + max(-adjacentWidth1, 0) / 2, (x1 + x2 + 1) / 2, y2 - max(-adjacentWidth2, 0) / 2,
                               side, color, s1, adjacent1BigHalf, adjacent2BigHalf, antialias);
                    drawLineForBoxSide(graphicsContext, (x1 + x2 + 1) / 2, y1 + max(adjacentWidth1 + 1, 0) / 2, x2, y2 - max(adjacentWidth2 + 1, 0) / 2,
                               side, color, s2, adjacentWidth1 / 2, adjacentWidth2 / 2, antialias);
                    break;
                case BSBottom:
                    drawLineForBoxSide(graphicsContext, x1 + max(adjacentWidth1, 0) / 2, y1, x2 - max(adjacentWidth2, 0) / 2, (y1 + y2 + 1) / 2,
                               side, color, s2, adjacent1BigHalf, adjacent2BigHalf, antialias);
                    drawLineForBoxSide(graphicsContext, x1 + max(-adjacentWidth1 + 1, 0) / 2, (y1 + y2 + 1) / 2, x2 - max(-adjacentWidth2 + 1, 0) / 2, y2,
                               side, color, s1, adjacentWidth1 / 2, adjacentWidth2 / 2, antialias);
                    break;
                case BSRight:
                    drawLineForBoxSide(graphicsContext, x1, y1 + max(adjacentWidth1, 0) / 2, (x1 + x2 + 1) / 2, y2 - max(adjacentWidth2, 0) / 2,
                               side, color, s2, adjacent1BigHalf, adjacent2BigHalf, antialias);
                    drawLineForBoxSide(graphicsContext, (x1 + x2 + 1) / 2, y1 + max(-adjacentWidth1 + 1, 0) / 2, x2, y2 - max(-adjacentWidth2 + 1, 0) / 2,
                               side, color, s1, adjacentWidth1 / 2, adjacentWidth2 / 2, antialias);
                    break;
            }
            break;
        }
        case INSET:
            // FIXME: Maybe we should lighten the colors on one side like Firefox.
            // https://bugs.webkit.org/show_bug.cgi?id=58608
            if (side == BSTop || side == BSLeft)
                color = color.dark();
            // fall through
        case OUTSET:
            if (style == OUTSET && (side == BSBottom || side == BSRight))
                color = color.dark();
            // fall through
        case SOLID: {
            StrokeStyle oldStrokeStyle = graphicsContext->strokeStyle();
            graphicsContext->setStrokeStyle(NoStroke);
            graphicsContext->setFillColor(color, m_style->colorSpace());
            ASSERT(x2 >= x1);
            ASSERT(y2 >= y1);
            if (!adjacentWidth1 && !adjacentWidth2) {
                // Turn off antialiasing to match the behavior of drawConvexPolygon();
                // this matters for rects in transformed contexts.
                bool wasAntialiased = graphicsContext->shouldAntialias();
                graphicsContext->setShouldAntialias(antialias);
                graphicsContext->drawRect(IntRect(x1, y1, x2 - x1, y2 - y1));
                graphicsContext->setShouldAntialias(wasAntialiased);
                graphicsContext->setStrokeStyle(oldStrokeStyle);
                return;
            }
            FloatPoint quad[4];
            switch (side) {
                case BSTop:
                    quad[0] = FloatPoint(x1 + max(-adjacentWidth1, 0), y1);
                    quad[1] = FloatPoint(x1 + max(adjacentWidth1, 0), y2);
                    quad[2] = FloatPoint(x2 - max(adjacentWidth2, 0), y2);
                    quad[3] = FloatPoint(x2 - max(-adjacentWidth2, 0), y1);
                    break;
                case BSBottom:
                    quad[0] = FloatPoint(x1 + max(adjacentWidth1, 0), y1);
                    quad[1] = FloatPoint(x1 + max(-adjacentWidth1, 0), y2);
                    quad[2] = FloatPoint(x2 - max(-adjacentWidth2, 0), y2);
                    quad[3] = FloatPoint(x2 - max(adjacentWidth2, 0), y1);
                    break;
                case BSLeft:
                    quad[0] = FloatPoint(x1, y1 + max(-adjacentWidth1, 0));
                    quad[1] = FloatPoint(x1, y2 - max(-adjacentWidth2, 0));
                    quad[2] = FloatPoint(x2, y2 - max(adjacentWidth2, 0));
                    quad[3] = FloatPoint(x2, y1 + max(adjacentWidth1, 0));
                    break;
                case BSRight:
                    quad[0] = FloatPoint(x1, y1 + max(adjacentWidth1, 0));
                    quad[1] = FloatPoint(x1, y2 - max(adjacentWidth2, 0));
                    quad[2] = FloatPoint(x2, y2 - max(-adjacentWidth2, 0));
                    quad[3] = FloatPoint(x2, y1 + max(-adjacentWidth1, 0));
                    break;
            }

            graphicsContext->drawConvexPolygon(4, quad, antialias);
            graphicsContext->setStrokeStyle(oldStrokeStyle);
            break;
        }
    }
}

void RenderObject::paintFocusRing(PaintInfo& paintInfo, const LayoutPoint& paintOffset, RenderStyle* style)
{
    Vector<IntRect> focusRingRects;
    addFocusRingRects(focusRingRects, paintOffset, paintInfo.paintContainer);
    if (style->outlineStyleIsAuto())
        paintInfo.context->drawFocusRing(focusRingRects, style->outlineWidth(), style->outlineOffset(), style->visitedDependentColor(CSSPropertyOutlineColor));
    else
        addPDFURLRect(paintInfo.context, unionRect(focusRingRects));
}

void RenderObject::addPDFURLRect(GraphicsContext* context, const LayoutRect& rect)
{
    if (rect.isEmpty())
        return;
    Node* n = node();
    if (!n || !n->isLink() || !n->isElementNode())
        return;
    const AtomicString& href = toElement(n)->getAttribute(hrefAttr);
    if (href.isNull())
        return;
    context->setURLForRect(n->document()->completeURL(href), pixelSnappedIntRect(rect));
}

void RenderObject::paintOutline(PaintInfo& paintInfo, const LayoutRect& paintRect)
{
    if (!hasOutline())
        return;

    RenderStyle* styleToUse = style();
    LayoutUnit outlineWidth = styleToUse->outlineWidth();

    int outlineOffset = styleToUse->outlineOffset();

    if (styleToUse->outlineStyleIsAuto() || hasOutlineAnnotation()) {
        if (!theme()->supportsFocusRing(styleToUse)) {
            // Only paint the focus ring by hand if the theme isn't able to draw the focus ring.
            paintFocusRing(paintInfo, paintRect.location(), styleToUse);
        }
    }

    if (styleToUse->outlineStyleIsAuto() || styleToUse->outlineStyle() == BNONE)
        return;

    IntRect inner = pixelSnappedIntRect(paintRect);
    inner.inflate(outlineOffset);

    IntRect outer = pixelSnappedIntRect(inner);
    outer.inflate(outlineWidth);

    // FIXME: This prevents outlines from painting inside the object. See bug 12042
    if (outer.isEmpty())
        return;

    EBorderStyle outlineStyle = styleToUse->outlineStyle();
    Color outlineColor = styleToUse->visitedDependentColor(CSSPropertyOutlineColor);

    GraphicsContext* graphicsContext = paintInfo.context;
    bool useTransparencyLayer = outlineColor.hasAlpha();
    if (useTransparencyLayer) {
        if (outlineStyle == SOLID) {
            Path path;
            path.addRect(outer);
            path.addRect(inner);
            graphicsContext->setFillRule(RULE_EVENODD);
            graphicsContext->setFillColor(outlineColor, styleToUse->colorSpace());
            graphicsContext->fillPath(path);
            return;
        }
        graphicsContext->beginTransparencyLayer(static_cast<float>(outlineColor.alpha()) / 255);
        outlineColor = Color(outlineColor.red(), outlineColor.green(), outlineColor.blue());
    }

    int leftOuter = outer.x();
    int leftInner = inner.x();
    int rightOuter = outer.maxX();
    int rightInner = inner.maxX();
    int topOuter = outer.y();
    int topInner = inner.y();
    int bottomOuter = outer.maxY();
    int bottomInner = inner.maxY();
    
    drawLineForBoxSide(graphicsContext, leftOuter, topOuter, leftInner, bottomOuter, BSLeft, outlineColor, outlineStyle, outlineWidth, outlineWidth);
    drawLineForBoxSide(graphicsContext, leftOuter, topOuter, rightOuter, topInner, BSTop, outlineColor, outlineStyle, outlineWidth, outlineWidth);
    drawLineForBoxSide(graphicsContext, rightInner, topOuter, rightOuter, bottomOuter, BSRight, outlineColor, outlineStyle, outlineWidth, outlineWidth);
    drawLineForBoxSide(graphicsContext, leftOuter, bottomInner, rightOuter, bottomOuter, BSBottom, outlineColor, outlineStyle, outlineWidth, outlineWidth);

    if (useTransparencyLayer)
        graphicsContext->endTransparencyLayer();
}

IntRect RenderObject::absoluteBoundingBoxRect(bool useTransforms) const
{
    if (useTransforms) {
        Vector<FloatQuad> quads;
        absoluteQuads(quads);

        size_t n = quads.size();
        if (!n)
            return IntRect();
    
        IntRect result = quads[0].enclosingBoundingBox();
        for (size_t i = 1; i < n; ++i)
            result.unite(quads[i].enclosingBoundingBox());
        return result;
    }

    FloatPoint absPos = localToAbsolute();
    Vector<IntRect> rects;
    absoluteRects(rects, flooredLayoutPoint(absPos));

    size_t n = rects.size();
    if (!n)
        return IntRect();

    LayoutRect result = rects[0];
    for (size_t i = 1; i < n; ++i)
        result.unite(rects[i]);
    return pixelSnappedIntRect(result);
}

void RenderObject::absoluteFocusRingQuads(Vector<FloatQuad>& quads)
{
    Vector<IntRect> rects;
    // FIXME: addFocusRingRects() needs to be passed this transform-unaware
    // localToAbsolute() offset here because RenderInline::addFocusRingRects()
    // implicitly assumes that. This doesn't work correctly with transformed
    // descendants.
    FloatPoint absolutePoint = localToAbsolute();
    addFocusRingRects(rects, flooredLayoutPoint(absolutePoint));
    size_t count = rects.size();
    for (size_t i = 0; i < count; ++i) {
        IntRect rect = rects[i];
        rect.move(-absolutePoint.x(), -absolutePoint.y());
        quads.append(localToAbsoluteQuad(FloatQuad(rect)));
    }
}

FloatRect RenderObject::absoluteBoundingBoxRectForRange(const Range* range)
{
    if (!range || !range->startContainer())
        return FloatRect();

    if (range->ownerDocument())
        range->ownerDocument()->updateLayout();

    Vector<FloatQuad> quads;
    range->textQuads(quads);

    if (quads.isEmpty())
        return FloatRect();

    FloatRect result = quads[0].boundingBox();
    for (size_t i = 1; i < quads.size(); ++i)
        result.uniteEvenIfEmpty(quads[i].boundingBox());

    return result;
}

void RenderObject::addAbsoluteRectForLayer(LayoutRect& result)
{
    if (hasLayer())
        result.unite(absoluteBoundingBoxRectIgnoringTransforms());
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
        current->addAbsoluteRectForLayer(result);
}

// FIXME: change this to use the subtreePaint terminology
LayoutRect RenderObject::paintingRootRect(LayoutRect& topLevelRect)
{
    LayoutRect result = absoluteBoundingBoxRectIgnoringTransforms();
    topLevelRect = result;
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
        current->addAbsoluteRectForLayer(result);
    return result;
}

void RenderObject::paint(PaintInfo&, const LayoutPoint&)
{
}

RenderLayerModelObject* RenderObject::containerForRepaint() const
{
    RenderView* v = view();
    if (!v)
        return 0;
    
    RenderLayerModelObject* repaintContainer = 0;

#if USE(ACCELERATED_COMPOSITING)
    if (v->usesCompositing()) {
        if (RenderLayer* parentLayer = enclosingLayer()) {
            RenderLayer* compLayer = parentLayer->enclosingCompositingLayerForRepaint();
            if (compLayer)
                repaintContainer = compLayer->renderer();
        }
    }
#endif
    
#if ENABLE(CSS_FILTERS)
    if (document()->view()->hasSoftwareFilters()) {
        if (RenderLayer* parentLayer = enclosingLayer()) {
            RenderLayer* enclosingFilterLayer = parentLayer->enclosingFilterLayer();
            if (enclosingFilterLayer)
                return enclosingFilterLayer->renderer();
        }
    }
#endif

    // If we have a flow thread, then we need to do individual repaints within the RenderRegions instead.
    // Return the flow thread as a repaint container in order to create a chokepoint that allows us to change
    // repainting to do individual region repaints.
    RenderFlowThread* parentRenderFlowThread = flowThreadContainingBlock();
    if (parentRenderFlowThread) {
        // The ancestor document will do the reparenting when the repaint propagates further up.
        // We're just a seamless child document, and we don't need to do the hacking.
        if (parentRenderFlowThread && parentRenderFlowThread->document() != document())
            return repaintContainer;
        // If we have already found a repaint container then we will repaint into that container only if it is part of the same
        // flow thread. Otherwise we will need to catch the repaint call and send it to the flow thread.
        RenderFlowThread* repaintContainerFlowThread = repaintContainer ? repaintContainer->flowThreadContainingBlock() : 0;
        if (!repaintContainerFlowThread || repaintContainerFlowThread != parentRenderFlowThread)
            repaintContainer = parentRenderFlowThread;
    }
    return repaintContainer;
}

void RenderObject::repaintUsingContainer(const RenderLayerModelObject* repaintContainer, const IntRect& r, bool immediate) const
{
    if (!repaintContainer) {
        view()->repaintViewRectangle(r, immediate);
        return;
    }

    if (repaintContainer->isRenderFlowThread()) {
        toRenderFlowThread(repaintContainer)->repaintRectangleInRegions(r, immediate);
        return;
    }

#if ENABLE(CSS_FILTERS)
    if (repaintContainer->hasFilter() && repaintContainer->layer() && repaintContainer->layer()->requiresFullLayerImageForFilters()) {
        repaintContainer->layer()->setFilterBackendNeedsRepaintingInRect(r, immediate);
        return;
    }
#endif

#if USE(ACCELERATED_COMPOSITING)
    RenderView* v = view();
    if (repaintContainer->isRenderView()) {
        ASSERT(repaintContainer == v);
        bool viewHasCompositedLayer = v->hasLayer() && v->layer()->isComposited();
        if (!viewHasCompositedLayer || v->layer()->backing()->paintsIntoWindow()) {
            v->repaintViewRectangle(viewHasCompositedLayer && v->layer()->transform() ? v->layer()->transform()->mapRect(r) : r, immediate);
            return;
        }
    }
    
    if (v->usesCompositing()) {
        ASSERT(repaintContainer->hasLayer() && repaintContainer->layer()->isComposited());
        repaintContainer->layer()->setBackingNeedsRepaintInRect(r);
    }
#else
    if (repaintContainer->isRenderView())
        toRenderView(repaintContainer)->repaintViewRectangle(r, immediate);
#endif
}

void RenderObject::repaint(bool immediate) const
{
    // Don't repaint if we're unrooted (note that view() still returns the view when unrooted)
    RenderView* view;
    if (!isRooted(&view))
        return;

    if (view->printing())
        return; // Don't repaint if we're printing.

    RenderLayerModelObject* repaintContainer = containerForRepaint();
    repaintUsingContainer(repaintContainer ? repaintContainer : view, pixelSnappedIntRect(clippedOverflowRectForRepaint(repaintContainer)), immediate);
}

void RenderObject::repaintRectangle(const LayoutRect& r, bool immediate) const
{
    // Don't repaint if we're unrooted (note that view() still returns the view when unrooted)
    RenderView* view;
    if (!isRooted(&view))
        return;

    if (view->printing())
        return; // Don't repaint if we're printing.

    LayoutRect dirtyRect(r);

    // FIXME: layoutDelta needs to be applied in parts before/after transforms and
    // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
    dirtyRect.move(view->layoutDelta());

    RenderLayerModelObject* repaintContainer = containerForRepaint();
    computeRectForRepaint(repaintContainer, dirtyRect);
    repaintUsingContainer(repaintContainer ? repaintContainer : view, pixelSnappedIntRect(dirtyRect), immediate);
}

IntRect RenderObject::pixelSnappedAbsoluteClippedOverflowRect() const
{
    return pixelSnappedIntRect(absoluteClippedOverflowRect());
}

bool RenderObject::repaintAfterLayoutIfNeeded(const RenderLayerModelObject* repaintContainer, const LayoutRect& oldBounds, const LayoutRect& oldOutlineBox, const LayoutRect* newBoundsPtr, const LayoutRect* newOutlineBoxRectPtr)
{
    RenderView* v = view();
    if (v->printing())
        return false; // Don't repaint if we're printing.

    // This ASSERT fails due to animations.  See https://bugs.webkit.org/show_bug.cgi?id=37048
    // ASSERT(!newBoundsPtr || *newBoundsPtr == clippedOverflowRectForRepaint(repaintContainer));
    LayoutRect newBounds = newBoundsPtr ? *newBoundsPtr : clippedOverflowRectForRepaint(repaintContainer);
    LayoutRect newOutlineBox;

    bool fullRepaint = selfNeedsLayout();
    // Presumably a background or a border exists if border-fit:lines was specified.
    if (!fullRepaint && style()->borderFit() == BorderFitLines)
        fullRepaint = true;
    if (!fullRepaint) {
        // This ASSERT fails due to animations.  See https://bugs.webkit.org/show_bug.cgi?id=37048
        // ASSERT(!newOutlineBoxRectPtr || *newOutlineBoxRectPtr == outlineBoundsForRepaint(repaintContainer));
        newOutlineBox = newOutlineBoxRectPtr ? *newOutlineBoxRectPtr : outlineBoundsForRepaint(repaintContainer);
        if (newOutlineBox.location() != oldOutlineBox.location() || (mustRepaintBackgroundOrBorder() && (newBounds != oldBounds || newOutlineBox != oldOutlineBox)))
            fullRepaint = true;
    }

    if (!repaintContainer)
        repaintContainer = v;

    if (fullRepaint) {
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(oldBounds));
        if (newBounds != oldBounds)
            repaintUsingContainer(repaintContainer, pixelSnappedIntRect(newBounds));
        return true;
    }

    if (newBounds == oldBounds && newOutlineBox == oldOutlineBox)
        return false;

    LayoutUnit deltaLeft = newBounds.x() - oldBounds.x();
    if (deltaLeft > 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(oldBounds.x(), oldBounds.y(), deltaLeft, oldBounds.height()));
    else if (deltaLeft < 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(newBounds.x(), newBounds.y(), -deltaLeft, newBounds.height()));

    LayoutUnit deltaRight = newBounds.maxX() - oldBounds.maxX();
    if (deltaRight > 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(oldBounds.maxX(), newBounds.y(), deltaRight, newBounds.height()));
    else if (deltaRight < 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(newBounds.maxX(), oldBounds.y(), -deltaRight, oldBounds.height()));

    LayoutUnit deltaTop = newBounds.y() - oldBounds.y();
    if (deltaTop > 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(oldBounds.x(), oldBounds.y(), oldBounds.width(), deltaTop));
    else if (deltaTop < 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(newBounds.x(), newBounds.y(), newBounds.width(), -deltaTop));

    LayoutUnit deltaBottom = newBounds.maxY() - oldBounds.maxY();
    if (deltaBottom > 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(newBounds.x(), oldBounds.maxY(), newBounds.width(), deltaBottom));
    else if (deltaBottom < 0)
        repaintUsingContainer(repaintContainer, pixelSnappedIntRect(oldBounds.x(), newBounds.maxY(), oldBounds.width(), -deltaBottom));

    if (newOutlineBox == oldOutlineBox)
        return false;

    // We didn't move, but we did change size. Invalidate the delta, which will consist of possibly
    // two rectangles (but typically only one).
    RenderStyle* outlineStyle = outlineStyleForRepaint();
    LayoutUnit outlineWidth = outlineStyle->outlineSize();
    LayoutBoxExtent insetShadowExtent = style()->getBoxShadowInsetExtent();
    LayoutUnit width = absoluteValue(newOutlineBox.width() - oldOutlineBox.width());
    if (width) {
        LayoutUnit shadowLeft;
        LayoutUnit shadowRight;
        style()->getBoxShadowHorizontalExtent(shadowLeft, shadowRight);
        int borderRight = isBox() ? toRenderBox(this)->borderRight() : 0;
        LayoutUnit boxWidth = isBox() ? toRenderBox(this)->width() : LayoutUnit();
        LayoutUnit minInsetRightShadowExtent = min<LayoutUnit>(-insetShadowExtent.right(), min<LayoutUnit>(newBounds.width(), oldBounds.width()));
        LayoutUnit borderWidth = max<LayoutUnit>(borderRight, max<LayoutUnit>(valueForLength(style()->borderTopRightRadius().width(), boxWidth, v), valueForLength(style()->borderBottomRightRadius().width(), boxWidth, v)));
        LayoutUnit decorationsWidth = max<LayoutUnit>(-outlineStyle->outlineOffset(), borderWidth + minInsetRightShadowExtent) + max<LayoutUnit>(outlineWidth, shadowRight);
        LayoutRect rightRect(newOutlineBox.x() + min(newOutlineBox.width(), oldOutlineBox.width()) - decorationsWidth,
            newOutlineBox.y(),
            width + decorationsWidth,
            max(newOutlineBox.height(), oldOutlineBox.height()));
        LayoutUnit right = min<LayoutUnit>(newBounds.maxX(), oldBounds.maxX());
        if (rightRect.x() < right) {
            rightRect.setWidth(min(rightRect.width(), right - rightRect.x()));
            repaintUsingContainer(repaintContainer, pixelSnappedIntRect(rightRect));
        }
    }
    LayoutUnit height = absoluteValue(newOutlineBox.height() - oldOutlineBox.height());
    if (height) {
        LayoutUnit shadowTop;
        LayoutUnit shadowBottom;
        style()->getBoxShadowVerticalExtent(shadowTop, shadowBottom);
        int borderBottom = isBox() ? toRenderBox(this)->borderBottom() : 0;
        LayoutUnit boxHeight = isBox() ? toRenderBox(this)->height() : LayoutUnit();
        LayoutUnit minInsetBottomShadowExtent = min<LayoutUnit>(-insetShadowExtent.bottom(), min<LayoutUnit>(newBounds.height(), oldBounds.height()));
        LayoutUnit borderHeight = max<LayoutUnit>(borderBottom, max<LayoutUnit>(valueForLength(style()->borderBottomLeftRadius().height(), boxHeight, v), valueForLength(style()->borderBottomRightRadius().height(), boxHeight, v)));
        LayoutUnit decorationsHeight = max<LayoutUnit>(-outlineStyle->outlineOffset(), borderHeight + minInsetBottomShadowExtent) + max<LayoutUnit>(outlineWidth, shadowBottom);
        LayoutRect bottomRect(newOutlineBox.x(),
            min(newOutlineBox.maxY(), oldOutlineBox.maxY()) - decorationsHeight,
            max(newOutlineBox.width(), oldOutlineBox.width()),
            height + decorationsHeight);
        LayoutUnit bottom = min(newBounds.maxY(), oldBounds.maxY());
        if (bottomRect.y() < bottom) {
            bottomRect.setHeight(min(bottomRect.height(), bottom - bottomRect.y()));
            repaintUsingContainer(repaintContainer, pixelSnappedIntRect(bottomRect));
        }
    }
    return false;
}

bool RenderObject::checkForRepaintDuringLayout() const
{
    return !document()->view()->needsFullRepaint() && !hasLayer() && everHadLayout();
}

LayoutRect RenderObject::rectWithOutlineForRepaint(const RenderLayerModelObject* repaintContainer, LayoutUnit outlineWidth) const
{
    LayoutRect r(clippedOverflowRectForRepaint(repaintContainer));
    r.inflate(outlineWidth);
    return r;
}

LayoutRect RenderObject::clippedOverflowRectForRepaint(const RenderLayerModelObject*) const
{
    ASSERT_NOT_REACHED();
    return LayoutRect();
}

void RenderObject::computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect& rect, bool fixed) const
{
    if (repaintContainer == this)
        return;

    if (RenderObject* o = parent()) {
        if (o->isBlockFlow()) {
            RenderBlock* cb = toRenderBlock(o);
            if (cb->hasColumns())
                cb->adjustRectForColumns(rect);
        }

        if (o->hasOverflowClip()) {
            RenderBox* boxParent = toRenderBox(o);
            boxParent->applyCachedClipAndScrollOffsetForRepaint(rect);
            if (rect.isEmpty())
                return;
        }

        o->computeRectForRepaint(repaintContainer, rect, fixed);
    }
}

void RenderObject::computeFloatRectForRepaint(const RenderLayerModelObject*, FloatRect&, bool) const
{
    ASSERT_NOT_REACHED();
}

void RenderObject::dirtyLinesFromChangedChild(RenderObject*)
{
}

#ifndef NDEBUG

void RenderObject::showTreeForThis() const
{
    if (node())
        node()->showTreeForThis();
}

void RenderObject::showRenderTreeForThis() const
{
    showRenderTree(this, 0);
}

void RenderObject::showLineTreeForThis() const
{
    if (containingBlock())
        containingBlock()->showLineTreeAndMark(0, 0, 0, 0, this);
}

void RenderObject::showRenderObject() const
{
    showRenderObject(0);
}

void RenderObject::showRenderObject(int printedCharacters) const
{
    // As this function is intended to be used when debugging, the
    // this pointer may be 0.
    if (!this) {
        fputs("(null)\n", stderr);
        return;
    }

    printedCharacters += fprintf(stderr, "%s %p", renderName(), this);

    if (node()) {
        if (printedCharacters)
            for (; printedCharacters < showTreeCharacterOffset; printedCharacters++)
                fputc(' ', stderr);
        fputc('\t', stderr);
        node()->showNode();
    } else
        fputc('\n', stderr);
}

void RenderObject::showRenderTreeAndMark(const RenderObject* markedObject1, const char* markedLabel1, const RenderObject* markedObject2, const char* markedLabel2, int depth) const
{
    int printedCharacters = 0;
    if (markedObject1 == this && markedLabel1)
        printedCharacters += fprintf(stderr, "%s", markedLabel1);
    if (markedObject2 == this && markedLabel2)
        printedCharacters += fprintf(stderr, "%s", markedLabel2);
    for (; printedCharacters < depth * 2; printedCharacters++)
        fputc(' ', stderr);

    showRenderObject(printedCharacters);
    if (!this)
        return;

    for (const RenderObject* child = firstChild(); child; child = child->nextSibling())
        child->showRenderTreeAndMark(markedObject1, markedLabel1, markedObject2, markedLabel2, depth + 1);
}

#endif // NDEBUG

Color RenderObject::selectionBackgroundColor() const
{
    Color color;
    if (style()->userSelect() != SELECT_NONE) {
        if (frame()->selection()->shouldShowBlockCursor() && frame()->selection()->isCaret())
            color = style()->visitedDependentColor(CSSPropertyColor).blendWithWhite();
        else {
            RefPtr<RenderStyle> pseudoStyle = getUncachedPseudoStyle(PseudoStyleRequest(SELECTION));
            if (pseudoStyle && pseudoStyle->visitedDependentColor(CSSPropertyBackgroundColor).isValid())
                color = pseudoStyle->visitedDependentColor(CSSPropertyBackgroundColor).blendWithWhite();
            else
                color = frame()->selection()->isFocusedAndActive() ? theme()->activeSelectionBackgroundColor() : theme()->inactiveSelectionBackgroundColor();
        }
    }

    return color;
}

Color RenderObject::selectionColor(int colorProperty) const
{
    Color color;
    // If the element is unselectable, or we are only painting the selection,
    // don't override the foreground color with the selection foreground color.
    if (style()->userSelect() == SELECT_NONE
        || (frame()->view()->paintBehavior() & PaintBehaviorSelectionOnly))
        return color;

    if (RefPtr<RenderStyle> pseudoStyle = getUncachedPseudoStyle(PseudoStyleRequest(SELECTION))) {
        color = pseudoStyle->visitedDependentColor(colorProperty);
        if (!color.isValid())
            color = pseudoStyle->visitedDependentColor(CSSPropertyColor);
    } else
        color = frame()->selection()->isFocusedAndActive() ?
                theme()->activeSelectionForegroundColor() :
                theme()->inactiveSelectionForegroundColor();

    return color;
}

Color RenderObject::selectionForegroundColor() const
{
    return selectionColor(CSSPropertyWebkitTextFillColor);
}

Color RenderObject::selectionEmphasisMarkColor() const
{
    return selectionColor(CSSPropertyWebkitTextEmphasisColor);
}

void RenderObject::selectionStartEnd(int& spos, int& epos) const
{
    view()->selectionStartEnd(spos, epos);
}

void RenderObject::handleDynamicFloatPositionChange()
{
    // We have gone from not affecting the inline status of the parent flow to suddenly
    // having an impact.  See if there is a mismatch between the parent flow's
    // childrenInline() state and our state.
    setInline(style()->isDisplayInlineType());
    if (isInline() != parent()->childrenInline()) {
        if (!isInline())
            toRenderBoxModelObject(parent())->childBecameNonInline(this);
        else {
            // An anonymous block must be made to wrap this inline.
            RenderBlock* block = toRenderBlock(parent())->createAnonymousBlock();
            RenderObjectChildList* childlist = parent()->virtualChildren();
            childlist->insertChildNode(parent(), block, this);
            block->children()->appendChildNode(block, childlist->removeChildNode(parent(), this));
        }
    }
}

void RenderObject::removeAnonymousWrappersForInlinesIfNecessary()
{
    // We have changed to floated or out-of-flow positioning so maybe all our parent's
    // children can be inline now. Bail if there are any block children left on the line,
    // otherwise we can proceed to stripping solitary anonymous wrappers from the inlines.
    // FIXME: We should also handle split inlines here - we exclude them at the moment by returning
    // if we find a continuation.
    RenderObject* curr = parent()->firstChild();
    while (curr && ((curr->isAnonymousBlock() && !toRenderBlock(curr)->isAnonymousBlockContinuation()) || curr->style()->isFloating() || curr->style()->hasOutOfFlowPosition()))
        curr = curr->nextSibling();

    if (curr)
        return;

    curr = parent()->firstChild();
    RenderBlock* parentBlock = toRenderBlock(parent());
    while (curr) {
        RenderObject* next = curr->nextSibling();
        if (curr->isAnonymousBlock())
            parentBlock->collapseAnonymousBoxChild(parentBlock, toRenderBlock(curr));
        curr = next;
    }
}

void RenderObject::setAnimatableStyle(PassRefPtr<RenderStyle> style)
{
    if (!isText() && style)
        setStyle(animation()->updateAnimations(this, style.get()));
    else
        setStyle(style);
}

StyleDifference RenderObject::adjustStyleDifference(StyleDifference diff, unsigned contextSensitiveProperties) const
{
#if USE(ACCELERATED_COMPOSITING)
    // If transform changed, and we are not composited, need to do a layout.
    if (contextSensitiveProperties & ContextSensitivePropertyTransform) {
        // Text nodes share style with their parents but transforms don't apply to them,
        // hence the !isText() check.
        // FIXME: when transforms are taken into account for overflow, we will need to do a layout.
        if (!isText() && (!hasLayer() || !toRenderLayerModelObject(this)->layer()->isComposited())) {
            // We need to set at least SimplifiedLayout, but if PositionedMovementOnly is already set
            // then we actually need SimplifiedLayoutAndPositionedMovement.
            if (!hasLayer())
                diff = StyleDifferenceLayout; // FIXME: Do this for now since SimplifiedLayout cannot handle updating floating objects lists.
            else if (diff < StyleDifferenceLayoutPositionedMovementOnly)
                diff = StyleDifferenceSimplifiedLayout;
            else if (diff < StyleDifferenceSimplifiedLayout)
                diff = StyleDifferenceSimplifiedLayoutAndPositionedMovement;
        } else if (diff < StyleDifferenceRecompositeLayer)
            diff = StyleDifferenceRecompositeLayer;
    }

    // If opacity changed, and we are not composited, need to repaint (also
    // ignoring text nodes)
    if (contextSensitiveProperties & ContextSensitivePropertyOpacity) {
        if (!isText() && (!hasLayer() || !toRenderLayerModelObject(this)->layer()->isComposited()))
            diff = StyleDifferenceRepaintLayer;
        else if (diff < StyleDifferenceRecompositeLayer)
            diff = StyleDifferenceRecompositeLayer;
    }
    
#if ENABLE(CSS_FILTERS)
    if ((contextSensitiveProperties & ContextSensitivePropertyFilter) && hasLayer()) {
        RenderLayer* layer = toRenderLayerModelObject(this)->layer();
        if (!layer->isComposited() || layer->paintsWithFilters())
            diff = StyleDifferenceRepaintLayer;
        else if (diff < StyleDifferenceRecompositeLayer)
            diff = StyleDifferenceRecompositeLayer;
    }
#endif
    
    // The answer to requiresLayer() for plugins, iframes, and canvas can change without the actual
    // style changing, since it depends on whether we decide to composite these elements. When the
    // layer status of one of these elements changes, we need to force a layout.
    if (diff == StyleDifferenceEqual && style() && isLayerModelObject()) {
        if (hasLayer() != toRenderLayerModelObject(this)->requiresLayer())
            diff = StyleDifferenceLayout;
    }
#else
    UNUSED_PARAM(contextSensitiveProperties);
#endif

    // If we have no layer(), just treat a RepaintLayer hint as a normal Repaint.
    if (diff == StyleDifferenceRepaintLayer && !hasLayer())
        diff = StyleDifferenceRepaint;

    return diff;
}

void RenderObject::setPseudoStyle(PassRefPtr<RenderStyle> pseudoStyle)
{
    ASSERT(pseudoStyle->styleType() == BEFORE || pseudoStyle->styleType() == AFTER);

    // Images are special and must inherit the pseudoStyle so the width and height of
    // the pseudo element doesn't change the size of the image. In all other cases we
    // can just share the style.
    if (isImage()) {
        RefPtr<RenderStyle> style = RenderStyle::create();
        style->inheritFrom(pseudoStyle.get());
        setStyle(style.release());
        return;
    }

    setStyle(pseudoStyle);
}

inline bool RenderObject::hasImmediateNonWhitespaceTextChild() const
{
    for (const RenderObject* r = firstChild(); r; r = r->nextSibling()) {
        if (r->isText() && !toRenderText(r)->isAllCollapsibleWhitespace())
            return true;
    }
    return false;
}

inline bool RenderObject::shouldRepaintForStyleDifference(StyleDifference diff) const
{
    return diff == StyleDifferenceRepaint || (diff == StyleDifferenceRepaintIfText && hasImmediateNonWhitespaceTextChild());
}

void RenderObject::setStyle(PassRefPtr<RenderStyle> style)
{
    if (m_style == style) {
#if USE(ACCELERATED_COMPOSITING)
        // We need to run through adjustStyleDifference() for iframes, plugins, and canvas so
        // style sharing is disabled for them. That should ensure that we never hit this code path.
        ASSERT(!isRenderIFrame() && !isEmbeddedObject() && !isCanvas());
#endif
        return;
    }

    StyleDifference diff = StyleDifferenceEqual;
    unsigned contextSensitiveProperties = ContextSensitivePropertyNone;
    if (m_style)
        diff = m_style->diff(style.get(), contextSensitiveProperties);

    diff = adjustStyleDifference(diff, contextSensitiveProperties);

    styleWillChange(diff, style.get());
    
    RefPtr<RenderStyle> oldStyle = m_style.release();
    setStyleInternal(style);

    updateFillImages(oldStyle ? oldStyle->backgroundLayers() : 0, m_style ? m_style->backgroundLayers() : 0);
    updateFillImages(oldStyle ? oldStyle->maskLayers() : 0, m_style ? m_style->maskLayers() : 0);

    updateImage(oldStyle ? oldStyle->borderImage().image() : 0, m_style ? m_style->borderImage().image() : 0);
    updateImage(oldStyle ? oldStyle->maskBoxImage().image() : 0, m_style ? m_style->maskBoxImage().image() : 0);

    // We need to ensure that view->maximalOutlineSize() is valid for any repaints that happen
    // during styleDidChange (it's used by clippedOverflowRectForRepaint()).
    if (m_style->outlineWidth() > 0 && m_style->outlineSize() > maximalOutlineSize(PaintPhaseOutline))
        toRenderView(document()->renderer())->setMaximalOutlineSize(m_style->outlineSize());

    bool doesNotNeedLayout = !m_parent || isText();

    styleDidChange(diff, oldStyle.get());

    // FIXME: |this| might be destroyed here. This can currently happen for a RenderTextFragment when
    // its first-letter block gets an update in RenderTextFragment::styleDidChange. For RenderTextFragment(s),
    // we will safely bail out with the doesNotNeedLayout flag. We might want to broaden this condition
    // in the future as we move renderer changes out of layout and into style changes.
    if (doesNotNeedLayout)
        return;

    // Now that the layer (if any) has been updated, we need to adjust the diff again,
    // check whether we should layout now, and decide if we need to repaint.
    StyleDifference updatedDiff = adjustStyleDifference(diff, contextSensitiveProperties);
    
    if (diff <= StyleDifferenceLayoutPositionedMovementOnly) {
        if (updatedDiff == StyleDifferenceLayout)
            setNeedsLayoutAndPrefWidthsRecalc();
        else if (updatedDiff == StyleDifferenceLayoutPositionedMovementOnly)
            setNeedsPositionedMovementLayout(oldStyle.get());
        else if (updatedDiff == StyleDifferenceSimplifiedLayoutAndPositionedMovement) {
            setNeedsPositionedMovementLayout(oldStyle.get());
            setNeedsSimplifiedNormalFlowLayout();
        } else if (updatedDiff == StyleDifferenceSimplifiedLayout)
            setNeedsSimplifiedNormalFlowLayout();
    }

    if (updatedDiff == StyleDifferenceRepaintLayer || shouldRepaintForStyleDifference(updatedDiff)) {
        // Do a repaint with the new style now, e.g., for example if we go from
        // not having an outline to having an outline.
        repaint();
    }
}

static inline bool rendererHasBackground(const RenderObject* renderer)
{
    return renderer && renderer->hasBackground();
}

void RenderObject::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (m_style) {
        // If our z-index changes value or our visibility changes,
        // we need to dirty our stacking context's z-order list.
        if (newStyle) {
            bool visibilityChanged = m_style->visibility() != newStyle->visibility() 
                || m_style->zIndex() != newStyle->zIndex() 
                || m_style->hasAutoZIndex() != newStyle->hasAutoZIndex();
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
            if (visibilityChanged)
                document()->setAnnotatedRegionsDirty(true);
#endif
            if (visibilityChanged) {
                if (AXObjectCache* cache = document()->existingAXObjectCache())
                    cache->childrenChanged(parent());
            }

            // Keep layer hierarchy visibility bits up to date if visibility changes.
            if (m_style->visibility() != newStyle->visibility()) {
                if (RenderLayer* l = enclosingLayer()) {
                    if (newStyle->visibility() == VISIBLE)
                        l->setHasVisibleContent();
                    else if (l->hasVisibleContent() && (this == l->renderer() || l->renderer()->style()->visibility() != VISIBLE)) {
                        l->dirtyVisibleContentStatus();
                        if (diff > StyleDifferenceRepaintLayer)
                            repaint();
                    }
                }
            }
        }

        if (m_parent && (newStyle->outlineSize() < m_style->outlineSize() || shouldRepaintForStyleDifference(diff)))
            repaint();
        if (isFloating() && (m_style->floating() != newStyle->floating()))
            // For changes in float styles, we need to conceivably remove ourselves
            // from the floating objects list.
            toRenderBox(this)->removeFloatingOrPositionedChildFromBlockLists();
        else if (isOutOfFlowPositioned() && (m_style->position() != newStyle->position()))
            // For changes in positioning styles, we need to conceivably remove ourselves
            // from the positioned objects list.
            toRenderBox(this)->removeFloatingOrPositionedChildFromBlockLists();

        s_affectsParentBlock = isFloatingOrOutOfFlowPositioned()
            && (!newStyle->isFloating() && !newStyle->hasOutOfFlowPosition())
            && parent() && (parent()->isBlockFlow() || parent()->isRenderInline());

        s_noLongerAffectsParentBlock = ((!isFloating() && newStyle->isFloating()) || (!isOutOfFlowPositioned() && newStyle->hasOutOfFlowPosition()))
            && parent() && parent()->isRenderBlock();

        // reset style flags
        if (diff == StyleDifferenceLayout || diff == StyleDifferenceLayoutPositionedMovementOnly) {
            setFloating(false);
            clearPositionedState();
        }
        setHorizontalWritingMode(true);
        setHasBoxDecorations(false);
        setHasOverflowClip(false);
        setHasTransform(false);
        setHasReflection(false);
    } else {
        s_affectsParentBlock = false;
        s_noLongerAffectsParentBlock = false;
    }

    if (FrameView* frameView = view()->frameView()) {
        bool repaintFixedBackgroundsOnScroll = shouldRepaintFixedBackgroundsOnScroll(frameView);

        bool newStyleSlowScroll = newStyle && repaintFixedBackgroundsOnScroll && newStyle->hasFixedBackgroundImage();
        bool oldStyleSlowScroll = m_style && repaintFixedBackgroundsOnScroll && m_style->hasFixedBackgroundImage();

#if USE(ACCELERATED_COMPOSITING)
        bool drawsRootBackground = isRoot() || (isBody() && !rendererHasBackground(document()->documentElement()->renderer()));
        if (drawsRootBackground && repaintFixedBackgroundsOnScroll) {
            if (view()->compositor()->supportsFixedRootBackgroundCompositing()) {
                if (newStyleSlowScroll && newStyle->hasEntirelyFixedBackground())
                    newStyleSlowScroll = false;

                if (oldStyleSlowScroll && m_style->hasEntirelyFixedBackground())
                    oldStyleSlowScroll = false;
            }
        }
#endif
        if (oldStyleSlowScroll != newStyleSlowScroll) {
            if (oldStyleSlowScroll)
                frameView->removeSlowRepaintObject(this);

            if (newStyleSlowScroll)
                frameView->addSlowRepaintObject(this);
        }
    }
}

static bool areNonIdenticalCursorListsEqual(const RenderStyle* a, const RenderStyle* b)
{
    ASSERT(a->cursors() != b->cursors());
    return a->cursors() && b->cursors() && *a->cursors() == *b->cursors();
}

static inline bool areCursorsEqual(const RenderStyle* a, const RenderStyle* b)
{
    return a->cursor() == b->cursor() && (a->cursors() == b->cursors() || areNonIdenticalCursorListsEqual(a, b));
}

void RenderObject::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    if (s_affectsParentBlock)
        handleDynamicFloatPositionChange();

    if (s_noLongerAffectsParentBlock)
        removeAnonymousWrappersForInlinesIfNecessary();
#if ENABLE(SVG)
    SVGRenderSupport::styleChanged(this);
#endif

    if (!m_parent)
        return;
    
    if (diff == StyleDifferenceLayout || diff == StyleDifferenceSimplifiedLayout) {
        RenderCounter::rendererStyleChanged(this, oldStyle, m_style.get());

        // If the object already needs layout, then setNeedsLayout won't do
        // any work. But if the containing block has changed, then we may need
        // to mark the new containing blocks for layout. The change that can
        // directly affect the containing block of this object is a change to
        // the position style.
        if (needsLayout() && oldStyle->position() != m_style->position())
            markContainingBlocksForLayout();

        if (diff == StyleDifferenceLayout)
            setNeedsLayoutAndPrefWidthsRecalc();
        else
            setNeedsSimplifiedNormalFlowLayout();
    } else if (diff == StyleDifferenceSimplifiedLayoutAndPositionedMovement) {
        setNeedsPositionedMovementLayout(oldStyle);
        setNeedsSimplifiedNormalFlowLayout();
    } else if (diff == StyleDifferenceLayoutPositionedMovementOnly)
        setNeedsPositionedMovementLayout(oldStyle);

    // Don't check for repaint here; we need to wait until the layer has been
    // updated by subclasses before we know if we have to repaint (in setStyle()).

    if (oldStyle && !areCursorsEqual(oldStyle, style())) {
        if (Frame* frame = this->frame())
            frame->eventHandler()->scheduleCursorUpdate();
    }
}

void RenderObject::propagateStyleToAnonymousChildren(bool blockChildrenOnly)
{
    // FIXME: We could save this call when the change only affected non-inherited properties.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isAnonymous() || child->style()->styleType() != NOPSEUDO)
            continue;

        if (blockChildrenOnly && !child->isRenderBlock())
            continue;

#if ENABLE(FULLSCREEN_API)
        if (child->isRenderFullScreen() || child->isRenderFullScreenPlaceholder())
            continue;
#endif

        RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(style(), child->style()->display());
        if (style()->specifiesColumns()) {
            if (child->style()->specifiesColumns())
                newStyle->inheritColumnPropertiesFrom(style());
            if (child->style()->columnSpan())
                newStyle->setColumnSpan(ColumnSpanAll);
        }

        // Preserve the position style of anonymous block continuations as they can have relative or sticky position when
        // they contain block descendants of relative or sticky positioned inlines.
        if (child->isInFlowPositioned() && toRenderBlock(child)->isAnonymousBlockContinuation())
            newStyle->setPosition(child->style()->position());

        child->setStyle(newStyle.release());
    }
}

void RenderObject::updateFillImages(const FillLayer* oldLayers, const FillLayer* newLayers)
{
    // Optimize the common case
    if (oldLayers && !oldLayers->next() && newLayers && !newLayers->next() && (oldLayers->image() == newLayers->image()))
        return;
    
    // Go through the new layers and addClients first, to avoid removing all clients of an image.
    for (const FillLayer* currNew = newLayers; currNew; currNew = currNew->next()) {
        if (currNew->image())
            currNew->image()->addClient(this);
    }

    for (const FillLayer* currOld = oldLayers; currOld; currOld = currOld->next()) {
        if (currOld->image())
            currOld->image()->removeClient(this);
    }
}

void RenderObject::updateImage(StyleImage* oldImage, StyleImage* newImage)
{
    if (oldImage != newImage) {
        if (oldImage)
            oldImage->removeClient(this);
        if (newImage)
            newImage->addClient(this);
    }
}

LayoutRect RenderObject::viewRect() const
{
    return view()->viewRect();
}

FloatPoint RenderObject::localToAbsolute(const FloatPoint& localPoint, MapCoordinatesFlags mode) const
{
    TransformState transformState(TransformState::ApplyTransformDirection, localPoint);
    mapLocalToContainer(0, transformState, mode | ApplyContainerFlip);
    transformState.flatten();
    
    return transformState.lastPlanarPoint();
}

FloatPoint RenderObject::absoluteToLocal(const FloatPoint& containerPoint, MapCoordinatesFlags mode) const
{
    TransformState transformState(TransformState::UnapplyInverseTransformDirection, containerPoint);
    mapAbsoluteToLocalPoint(mode, transformState);
    transformState.flatten();
    
    return transformState.lastPlanarPoint();
}

FloatQuad RenderObject::absoluteToLocalQuad(const FloatQuad& quad, MapCoordinatesFlags mode) const
{
    TransformState transformState(TransformState::UnapplyInverseTransformDirection, quad.boundingBox().center(), quad);
    mapAbsoluteToLocalPoint(mode, transformState);
    transformState.flatten();
    return transformState.lastPlanarQuad();
}

void RenderObject::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
    if (repaintContainer == this)
        return;

    RenderObject* o = parent();
    if (!o)
        return;

    // FIXME: this should call offsetFromContainer to share code, but I'm not sure it's ever called.
    LayoutPoint centerPoint = roundedLayoutPoint(transformState.mappedPoint());
    if (mode & ApplyContainerFlip && o->isBox()) {
        if (o->style()->isFlippedBlocksWritingMode())
            transformState.move(toRenderBox(o)->flipForWritingModeIncludingColumns(roundedLayoutPoint(transformState.mappedPoint())) - centerPoint);
        mode &= ~ApplyContainerFlip;
    }

    LayoutSize columnOffset;
    o->adjustForColumns(columnOffset, roundedLayoutPoint(transformState.mappedPoint()));
    if (!columnOffset.isZero())
        transformState.move(columnOffset);

    if (o->hasOverflowClip())
        transformState.move(-toRenderBox(o)->scrolledContentOffset());

    o->mapLocalToContainer(repaintContainer, transformState, mode, wasFixed);
}

const RenderObject* RenderObject::pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap& geometryMap) const
{
    ASSERT_UNUSED(ancestorToStopAt, ancestorToStopAt != this);

    RenderObject* container = parent();
    if (!container)
        return 0;

    // FIXME: this should call offsetFromContainer to share code, but I'm not sure it's ever called.
    LayoutSize offset;
    if (container->hasOverflowClip())
        offset = -toRenderBox(container)->scrolledContentOffset();

    geometryMap.push(this, offset, hasColumns());
    
    return container;
}

void RenderObject::mapAbsoluteToLocalPoint(MapCoordinatesFlags mode, TransformState& transformState) const
{
    RenderObject* o = parent();
    if (o) {
        o->mapAbsoluteToLocalPoint(mode, transformState);
        if (o->hasOverflowClip())
            transformState.move(toRenderBox(o)->scrolledContentOffset());
    }
}

bool RenderObject::shouldUseTransformFromContainer(const RenderObject* containerObject) const
{
#if ENABLE(3D_RENDERING)
    // hasTransform() indicates whether the object has transform, transform-style or perspective. We just care about transform,
    // so check the layer's transform directly.
    return (hasLayer() && toRenderLayerModelObject(this)->layer()->transform()) || (containerObject && containerObject->style()->hasPerspective());
#else
    UNUSED_PARAM(containerObject);
    return hasTransform();
#endif
}

void RenderObject::getTransformFromContainer(const RenderObject* containerObject, const LayoutSize& offsetInContainer, TransformationMatrix& transform) const
{
    transform.makeIdentity();
    transform.translate(offsetInContainer.width(), offsetInContainer.height());
    RenderLayer* layer;
    if (hasLayer() && (layer = toRenderLayerModelObject(this)->layer()) && layer->transform())
        transform.multiply(layer->currentTransform());
    
#if ENABLE(3D_RENDERING)
    if (containerObject && containerObject->hasLayer() && containerObject->style()->hasPerspective()) {
        // Perpsective on the container affects us, so we have to factor it in here.
        ASSERT(containerObject->hasLayer());
        FloatPoint perspectiveOrigin = toRenderLayerModelObject(containerObject)->layer()->perspectiveOrigin();

        TransformationMatrix perspectiveMatrix;
        perspectiveMatrix.applyPerspective(containerObject->style()->perspective());
        
        transform.translateRight3d(-perspectiveOrigin.x(), -perspectiveOrigin.y(), 0);
        transform = perspectiveMatrix * transform;
        transform.translateRight3d(perspectiveOrigin.x(), perspectiveOrigin.y(), 0);
    }
#else
    UNUSED_PARAM(containerObject);
#endif
}

FloatQuad RenderObject::localToContainerQuad(const FloatQuad& localQuad, const RenderLayerModelObject* repaintContainer, MapCoordinatesFlags mode, bool* wasFixed) const
{
    // Track the point at the center of the quad's bounding box. As mapLocalToContainer() calls offsetFromContainer(),
    // it will use that point as the reference point to decide which column's transform to apply in multiple-column blocks.
    TransformState transformState(TransformState::ApplyTransformDirection, localQuad.boundingBox().center(), localQuad);
    mapLocalToContainer(repaintContainer, transformState, mode | ApplyContainerFlip | UseTransforms, wasFixed);
    transformState.flatten();
    
    return transformState.lastPlanarQuad();
}

FloatPoint RenderObject::localToContainerPoint(const FloatPoint& localPoint, const RenderLayerModelObject* repaintContainer, MapCoordinatesFlags mode, bool* wasFixed) const
{
    TransformState transformState(TransformState::ApplyTransformDirection, localPoint);
    mapLocalToContainer(repaintContainer, transformState, mode | ApplyContainerFlip | UseTransforms, wasFixed);
    transformState.flatten();

    return transformState.lastPlanarPoint();
}

LayoutSize RenderObject::offsetFromContainer(RenderObject* o, const LayoutPoint& point, bool* offsetDependsOnPoint) const
{
    ASSERT(o == container());

    LayoutSize offset;

    o->adjustForColumns(offset, point);

    if (o->hasOverflowClip())
        offset -= toRenderBox(o)->scrolledContentOffset();

    if (offsetDependsOnPoint)
        *offsetDependsOnPoint = hasColumns() || o->isRenderFlowThread();

    return offset;
}

LayoutSize RenderObject::offsetFromAncestorContainer(RenderObject* container) const
{
    LayoutSize offset;
    LayoutPoint referencePoint;
    const RenderObject* currContainer = this;
    do {
        RenderObject* nextContainer = currContainer->container();
        ASSERT(nextContainer);  // This means we reached the top without finding container.
        if (!nextContainer)
            break;
        ASSERT(!currContainer->hasTransform());
        LayoutSize currentOffset = currContainer->offsetFromContainer(nextContainer, referencePoint);
        offset += currentOffset;
        referencePoint.move(currentOffset);
        currContainer = nextContainer;
    } while (currContainer != container);

    return offset;
}

LayoutRect RenderObject::localCaretRect(InlineBox*, int, LayoutUnit* extraWidthToEndOfLine)
{
    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = 0;

    return LayoutRect();
}

bool RenderObject::isRooted(RenderView** view) const
{
    const RenderObject* o = this;
    while (o->parent())
        o = o->parent();

    if (!o->isRenderView())
        return false;

    if (view)
        *view = const_cast<RenderView*>(toRenderView(o));

    return true;
}

RenderObject* RenderObject::rendererForRootBackground()
{
    ASSERT(isRoot());
    if (!hasBackground() && node() && node()->hasTagName(HTMLNames::htmlTag)) {
        // Locate the <body> element using the DOM. This is easier than trying
        // to crawl around a render tree with potential :before/:after content and
        // anonymous blocks created by inline <body> tags etc. We can locate the <body>
        // render object very easily via the DOM.
        HTMLElement* body = document()->body();
        RenderObject* bodyObject = (body && body->hasLocalName(bodyTag)) ? body->renderer() : 0;
        if (bodyObject)
            return bodyObject;
    }
    
    return this;
}

RespectImageOrientationEnum RenderObject::shouldRespectImageOrientation() const
{
    // Respect the image's orientation if it's being used as a full-page image or it's
    // an <img> and the setting to respect it everywhere is set.
    return
#if USE(CG) || USE(CAIRO) || PLATFORM(BLACKBERRY)
        // This can only be enabled for ports which honor the orientation flag in their drawing code.
        document()->isImageDocument() ||
#endif
        (document()->settings() && document()->settings()->shouldRespectImageOrientation() && node() && isHTMLImageElement(node())) ? RespectImageOrientation : DoNotRespectImageOrientation;
}

bool RenderObject::hasOutlineAnnotation() const
{
    return node() && node()->isLink() && document()->printing();
}

bool RenderObject::hasEntirelyFixedBackground() const
{
    return m_style->hasEntirelyFixedBackground();
}

RenderObject* RenderObject::container(const RenderLayerModelObject* repaintContainer, bool* repaintContainerSkipped) const
{
    if (repaintContainerSkipped)
        *repaintContainerSkipped = false;

    // This method is extremely similar to containingBlock(), but with a few notable
    // exceptions.
    // (1) It can be used on orphaned subtrees, i.e., it can be called safely even when
    // the object is not part of the primary document subtree yet.
    // (2) For normal flow elements, it just returns the parent.
    // (3) For absolute positioned elements, it will return a relative positioned inline.
    // containingBlock() simply skips relpositioned inlines and lets an enclosing block handle
    // the layout of the positioned object.  This does mean that computePositionedLogicalWidth and
    // computePositionedLogicalHeight have to use container().
    RenderObject* o = parent();

    if (isText())
        return o;

    EPosition pos = m_style->position();
    if (pos == FixedPosition) {
        // container() can be called on an object that is not in the
        // tree yet.  We don't call view() since it will assert if it
        // can't get back to the canvas.  Instead we just walk as high up
        // as we can.  If we're in the tree, we'll get the root.  If we
        // aren't we'll get the root of our little subtree (most likely
        // we'll just return 0).
        // FIXME: The definition of view() has changed to not crawl up the render tree.  It might
        // be safe now to use it.
        while (o && o->parent() && !(o->hasTransform() && o->isRenderBlock())) {
#if ENABLE(SVG)
            // foreignObject is the containing block for its contents.
            if (o->isSVGForeignObject())
                break;
#endif
            // The render flow thread is the top most containing block
            // for the fixed positioned elements.
            if (o->isOutOfFlowRenderFlowThread())
                break;

            if (repaintContainerSkipped && o == repaintContainer)
                *repaintContainerSkipped = true;

            o = o->parent();
        }
    } else if (pos == AbsolutePosition) {
        // Same goes here.  We technically just want our containing block, but
        // we may not have one if we're part of an uninstalled subtree.  We'll
        // climb as high as we can though.
        while (o && o->style()->position() == StaticPosition && !o->isRenderView() && !(o->hasTransform() && o->isRenderBlock())) {
#if ENABLE(SVG)
            if (o->isSVGForeignObject()) // foreignObject is the containing block for contents inside it
                break;
#endif
            if (repaintContainerSkipped && o == repaintContainer)
                *repaintContainerSkipped = true;

            o = o->parent();
        }
    }

    return o;
}

bool RenderObject::isSelectionBorder() const
{
    SelectionState st = selectionState();
    return st == SelectionStart || st == SelectionEnd || st == SelectionBoth;
}

inline void RenderObject::clearLayoutRootIfNeeded() const
{
    if (!documentBeingDestroyed() && frame()) {
        if (FrameView* view = frame()->view()) {
            if (view->layoutRoot() == this) {
                ASSERT_NOT_REACHED();
                // This indicates a failure to layout the child, which is why
                // the layout root is still set to |this|. Make sure to clear it
                // since we are getting destroyed.
                view->clearLayoutRoot();
            }
        }
    }
}

void RenderObject::willBeDestroyed()
{
    // Destroy any leftover anonymous children.
    RenderObjectChildList* children = virtualChildren();
    if (children)
        children->destroyLeftoverChildren();

    // If this renderer is being autoscrolled, stop the autoscroll timer
    
    // FIXME: RenderObject::destroy should not get called with a renderer whose document
    // has a null frame, so we assert this. However, we don't want release builds to crash which is why we
    // check that the frame is not null.
    ASSERT(frame());
    if (frame() && frame()->eventHandler()->autoscrollRenderer() == this)
        frame()->eventHandler()->stopAutoscrollTimer(true);

    animation()->cancelAnimations(this);

    // For accessibility management, notify the parent of the imminent change to its child set.
    // We do it now, before remove(), while the parent pointer is still available.
    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->childrenChanged(this->parent());

    remove();

    ASSERT(documentBeingDestroyed() || !frame()->view()->hasSlowRepaintObject(this));

    // The remove() call above may invoke axObjectCache()->childrenChanged() on the parent, which may require the AX render
    // object for this renderer. So we remove the AX render object now, after the renderer is removed.
    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->remove(this);

#ifndef NDEBUG
    if (!documentBeingDestroyed() && view() && view()->hasRenderNamedFlowThreads()) {
        // After remove, the object and the associated information should not be in any flow thread.
        const RenderNamedFlowThreadList* flowThreadList = view()->flowThreadController()->renderNamedFlowThreadList();
        for (RenderNamedFlowThreadList::const_iterator iter = flowThreadList->begin(); iter != flowThreadList->end(); ++iter) {
            const RenderNamedFlowThread* renderFlowThread = *iter;
            ASSERT(!renderFlowThread->hasChild(this));
            ASSERT(!renderFlowThread->hasChildInfo(this));
        }
    }
#endif

    // If this renderer had a parent, remove should have destroyed any counters
    // attached to this renderer and marked the affected other counters for
    // reevaluation. This apparently redundant check is here for the case when
    // this renderer had no parent at the time remove() was called.

    if (hasCounterNodeMap())
        RenderCounter::destroyCounterNodes(this);

    // FIXME: Would like to do this in RenderBoxModelObject, but the timing is so complicated that this can't easily
    // be moved into RenderBoxModelObject::destroy.
    if (hasLayer()) {
        setHasLayer(false);
        toRenderLayerModelObject(this)->destroyLayer();
    }

    setAncestorLineBoxDirty(false);

    clearLayoutRootIfNeeded();
}

void RenderObject::insertedIntoTree()
{
    // FIXME: We should ASSERT(isRooted()) here but generated content makes some out-of-order insertion.

    // Keep our layer hierarchy updated. Optimize for the common case where we don't have any children
    // and don't have a layer attached to ourselves.
    RenderLayer* layer = 0;
    if (firstChild() || hasLayer()) {
        layer = parent()->enclosingLayer();
        addLayers(layer);
    }

    // If |this| is visible but this object was not, tell the layer it has some visible content
    // that needs to be drawn and layer visibility optimization can't be used
    if (parent()->style()->visibility() != VISIBLE && style()->visibility() == VISIBLE && !hasLayer()) {
        if (!layer)
            layer = parent()->enclosingLayer();
        if (layer)
            layer->setHasVisibleContent();
    }

    if (!isFloating() && parent()->childrenInline())
        parent()->dirtyLinesFromChangedChild(this);

    if (RenderNamedFlowThread* containerFlowThread = parent()->renderNamedFlowThreadWrapper())
        containerFlowThread->addFlowChild(this);
}

void RenderObject::willBeRemovedFromTree()
{
    // FIXME: We should ASSERT(isRooted()) but we have some out-of-order removals which would need to be fixed first.

    if (!isText()) {
        if (FrameView* frameView = view()->frameView()) {
            bool repaintFixedBackgroundsOnScroll = shouldRepaintFixedBackgroundsOnScroll(frameView);
            if (repaintFixedBackgroundsOnScroll && m_style && m_style->hasFixedBackgroundImage())
                frameView->removeSlowRepaintObject(this);
        }
    }

    // If we remove a visible child from an invisible parent, we don't know the layer visibility any more.
    RenderLayer* layer = 0;
    if (parent()->style()->visibility() != VISIBLE && style()->visibility() == VISIBLE && !hasLayer()) {
        if ((layer = parent()->enclosingLayer()))
            layer->dirtyVisibleContentStatus();
    }

    // Keep our layer hierarchy updated.
    if (firstChild() || hasLayer()) {
        if (!layer)
            layer = parent()->enclosingLayer();
        removeLayers(layer);
    }

    if (isOutOfFlowPositioned() && parent()->childrenInline())
        parent()->dirtyLinesFromChangedChild(this);

    removeFromRenderFlowThread();

    if (RenderNamedFlowThread* containerFlowThread = parent()->renderNamedFlowThreadWrapper())
        containerFlowThread->removeFlowChild(this);

#if ENABLE(SVG)
    // Update cached boundaries in SVG renderers, if a child is removed.
    parent()->setNeedsBoundariesUpdate();
#endif
}

void RenderObject::removeFromRenderFlowThread()
{
    if (flowThreadState() == NotInsideFlowThread)
        return;
    
    // Sometimes we remove the element from the flow, but it's not destroyed at that time.
    // It's only until later when we actually destroy it and remove all the children from it. 
    // Currently, that happens for firstLetter elements and list markers.
    // Pass in the flow thread so that we don't have to look it up for all the children.
    removeFromRenderFlowThreadRecursive(flowThreadContainingBlock());
}

void RenderObject::removeFromRenderFlowThreadRecursive(RenderFlowThread* renderFlowThread)
{
    if (const RenderObjectChildList* children = virtualChildren()) {
        for (RenderObject* child = children->firstChild(); child; child = child->nextSibling())
            child->removeFromRenderFlowThreadRecursive(renderFlowThread);
    }
    
    RenderFlowThread* localFlowThread = renderFlowThread;
    if (flowThreadState() == InsideInFlowThread)
        localFlowThread = flowThreadContainingBlock(); // We have to ask. We can't just assume we are in the same flow thread.
    if (localFlowThread)
        localFlowThread->removeFlowChildInfo(this);
    setFlowThreadState(NotInsideFlowThread);
}

void RenderObject::destroyAndCleanupAnonymousWrappers()
{
    // If the tree is destroyed, there is no need for a clean-up phase.
    if (documentBeingDestroyed()) {
        destroy();
        return;
    }

    RenderObject* destroyRoot = this;
    for (RenderObject* destroyRootParent = destroyRoot->parent(); destroyRootParent && destroyRootParent->isAnonymous(); destroyRoot = destroyRootParent, destroyRootParent = destroyRootParent->parent()) {
        // Currently we only remove anonymous cells' and table sections' wrappers but we should remove all unneeded
        // wrappers. See http://webkit.org/b/52123 as an example where this is needed.
        if (!destroyRootParent->isTableCell() && !destroyRootParent->isTableSection())
            break;

        if (destroyRootParent->firstChild() != this || destroyRootParent->lastChild() != this)
            break;
    }

    destroyRoot->destroy();

    // WARNING: |this| is deleted here.
}

void RenderObject::destroy()
{
    willBeDestroyed();
    arenaDelete(renderArena(), this);
}

void RenderObject::arenaDelete(RenderArena* arena, void* base)
{
    if (m_style) {
        for (const FillLayer* bgLayer = m_style->backgroundLayers(); bgLayer; bgLayer = bgLayer->next()) {
            if (StyleImage* backgroundImage = bgLayer->image())
                backgroundImage->removeClient(this);
        }

        for (const FillLayer* maskLayer = m_style->maskLayers(); maskLayer; maskLayer = maskLayer->next()) {
            if (StyleImage* maskImage = maskLayer->image())
                maskImage->removeClient(this);
        }

        if (StyleImage* borderImage = m_style->borderImage().image())
            borderImage->removeClient(this);

        if (StyleImage* maskBoxImage = m_style->maskBoxImage().image())
            maskBoxImage->removeClient(this);
    }

#ifndef NDEBUG
    void* savedBase = baseOfRenderObjectBeingDeleted;
    baseOfRenderObjectBeingDeleted = base;
#endif
    delete this;
#ifndef NDEBUG
    baseOfRenderObjectBeingDeleted = savedBase;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    arena->free(*(size_t*)base, base);
}

VisiblePosition RenderObject::positionForPoint(const LayoutPoint&)
{
    return createVisiblePosition(caretMinOffset(), DOWNSTREAM);
}

void RenderObject::updateDragState(bool dragOn)
{
    bool valueChanged = (dragOn != isDragging());
    setIsDragging(dragOn);
    if (valueChanged && node() && (style()->affectedByDrag() || (node()->isElementNode() && toElement(node())->childrenAffectedByDrag())))
        node()->setNeedsStyleRecalc();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->updateDragState(dragOn);
}

bool RenderObject::isComposited() const
{
    return hasLayer() && toRenderLayerModelObject(this)->layer()->isComposited();
}

bool RenderObject::hitTest(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestFilter hitTestFilter)
{
    bool inside = false;
    if (hitTestFilter != HitTestSelf) {
        // First test the foreground layer (lines and inlines).
        inside = nodeAtPoint(request, result, locationInContainer, accumulatedOffset, HitTestForeground);

        // Test floats next.
        if (!inside)
            inside = nodeAtPoint(request, result, locationInContainer, accumulatedOffset, HitTestFloat);

        // Finally test to see if the mouse is in the background (within a child block's background).
        if (!inside)
            inside = nodeAtPoint(request, result, locationInContainer, accumulatedOffset, HitTestChildBlockBackgrounds);
    }

    // See if the mouse is inside us but not any of our descendants
    if (hitTestFilter != HitTestDescendants && !inside)
        inside = nodeAtPoint(request, result, locationInContainer, accumulatedOffset, HitTestBlockBackground);

    return inside;
}

void RenderObject::updateHitTestResult(HitTestResult& result, const LayoutPoint& point)
{
    if (result.innerNode())
        return;

    Node* node = this->node();

    // If we hit the anonymous renderers inside generated content we should
    // actually hit the generated content so walk up to the PseudoElement.
    if (!node && parent() && parent()->isBeforeOrAfterContent()) {
        for (RenderObject* renderer = parent(); renderer && !node; renderer = renderer->parent())
            node = renderer->node();
    }

    if (node) {
        result.setInnerNode(node);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(node);
        result.setLocalPoint(point);
    }
}

bool RenderObject::nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& /*locationInContainer*/, const LayoutPoint& /*accumulatedOffset*/, HitTestAction)
{
    return false;
}

void RenderObject::scheduleRelayout()
{
    if (isRenderView()) {
        FrameView* view = toRenderView(this)->frameView();
        if (view)
            view->scheduleRelayout();
    } else {
        if (isRooted()) {
            if (RenderView* renderView = view()) {
                if (FrameView* frameView = renderView->frameView())
                    frameView->scheduleRelayoutOfSubtree(this);
            }
        }
    }
}

void RenderObject::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());
    RenderObject* child = firstChild();
    while (child) {
        child->layoutIfNeeded();
        ASSERT(!child->needsLayout());
        child = child->nextSibling();
    }
    setNeedsLayout(false);
}

enum StyleCacheState {
    Cached,
    Uncached
};

static PassRefPtr<RenderStyle> firstLineStyleForCachedUncachedType(StyleCacheState type, const RenderObject* renderer, RenderStyle* style)
{
    const RenderObject* rendererForFirstLineStyle = renderer;
    if (renderer->isBeforeOrAfterContent())
        rendererForFirstLineStyle = renderer->parent();

    if (rendererForFirstLineStyle->isBlockFlow()) {
        if (RenderBlock* firstLineBlock = rendererForFirstLineStyle->firstLineBlock()) {
            if (type == Cached)
                return firstLineBlock->getCachedPseudoStyle(FIRST_LINE, style);
            return firstLineBlock->getUncachedPseudoStyle(PseudoStyleRequest(FIRST_LINE), style, firstLineBlock == renderer ? style : 0);
        }
    } else if (!rendererForFirstLineStyle->isAnonymous() && rendererForFirstLineStyle->isRenderInline()) {
        RenderStyle* parentStyle = rendererForFirstLineStyle->parent()->firstLineStyle();
        if (parentStyle != rendererForFirstLineStyle->parent()->style()) {
            if (type == Cached) {
                // A first-line style is in effect. Cache a first-line style for ourselves.
                rendererForFirstLineStyle->style()->setHasPseudoStyle(FIRST_LINE_INHERITED);
                return rendererForFirstLineStyle->getCachedPseudoStyle(FIRST_LINE_INHERITED, parentStyle);
            }
            return rendererForFirstLineStyle->getUncachedPseudoStyle(PseudoStyleRequest(FIRST_LINE_INHERITED), parentStyle, style);
        }
    }
    return 0;
}

PassRefPtr<RenderStyle> RenderObject::uncachedFirstLineStyle(RenderStyle* style) const
{
    if (!document()->styleSheetCollection()->usesFirstLineRules())
        return 0;

    ASSERT(!isText());

    return firstLineStyleForCachedUncachedType(Uncached, this, style);
}

RenderStyle* RenderObject::cachedFirstLineStyle() const
{
    ASSERT(document()->styleSheetCollection()->usesFirstLineRules());

    if (RefPtr<RenderStyle> style = firstLineStyleForCachedUncachedType(Cached, isText() ? parent() : this, m_style.get()))
        return style.get();

    return m_style.get();
}

RenderStyle* RenderObject::getCachedPseudoStyle(PseudoId pseudo, RenderStyle* parentStyle) const
{
    if (pseudo < FIRST_INTERNAL_PSEUDOID && !style()->hasPseudoStyle(pseudo))
        return 0;

    RenderStyle* cachedStyle = style()->getCachedPseudoStyle(pseudo);
    if (cachedStyle)
        return cachedStyle;
    
    RefPtr<RenderStyle> result = getUncachedPseudoStyle(PseudoStyleRequest(pseudo), parentStyle);
    if (result)
        return style()->addCachedPseudoStyle(result.release());
    return 0;
}

PassRefPtr<RenderStyle> RenderObject::getUncachedPseudoStyle(const PseudoStyleRequest& pseudoStyleRequest, RenderStyle* parentStyle, RenderStyle* ownStyle) const
{
    if (pseudoStyleRequest.pseudoId < FIRST_INTERNAL_PSEUDOID && !ownStyle && !style()->hasPseudoStyle(pseudoStyleRequest.pseudoId))
        return 0;
    
    if (!parentStyle) {
        ASSERT(!ownStyle);
        parentStyle = style();
    }

    // FIXME: This "find nearest element parent" should be a helper function.
    Node* n = node();
    while (n && !n->isElementNode())
        n = n->parentNode();
    if (!n)
        return 0;
    Element* element = toElement(n);

    if (pseudoStyleRequest.pseudoId == FIRST_LINE_INHERITED) {
        RefPtr<RenderStyle> result = document()->ensureStyleResolver()->styleForElement(element, parentStyle, DisallowStyleSharing);
        result->setStyleType(FIRST_LINE_INHERITED);
        return result.release();
    }

    return document()->ensureStyleResolver()->pseudoStyleForElement(element, pseudoStyleRequest, parentStyle);
}

static Color decorationColor(RenderStyle* style)
{
    Color result;
#if ENABLE(CSS3_TEXT)
    // Check for text decoration color first.
    result = style->visitedDependentColor(CSSPropertyWebkitTextDecorationColor);
    if (result.isValid())
        return result;
#endif // CSS3_TEXT
    if (style->textStrokeWidth() > 0) {
        // Prefer stroke color if possible but not if it's fully transparent.
        result = style->visitedDependentColor(CSSPropertyWebkitTextStrokeColor);
        if (result.alpha())
            return result;
    }
    
    result = style->visitedDependentColor(CSSPropertyWebkitTextFillColor);
    return result;
}

void RenderObject::getTextDecorationColors(int decorations, Color& underline, Color& overline,
                                           Color& linethrough, bool quirksMode, bool firstlineStyle)
{
    RenderObject* curr = this;
    RenderStyle* styleToUse = 0;
    TextDecoration currDecs = TextDecorationNone;
    Color resultColor;
    do {
        styleToUse = curr->style(firstlineStyle);
        currDecs = styleToUse->textDecoration();
        resultColor = decorationColor(styleToUse);
        // Parameter 'decorations' is cast as an int to enable the bitwise operations below.
        if (currDecs) {
            if (currDecs & TextDecorationUnderline) {
                decorations &= ~TextDecorationUnderline;
                underline = resultColor;
            }
            if (currDecs & TextDecorationOverline) {
                decorations &= ~TextDecorationOverline;
                overline = resultColor;
            }
            if (currDecs & TextDecorationLineThrough) {
                decorations &= ~TextDecorationLineThrough;
                linethrough = resultColor;
            }
        }
        if (curr->isRubyText())
            return;
        curr = curr->parent();
        if (curr && curr->isAnonymousBlock() && toRenderBlock(curr)->continuation())
            curr = toRenderBlock(curr)->continuation();
    } while (curr && decorations && (!quirksMode || !curr->node() || (!isHTMLAnchorElement(curr->node()) && !curr->node()->hasTagName(fontTag))));

    // If we bailed out, use the element we bailed out at (typically a <font> or <a> element).
    if (decorations && curr) {
        styleToUse = curr->style(firstlineStyle);
        resultColor = decorationColor(styleToUse);
        if (decorations & TextDecorationUnderline)
            underline = resultColor;
        if (decorations & TextDecorationOverline)
            overline = resultColor;
        if (decorations & TextDecorationLineThrough)
            linethrough = resultColor;
    }
}

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
void RenderObject::addAnnotatedRegions(Vector<AnnotatedRegionValue>& regions)
{
    // Convert the style regions to absolute coordinates.
    if (style()->visibility() != VISIBLE || !isBox())
        return;
    
    RenderBox* box = toRenderBox(this);
    FloatPoint absPos = localToAbsolute();

#if ENABLE(DASHBOARD_SUPPORT)
    const Vector<StyleDashboardRegion>& styleRegions = style()->dashboardRegions();
    unsigned i, count = styleRegions.size();
    for (i = 0; i < count; i++) {
        StyleDashboardRegion styleRegion = styleRegions[i];

        LayoutUnit w = box->width();
        LayoutUnit h = box->height();

        AnnotatedRegionValue region;
        region.label = styleRegion.label;
        region.bounds = LayoutRect(styleRegion.offset.left().value(),
                                   styleRegion.offset.top().value(),
                                   w - styleRegion.offset.left().value() - styleRegion.offset.right().value(),
                                   h - styleRegion.offset.top().value() - styleRegion.offset.bottom().value());
        region.type = styleRegion.type;

        region.clip = region.bounds;
        computeAbsoluteRepaintRect(region.clip);
        if (region.clip.height() < 0) {
            region.clip.setHeight(0);
            region.clip.setWidth(0);
        }

        region.bounds.setX(absPos.x() + styleRegion.offset.left().value());
        region.bounds.setY(absPos.y() + styleRegion.offset.top().value());

        regions.append(region);
    }
#else // ENABLE(DRAGGABLE_REGION)
    if (style()->getDraggableRegionMode() == DraggableRegionNone)
        return;
    AnnotatedRegionValue region;
    region.draggable = style()->getDraggableRegionMode() == DraggableRegionDrag;
    region.bounds = LayoutRect(absPos.x(), absPos.y(), box->width(), box->height());
    regions.append(region);
#endif
}

void RenderObject::collectAnnotatedRegions(Vector<AnnotatedRegionValue>& regions)
{
    // RenderTexts don't have their own style, they just use their parent's style,
    // so we don't want to include them.
    if (isText())
        return;

    addAnnotatedRegions(regions);
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->collectAnnotatedRegions(regions);
}
#endif

bool RenderObject::willRenderImage(CachedImage*)
{
    // Without visibility we won't render (and therefore don't care about animation).
    if (style()->visibility() != VISIBLE)
        return false;

    // We will not render a new image when Active DOM is suspended
    if (document()->activeDOMObjectsAreSuspended())
        return false;

    // If we're not in a window (i.e., we're dormant from being put in the b/f cache or in a background tab)
    // then we don't want to render either.
    return !document()->inPageCache() && !document()->view()->isOffscreen();
}

int RenderObject::maximalOutlineSize(PaintPhase p) const
{
    if (p != PaintPhaseOutline && p != PaintPhaseSelfOutline && p != PaintPhaseChildOutlines)
        return 0;
    return view()->maximalOutlineSize();
}

int RenderObject::caretMinOffset() const
{
    return 0;
}

int RenderObject::caretMaxOffset() const
{
    if (isReplaced())
        return node() ? max(1U, node()->childNodeCount()) : 1;
    if (isHR())
        return 1;
    return 0;
}

int RenderObject::previousOffset(int current) const
{
    return current - 1;
}

int RenderObject::previousOffsetForBackwardDeletion(int current) const
{
    return current - 1;
}

int RenderObject::nextOffset(int current) const
{
    return current + 1;
}

void RenderObject::adjustRectForOutlineAndShadow(LayoutRect& rect) const
{
    int outlineSize = outlineStyleForRepaint()->outlineSize();
    if (const ShadowData* boxShadow = style()->boxShadow()) {
        boxShadow->adjustRectForShadow(rect, outlineSize);
        return;
    }

    rect.inflate(outlineSize);
}

AnimationController* RenderObject::animation() const
{
    return frame()->animation();
}

void RenderObject::imageChanged(CachedImage* image, const IntRect* rect)
{
    imageChanged(static_cast<WrappedImagePtr>(image), rect);
}
    
RenderObject* RenderObject::hoverAncestor() const
{
    // When searching for the hover ancestor and encountering a named flow thread,
    // the search will continue with the DOM ancestor of the top-most element
    // in the named flow thread.
    // See https://bugs.webkit.org/show_bug.cgi?id=111749
    RenderObject* hoverAncestor = parent();
    
    // Skip anonymous blocks directly flowed into flow threads as it would
    // prevent us from continuing the search on the DOM tree when reaching the named flow thread.
    if (hoverAncestor && hoverAncestor->isAnonymousBlock() && hoverAncestor->parent() && hoverAncestor->parent()->isRenderNamedFlowThread())
        hoverAncestor = hoverAncestor->parent();

    if (hoverAncestor && hoverAncestor->isRenderNamedFlowThread()) {
        hoverAncestor = 0;
        
        Node* node = this->node();
        if (node) {
            Node* domAncestorNode = node->parentNode();
            if (domAncestorNode)
                hoverAncestor = domAncestorNode->renderer();
        }
    }
    
    return hoverAncestor;
}

RenderBoxModelObject* RenderObject::offsetParent() const
{
    // If any of the following holds true return null and stop this algorithm:
    // A is the root element.
    // A is the HTML body element.
    // The computed value of the position property for element A is fixed.
    if (isRoot() || isBody() || (isOutOfFlowPositioned() && style()->position() == FixedPosition))
        return 0;

    // If A is an area HTML element which has a map HTML element somewhere in the ancestor
    // chain return the nearest ancestor map HTML element and stop this algorithm.
    // FIXME: Implement!
    
    // Return the nearest ancestor element of A for which at least one of the following is
    // true and stop this algorithm if such an ancestor is found:
    //     * The computed value of the position property is not static.
    //     * It is the HTML body element.
    //     * The computed value of the position property of A is static and the ancestor
    //       is one of the following HTML elements: td, th, or table.
    //     * Our own extension: if there is a difference in the effective zoom

    bool skipTables = isPositioned();
    float currZoom = style()->effectiveZoom();
    RenderObject* curr = parent();
    while (curr && (!curr->node() || (!curr->isPositioned() && !curr->isBody())) && !curr->isRenderNamedFlowThread()) {
        Node* element = curr->node();
        if (!skipTables && element && (isHTMLTableElement(element) || element->hasTagName(tdTag) || element->hasTagName(thTag)))
            break;
 
        float newZoom = curr->style()->effectiveZoom();
        if (currZoom != newZoom)
            break;
        currZoom = newZoom;
        curr = curr->parent();
    }
    
    // CSS regions specification says that region flows should return the body element as their offsetParent.
    if (curr && curr->isRenderNamedFlowThread())
        curr = document()->body() ? document()->body()->renderer() : 0;
    
    return curr && curr->isBoxModelObject() ? toRenderBoxModelObject(curr) : 0;
}

VisiblePosition RenderObject::createVisiblePosition(int offset, EAffinity affinity)
{
    // If this is a non-anonymous renderer in an editable area, then it's simple.
    if (Node* node = nonPseudoNode()) {
        if (!node->rendererIsEditable()) {
            // If it can be found, we prefer a visually equivalent position that is editable. 
            Position position = createLegacyEditingPosition(node, offset);
            Position candidate = position.downstream(CanCrossEditingBoundary);
            if (candidate.deprecatedNode()->rendererIsEditable())
                return VisiblePosition(candidate, affinity);
            candidate = position.upstream(CanCrossEditingBoundary);
            if (candidate.deprecatedNode()->rendererIsEditable())
                return VisiblePosition(candidate, affinity);
        }
        // FIXME: Eliminate legacy editing positions
        return VisiblePosition(createLegacyEditingPosition(node, offset), affinity);
    }

    // We don't want to cross the boundary between editable and non-editable
    // regions of the document, but that is either impossible or at least
    // extremely unlikely in any normal case because we stop as soon as we
    // find a single non-anonymous renderer.

    // Find a nearby non-anonymous renderer.
    RenderObject* child = this;
    while (RenderObject* parent = child->parent()) {
        // Find non-anonymous content after.
        RenderObject* renderer = child;
        while ((renderer = renderer->nextInPreOrder(parent))) {
            if (Node* node = renderer->nonPseudoNode())
                return VisiblePosition(firstPositionInOrBeforeNode(node), DOWNSTREAM);
        }

        // Find non-anonymous content before.
        renderer = child;
        while ((renderer = renderer->previousInPreOrder())) {
            if (renderer == parent)
                break;
            if (Node* node = renderer->nonPseudoNode())
                return VisiblePosition(lastPositionInOrAfterNode(node), DOWNSTREAM);
        }

        // Use the parent itself unless it too is anonymous.
        if (Node* node = parent->nonPseudoNode())
            return VisiblePosition(firstPositionInOrBeforeNode(node), DOWNSTREAM);

        // Repeat at the next level up.
        child = parent;
    }

    // Everything was anonymous. Give up.
    return VisiblePosition();
}

VisiblePosition RenderObject::createVisiblePosition(const Position& position)
{
    if (position.isNotNull())
        return VisiblePosition(position);

    ASSERT(!node());
    return createVisiblePosition(0, DOWNSTREAM);
}

CursorDirective RenderObject::getCursor(const LayoutPoint&, Cursor&) const
{
    return SetCursorBasedOnStyle;
}

bool RenderObject::canUpdateSelectionOnRootLineBoxes()
{
    if (needsLayout())
        return false;

    RenderBlock* containingBlock = this->containingBlock();
    return containingBlock ? !containingBlock->needsLayout() : true;
}

// We only create "generated" child renderers like one for first-letter if:
// - the firstLetterBlock can have children in the DOM and
// - the block doesn't have any special assumption on its text children.
// This correctly prevents form controls from having such renderers.
bool RenderObject::canHaveGeneratedChildren() const
{
    return canHaveChildren();
}

bool RenderObject::canBeReplacedWithInlineRunIn() const
{
    return true;
}

#if ENABLE(SVG)

RenderSVGResourceContainer* RenderObject::toRenderSVGResourceContainer()
{
    ASSERT_NOT_REACHED();
    return 0;
}

void RenderObject::setNeedsBoundariesUpdate()
{
    if (RenderObject* renderer = parent())
        renderer->setNeedsBoundariesUpdate();
}

FloatRect RenderObject::objectBoundingBox() const
{
    ASSERT_NOT_REACHED();
    return FloatRect();
}

FloatRect RenderObject::strokeBoundingBox() const
{
    ASSERT_NOT_REACHED();
    return FloatRect();
}

// Returns the smallest rectangle enclosing all of the painted content
// respecting clipping, masking, filters, opacity, stroke-width and markers
FloatRect RenderObject::repaintRectInLocalCoordinates() const
{
    ASSERT_NOT_REACHED();
    return FloatRect();
}

AffineTransform RenderObject::localTransform() const
{
    static const AffineTransform identity;
    return identity;
}

const AffineTransform& RenderObject::localToParentTransform() const
{
    static const AffineTransform identity;
    return identity;
}

bool RenderObject::nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint&, HitTestAction)
{
    ASSERT_NOT_REACHED();
    return false;
}

#endif // ENABLE(SVG)

} // namespace WebCore

#ifndef NDEBUG

void showTree(const WebCore::RenderObject* object)
{
    if (object)
        object->showTreeForThis();
}

void showLineTree(const WebCore::RenderObject* object)
{
    if (object)
        object->showLineTreeForThis();
}

void showRenderTree(const WebCore::RenderObject* object1)
{
    showRenderTree(object1, 0);
}

void showRenderTree(const WebCore::RenderObject* object1, const WebCore::RenderObject* object2)
{
    if (object1) {
        const WebCore::RenderObject* root = object1;
        while (root->parent())
            root = root->parent();
        root->showRenderTreeAndMark(object1, "*", object2, "-", 0);
    }
}

#endif
