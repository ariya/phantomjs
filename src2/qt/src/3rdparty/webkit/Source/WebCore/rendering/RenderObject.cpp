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
#include "CSSStyleSelector.h"
#include "Chrome.h"
#include "ContentData.h"
#include "CursorList.h"
#include "DashArray.h"
#include "EditingBoundary.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "Page.h"
#include "RenderArena.h"
#include "RenderCounter.h"
#include "RenderFlexibleBox.h"
#include "RenderImage.h"
#include "RenderImageResourceStyleImage.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListItem.h"
#include "RenderRuby.h"
#include "RenderRubyText.h"
#include "RenderTableCell.h"
#include "RenderTableCol.h"
#include "RenderTableRow.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "TransformState.h"
#include "htmlediting.h"
#include <algorithm>
#include <stdio.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/UnusedParam.h>

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
#endif

bool RenderObject::s_affectsParentBlock = false;

void* RenderObject::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void RenderObject::operator delete(void* ptr, size_t sz)
{
    ASSERT(baseOfRenderObjectBeingDeleted == ptr);

    // Stash size where destroy can find it.
    *(size_t *)ptr = sz;
}

RenderObject* RenderObject::createObject(Node* node, RenderStyle* style)
{
    Document* doc = node->document();
    RenderArena* arena = doc->renderArena();

    // Minimal support for content properties replacing an entire element.
    // Works only if we have exactly one piece of content and it's a URL.
    // Otherwise acts as if we didn't support this feature.
    const ContentData* contentData = style->contentData();
    if (contentData && !contentData->next() && contentData->isImage() && doc != node) {
        RenderImage* image = new (arena) RenderImage(node);
        image->setStyle(style);
        if (StyleImage* styleImage = contentData->image())
            image->setImageResource(RenderImageResourceStyleImage::create(styleImage));
        else
            image->setImageResource(RenderImageResource::create());
        return image;
    }

    if (node->hasTagName(rubyTag)) {
        if (style->display() == INLINE)
            return new (arena) RenderRubyAsInline(node);
        else if (style->display() == BLOCK)
            return new (arena) RenderRubyAsBlock(node);
    }
    // treat <rt> as ruby text ONLY if it still has its default treatment of block
    if (node->hasTagName(rtTag) && style->display() == BLOCK)
        return new (arena) RenderRubyText(node);

    switch (style->display()) {
        case NONE:
            return 0;
        case INLINE:
            return new (arena) RenderInline(node);
        case BLOCK:
        case INLINE_BLOCK:
        case RUN_IN:
        case COMPACT:
            return new (arena) RenderBlock(node);
        case LIST_ITEM:
            return new (arena) RenderListItem(node);
        case TABLE:
        case INLINE_TABLE:
            return new (arena) RenderTable(node);
        case TABLE_ROW_GROUP:
        case TABLE_HEADER_GROUP:
        case TABLE_FOOTER_GROUP:
            return new (arena) RenderTableSection(node);
        case TABLE_ROW:
            return new (arena) RenderTableRow(node);
        case TABLE_COLUMN_GROUP:
        case TABLE_COLUMN:
            return new (arena) RenderTableCol(node);
        case TABLE_CELL:
            return new (arena) RenderTableCell(node);
        case TABLE_CAPTION:
#if ENABLE(WCSS)
        // As per the section 17.1 of the spec WAP-239-WCSS-20011026-a.pdf, 
        // the marquee box inherits and extends the characteristics of the 
        // principal block box ([CSS2] section 9.2.1).
        case WAP_MARQUEE:
#endif
            return new (arena) RenderBlock(node);
        case BOX:
        case INLINE_BOX:
            return new (arena) RenderFlexibleBox(node);
    }

    return 0;
}

#ifndef NDEBUG 
static WTF::RefCountedLeakCounter renderObjectCounter("RenderObject");
#endif

RenderObject::RenderObject(Node* node)
    : CachedResourceClient()
    , m_style(0)
    , m_node(node)
    , m_parent(0)
    , m_previous(0)
    , m_next(0)
#ifndef NDEBUG
    , m_hasAXObject(false)
    , m_setNeedsLayoutForbidden(false)
#endif
    , m_needsLayout(false)
    , m_needsPositionedMovementLayout(false)
    , m_normalChildNeedsLayout(false)
    , m_posChildNeedsLayout(false)
    , m_needsSimplifiedNormalFlowLayout(false)
    , m_preferredLogicalWidthsDirty(false)
    , m_floating(false)
    , m_positioned(false)
    , m_relPositioned(false)
    , m_paintBackground(false)
    , m_isAnonymous(node == node->document())
    , m_isText(false)
    , m_isBox(false)
    , m_inline(true)
    , m_replaced(false)
    , m_horizontalWritingMode(true)
    , m_isDragging(false)
    , m_hasLayer(false)
    , m_hasOverflowClip(false)
    , m_hasTransform(false)
    , m_hasReflection(false)
    , m_hasOverrideSize(false)
    , m_hasCounterNodeMap(false)
    , m_everHadLayout(false)
    , m_childrenInline(false)
    , m_marginBeforeQuirk(false) 
    , m_marginAfterQuirk(false)
    , m_hasMarkupTruncation(false)
    , m_selectionState(SelectionNone)
    , m_hasColumns(false)
{
#ifndef NDEBUG
    renderObjectCounter.increment();
#endif
    ASSERT(node);
}

RenderObject::~RenderObject()
{
    ASSERT(!node() || documentBeingDestroyed() || !frame()->view() || frame()->view()->layoutRoot() != this);
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

void RenderObject::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    RenderObjectChildList* children = virtualChildren();
    ASSERT(children);
    if (!children)
        return;

    bool needsTable = false;

    if (newChild->isTableCol() && newChild->style()->display() == TABLE_COLUMN_GROUP)
        needsTable = !isTable();
    else if (newChild->isRenderBlock() && newChild->style()->display() == TABLE_CAPTION)
        needsTable = !isTable();
    else if (newChild->isTableSection())
        needsTable = !isTable();
    else if (newChild->isTableRow())
        needsTable = !isTableSection();
    else if (newChild->isTableCell()) {
        needsTable = !isTableRow();
        // I'm not 100% sure this is the best way to fix this, but without this
        // change we recurse infinitely when trying to render the CSS2 test page:
        // http://www.bath.ac.uk/%7Epy8ieh/internet/eviltests/htmlbodyheadrendering2.html.
        // See Radar 2925291.
        if (needsTable && isTableCell() && !children->firstChild() && !newChild->isTableCell())
            needsTable = false;
    }

    if (needsTable) {
        RenderTable* table;
        RenderObject* afterChild = beforeChild ? beforeChild->previousSibling() : children->lastChild();
        if (afterChild && afterChild->isAnonymous() && afterChild->isTable())
            table = toRenderTable(afterChild);
        else {
            table = new (renderArena()) RenderTable(document() /* is anonymous */);
            RefPtr<RenderStyle> newStyle = RenderStyle::create();
            newStyle->inheritFrom(style());
            newStyle->setDisplay(TABLE);
            table->setStyle(newStyle.release());
            addChild(table, beforeChild);
        }
        table->addChild(newChild);
    } else {
        // Just add it...
        children->insertChildNode(this, newChild, beforeChild);
    }
    if (newChild->isText() && newChild->style()->textTransform() == CAPITALIZE) {
        RefPtr<StringImpl> textToTransform = toRenderText(newChild)->originalText();
        if (textToTransform)
            toRenderText(newChild)->setText(textToTransform.release(), true);
    }
}

void RenderObject::removeChild(RenderObject* oldChild)
{
    RenderObjectChildList* children = virtualChildren();
    ASSERT(children);
    if (!children)
        return;

    // We do this here instead of in removeChildNode, since the only extremely low-level uses of remove/appendChildNode
    // cannot affect the positioned object list, and the floating object list is irrelevant (since the list gets cleared on
    // layout anyway).
    if (oldChild->isFloatingOrPositioned())
        toRenderBox(oldChild)->removeFloatingOrPositionedChildFromBlockLists();
        
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

RenderObject* RenderObject::nextInPreOrder(RenderObject* stayWithin) const
{
    if (RenderObject* o = firstChild())
        return o;

    return nextInPreOrderAfterChildren(stayWithin);
}

RenderObject* RenderObject::nextInPreOrderAfterChildren(RenderObject* stayWithin) const
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
            // We need to figure out the layer that follows newObject.  We only do
            // this the first time we find a child layer, and then we update the
            // pointer values for newObject and beforeChild used by everyone else.
            beforeChild = newObject->parent()->findNextLayer(parentLayer, newObject);
            newObject = 0;
        }
        parentLayer->addChild(toRenderBoxModelObject(obj)->layer(), beforeChild);
        return;
    }

    for (RenderObject* curr = obj->firstChild(); curr; curr = curr->nextSibling())
        addLayers(curr, parentLayer, newObject, beforeChild);
}

void RenderObject::addLayers(RenderLayer* parentLayer, RenderObject* newObject)
{
    if (!parentLayer)
        return;

    RenderObject* object = newObject;
    RenderLayer* beforeChild = 0;
    WebCore::addLayers(this, parentLayer, object, beforeChild);
}

void RenderObject::removeLayers(RenderLayer* parentLayer)
{
    if (!parentLayer)
        return;

    if (hasLayer()) {
        parentLayer->removeChild(toRenderBoxModelObject(this)->layer());
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
        RenderLayer* layer = toRenderBoxModelObject(this)->layer();
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
    // Error check the parent layer passed in.  If it's null, we can't find anything.
    if (!parentLayer)
        return 0;

    // Step 1: If our layer is a child of the desired parent, then return our layer.
    RenderLayer* ourLayer = hasLayer() ? toRenderBoxModelObject(this)->layer() : 0;
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

    // Step 3: If our layer is the desired parent layer, then we're finished.  We didn't
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
        RenderLayer* layer = curr->hasLayer() ? toRenderBoxModelObject(curr)->layer() : 0;
        if (layer)
            return layer;
        curr = curr->parent();
    }
    return 0;
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

RenderBlock* RenderObject::firstLineBlock() const
{
    return 0;
}

void RenderObject::setPreferredLogicalWidthsDirty(bool b, bool markParents)
{
    bool alreadyDirty = m_preferredLogicalWidthsDirty;
    m_preferredLogicalWidthsDirty = b;
    if (b && !alreadyDirty && markParents && (isText() || (style()->position() != FixedPosition && style()->position() != AbsolutePosition)))
        invalidateContainerPreferredLogicalWidths();
}

void RenderObject::invalidateContainerPreferredLogicalWidths()
{
    // In order to avoid pathological behavior when inlines are deeply nested, we do include them
    // in the chain that we mark dirty (even though they're kind of irrelevant).
    RenderObject* o = isTableCell() ? containingBlock() : container();
    while (o && !o->m_preferredLogicalWidthsDirty) {
        // Don't invalidate the outermost object of an unrooted subtree. That object will be 
        // invalidated when the subtree is added to the document.
        RenderObject* container = o->isTableCell() ? o->containingBlock() : o->container();
        if (!container && !o->isRenderView())
            break;

        o->m_preferredLogicalWidthsDirty = true;
        if (o->style()->position() == FixedPosition || o->style()->position() == AbsolutePosition)
            // A positioned object has no effect on the min/max width of its containing block ever.
            // We can optimize this case and not go up any further.
            break;
        o = container;
    }
}

void RenderObject::setLayerNeedsFullRepaint()
{
    ASSERT(hasLayer());
    toRenderBoxModelObject(this)->layer()->setNeedsFullRepaint(true);
}

RenderBlock* RenderObject::containingBlock() const
{
    if (isTableCell()) {
        const RenderTableCell* cell = toRenderTableCell(this);
        if (parent() && cell->section())
            return cell->table();
        return 0;
    }

    if (isRenderView())
        return const_cast<RenderView*>(toRenderView(this));

    RenderObject* o = parent();
    if (!isText() && m_style->position() == FixedPosition) {
        while (o && !o->isRenderView() && !(o->hasTransform() && o->isRenderBlock()))
            o = o->parent();
    } else if (!isText() && m_style->position() == AbsolutePosition) {
        while (o && (o->style()->position() == StaticPosition || (o->isInline() && !o->isReplaced())) && !o->isRenderView() && !(o->hasTransform() && o->isRenderBlock())) {
            // For relpositioned inlines, we return the nearest enclosing block.  We don't try
            // to return the inline itself.  This allows us to avoid having a positioned objects
            // list in all RenderInlines and lets us return a strongly-typed RenderBlock* result
            // from this method.  The container() method can actually be used to obtain the
            // inline directly.
            if (o->style()->position() == RelativePosition && o->isInline() && !o->isReplaced())
                return o->containingBlock();
#if ENABLE(SVG)
            if (o->isSVGForeignObject()) //foreignObject is the containing block for contents inside it
                break;
#endif

            o = o->parent();
        }
    } else {
        while (o && ((o->isInline() && !o->isReplaced()) || o->isTableRow() || o->isTableSection()
                     || o->isTableCol() || o->isFrameSet() || o->isMedia()
#if ENABLE(SVG)
                     || o->isSVGContainer() || o->isSVGRoot()
#endif
                     ))
            o = o->parent();
    }

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
    if (!img || !img->canRender(renderer->style()->effectiveZoom()))
        return false;

    if (!layer->xPosition().isZero() || !layer->yPosition().isZero())
        return true;

    if (layer->size().type == SizeLength) {
        if (layer->size().size.width().isPercent() || layer->size().size.height().isPercent())
            return true;
    } else if (layer->size().type == Contain || layer->size().type == Cover || img->usesImageContainerSize())
        return true;

    return false;
}

bool RenderObject::borderImageIsLoadedAndCanBeRendered() const
{
    ASSERT(style()->hasBorder());

    StyleImage* borderImage = style()->borderImage().image();
    return borderImage && borderImage->canRender(style()->effectiveZoom()) && borderImage->isLoaded();
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
    int width = (side == BSTop || side == BSBottom ? y2 - y1 : x2 - x1);

    if (style == DOUBLE && width < 3)
        style = SOLID;

    switch (style) {
        case BNONE:
        case BHIDDEN:
            return;
        case DOTTED:
        case DASHED:
            graphicsContext->setStrokeColor(color, m_style->colorSpace());
            graphicsContext->setStrokeThickness(width);
            graphicsContext->setStrokeStyle(style == DASHED ? DashedStroke : DottedStroke);

            if (width > 0) {
                bool wasAntialiased = graphicsContext->shouldAntialias();
                graphicsContext->setShouldAntialias(antialias);

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
            }
            break;
        case DOUBLE: {
            int third = (width + 1) / 3;

            if (adjacentWidth1 == 0 && adjacentWidth2 == 0) {
                graphicsContext->setStrokeStyle(NoStroke);
                graphicsContext->setFillColor(color, m_style->colorSpace());
                
                bool wasAntialiased = graphicsContext->shouldAntialias();
                graphicsContext->setShouldAntialias(antialias);

                switch (side) {
                    case BSTop:
                    case BSBottom:
                        graphicsContext->drawRect(IntRect(x1, y1, x2 - x1, third));
                        graphicsContext->drawRect(IntRect(x1, y2 - third, x2 - x1, third));
                        break;
                    case BSLeft:
                        graphicsContext->drawRect(IntRect(x1, y1 + 1, third, y2 - y1 - 1));
                        graphicsContext->drawRect(IntRect(x2 - third, y1 + 1, third, y2 - y1 - 1));
                        break;
                    case BSRight:
                        graphicsContext->drawRect(IntRect(x1, y1 + 1, third, y2 - y1 - 1));
                        graphicsContext->drawRect(IntRect(x2 - third, y1 + 1, third, y2 - y1 - 1));
                        break;
                }

                graphicsContext->setShouldAntialias(wasAntialiased);
            } else {
                int adjacent1BigThird = ((adjacentWidth1 > 0) ? adjacentWidth1 + 1 : adjacentWidth1 - 1) / 3;
                int adjacent2BigThird = ((adjacentWidth2 > 0) ? adjacentWidth2 + 1 : adjacentWidth2 - 1) / 3;

                switch (side) {
                    case BSTop:
                        drawLineForBoxSide(graphicsContext, x1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
                                   y1, x2 - max((-adjacentWidth2 * 2 + 1) / 3, 0), y1 + third,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   y2 - third, x2 - max((adjacentWidth2 * 2 + 1) / 3, 0), y2,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        break;
                    case BSLeft:
                        drawLineForBoxSide(graphicsContext, x1, y1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
                                   x1 + third, y2 - max((-adjacentWidth2 * 2 + 1) / 3, 0),
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x2 - third, y1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   x2, y2 - max((adjacentWidth2 * 2 + 1) / 3, 0),
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        break;
                    case BSBottom:
                        drawLineForBoxSide(graphicsContext, x1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   y1, x2 - max((adjacentWidth2 * 2 + 1) / 3, 0), y1 + third,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
                                   y2 - third, x2 - max((-adjacentWidth2 * 2 + 1) / 3, 0), y2,
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        break;
                    case BSRight:
                        drawLineForBoxSide(graphicsContext, x1, y1 + max((adjacentWidth1 * 2 + 1) / 3, 0),
                                   x1 + third, y2 - max((adjacentWidth2 * 2 + 1) / 3, 0),
                                   side, color, SOLID, adjacent1BigThird, adjacent2BigThird, antialias);
                        drawLineForBoxSide(graphicsContext, x2 - third, y1 + max((-adjacentWidth1 * 2 + 1) / 3, 0),
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
            break;
        }
    }
}

IntRect RenderObject::borderInnerRect(const IntRect& borderRect, unsigned short topWidth, unsigned short bottomWidth, unsigned short leftWidth, unsigned short rightWidth) const
{
    return IntRect(
            borderRect.x() + leftWidth, 
            borderRect.y() + topWidth, 
            borderRect.width() - leftWidth - rightWidth, 
            borderRect.height() - topWidth - bottomWidth);
}

#if !HAVE(PATH_BASED_BORDER_RADIUS_DRAWING)
void RenderObject::drawArcForBoxSide(GraphicsContext* graphicsContext, int x, int y, float thickness, const IntSize& radius,
                                     int angleStart, int angleSpan, BoxSide s, Color color,
                                     EBorderStyle style, bool firstCorner)
{
    // FIXME: This function should be removed when all ports implement GraphicsContext::clipConvexPolygon()!!
    // At that time, everyone can use RenderObject::drawBoxSideFromPath() instead. This should happen soon.
    if ((style == DOUBLE && thickness / 2 < 3) || ((style == RIDGE || style == GROOVE) && thickness / 2 < 2))
        style = SOLID;

    switch (style) {
        case BNONE:
        case BHIDDEN:
            return;
        case DOTTED:
        case DASHED:
            graphicsContext->setStrokeColor(color, m_style->colorSpace());
            graphicsContext->setStrokeStyle(style == DOTTED ? DottedStroke : DashedStroke);
            graphicsContext->setStrokeThickness(thickness);
            graphicsContext->strokeArc(IntRect(x, y, radius.width() * 2, radius.height() * 2), angleStart, angleSpan);
            break;
        case DOUBLE: {
            float third = thickness / 3.0f;
            float innerThird = (thickness + 1.0f) / 6.0f;
            int shiftForInner = static_cast<int>(innerThird * 2.5f);

            int outerY = y;
            int outerHeight = radius.height() * 2;
            int innerX = x + shiftForInner;
            int innerY = y + shiftForInner;
            int innerWidth = (radius.width() - shiftForInner) * 2;
            int innerHeight = (radius.height() - shiftForInner) * 2;
            if (innerThird > 1 && (s == BSTop || (firstCorner && (s == BSLeft || s == BSRight)))) {
                outerHeight += 2;
                innerHeight += 2;
            }

            graphicsContext->setStrokeStyle(SolidStroke);
            graphicsContext->setStrokeColor(color, m_style->colorSpace());
            graphicsContext->setStrokeThickness(third);
            graphicsContext->strokeArc(IntRect(x, outerY, radius.width() * 2, outerHeight), angleStart, angleSpan);
            graphicsContext->setStrokeThickness(innerThird > 2 ? innerThird - 1 : innerThird);
            graphicsContext->strokeArc(IntRect(innerX, innerY, innerWidth, innerHeight), angleStart, angleSpan);
            break;
        }
        case GROOVE:
        case RIDGE: {
            Color c2;
            if ((style == RIDGE && (s == BSTop || s == BSLeft)) ||
                    (style == GROOVE && (s == BSBottom || s == BSRight)))
                c2 = color.dark();
            else {
                c2 = color;
                color = color.dark();
            }

            graphicsContext->setStrokeStyle(SolidStroke);
            graphicsContext->setStrokeColor(color, m_style->colorSpace());
            graphicsContext->setStrokeThickness(thickness);
            graphicsContext->strokeArc(IntRect(x, y, radius.width() * 2, radius.height() * 2), angleStart, angleSpan);

            float halfThickness = (thickness + 1.0f) / 4.0f;
            int shiftForInner = static_cast<int>(halfThickness * 1.5f);
            graphicsContext->setStrokeColor(c2, m_style->colorSpace());
            graphicsContext->setStrokeThickness(halfThickness > 2 ? halfThickness - 1 : halfThickness);
            graphicsContext->strokeArc(IntRect(x + shiftForInner, y + shiftForInner, (radius.width() - shiftForInner) * 2,
                                       (radius.height() - shiftForInner) * 2), angleStart, angleSpan);
            break;
        }
        case INSET:
            if (s == BSTop || s == BSLeft)
                color = color.dark();
        case OUTSET:
            if (style == OUTSET && (s == BSBottom || s == BSRight))
                color = color.dark();
        case SOLID:
            graphicsContext->setStrokeStyle(SolidStroke);
            graphicsContext->setStrokeColor(color, m_style->colorSpace());
            graphicsContext->setStrokeThickness(thickness);
            graphicsContext->strokeArc(IntRect(x, y, radius.width() * 2, radius.height() * 2), angleStart, angleSpan);
            break;
    }
}
#endif
    
void RenderObject::paintFocusRing(GraphicsContext* context, int tx, int ty, RenderStyle* style)
{
    Vector<IntRect> focusRingRects;
    addFocusRingRects(focusRingRects, tx, ty);
    if (style->outlineStyleIsAuto())
        context->drawFocusRing(focusRingRects, style->outlineWidth(), style->outlineOffset(), style->visitedDependentColor(CSSPropertyOutlineColor));
    else
        addPDFURLRect(context, unionRect(focusRingRects));
}        

void RenderObject::addPDFURLRect(GraphicsContext* context, const IntRect& rect)
{
    if (rect.isEmpty())
        return;
    Node* n = node();
    if (!n || !n->isLink() || !n->isElementNode())
        return;
    const AtomicString& href = static_cast<Element*>(n)->getAttribute(hrefAttr);
    if (href.isNull())
        return;
    context->setURLForRect(n->document()->completeURL(href), rect);
}

void RenderObject::paintOutline(GraphicsContext* graphicsContext, int tx, int ty, int w, int h)
{
    if (!hasOutline())
        return;

    RenderStyle* styleToUse = style();
    int outlineWidth = styleToUse->outlineWidth();
    EBorderStyle outlineStyle = styleToUse->outlineStyle();

    Color outlineColor = styleToUse->visitedDependentColor(CSSPropertyOutlineColor);

    int offset = styleToUse->outlineOffset();

    if (styleToUse->outlineStyleIsAuto() || hasOutlineAnnotation()) {
        if (!theme()->supportsFocusRing(styleToUse)) {
            // Only paint the focus ring by hand if the theme isn't able to draw the focus ring.
            paintFocusRing(graphicsContext, tx, ty, styleToUse);
        }
    }

    if (styleToUse->outlineStyleIsAuto() || styleToUse->outlineStyle() == BNONE)
        return;

    tx -= offset;
    ty -= offset;
    w += 2 * offset;
    h += 2 * offset;

    if (h < 0 || w < 0)
        return;

    int leftOuter = tx - outlineWidth;
    int leftInner = tx;
    int rightOuter = tx + w + outlineWidth;
    int rightInner = tx + w;
    int topOuter = ty - outlineWidth;
    int topInner = ty;
    int bottomOuter = ty + h + outlineWidth;
    int bottomInner = ty + h;
    
    drawLineForBoxSide(graphicsContext, leftOuter, topOuter, leftInner, bottomOuter, BSLeft, outlineColor, outlineStyle, outlineWidth, outlineWidth);
    drawLineForBoxSide(graphicsContext, leftOuter, topOuter, rightOuter, topInner, BSTop, outlineColor, outlineStyle, outlineWidth, outlineWidth);
    drawLineForBoxSide(graphicsContext, rightInner, topOuter, rightOuter, bottomOuter, BSRight, outlineColor, outlineStyle, outlineWidth, outlineWidth);
    drawLineForBoxSide(graphicsContext, leftOuter, bottomInner, rightOuter, bottomOuter, BSBottom, outlineColor, outlineStyle, outlineWidth, outlineWidth);
}

IntRect RenderObject::absoluteBoundingBoxRect(bool useTransforms)
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
    absoluteRects(rects, absPos.x(), absPos.y());

    size_t n = rects.size();
    if (!n)
        return IntRect();

    IntRect result = rects[0];
    for (size_t i = 1; i < n; ++i)
        result.unite(rects[i]);
    return result;
}

void RenderObject::absoluteFocusRingQuads(Vector<FloatQuad>& quads)
{
    Vector<IntRect> rects;
    // FIXME: addFocusRingRects() needs to be passed this transform-unaware
    // localToAbsolute() offset here because RenderInline::addFocusRingRects()
    // implicitly assumes that. This doesn't work correctly with transformed
    // descendants.
    FloatPoint absolutePoint = localToAbsolute();
    addFocusRingRects(rects, absolutePoint.x(), absolutePoint.y());
    size_t count = rects.size();
    for (size_t i = 0; i < count; ++i) {
        IntRect rect = rects[i];
        rect.move(-absolutePoint.x(), -absolutePoint.y());
        quads.append(localToAbsoluteQuad(FloatQuad(rect)));
    }
}

void RenderObject::addAbsoluteRectForLayer(IntRect& result)
{
    if (hasLayer())
        result.unite(absoluteBoundingBoxRect());
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
        current->addAbsoluteRectForLayer(result);
}

IntRect RenderObject::paintingRootRect(IntRect& topLevelRect)
{
    IntRect result = absoluteBoundingBoxRect();
    topLevelRect = result;
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
        current->addAbsoluteRectForLayer(result);
    return result;
}

void RenderObject::paint(PaintInfo& /*paintInfo*/, int /*tx*/, int /*ty*/)
{
}

RenderBoxModelObject* RenderObject::containerForRepaint() const
{
#if USE(ACCELERATED_COMPOSITING)
    if (RenderView* v = view()) {
        if (v->usesCompositing()) {
            RenderLayer* compLayer = enclosingLayer()->enclosingCompositingLayer();
            return compLayer ? compLayer->renderer() : 0;
        }
    }
#endif
    // Do root-relative repaint.
    return 0;
}

void RenderObject::repaintUsingContainer(RenderBoxModelObject* repaintContainer, const IntRect& r, bool immediate)
{
    if (!repaintContainer) {
        view()->repaintViewRectangle(r, immediate);
        return;
    }

#if USE(ACCELERATED_COMPOSITING)
    RenderView* v = view();
    if (repaintContainer->isRenderView()) {
        ASSERT(repaintContainer == v);
        bool viewHasCompositedLayer = v->hasLayer() && v->layer()->isComposited();
        if (!viewHasCompositedLayer || v->layer()->backing()->paintingGoesToWindow()) {
            IntRect repaintRectangle = r;
            if (viewHasCompositedLayer &&  v->layer()->transform())
                repaintRectangle = v->layer()->transform()->mapRect(r);
            v->repaintViewRectangle(repaintRectangle, immediate);
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

void RenderObject::repaint(bool immediate)
{
    // Don't repaint if we're unrooted (note that view() still returns the view when unrooted)
    RenderView* view;
    if (!isRooted(&view))
        return;

    if (view->printing())
        return; // Don't repaint if we're printing.

    RenderBoxModelObject* repaintContainer = containerForRepaint();
    repaintUsingContainer(repaintContainer ? repaintContainer : view, clippedOverflowRectForRepaint(repaintContainer), immediate);
}

void RenderObject::repaintRectangle(const IntRect& r, bool immediate)
{
    // Don't repaint if we're unrooted (note that view() still returns the view when unrooted)
    RenderView* view;
    if (!isRooted(&view))
        return;

    if (view->printing())
        return; // Don't repaint if we're printing.

    IntRect dirtyRect(r);

    // FIXME: layoutDelta needs to be applied in parts before/after transforms and
    // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
    dirtyRect.move(view->layoutDelta());

    RenderBoxModelObject* repaintContainer = containerForRepaint();
    computeRectForRepaint(repaintContainer, dirtyRect);
    repaintUsingContainer(repaintContainer ? repaintContainer : view, dirtyRect, immediate);
}

bool RenderObject::repaintAfterLayoutIfNeeded(RenderBoxModelObject* repaintContainer, const IntRect& oldBounds, const IntRect& oldOutlineBox, const IntRect* newBoundsPtr, const IntRect* newOutlineBoxRectPtr)
{
    RenderView* v = view();
    if (v->printing())
        return false; // Don't repaint if we're printing.

    // This ASSERT fails due to animations.  See https://bugs.webkit.org/show_bug.cgi?id=37048
    // ASSERT(!newBoundsPtr || *newBoundsPtr == clippedOverflowRectForRepaint(repaintContainer));
    IntRect newBounds = newBoundsPtr ? *newBoundsPtr : clippedOverflowRectForRepaint(repaintContainer);
    IntRect newOutlineBox;

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
        repaintUsingContainer(repaintContainer, oldBounds);
        if (newBounds != oldBounds)
            repaintUsingContainer(repaintContainer, newBounds);
        return true;
    }

    if (newBounds == oldBounds && newOutlineBox == oldOutlineBox)
        return false;

    int deltaLeft = newBounds.x() - oldBounds.x();
    if (deltaLeft > 0)
        repaintUsingContainer(repaintContainer, IntRect(oldBounds.x(), oldBounds.y(), deltaLeft, oldBounds.height()));
    else if (deltaLeft < 0)
        repaintUsingContainer(repaintContainer, IntRect(newBounds.x(), newBounds.y(), -deltaLeft, newBounds.height()));

    int deltaRight = newBounds.maxX() - oldBounds.maxX();
    if (deltaRight > 0)
        repaintUsingContainer(repaintContainer, IntRect(oldBounds.maxX(), newBounds.y(), deltaRight, newBounds.height()));
    else if (deltaRight < 0)
        repaintUsingContainer(repaintContainer, IntRect(newBounds.maxX(), oldBounds.y(), -deltaRight, oldBounds.height()));

    int deltaTop = newBounds.y() - oldBounds.y();
    if (deltaTop > 0)
        repaintUsingContainer(repaintContainer, IntRect(oldBounds.x(), oldBounds.y(), oldBounds.width(), deltaTop));
    else if (deltaTop < 0)
        repaintUsingContainer(repaintContainer, IntRect(newBounds.x(), newBounds.y(), newBounds.width(), -deltaTop));

    int deltaBottom = newBounds.maxY() - oldBounds.maxY();
    if (deltaBottom > 0)
        repaintUsingContainer(repaintContainer, IntRect(newBounds.x(), oldBounds.maxY(), newBounds.width(), deltaBottom));
    else if (deltaBottom < 0)
        repaintUsingContainer(repaintContainer, IntRect(oldBounds.x(), newBounds.maxY(), oldBounds.width(), -deltaBottom));

    if (newOutlineBox == oldOutlineBox)
        return false;

    // We didn't move, but we did change size.  Invalidate the delta, which will consist of possibly
    // two rectangles (but typically only one).
    RenderStyle* outlineStyle = outlineStyleForRepaint();
    int ow = outlineStyle->outlineSize();
    int width = abs(newOutlineBox.width() - oldOutlineBox.width());
    if (width) {
        int shadowLeft;
        int shadowRight;
        style()->getBoxShadowHorizontalExtent(shadowLeft, shadowRight);

        int borderRight = isBox() ? toRenderBox(this)->borderRight() : 0;
        int boxWidth = isBox() ? toRenderBox(this)->width() : 0;
        int borderWidth = max(-outlineStyle->outlineOffset(), max(borderRight, max(style()->borderTopRightRadius().width().calcValue(boxWidth), style()->borderBottomRightRadius().width().calcValue(boxWidth)))) + max(ow, shadowRight);
        IntRect rightRect(newOutlineBox.x() + min(newOutlineBox.width(), oldOutlineBox.width()) - borderWidth,
            newOutlineBox.y(),
            width + borderWidth,
            max(newOutlineBox.height(), oldOutlineBox.height()));
        int right = min(newBounds.maxX(), oldBounds.maxX());
        if (rightRect.x() < right) {
            rightRect.setWidth(min(rightRect.width(), right - rightRect.x()));
            repaintUsingContainer(repaintContainer, rightRect);
        }
    }
    int height = abs(newOutlineBox.height() - oldOutlineBox.height());
    if (height) {
        int shadowTop;
        int shadowBottom;
        style()->getBoxShadowVerticalExtent(shadowTop, shadowBottom);

        int borderBottom = isBox() ? toRenderBox(this)->borderBottom() : 0;
        int boxHeight = isBox() ? toRenderBox(this)->height() : 0;
        int borderHeight = max(-outlineStyle->outlineOffset(), max(borderBottom, max(style()->borderBottomLeftRadius().height().calcValue(boxHeight), style()->borderBottomRightRadius().height().calcValue(boxHeight)))) + max(ow, shadowBottom);
        IntRect bottomRect(newOutlineBox.x(),
            min(newOutlineBox.maxY(), oldOutlineBox.maxY()) - borderHeight,
            max(newOutlineBox.width(), oldOutlineBox.width()),
            height + borderHeight);
        int bottom = min(newBounds.maxY(), oldBounds.maxY());
        if (bottomRect.y() < bottom) {
            bottomRect.setHeight(min(bottomRect.height(), bottom - bottomRect.y()));
            repaintUsingContainer(repaintContainer, bottomRect);
        }
    }
    return false;
}

void RenderObject::repaintDuringLayoutIfMoved(const IntRect&)
{
}

void RenderObject::repaintOverhangingFloats(bool)
{
}

bool RenderObject::checkForRepaintDuringLayout() const
{
    // FIXME: <https://bugs.webkit.org/show_bug.cgi?id=20885> It is probably safe to also require
    // m_everHadLayout. Currently, only RenderBlock::layoutBlock() adds this condition. See also
    // <https://bugs.webkit.org/show_bug.cgi?id=15129>.
    return !document()->view()->needsFullRepaint() && !hasLayer();
}

IntRect RenderObject::rectWithOutlineForRepaint(RenderBoxModelObject* repaintContainer, int outlineWidth)
{
    IntRect r(clippedOverflowRectForRepaint(repaintContainer));
    r.inflate(outlineWidth);
    return r;
}

IntRect RenderObject::clippedOverflowRectForRepaint(RenderBoxModelObject*)
{
    ASSERT_NOT_REACHED();
    return IntRect();
}

void RenderObject::computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& rect, bool fixed)
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
            // o->height() is inaccurate if we're in the middle of a layout of |o|, so use the
            // layer's size instead.  Even if the layer's size is wrong, the layer itself will repaint
            // anyway if its size does change.
            RenderBox* boxParent = toRenderBox(o);

            IntRect repaintRect(rect);
            repaintRect.move(-boxParent->layer()->scrolledContentOffset()); // For overflow:auto/scroll/hidden.

            IntRect boxRect(0, 0, boxParent->layer()->width(), boxParent->layer()->height());
            rect = intersection(repaintRect, boxRect);
            if (rect.isEmpty())
                return;
        }

        o->computeRectForRepaint(repaintContainer, rect, fixed);
    }
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
        RefPtr<RenderStyle> pseudoStyle = getUncachedPseudoStyle(SELECTION);
        if (pseudoStyle && pseudoStyle->visitedDependentColor(CSSPropertyBackgroundColor).isValid())
            color = pseudoStyle->visitedDependentColor(CSSPropertyBackgroundColor).blendWithWhite();
        else
            color = frame()->selection()->isFocusedAndActive() ?
                    theme()->activeSelectionBackgroundColor() :
                    theme()->inactiveSelectionBackgroundColor();
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

    if (RefPtr<RenderStyle> pseudoStyle = getUncachedPseudoStyle(SELECTION)) {
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

#if ENABLE(DRAG_SUPPORT)
Node* RenderObject::draggableNode(bool dhtmlOK, bool uaOK, int x, int y, bool& dhtmlWillDrag) const
{
    if (!dhtmlOK && !uaOK)
        return 0;

    for (const RenderObject* curr = this; curr; curr = curr->parent()) {
        Node* elt = curr->node();
        if (elt && elt->nodeType() == Node::TEXT_NODE) {
            // Since there's no way for the author to address the -webkit-user-drag style for a text node,
            // we use our own judgement.
            if (uaOK && view()->frameView()->frame()->eventHandler()->shouldDragAutoNode(curr->node(), IntPoint(x, y))) {
                dhtmlWillDrag = false;
                return curr->node();
            }
            if (elt->canStartSelection())
                // In this case we have a click in the unselected portion of text.  If this text is
                // selectable, we want to start the selection process instead of looking for a parent
                // to try to drag.
                return 0;
        } else {
            EUserDrag dragMode = curr->style()->userDrag();
            if (dhtmlOK && dragMode == DRAG_ELEMENT) {
                dhtmlWillDrag = true;
                return curr->node();
            }
            if (uaOK && dragMode == DRAG_AUTO
                    && view()->frameView()->frame()->eventHandler()->shouldDragAutoNode(curr->node(), IntPoint(x, y))) {
                dhtmlWillDrag = false;
                return curr->node();
            }
        }
    }
    return 0;
}
#endif // ENABLE(DRAG_SUPPORT)

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
        if (!isText() && (!hasLayer() || !toRenderBoxModelObject(this)->layer()->isComposited())) {
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
        if (!isText() && (!hasLayer() || !toRenderBoxModelObject(this)->layer()->isComposited()))
            diff = StyleDifferenceRepaintLayer;
        else if (diff < StyleDifferenceRecompositeLayer)
            diff = StyleDifferenceRecompositeLayer;
    }
    
    // The answer to requiresLayer() for plugins and iframes can change outside of the style system,
    // since it depends on whether we decide to composite these elements. When the layer status of
    // one of these elements changes, we need to force a layout.
    if (diff == StyleDifferenceEqual && style() && isBoxModelObject()) {
        if (hasLayer() != toRenderBoxModelObject(this)->requiresLayer())
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

void RenderObject::setStyle(PassRefPtr<RenderStyle> style)
{
    if (m_style == style) {
#if USE(ACCELERATED_COMPOSITING)
        // We need to run through adjustStyleDifference() for iframes and plugins, so
        // style sharing is disabled for them. That should ensure that we never hit this code path.
        ASSERT(!isRenderIFrame() && !isEmbeddedObject() &&!isApplet());
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
    m_style = style;

    updateFillImages(oldStyle ? oldStyle->backgroundLayers() : 0, m_style ? m_style->backgroundLayers() : 0);
    updateFillImages(oldStyle ? oldStyle->maskLayers() : 0, m_style ? m_style->maskLayers() : 0);

    updateImage(oldStyle ? oldStyle->borderImage().image() : 0, m_style ? m_style->borderImage().image() : 0);
    updateImage(oldStyle ? oldStyle->maskBoxImage().image() : 0, m_style ? m_style->maskBoxImage().image() : 0);

    // We need to ensure that view->maximalOutlineSize() is valid for any repaints that happen
    // during styleDidChange (it's used by clippedOverflowRectForRepaint()).
    if (m_style->outlineWidth() > 0 && m_style->outlineSize() > maximalOutlineSize(PaintPhaseOutline))
        toRenderView(document()->renderer())->setMaximalOutlineSize(m_style->outlineSize());

    styleDidChange(diff, oldStyle.get());

    if (!m_parent || isText())
        return;

    // Now that the layer (if any) has been updated, we need to adjust the diff again,
    // check whether we should layout now, and decide if we need to repaint.
    StyleDifference updatedDiff = adjustStyleDifference(diff, contextSensitiveProperties);
    
    if (diff <= StyleDifferenceLayoutPositionedMovementOnly) {
        if (updatedDiff == StyleDifferenceLayout)
            setNeedsLayoutAndPrefWidthsRecalc();
        else if (updatedDiff == StyleDifferenceLayoutPositionedMovementOnly)
            setNeedsPositionedMovementLayout();
        else if (updatedDiff == StyleDifferenceSimplifiedLayoutAndPositionedMovement) {
            setNeedsPositionedMovementLayout();
            setNeedsSimplifiedNormalFlowLayout();
        } else if (updatedDiff == StyleDifferenceSimplifiedLayout)
            setNeedsSimplifiedNormalFlowLayout();
    }
    
    if (updatedDiff == StyleDifferenceRepaintLayer || updatedDiff == StyleDifferenceRepaint) {
        // Do a repaint with the new style now, e.g., for example if we go from
        // not having an outline to having an outline.
        repaint();
    }
}

void RenderObject::setStyleInternal(PassRefPtr<RenderStyle> style)
{
    m_style = style;
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
#if ENABLE(DASHBOARD_SUPPORT)
            if (visibilityChanged)
                document()->setDashboardRegionsDirty(true);
#endif
            if (visibilityChanged && AXObjectCache::accessibilityEnabled())
                document()->axObjectCache()->childrenChanged(this);

            // Keep layer hierarchy visibility bits up to date if visibility changes.
            if (m_style->visibility() != newStyle->visibility()) {
                if (RenderLayer* l = enclosingLayer()) {
                    if (newStyle->visibility() == VISIBLE)
                        l->setHasVisibleContent(true);
                    else if (l->hasVisibleContent() && (this == l->renderer() || l->renderer()->style()->visibility() != VISIBLE)) {
                        l->dirtyVisibleContentStatus();
                        if (diff > StyleDifferenceRepaintLayer)
                            repaint();
                    }
                }
            }
        }

        if (m_parent && (diff == StyleDifferenceRepaint || newStyle->outlineSize() < m_style->outlineSize()))
            repaint();
        if (isFloating() && (m_style->floating() != newStyle->floating()))
            // For changes in float styles, we need to conceivably remove ourselves
            // from the floating objects list.
            toRenderBox(this)->removeFloatingOrPositionedChildFromBlockLists();
        else if (isPositioned() && (m_style->position() != newStyle->position()))
            // For changes in positioning styles, we need to conceivably remove ourselves
            // from the positioned objects list.
            toRenderBox(this)->removeFloatingOrPositionedChildFromBlockLists();

        s_affectsParentBlock = isFloatingOrPositioned() &&
            (!newStyle->isFloating() && newStyle->position() != AbsolutePosition && newStyle->position() != FixedPosition)
            && parent() && (parent()->isBlockFlow() || parent()->isRenderInline());

        // reset style flags
        if (diff == StyleDifferenceLayout || diff == StyleDifferenceLayoutPositionedMovementOnly) {
            m_floating = false;
            m_positioned = false;
            m_relPositioned = false;
        }
        m_horizontalWritingMode = true;
        m_paintBackground = false;
        m_hasOverflowClip = false;
        m_hasTransform = false;
        m_hasReflection = false;
    } else
        s_affectsParentBlock = false;

    if (view()->frameView()) {
        bool shouldBlitOnFixedBackgroundImage = false;
#if ENABLE(FAST_MOBILE_SCROLLING)
        // On low-powered/mobile devices, preventing blitting on a scroll can cause noticeable delays
        // when scrolling a page with a fixed background image. As an optimization, assuming there are
        // no fixed positoned elements on the page, we can acclerate scrolling (via blitting) if we
        // ignore the CSS property "background-attachment: fixed".
        shouldBlitOnFixedBackgroundImage = true;
#endif

        bool newStyleSlowScroll = newStyle && !shouldBlitOnFixedBackgroundImage && newStyle->hasFixedBackgroundImage();
        bool oldStyleSlowScroll = m_style && !shouldBlitOnFixedBackgroundImage && m_style->hasFixedBackgroundImage();
        if (oldStyleSlowScroll != newStyleSlowScroll) {
            if (oldStyleSlowScroll)
                view()->frameView()->removeSlowRepaintObject();
            if (newStyleSlowScroll)
                view()->frameView()->addSlowRepaintObject();
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

    if (!m_parent)
        return;
    
    if (diff == StyleDifferenceLayout || diff == StyleDifferenceSimplifiedLayout) {
        RenderCounter::rendererStyleChanged(this, oldStyle, m_style.get());

        // If the object already needs layout, then setNeedsLayout won't do
        // any work. But if the containing block has changed, then we may need
        // to mark the new containing blocks for layout. The change that can
        // directly affect the containing block of this object is a change to
        // the position style.
        if (m_needsLayout && oldStyle->position() != m_style->position())
            markContainingBlocksForLayout();

        if (diff == StyleDifferenceLayout)
            setNeedsLayoutAndPrefWidthsRecalc();
        else
            setNeedsSimplifiedNormalFlowLayout();
    } else if (diff == StyleDifferenceSimplifiedLayoutAndPositionedMovement) {
        setNeedsPositionedMovementLayout();
        setNeedsSimplifiedNormalFlowLayout();
    } else if (diff == StyleDifferenceLayoutPositionedMovementOnly)
        setNeedsPositionedMovementLayout();

    // Don't check for repaint here; we need to wait until the layer has been
    // updated by subclasses before we know if we have to repaint (in setStyle()).

    if (oldStyle && !areCursorsEqual(oldStyle, style())) {
        if (Frame* frame = this->frame())
            frame->eventHandler()->dispatchFakeMouseMoveEventSoon();
    }
}

void RenderObject::propagateStyleToAnonymousChildren(bool blockChildrenOnly)
{
    // FIXME: We could save this call when the change only affected non-inherited properties.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (blockChildrenOnly ? child->isAnonymousBlock() : child->isAnonymous() && !child->isBeforeOrAfterContent()) {
            RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyle(style());
            if (style()->specifiesColumns()) {
                if (child->style()->specifiesColumns())
                    newStyle->inheritColumnPropertiesFrom(style());
                if (child->style()->columnSpan())
                    newStyle->setColumnSpan(true);
            }
            newStyle->setDisplay(blockChildrenOnly ? BLOCK : child->style()->display());
            child->setStyle(newStyle.release());
        }
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

IntRect RenderObject::viewRect() const
{
    return view()->viewRect();
}

FloatPoint RenderObject::localToAbsolute(const FloatPoint& localPoint, bool fixed, bool useTransforms) const
{
    TransformState transformState(TransformState::ApplyTransformDirection, localPoint);
    mapLocalToContainer(0, fixed, useTransforms, transformState);
    transformState.flatten();
    
    return transformState.lastPlanarPoint();
}

FloatPoint RenderObject::absoluteToLocal(const FloatPoint& containerPoint, bool fixed, bool useTransforms) const
{
    TransformState transformState(TransformState::UnapplyInverseTransformDirection, containerPoint);
    mapAbsoluteToLocalPoint(fixed, useTransforms, transformState);
    transformState.flatten();
    
    return transformState.lastPlanarPoint();
}

void RenderObject::mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed, bool useTransforms, TransformState& transformState) const
{
    if (repaintContainer == this)
        return;

    RenderObject* o = parent();
    if (!o)
        return;

    IntPoint centerPoint = roundedIntPoint(transformState.mappedPoint());
    if (o->isBox() && o->style()->isFlippedBlocksWritingMode())
        transformState.move(toRenderBox(o)->flipForWritingModeIncludingColumns(roundedIntPoint(transformState.mappedPoint())) - centerPoint);

    IntSize columnOffset;
    o->adjustForColumns(columnOffset, roundedIntPoint(transformState.mappedPoint()));
    if (!columnOffset.isZero())
        transformState.move(columnOffset);

    if (o->hasOverflowClip())
        transformState.move(-toRenderBox(o)->layer()->scrolledContentOffset());

    o->mapLocalToContainer(repaintContainer, fixed, useTransforms, transformState);
}

void RenderObject::mapAbsoluteToLocalPoint(bool fixed, bool useTransforms, TransformState& transformState) const
{
    RenderObject* o = parent();
    if (o) {
        o->mapAbsoluteToLocalPoint(fixed, useTransforms, transformState);
        if (o->hasOverflowClip())
            transformState.move(toRenderBox(o)->layer()->scrolledContentOffset());
    }
}

bool RenderObject::shouldUseTransformFromContainer(const RenderObject* containerObject) const
{
#if ENABLE(3D_RENDERING)
    // hasTransform() indicates whether the object has transform, transform-style or perspective. We just care about transform,
    // so check the layer's transform directly.
    return (hasLayer() && toRenderBoxModelObject(this)->layer()->transform()) || (containerObject && containerObject->style()->hasPerspective());
#else
    UNUSED_PARAM(containerObject);
    return hasTransform();
#endif
}

void RenderObject::getTransformFromContainer(const RenderObject* containerObject, const IntSize& offsetInContainer, TransformationMatrix& transform) const
{
    transform.makeIdentity();
    transform.translate(offsetInContainer.width(), offsetInContainer.height());
    RenderLayer* layer;
    if (hasLayer() && (layer = toRenderBoxModelObject(this)->layer()) && layer->transform())
        transform.multiply(layer->currentTransform());
    
#if ENABLE(3D_RENDERING)
    if (containerObject && containerObject->hasLayer() && containerObject->style()->hasPerspective()) {
        // Perpsective on the container affects us, so we have to factor it in here.
        ASSERT(containerObject->hasLayer());
        FloatPoint perspectiveOrigin = toRenderBoxModelObject(containerObject)->layer()->perspectiveOrigin();

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

FloatQuad RenderObject::localToContainerQuad(const FloatQuad& localQuad, RenderBoxModelObject* repaintContainer, bool fixed) const
{
    // Track the point at the center of the quad's bounding box. As mapLocalToContainer() calls offsetFromContainer(),
    // it will use that point as the reference point to decide which column's transform to apply in multiple-column blocks.
    TransformState transformState(TransformState::ApplyTransformDirection, localQuad.boundingBox().center(), &localQuad);
    mapLocalToContainer(repaintContainer, fixed, true, transformState);
    transformState.flatten();
    
    return transformState.lastPlanarQuad();
}

IntSize RenderObject::offsetFromContainer(RenderObject* o, const IntPoint& point) const
{
    ASSERT(o == container());

    IntSize offset;

    o->adjustForColumns(offset, point);

    if (o->hasOverflowClip())
        offset -= toRenderBox(o)->layer()->scrolledContentOffset();

    return offset;
}

IntSize RenderObject::offsetFromAncestorContainer(RenderObject* container) const
{
    IntSize offset;
    IntPoint referencePoint;
    const RenderObject* currContainer = this;
    do {
        RenderObject* nextContainer = currContainer->container();
        ASSERT(nextContainer);  // This means we reached the top without finding container.
        if (!nextContainer)
            break;
        ASSERT(!currContainer->hasTransform());
        IntSize currentOffset = currContainer->offsetFromContainer(nextContainer, referencePoint);
        offset += currentOffset;
        referencePoint.move(currentOffset);
        currContainer = nextContainer;
    } while (currContainer != container);

    return offset;
}

IntRect RenderObject::localCaretRect(InlineBox*, int, int* extraWidthToEndOfLine)
{
    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = 0;

    return IntRect();
}

RenderView* RenderObject::view() const
{
    return toRenderView(document()->renderer());
}

bool RenderObject::isRooted(RenderView** view)
{
    RenderObject* o = this;
    while (o->parent())
        o = o->parent();

    if (!o->isRenderView())
        return false;

    if (view)
        *view = toRenderView(o);

    return true;
}

bool RenderObject::hasOutlineAnnotation() const
{
    return node() && node()->isLink() && document()->printing();
}

RenderObject* RenderObject::container(RenderBoxModelObject* repaintContainer, bool* repaintContainerSkipped) const
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
            if (repaintContainerSkipped && o == repaintContainer)
                *repaintContainerSkipped = true;
            o = o->parent();
        }
    } else if (pos == AbsolutePosition) {
        // Same goes here.  We technically just want our containing block, but
        // we may not have one if we're part of an uninstalled subtree.  We'll
        // climb as high as we can though.
        while (o && o->style()->position() == StaticPosition && !o->isRenderView() && !(o->hasTransform() && o->isRenderBlock())) {
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

void RenderObject::destroy()
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

    if (AXObjectCache::accessibilityEnabled()) {
        document()->axObjectCache()->childrenChanged(this->parent());
        document()->axObjectCache()->remove(this);
    }
    animation()->cancelAnimations(this);

    // By default no ref-counting. RenderWidget::destroy() doesn't call
    // this function because it needs to do ref-counting. If anything
    // in this function changes, be sure to fix RenderWidget::destroy() as well.

    remove();

    // If this renderer had a parent, remove should have destroyed any counters
    // attached to this renderer and marked the affected other counters for
    // reevaluation. This apparently redundant check is here for the case when
    // this renderer had no parent at the time remove() was called.

    if (m_hasCounterNodeMap)
        RenderCounter::destroyCounterNodes(this);

    // FIXME: Would like to do this in RenderBoxModelObject, but the timing is so complicated that this can't easily
    // be moved into RenderBoxModelObject::destroy.
    if (hasLayer()) {
        setHasLayer(false);
        toRenderBoxModelObject(this)->destroyLayer();
    }
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

VisiblePosition RenderObject::positionForCoordinates(int x, int y)
{
    return positionForPoint(IntPoint(x, y));
}

VisiblePosition RenderObject::positionForPoint(const IntPoint&)
{
    return createVisiblePosition(caretMinOffset(), DOWNSTREAM);
}

void RenderObject::updateDragState(bool dragOn)
{
    bool valueChanged = (dragOn != m_isDragging);
    m_isDragging = dragOn;
    if (valueChanged && style()->affectedByDragRules() && node())
        node()->setNeedsStyleRecalc();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->updateDragState(dragOn);
}

bool RenderObject::hitTest(const HitTestRequest& request, HitTestResult& result, const IntPoint& point, int tx, int ty, HitTestFilter hitTestFilter)
{
    bool inside = false;
    if (hitTestFilter != HitTestSelf) {
        // First test the foreground layer (lines and inlines).
        inside = nodeAtPoint(request, result, point.x(), point.y(), tx, ty, HitTestForeground);

        // Test floats next.
        if (!inside)
            inside = nodeAtPoint(request, result, point.x(), point.y(), tx, ty, HitTestFloat);

        // Finally test to see if the mouse is in the background (within a child block's background).
        if (!inside)
            inside = nodeAtPoint(request, result, point.x(), point.y(), tx, ty, HitTestChildBlockBackgrounds);
    }

    // See if the mouse is inside us but not any of our descendants
    if (hitTestFilter != HitTestDescendants && !inside)
        inside = nodeAtPoint(request, result, point.x(), point.y(), tx, ty, HitTestBlockBackground);

    return inside;
}

void RenderObject::updateHitTestResult(HitTestResult& result, const IntPoint& point)
{
    if (result.innerNode())
        return;

    Node* n = node();
    if (n) {
        result.setInnerNode(n);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(n);
        result.setLocalPoint(point);
    }
}

bool RenderObject::nodeAtPoint(const HitTestRequest&, HitTestResult&, int /*x*/, int /*y*/, int /*tx*/, int /*ty*/, HitTestAction)
{
    return false;
}

void RenderObject::scheduleRelayout()
{
    if (isRenderView()) {
        FrameView* view = toRenderView(this)->frameView();
        if (view)
            view->scheduleRelayout();
    } else if (parent()) {
        FrameView* v = view() ? view()->frameView() : 0;
        if (v)
            v->scheduleRelayoutOfSubtree(this);
    }
}

void RenderObject::layout()
{
    ASSERT(needsLayout());
    RenderObject* child = firstChild();
    while (child) {
        child->layoutIfNeeded();
        ASSERT(!child->needsLayout());
        child = child->nextSibling();
    }
    setNeedsLayout(false);
}

PassRefPtr<RenderStyle> RenderObject::uncachedFirstLineStyle(RenderStyle* style) const
{
    if (!document()->usesFirstLineRules())
        return 0;

    ASSERT(!isText());

    RefPtr<RenderStyle> result;

    if (isBlockFlow()) {
        if (RenderBlock* firstLineBlock = this->firstLineBlock())
            result = firstLineBlock->getUncachedPseudoStyle(FIRST_LINE, style, firstLineBlock == this ? style : 0);
    } else if (!isAnonymous() && isRenderInline()) {
        RenderStyle* parentStyle = parent()->firstLineStyle();
        if (parentStyle != parent()->style())
            result = getUncachedPseudoStyle(FIRST_LINE_INHERITED, parentStyle, style);
    }

    return result.release();
}

RenderStyle* RenderObject::firstLineStyleSlowCase() const
{
    ASSERT(document()->usesFirstLineRules());

    RenderStyle* style = m_style.get();
    const RenderObject* renderer = isText() ? parent() : this;
    if (renderer->isBlockFlow()) {
        if (RenderBlock* firstLineBlock = renderer->firstLineBlock())
            style = firstLineBlock->getCachedPseudoStyle(FIRST_LINE, style);
    } else if (!renderer->isAnonymous() && renderer->isRenderInline()) {
        RenderStyle* parentStyle = renderer->parent()->firstLineStyle();
        if (parentStyle != renderer->parent()->style()) {
            // A first-line style is in effect. Cache a first-line style for ourselves.
            renderer->style()->setHasPseudoStyle(FIRST_LINE_INHERITED);
            style = renderer->getCachedPseudoStyle(FIRST_LINE_INHERITED, parentStyle);
        }
    }

    return style;
}

RenderStyle* RenderObject::getCachedPseudoStyle(PseudoId pseudo, RenderStyle* parentStyle) const
{
    if (pseudo < FIRST_INTERNAL_PSEUDOID && !style()->hasPseudoStyle(pseudo))
        return 0;

    RenderStyle* cachedStyle = style()->getCachedPseudoStyle(pseudo);
    if (cachedStyle)
        return cachedStyle;
    
    RefPtr<RenderStyle> result = getUncachedPseudoStyle(pseudo, parentStyle);
    if (result)
        return style()->addCachedPseudoStyle(result.release());
    return 0;
}

PassRefPtr<RenderStyle> RenderObject::getUncachedPseudoStyle(PseudoId pseudo, RenderStyle* parentStyle, RenderStyle* ownStyle) const
{
    if (pseudo < FIRST_INTERNAL_PSEUDOID && !ownStyle && !style()->hasPseudoStyle(pseudo))
        return 0;
    
    if (!parentStyle) {
        ASSERT(!ownStyle);
        parentStyle = style();
    }

    Node* n = node();
    while (n && !n->isElementNode())
        n = n->parentNode();
    if (!n)
        return 0;

    RefPtr<RenderStyle> result;
    if (pseudo == FIRST_LINE_INHERITED) {
        result = document()->styleSelector()->styleForElement(static_cast<Element*>(n), parentStyle, false);
        result->setStyleType(FIRST_LINE_INHERITED);
    } else
        result = document()->styleSelector()->pseudoStyleForElement(pseudo, static_cast<Element*>(n), parentStyle);
    return result.release();
}

static Color decorationColor(RenderObject* renderer)
{
    Color result;
    if (renderer->style()->textStrokeWidth() > 0) {
        // Prefer stroke color if possible but not if it's fully transparent.
        result = renderer->style()->visitedDependentColor(CSSPropertyWebkitTextStrokeColor);
        if (result.alpha())
            return result;
    }
    
    result = renderer->style()->visitedDependentColor(CSSPropertyWebkitTextFillColor);
    return result;
}

void RenderObject::getTextDecorationColors(int decorations, Color& underline, Color& overline,
                                           Color& linethrough, bool quirksMode)
{
    RenderObject* curr = this;
    do {
        int currDecs = curr->style()->textDecoration();
        if (currDecs) {
            if (currDecs & UNDERLINE) {
                decorations &= ~UNDERLINE;
                underline = decorationColor(curr);
            }
            if (currDecs & OVERLINE) {
                decorations &= ~OVERLINE;
                overline = decorationColor(curr);
            }
            if (currDecs & LINE_THROUGH) {
                decorations &= ~LINE_THROUGH;
                linethrough = decorationColor(curr);
            }
        }
        curr = curr->parent();
        if (curr && curr->isAnonymousBlock() && toRenderBlock(curr)->continuation())
            curr = toRenderBlock(curr)->continuation();
    } while (curr && decorations && (!quirksMode || !curr->node() ||
                                     (!curr->node()->hasTagName(aTag) && !curr->node()->hasTagName(fontTag))));

    // If we bailed out, use the element we bailed out at (typically a <font> or <a> element).
    if (decorations && curr) {
        if (decorations & UNDERLINE)
            underline = decorationColor(curr);
        if (decorations & OVERLINE)
            overline = decorationColor(curr);
        if (decorations & LINE_THROUGH)
            linethrough = decorationColor(curr);
    }
}

#if ENABLE(DASHBOARD_SUPPORT)
void RenderObject::addDashboardRegions(Vector<DashboardRegionValue>& regions)
{
    // Convert the style regions to absolute coordinates.
    if (style()->visibility() != VISIBLE || !isBox())
        return;
    
    RenderBox* box = toRenderBox(this);

    const Vector<StyleDashboardRegion>& styleRegions = style()->dashboardRegions();
    unsigned i, count = styleRegions.size();
    for (i = 0; i < count; i++) {
        StyleDashboardRegion styleRegion = styleRegions[i];

        int w = box->width();
        int h = box->height();

        DashboardRegionValue region;
        region.label = styleRegion.label;
        region.bounds = IntRect(styleRegion.offset.left().value(),
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

        FloatPoint absPos = localToAbsolute();
        region.bounds.setX(absPos.x() + styleRegion.offset.left().value());
        region.bounds.setY(absPos.y() + styleRegion.offset.top().value());

        if (frame()) {
            float pageScaleFactor = frame()->page()->chrome()->scaleFactor();
            if (pageScaleFactor != 1.0f) {
                region.bounds.scale(pageScaleFactor);
                region.clip.scale(pageScaleFactor);
            }
        }

        regions.append(region);
    }
}

void RenderObject::collectDashboardRegions(Vector<DashboardRegionValue>& regions)
{
    // RenderTexts don't have their own style, they just use their parent's style,
    // so we don't want to include them.
    if (isText())
        return;

    addDashboardRegions(regions);
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->collectDashboardRegions(regions);
}
#endif

bool RenderObject::willRenderImage(CachedImage*)
{
    // Without visibility we won't render (and therefore don't care about animation).
    if (style()->visibility() != VISIBLE)
        return false;

    // If we're not in a window (i.e., we're dormant from being put in the b/f cache or in a background tab)
    // then we don't want to render either.
    return !document()->inPageCache() && !document()->view()->isOffscreen();
}

int RenderObject::maximalOutlineSize(PaintPhase p) const
{
    if (p != PaintPhaseOutline && p != PaintPhaseSelfOutline && p != PaintPhaseChildOutlines)
        return 0;
    return toRenderView(document()->renderer())->maximalOutlineSize();
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

unsigned RenderObject::caretMaxRenderedOffset() const
{
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

void RenderObject::adjustRectForOutlineAndShadow(IntRect& rect) const
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

RenderBoxModelObject* RenderObject::offsetParent() const
{
    // If any of the following holds true return null and stop this algorithm:
    // A is the root element.
    // A is the HTML body element.
    // The computed value of the position property for element A is fixed.
    if (isRoot() || isBody() || (isPositioned() && style()->position() == FixedPosition))
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

    bool skipTables = isPositioned() || isRelPositioned();
    float currZoom = style()->effectiveZoom();
    RenderObject* curr = parent();
    while (curr && (!curr->node() || (!curr->isPositioned() && !curr->isRelPositioned() && !curr->isBody()))) {
        Node* element = curr->node();
        if (!skipTables && element && (element->hasTagName(tableTag) || element->hasTagName(tdTag) || element->hasTagName(thTag)))
            break;

        float newZoom = curr->style()->effectiveZoom();
        if (currZoom != newZoom)
            break;
        currZoom = newZoom;
        curr = curr->parent();
    }
    return curr && curr->isBoxModelObject() ? toRenderBoxModelObject(curr) : 0;
}

VisiblePosition RenderObject::createVisiblePosition(int offset, EAffinity affinity)
{
    // If this is a non-anonymous renderer in an editable area, then it's simple.
    if (Node* node = this->node()) {
        if (!node->rendererIsEditable()) {
            // If it can be found, we prefer a visually equivalent position that is editable. 
            Position position(node, offset);
            Position candidate = position.downstream(CanCrossEditingBoundary);
            if (candidate.deprecatedNode()->rendererIsEditable())
                return VisiblePosition(candidate, affinity);
            candidate = position.upstream(CanCrossEditingBoundary);
            if (candidate.deprecatedNode()->rendererIsEditable())
                return VisiblePosition(candidate, affinity);
        }
        // FIXME: Eliminate legacy editing positions
        return VisiblePosition(Position(node, offset), affinity);
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
            if (Node* node = renderer->node())
                return VisiblePosition(firstPositionInOrBeforeNode(node), DOWNSTREAM);
        }

        // Find non-anonymous content before.
        renderer = child;
        while ((renderer = renderer->previousInPreOrder())) {
            if (renderer == parent)
                break;
            if (Node* node = renderer->node())
                return VisiblePosition(lastPositionInOrAfterNode(node), DOWNSTREAM);
        }

        // Use the parent itself unless it too is anonymous.
        if (Node* node = parent->node())
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
