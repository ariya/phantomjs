/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef RenderObject_h
#define RenderObject_h

#include "CachedResourceClient.h"
#include "Document.h"
#include "Element.h"
#include "FloatQuad.h"
#include "PaintPhase.h"
#include "RenderObjectChildList.h"
#include "RenderStyle.h"
#include "TextAffinity.h"
#include "TransformationMatrix.h"
#include <wtf/UnusedParam.h>

#if USE(CG) || USE(CAIRO) || PLATFORM(QT)
#define HAVE_PATH_BASED_BORDER_RADIUS_DRAWING 1
#endif

namespace WebCore {

class AffineTransform;
class AnimationController;
class HitTestResult;
class InlineBox;
class InlineFlowBox;
class OverlapTestRequestClient;
class Path;
class Position;
class RenderBoxModelObject;
class RenderInline;
class RenderBlock;
class RenderFlow;
class RenderLayer;
class RenderTheme;
class TransformState;
class VisiblePosition;
#if ENABLE(SVG)
class RenderSVGResourceContainer;
#endif

struct PaintInfo;

enum HitTestFilter {
    HitTestAll,
    HitTestSelf,
    HitTestDescendants
};

enum HitTestAction {
    HitTestBlockBackground,
    HitTestChildBlockBackground,
    HitTestChildBlockBackgrounds,
    HitTestFloat,
    HitTestForeground
};

// Sides used when drawing borders and outlines. The values should run clockwise from top.
enum BoxSide {
    BSTop,
    BSRight,
    BSBottom,
    BSLeft
};

const int caretWidth = 1;

#if ENABLE(DASHBOARD_SUPPORT)
struct DashboardRegionValue {
    bool operator==(const DashboardRegionValue& o) const
    {
        return type == o.type && bounds == o.bounds && clip == o.clip && label == o.label;
    }
    bool operator!=(const DashboardRegionValue& o) const
    {
        return !(*this == o);
    }

    String label;
    IntRect bounds;
    IntRect clip;
    int type;
};
#endif

#ifndef NDEBUG
const int showTreeCharacterOffset = 39;
#endif

// Base class for all rendering tree objects.
class RenderObject : public CachedResourceClient {
    friend class RenderBlock;
    friend class RenderBox;
    friend class RenderLayer;
    friend class RenderObjectChildList;
    friend class RenderSVGContainer;
public:
    // Anonymous objects should pass the document as their node, and they will then automatically be
    // marked as anonymous in the constructor.
    RenderObject(Node*);
    virtual ~RenderObject();

    RenderTheme* theme() const;

    virtual const char* renderName() const = 0;

    RenderObject* parent() const { return m_parent; }
    bool isDescendantOf(const RenderObject*) const;

    RenderObject* previousSibling() const { return m_previous; }
    RenderObject* nextSibling() const { return m_next; }

    RenderObject* firstChild() const
    {
        if (const RenderObjectChildList* children = virtualChildren())
            return children->firstChild();
        return 0;
    }
    RenderObject* lastChild() const
    {
        if (const RenderObjectChildList* children = virtualChildren())
            return children->lastChild();
        return 0;
    }
    RenderObject* beforePseudoElementRenderer() const
    {
        if (const RenderObjectChildList* children = virtualChildren())
            return children->beforePseudoElementRenderer(this);
        return 0;
    }
    RenderObject* afterPseudoElementRenderer() const
    {
        if (const RenderObjectChildList* children = virtualChildren())
            return children->afterPseudoElementRenderer(this);
        return 0;
    }
    virtual RenderObjectChildList* virtualChildren() { return 0; }
    virtual const RenderObjectChildList* virtualChildren() const { return 0; }

    RenderObject* nextInPreOrder() const;
    RenderObject* nextInPreOrder(RenderObject* stayWithin) const;
    RenderObject* nextInPreOrderAfterChildren() const;
    RenderObject* nextInPreOrderAfterChildren(RenderObject* stayWithin) const;
    RenderObject* previousInPreOrder() const;
    RenderObject* childAt(unsigned) const;

    RenderObject* firstLeafChild() const;
    RenderObject* lastLeafChild() const;

    // The following six functions are used when the render tree hierarchy changes to make sure layers get
    // properly added and removed.  Since containership can be implemented by any subclass, and since a hierarchy
    // can contain a mixture of boxes and other object types, these functions need to be in the base class.
    RenderLayer* enclosingLayer() const;
    void addLayers(RenderLayer* parentLayer, RenderObject* newObject);
    void removeLayers(RenderLayer* parentLayer);
    void moveLayers(RenderLayer* oldParent, RenderLayer* newParent);
    RenderLayer* findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint, bool checkParent = true);

    // Convenience function for getting to the nearest enclosing box of a RenderObject.
    RenderBox* enclosingBox() const;
    RenderBoxModelObject* enclosingBoxModelObject() const;

    virtual bool isEmpty() const { return firstChild() == 0; }

#ifndef NDEBUG
    void setHasAXObject(bool flag) { m_hasAXObject = flag; }
    bool hasAXObject() const { return m_hasAXObject; }
    bool isSetNeedsLayoutForbidden() const { return m_setNeedsLayoutForbidden; }
    void setNeedsLayoutIsForbidden(bool flag) { m_setNeedsLayoutForbidden = flag; }
#endif

    // Obtains the nearest enclosing block (including this block) that contributes a first-line style to our inline
    // children.
    virtual RenderBlock* firstLineBlock() const;

    // Called when an object that was floating or positioned becomes a normal flow object
    // again.  We have to make sure the render tree updates as needed to accommodate the new
    // normal flow object.
    void handleDynamicFloatPositionChange();
    
    // RenderObject tree manipulation
    //////////////////////////////////////////
    virtual bool canHaveChildren() const { return virtualChildren(); }
    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const { return true; }
    virtual void addChild(RenderObject* newChild, RenderObject* beforeChild = 0);
    virtual void addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild = 0) { return addChild(newChild, beforeChild); }
    virtual void removeChild(RenderObject*);
    virtual bool createsAnonymousWrapper() const { return false; }
    //////////////////////////////////////////

protected:
    //////////////////////////////////////////
    // Helper functions. Dangerous to use!
    void setPreviousSibling(RenderObject* previous) { m_previous = previous; }
    void setNextSibling(RenderObject* next) { m_next = next; }
    void setParent(RenderObject* parent) { m_parent = parent; }
    //////////////////////////////////////////
private:
    void addAbsoluteRectForLayer(IntRect& result);
    void setLayerNeedsFullRepaint();

public:
#ifndef NDEBUG
    void showTreeForThis() const;
    void showRenderTreeForThis() const;
    void showLineTreeForThis() const;

    void showRenderObject() const;
    // We don't make printedCharacters an optional parameter so that
    // showRenderObject can be called from gdb easily.
    void showRenderObject(int printedCharacters) const;
    void showRenderTreeAndMark(const RenderObject* markedObject1 = 0, const char* markedLabel1 = 0, const RenderObject* markedObject2 = 0, const char* markedLabel2 = 0, int depth = 0) const;
#endif

    static RenderObject* createObject(Node*, RenderStyle*);

    // Overloaded new operator.  Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t, RenderArena*) throw();

    // Overridden to prevent the normal delete from being called.
    void operator delete(void*, size_t);

private:
    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t) throw();

public:
    RenderArena* renderArena() const { return document()->renderArena(); }

    virtual bool isApplet() const { return false; }
    virtual bool isBR() const { return false; }
    virtual bool isBlockFlow() const { return false; }
    virtual bool isBoxModelObject() const { return false; }
    virtual bool isCounter() const { return false; }
    virtual bool isQuote() const { return false; }
#if ENABLE(DETAILS)
    virtual bool isDetails() const { return false; }
    virtual bool isDetailsMarker() const { return false; }
#endif
    virtual bool isEmbeddedObject() const { return false; }
    virtual bool isFieldset() const { return false; }
    virtual bool isFileUploadControl() const { return false; }
    virtual bool isFrame() const { return false; }
    virtual bool isFrameSet() const { return false; }
    virtual bool isImage() const { return false; }
    virtual bool isInlineBlockOrInlineTable() const { return false; }
    virtual bool isListBox() const { return false; }
    virtual bool isListItem() const { return false; }
    virtual bool isListMarker() const { return false; }
    virtual bool isMedia() const { return false; }
    virtual bool isMenuList() const { return false; }
#if ENABLE(METER_TAG)
    virtual bool isMeter() const { return false; }
#endif
#if ENABLE(PROGRESS_TAG)
    virtual bool isProgress() const { return false; }
#endif
    virtual bool isRenderBlock() const { return false; }
    virtual bool isRenderButton() const { return false; }
    virtual bool isRenderIFrame() const { return false; }
    virtual bool isRenderImage() const { return false; }
    virtual bool isRenderInline() const { return false; }
    virtual bool isRenderPart() const { return false; }
    virtual bool isRenderView() const { return false; }
    virtual bool isReplica() const { return false; }

    virtual bool isRuby() const { return false; }
    virtual bool isRubyBase() const { return false; }
    virtual bool isRubyRun() const { return false; }
    virtual bool isRubyText() const { return false; }

    virtual bool isSlider() const { return false; }
#if ENABLE(DETAILS)
    virtual bool isSummary() const { return false; }
#endif
    virtual bool isTable() const { return false; }
    virtual bool isTableCell() const { return false; }
    virtual bool isTableCol() const { return false; }
    virtual bool isTableRow() const { return false; }
    virtual bool isTableSection() const { return false; }
    virtual bool isTextControl() const { return false; }
    virtual bool isTextArea() const { return false; }
    virtual bool isTextField() const { return false; }
    virtual bool isVideo() const { return false; }
    virtual bool isWidget() const { return false; }
    virtual bool isCanvas() const { return false; }
#if ENABLE(FULLSCREEN_API)
    virtual bool isRenderFullScreen() const { return false; }
#endif

    bool isRoot() const { return document()->documentElement() == m_node; }
    bool isBody() const;
    bool isHR() const;
    bool isLegend() const;

    bool isHTMLMarquee() const;

    inline bool isBeforeContent() const;
    inline bool isAfterContent() const;
    inline bool isBeforeOrAfterContent() const;
    static inline bool isBeforeContent(const RenderObject* obj) { return obj && obj->isBeforeContent(); }
    static inline bool isAfterContent(const RenderObject* obj) { return obj && obj->isAfterContent(); }
    static inline bool isBeforeOrAfterContent(const RenderObject* obj) { return obj && obj->isBeforeOrAfterContent(); }

    bool childrenInline() const { return m_childrenInline; }
    void setChildrenInline(bool b = true) { m_childrenInline = b; }
    bool hasColumns() const { return m_hasColumns; }
    void setHasColumns(bool b = true) { m_hasColumns = b; }

    virtual bool requiresForcedStyleRecalcPropagation() const { return false; }

#if ENABLE(MATHML)
    virtual bool isRenderMathMLBlock() const { return false; }
#endif // ENABLE(MATHML)

#if ENABLE(SVG)
    // FIXME: Until all SVG renders can be subclasses of RenderSVGModelObject we have
    // to add SVG renderer methods to RenderObject with an ASSERT_NOT_REACHED() default implementation.
    virtual bool isSVGRoot() const { return false; }
    virtual bool isSVGContainer() const { return false; }
    virtual bool isSVGViewportContainer() const { return false; } 
    virtual bool isSVGGradientStop() const { return false; }
    virtual bool isSVGHiddenContainer() const { return false; }
    virtual bool isSVGPath() const { return false; }
    virtual bool isSVGText() const { return false; }
    virtual bool isSVGTextPath() const { return false; }
    virtual bool isSVGInline() const { return false; }
    virtual bool isSVGInlineText() const { return false; }
    virtual bool isSVGImage() const { return false; }
    virtual bool isSVGForeignObject() const { return false; }
    virtual bool isSVGResourceContainer() const { return false; }
    virtual bool isSVGResourceFilter() const { return false; }
    virtual bool isSVGResourceFilterPrimitive() const { return false; }
    virtual bool isSVGShadowTreeRootContainer() const { return false; }

    virtual RenderSVGResourceContainer* toRenderSVGResourceContainer();

    // FIXME: Those belong into a SVG specific base-class for all renderers (see above)
    // Unfortunately we don't have such a class yet, because it's not possible for all renderers
    // to inherit from RenderSVGObject -> RenderObject (some need RenderBlock inheritance for instance)
    virtual void setNeedsTransformUpdate() { }
    virtual void setNeedsBoundariesUpdate();

    // Per SVG 1.1 objectBoundingBox ignores clipping, masking, filter effects, opacity and stroke-width.
    // This is used for all computation of objectBoundingBox relative units and by SVGLocateable::getBBox().
    // NOTE: Markers are not specifically ignored here by SVG 1.1 spec, but we ignore them
    // since stroke-width is ignored (and marker size can depend on stroke-width).
    // objectBoundingBox is returned local coordinates.
    // The name objectBoundingBox is taken from the SVG 1.1 spec.
    virtual FloatRect objectBoundingBox() const;
    virtual FloatRect strokeBoundingBox() const;

    // Returns the smallest rectangle enclosing all of the painted content
    // respecting clipping, masking, filters, opacity, stroke-width and markers
    virtual FloatRect repaintRectInLocalCoordinates() const;

    // This only returns the transform="" value from the element
    // most callsites want localToParentTransform() instead.
    virtual AffineTransform localTransform() const;

    // Returns the full transform mapping from local coordinates to local coords for the parent SVG renderer
    // This includes any viewport transforms and x/y offsets as well as the transform="" value off the element.
    virtual const AffineTransform& localToParentTransform() const;

    // SVG uses FloatPoint precise hit testing, and passes the point in parent
    // coordinates instead of in repaint container coordinates.  Eventually the
    // rest of the rendering tree will move to a similar model.
    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
#endif

    bool isAnonymous() const { return m_isAnonymous; }
    void setIsAnonymous(bool b) { m_isAnonymous = b; }
    bool isAnonymousBlock() const
    {
        // This function is kept in sync with anonymous block creation conditions in
        // RenderBlock::createAnonymousBlock(). This includes creating an anonymous
        // RenderBlock having a BLOCK or BOX display. Other classes such as RenderTextFragment
        // are not RenderBlocks and will return false. See https://bugs.webkit.org/show_bug.cgi?id=56709. 
        return m_isAnonymous && (style()->display() == BLOCK || style()->display() == BOX) && style()->styleType() == NOPSEUDO && isRenderBlock() && !isListMarker()
#if ENABLE(FULLSCREEN_API)
            && !isRenderFullScreen()
#endif
            ;
    }
    bool isAnonymousColumnsBlock() const { return style()->specifiesColumns() && isAnonymousBlock(); }
    bool isAnonymousColumnSpanBlock() const { return style()->columnSpan() && isAnonymousBlock(); }
    bool isElementContinuation() const { return node() && node()->renderer() != this; }
    bool isInlineElementContinuation() const { return isElementContinuation() && isInline(); }
    bool isBlockElementContinuation() const { return isElementContinuation() && !isInline(); }
    virtual RenderBoxModelObject* virtualContinuation() const { return 0; }

    bool isFloating() const { return m_floating; }
    bool isPositioned() const { return m_positioned; } // absolute or fixed positioning
    bool isRelPositioned() const { return m_relPositioned; } // relative positioning
    bool isText() const  { return m_isText; }
    bool isBox() const { return m_isBox; }
    bool isInline() const { return m_inline; }  // inline object
    bool isRunIn() const { return style()->display() == RUN_IN; } // run-in object
    bool isDragging() const { return m_isDragging; }
    bool isReplaced() const { return m_replaced; } // a "replaced" element (see CSS)
    bool isHorizontalWritingMode() const { return m_horizontalWritingMode; }

    bool hasLayer() const { return m_hasLayer; }
    
    bool hasBoxDecorations() const { return m_paintBackground; }
    bool borderImageIsLoadedAndCanBeRendered() const;
    bool mustRepaintBackgroundOrBorder() const;
    bool hasBackground() const { return style()->hasBackground(); }
    bool needsLayout() const { return m_needsLayout || m_normalChildNeedsLayout || m_posChildNeedsLayout || m_needsSimplifiedNormalFlowLayout || m_needsPositionedMovementLayout; }
    bool selfNeedsLayout() const { return m_needsLayout; }
    bool needsPositionedMovementLayout() const { return m_needsPositionedMovementLayout; }
    bool needsPositionedMovementLayoutOnly() const { return m_needsPositionedMovementLayout && !m_needsLayout && !m_normalChildNeedsLayout && !m_posChildNeedsLayout && !m_needsSimplifiedNormalFlowLayout; }
    bool posChildNeedsLayout() const { return m_posChildNeedsLayout; }
    bool needsSimplifiedNormalFlowLayout() const { return m_needsSimplifiedNormalFlowLayout; }
    bool normalChildNeedsLayout() const { return m_normalChildNeedsLayout; }
    
    bool preferredLogicalWidthsDirty() const { return m_preferredLogicalWidthsDirty; }

    bool isSelectionBorder() const;

    bool hasClip() const { return isPositioned() && style()->hasClip(); }
    bool hasOverflowClip() const { return m_hasOverflowClip; }

    bool hasTransform() const { return m_hasTransform; }
    bool hasMask() const { return style() && style()->hasMask(); }

    inline bool preservesNewline() const;

#if !HAVE(PATH_BASED_BORDER_RADIUS_DRAWING)
    // FIXME: This function should be removed when all ports implement GraphicsContext::clipConvexPolygon()!!
    // At that time, everyone can use RenderObject::drawBoxSideFromPath() instead. This should happen soon.
    void drawArcForBoxSide(GraphicsContext*, int x, int y, float thickness, const IntSize& radius, int angleStart,
                           int angleSpan, BoxSide, Color, EBorderStyle, bool firstCorner);
#endif

    IntRect borderInnerRect(const IntRect&, unsigned short topWidth, unsigned short bottomWidth,
                            unsigned short leftWidth, unsigned short rightWidth) const;

    // The pseudo element style can be cached or uncached.  Use the cached method if the pseudo element doesn't respect
    // any pseudo classes (and therefore has no concept of changing state).
    RenderStyle* getCachedPseudoStyle(PseudoId, RenderStyle* parentStyle = 0) const;
    PassRefPtr<RenderStyle> getUncachedPseudoStyle(PseudoId, RenderStyle* parentStyle = 0, RenderStyle* ownStyle = 0) const;
    
    virtual void updateDragState(bool dragOn);

    RenderView* view() const;

    // Returns true if this renderer is rooted, and optionally returns the hosting view (the root of the hierarchy).
    bool isRooted(RenderView** = 0);

    Node* node() const { return m_isAnonymous ? 0 : m_node; }

    // Returns the styled node that caused the generation of this renderer.
    // This is the same as node() except for renderers of :before and :after
    // pseudo elements for which their parent node is returned.
    Node* generatingNode() const { return m_node == document() ? 0 : m_node; }
    void setNode(Node* node) { m_node = node; }

    Document* document() const { return m_node->document(); }
    Frame* frame() const { return document()->frame(); }

    bool hasOutlineAnnotation() const;
    bool hasOutline() const { return style()->hasOutline() || hasOutlineAnnotation(); }

    // Returns the object containing this one. Can be different from parent for positioned elements.
    // If repaintContainer and repaintContainerSkipped are not null, on return *repaintContainerSkipped
    // is true if the renderer returned is an ancestor of repaintContainer.
    RenderObject* container(RenderBoxModelObject* repaintContainer = 0, bool* repaintContainerSkipped = 0) const;

    virtual RenderObject* hoverAncestor() const { return parent(); }

    // IE Extension that can be called on any RenderObject.  See the implementation for the details.
    RenderBoxModelObject* offsetParent() const;

    void markContainingBlocksForLayout(bool scheduleRelayout = true, RenderObject* newRoot = 0);
    void setNeedsLayout(bool b, bool markParents = true);
    void setChildNeedsLayout(bool b, bool markParents = true);
    void setNeedsPositionedMovementLayout();
    void setNeedsSimplifiedNormalFlowLayout();
    void setPreferredLogicalWidthsDirty(bool, bool markParents = true);
    void invalidateContainerPreferredLogicalWidths();
    
    void setNeedsLayoutAndPrefWidthsRecalc()
    {
        setNeedsLayout(true);
        setPreferredLogicalWidthsDirty(true);
    }

    void setPositioned(bool b = true)  { m_positioned = b;  }
    void setRelPositioned(bool b = true) { m_relPositioned = b; }
    void setFloating(bool b = true) { m_floating = b; }
    void setInline(bool b = true) { m_inline = b; }
    void setHasBoxDecorations(bool b = true) { m_paintBackground = b; }
    void setIsText() { m_isText = true; }
    void setIsBox() { m_isBox = true; }
    void setReplaced(bool b = true) { m_replaced = b; }
    void setHorizontalWritingMode(bool b = true) { m_horizontalWritingMode = b; }
    void setHasOverflowClip(bool b = true) { m_hasOverflowClip = b; }
    void setHasLayer(bool b = true) { m_hasLayer = b; }
    void setHasTransform(bool b = true) { m_hasTransform = b; }
    void setHasReflection(bool b = true) { m_hasReflection = b; }

    void scheduleRelayout();

    void updateFillImages(const FillLayer*, const FillLayer*);
    void updateImage(StyleImage*, StyleImage*);

    virtual void paint(PaintInfo&, int tx, int ty);

    // Recursive function that computes the size and position of this object and all its descendants.
    virtual void layout();

    /* This function performs a layout only if one is needed. */
    void layoutIfNeeded() { if (needsLayout()) layout(); }
    
    // used for element state updates that cannot be fixed with a
    // repaint and do not need a relayout
    virtual void updateFromElement() { }

#if ENABLE(DASHBOARD_SUPPORT)
    virtual void addDashboardRegions(Vector<DashboardRegionValue>&);
    void collectDashboardRegions(Vector<DashboardRegionValue>&);
#endif

    bool hitTest(const HitTestRequest&, HitTestResult&, const IntPoint&, int tx, int ty, HitTestFilter = HitTestAll);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);
    virtual void updateHitTestResult(HitTestResult&, const IntPoint&);

    VisiblePosition positionForCoordinates(int x, int y);
    virtual VisiblePosition positionForPoint(const IntPoint&);
    VisiblePosition createVisiblePosition(int offset, EAffinity);
    VisiblePosition createVisiblePosition(const Position&);

    virtual void dirtyLinesFromChangedChild(RenderObject*);

    // Called to update a style that is allowed to trigger animations.
    // FIXME: Right now this will typically be called only when updating happens from the DOM on explicit elements.
    // We don't yet handle generated content animation such as first-letter or before/after (we'll worry about this later).
    void setAnimatableStyle(PassRefPtr<RenderStyle>);

    // Set the style of the object and update the state of the object accordingly.
    virtual void setStyle(PassRefPtr<RenderStyle>);

    // Updates only the local style ptr of the object.  Does not update the state of the object,
    // and so only should be called when the style is known not to have changed (or from setStyle).
    void setStyleInternal(PassRefPtr<RenderStyle>);

    // returns the containing block level element for this element.
    RenderBlock* containingBlock() const;

    // Convert the given local point to absolute coordinates
    // FIXME: Temporary. If useTransforms is true, take transforms into account. Eventually localToAbsolute() will always be transform-aware.
    FloatPoint localToAbsolute(const FloatPoint& localPoint = FloatPoint(), bool fixed = false, bool useTransforms = false) const;
    FloatPoint absoluteToLocal(const FloatPoint&, bool fixed = false, bool useTransforms = false) const;

    // Convert a local quad to absolute coordinates, taking transforms into account.
    FloatQuad localToAbsoluteQuad(const FloatQuad& quad, bool fixed = false) const
    {
        return localToContainerQuad(quad, 0, fixed);
    }
    // Convert a local quad into the coordinate system of container, taking transforms into account.
    FloatQuad localToContainerQuad(const FloatQuad&, RenderBoxModelObject* repaintContainer, bool fixed = false) const;

    // Return the offset from the container() renderer (excluding transforms). In multi-column layout,
    // different offsets apply at different points, so return the offset that applies to the given point.
    virtual IntSize offsetFromContainer(RenderObject*, const IntPoint&) const;
    // Return the offset from an object up the container() chain. Asserts that none of the intermediate objects have transforms.
    IntSize offsetFromAncestorContainer(RenderObject*) const;
    
    virtual void absoluteRects(Vector<IntRect>&, int, int) { }
    // FIXME: useTransforms should go away eventually
    IntRect absoluteBoundingBoxRect(bool useTransforms = false);

    // Build an array of quads in absolute coords for line boxes
    virtual void absoluteQuads(Vector<FloatQuad>&) { }

    void absoluteFocusRingQuads(Vector<FloatQuad>&);

    // the rect that will be painted if this object is passed as the paintingRoot
    IntRect paintingRootRect(IntRect& topLevelRect);

    virtual int minPreferredLogicalWidth() const { return 0; }
    virtual int maxPreferredLogicalWidth() const { return 0; }

    RenderStyle* style() const { return m_style.get(); }
    RenderStyle* firstLineStyle() const { return document()->usesFirstLineRules() ? firstLineStyleSlowCase() : style(); }
    RenderStyle* style(bool firstLine) const { return firstLine ? firstLineStyle() : style(); }

    // Used only by Element::pseudoStyleCacheIsInvalid to get a first line style based off of a
    // given new style, without accessing the cache.
    PassRefPtr<RenderStyle> uncachedFirstLineStyle(RenderStyle*) const;

    // Anonymous blocks that are part of of a continuation chain will return their inline continuation's outline style instead.
    // This is typically only relevant when repainting.
    virtual RenderStyle* outlineStyleForRepaint() const { return style(); }
    
    void getTextDecorationColors(int decorations, Color& underline, Color& overline,
                                 Color& linethrough, bool quirksMode = false);

    // Return the RenderBox in the container chain which is responsible for painting this object, or 0
    // if painting is root-relative. This is the container that should be passed to the 'forRepaint'
    // methods.
    RenderBoxModelObject* containerForRepaint() const;
    // Actually do the repaint of rect r for this object which has been computed in the coordinate space
    // of repaintContainer. If repaintContainer is 0, repaint via the view.
    void repaintUsingContainer(RenderBoxModelObject* repaintContainer, const IntRect& r, bool immediate = false);
    
    // Repaint the entire object.  Called when, e.g., the color of a border changes, or when a border
    // style changes.
    void repaint(bool immediate = false);

    // Repaint a specific subrectangle within a given object.  The rect |r| is in the object's coordinate space.
    void repaintRectangle(const IntRect&, bool immediate = false);

    // Repaint only if our old bounds and new bounds are different. The caller may pass in newBounds and newOutlineBox if they are known.
    bool repaintAfterLayoutIfNeeded(RenderBoxModelObject* repaintContainer, const IntRect& oldBounds, const IntRect& oldOutlineBox, const IntRect* newBoundsPtr = 0, const IntRect* newOutlineBoxPtr = 0);

    // Repaint only if the object moved.
    virtual void repaintDuringLayoutIfMoved(const IntRect& rect);

    // Called to repaint a block's floats.
    virtual void repaintOverhangingFloats(bool paintAllDescendants = false);

    bool checkForRepaintDuringLayout() const;

    // Returns the rect that should be repainted whenever this object changes.  The rect is in the view's
    // coordinate space.  This method deals with outlines and overflow.
    IntRect absoluteClippedOverflowRect()
    {
        return clippedOverflowRectForRepaint(0);
    }
    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);    
    virtual IntRect rectWithOutlineForRepaint(RenderBoxModelObject* repaintContainer, int outlineWidth);

    // Given a rect in the object's coordinate space, compute a rect suitable for repainting
    // that rect in view coordinates.
    void computeAbsoluteRepaintRect(IntRect& r, bool fixed = false)
    {
        return computeRectForRepaint(0, r, fixed);
    }
    // Given a rect in the object's coordinate space, compute a rect suitable for repainting
    // that rect in the coordinate space of repaintContainer.
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);

    // If multiple-column layout results in applying an offset to the given point, add the same
    // offset to the given size.
    virtual void adjustForColumns(IntSize&, const IntPoint&) const { }

    virtual unsigned int length() const { return 1; }

    bool isFloatingOrPositioned() const { return (isFloating() || isPositioned()); }

    bool isTransparent() const { return style()->opacity() < 1.0f; }
    float opacity() const { return style()->opacity(); }

    bool hasReflection() const { return m_hasReflection; }

    // Applied as a "slop" to dirty rect checks during the outline painting phase's dirty-rect checks.
    int maximalOutlineSize(PaintPhase) const;

    void setHasMarkupTruncation(bool b = true) { m_hasMarkupTruncation = b; }
    bool hasMarkupTruncation() const { return m_hasMarkupTruncation; }

    enum SelectionState {
        SelectionNone, // The object is not selected.
        SelectionStart, // The object either contains the start of a selection run or is the start of a run
        SelectionInside, // The object is fully encompassed by a selection run
        SelectionEnd, // The object either contains the end of a selection run or is the end of a run
        SelectionBoth // The object contains an entire run or is the sole selected object in that run
    };

    // The current selection state for an object.  For blocks, the state refers to the state of the leaf
    // descendants (as described above in the SelectionState enum declaration).
    SelectionState selectionState() const { return static_cast<SelectionState>(m_selectionState);; }

    // Sets the selection state for an object.
    virtual void setSelectionState(SelectionState state) { m_selectionState = state; }

    // A single rectangle that encompasses all of the selected objects within this object.  Used to determine the tightest
    // possible bounding box for the selection.
    IntRect selectionRect(bool clipToVisibleContent = true) { return selectionRectForRepaint(0, clipToVisibleContent); }
    virtual IntRect selectionRectForRepaint(RenderBoxModelObject* /*repaintContainer*/, bool /*clipToVisibleContent*/ = true) { return IntRect(); }

    // Whether or not an object can be part of the leaf elements of the selection.
    virtual bool canBeSelectionLeaf() const { return false; }

    // Whether or not a block has selected children.
    bool hasSelectedChildren() const { return m_selectionState != SelectionNone; }

    // Obtains the selection colors that should be used when painting a selection.
    Color selectionBackgroundColor() const;
    Color selectionForegroundColor() const;
    Color selectionEmphasisMarkColor() const;

    // Whether or not a given block needs to paint selection gaps.
    virtual bool shouldPaintSelectionGaps() const { return false; }

#if ENABLE(DRAG_SUPPORT)
    Node* draggableNode(bool dhtmlOK, bool uaOK, int x, int y, bool& dhtmlWillDrag) const;
#endif

    /**
     * Returns the local coordinates of the caret within this render object.
     * @param caretOffset zero-based offset determining position within the render object.
     * @param extraWidthToEndOfLine optional out arg to give extra width to end of line -
     * useful for character range rect computations
     */
    virtual IntRect localCaretRect(InlineBox*, int caretOffset, int* extraWidthToEndOfLine = 0);

    bool isMarginBeforeQuirk() const { return m_marginBeforeQuirk; }
    bool isMarginAfterQuirk() const { return m_marginAfterQuirk; }
    void setMarginBeforeQuirk(bool b = true) { m_marginBeforeQuirk = b; }
    void setMarginAfterQuirk(bool b = true) { m_marginAfterQuirk = b; }

    // When performing a global document tear-down, the renderer of the document is cleared.  We use this
    // as a hook to detect the case of document destruction and don't waste time doing unnecessary work.
    bool documentBeingDestroyed() const;

    virtual void destroy();

    // Virtual function helpers for CSS3 Flexible Box Layout
    virtual bool isFlexibleBox() const { return false; }
    virtual bool isFlexingChildren() const { return false; }
    virtual bool isStretchingChildren() const { return false; }

    virtual bool isCombineText() const { return false; }

    virtual int caretMinOffset() const;
    virtual int caretMaxOffset() const;
    virtual unsigned caretMaxRenderedOffset() const;

    virtual int previousOffset(int current) const;
    virtual int previousOffsetForBackwardDeletion(int current) const;
    virtual int nextOffset(int current) const;

    virtual void imageChanged(CachedImage*, const IntRect* = 0);
    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0) { }
    virtual bool willRenderImage(CachedImage*);

    void selectionStartEnd(int& spos, int& epos) const;

    bool hasOverrideSize() const { return m_hasOverrideSize; }
    void setHasOverrideSize(bool b) { m_hasOverrideSize = b; }
    
    void remove() { if (parent()) parent()->removeChild(this); }

    AnimationController* animation() const;

    bool visibleToHitTesting() const { return style()->visibility() == VISIBLE && style()->pointerEvents() != PE_NONE; }

    // Map points and quads through elements, potentially via 3d transforms. You should never need to call these directly; use
    // localToAbsolute/absoluteToLocal methods instead.
    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&) const;
    virtual void mapAbsoluteToLocalPoint(bool fixed, bool useTransforms, TransformState&) const;

    bool shouldUseTransformFromContainer(const RenderObject* container) const;
    void getTransformFromContainer(const RenderObject* container, const IntSize& offsetInContainer, TransformationMatrix&) const;
    
    virtual void addFocusRingRects(Vector<IntRect>&, int /*tx*/, int /*ty*/) { };

    IntRect absoluteOutlineBounds() const
    {
        return outlineBoundsForRepaint(0);
    }

protected:
    // Overrides should call the superclass at the end
    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle);
    // Overrides should call the superclass at the start
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    void propagateStyleToAnonymousChildren(bool blockChildrenOnly = false);

    void drawLineForBoxSide(GraphicsContext*, int x1, int y1, int x2, int y2, BoxSide,
                            Color, EBorderStyle, int adjbw1, int adjbw2, bool antialias = false);

    void paintFocusRing(GraphicsContext*, int tx, int ty, RenderStyle*);
    void paintOutline(GraphicsContext*, int tx, int ty, int w, int h);
    void addPDFURLRect(GraphicsContext*, const IntRect&);

    virtual IntRect viewRect() const;

    void adjustRectForOutlineAndShadow(IntRect&) const;

    void arenaDelete(RenderArena*, void* objectBase);

    virtual IntRect outlineBoundsForRepaint(RenderBoxModelObject* /*repaintContainer*/, IntPoint* /*cachedOffsetToRepaintContainer*/ = 0) const { return IntRect(); }

    class LayoutRepainter {
    public:
        LayoutRepainter(RenderObject& object, bool checkForRepaint, const IntRect* oldBounds = 0)
            : m_object(object)
            , m_repaintContainer(0)
            , m_checkForRepaint(checkForRepaint)
        {
            if (m_checkForRepaint) {
                m_repaintContainer = m_object.containerForRepaint();
                m_oldBounds = oldBounds ? *oldBounds : m_object.clippedOverflowRectForRepaint(m_repaintContainer);
                m_oldOutlineBox = m_object.outlineBoundsForRepaint(m_repaintContainer);
            }
        }
        
        // Return true if it repainted.
        bool repaintAfterLayout()
        {
            return m_checkForRepaint ? m_object.repaintAfterLayoutIfNeeded(m_repaintContainer, m_oldBounds, m_oldOutlineBox) : false;
        }
        
        bool checkForRepaint() const { return m_checkForRepaint; }
        
    private:
        RenderObject& m_object;
        RenderBoxModelObject* m_repaintContainer;
        IntRect m_oldBounds;
        IntRect m_oldOutlineBox;
        bool m_checkForRepaint;
    };
    
private:
    RenderStyle* firstLineStyleSlowCase() const;
    StyleDifference adjustStyleDifference(StyleDifference, unsigned contextSensitiveProperties) const;

    Color selectionColor(int colorProperty) const;
    
    RefPtr<RenderStyle> m_style;

    Node* m_node;

    RenderObject* m_parent;
    RenderObject* m_previous;
    RenderObject* m_next;

#ifndef NDEBUG
    bool m_hasAXObject;
    bool m_setNeedsLayoutForbidden : 1;
#endif

    // 32 bits have been used here. THERE ARE NO FREE BITS AVAILABLE.
    bool m_needsLayout               : 1;
    bool m_needsPositionedMovementLayout :1;
    bool m_normalChildNeedsLayout    : 1;
    bool m_posChildNeedsLayout       : 1;
    bool m_needsSimplifiedNormalFlowLayout  : 1;
    bool m_preferredLogicalWidthsDirty           : 1;
    bool m_floating                  : 1;

    bool m_positioned                : 1;
    bool m_relPositioned             : 1;
    bool m_paintBackground           : 1; // if the box has something to paint in the
                                          // background painting phase (background, border, etc)

    bool m_isAnonymous               : 1;
    bool m_isText                    : 1;
    bool m_isBox                     : 1;
    bool m_inline                    : 1;
    bool m_replaced                  : 1;
    bool m_horizontalWritingMode : 1;
    bool m_isDragging                : 1;

    bool m_hasLayer                  : 1;
    bool m_hasOverflowClip           : 1; // Set in the case of overflow:auto/scroll/hidden
    bool m_hasTransform              : 1;
    bool m_hasReflection             : 1;

    bool m_hasOverrideSize           : 1;
    
public:
    bool m_hasCounterNodeMap         : 1;
    bool m_everHadLayout             : 1;

private:
    // These bitfields are moved here from subclasses to pack them together
    // from RenderBlock
    bool m_childrenInline : 1;
    bool m_marginBeforeQuirk : 1;
    bool m_marginAfterQuirk : 1;
    bool m_hasMarkupTruncation : 1;
    unsigned m_selectionState : 3; // SelectionState
    bool m_hasColumns : 1;
    
    // from RenderTableCell
    bool m_cellWidthChanged : 1;

private:
    // Store state between styleWillChange and styleDidChange
    static bool s_affectsParentBlock;
};

inline bool RenderObject::documentBeingDestroyed() const
{
    return !document()->renderer();
}

inline bool RenderObject::isBeforeContent() const
{
    if (style()->styleType() != BEFORE)
        return false;
    // Text nodes don't have their own styles, so ignore the style on a text node.
    if (isText() && !isBR())
        return false;
    return true;
}

inline bool RenderObject::isAfterContent() const
{
    if (style()->styleType() != AFTER)
        return false;
    // Text nodes don't have their own styles, so ignore the style on a text node.
    if (isText() && !isBR())
        return false;
    return true;
}

inline bool RenderObject::isBeforeOrAfterContent() const
{
    return isBeforeContent() || isAfterContent();
}

inline void RenderObject::setNeedsLayout(bool b, bool markParents)
{
    bool alreadyNeededLayout = m_needsLayout;
    m_needsLayout = b;
    if (b) {
        ASSERT(!isSetNeedsLayoutForbidden());
        if (!alreadyNeededLayout) {
            if (markParents)
                markContainingBlocksForLayout();
            if (hasLayer())
                setLayerNeedsFullRepaint();
        }
    } else {
        m_everHadLayout = true;
        m_posChildNeedsLayout = false;
        m_needsSimplifiedNormalFlowLayout = false;
        m_normalChildNeedsLayout = false;
        m_needsPositionedMovementLayout = false;
    }
}

inline void RenderObject::setChildNeedsLayout(bool b, bool markParents)
{
    bool alreadyNeededLayout = m_normalChildNeedsLayout;
    m_normalChildNeedsLayout = b;
    if (b) {
        ASSERT(!isSetNeedsLayoutForbidden());
        if (!alreadyNeededLayout && markParents)
            markContainingBlocksForLayout();
    } else {
        m_posChildNeedsLayout = false;
        m_needsSimplifiedNormalFlowLayout = false;
        m_normalChildNeedsLayout = false;
        m_needsPositionedMovementLayout = false;
    }
}

inline void RenderObject::setNeedsPositionedMovementLayout()
{
    bool alreadyNeededLayout = m_needsPositionedMovementLayout;
    m_needsPositionedMovementLayout = true;
    ASSERT(!isSetNeedsLayoutForbidden());
    if (!alreadyNeededLayout) {
        markContainingBlocksForLayout();
        if (hasLayer())
            setLayerNeedsFullRepaint();
    }
}

inline void RenderObject::setNeedsSimplifiedNormalFlowLayout()
{
    bool alreadyNeededLayout = m_needsSimplifiedNormalFlowLayout;
    m_needsSimplifiedNormalFlowLayout = true;
    ASSERT(!isSetNeedsLayoutForbidden());
    if (!alreadyNeededLayout) {
        markContainingBlocksForLayout();
        if (hasLayer())
            setLayerNeedsFullRepaint();
    }
}

inline bool objectIsRelayoutBoundary(const RenderObject *obj) 
{
    // FIXME: In future it may be possible to broaden this condition in order to improve performance.
    // Table cells are excluded because even when their CSS height is fixed, their height()
    // may depend on their contents.
    return obj->isTextControl()
        || (obj->hasOverflowClip() && !obj->style()->width().isIntrinsicOrAuto() && !obj->style()->height().isIntrinsicOrAuto() && !obj->style()->height().isPercent() && !obj->isTableCell())
#if ENABLE(SVG)
           || obj->isSVGRoot()
#endif
           ;
}

inline void RenderObject::markContainingBlocksForLayout(bool scheduleRelayout, RenderObject* newRoot)
{
    ASSERT(!scheduleRelayout || !newRoot);

    RenderObject* o = container();
    RenderObject* last = this;

    bool simplifiedNormalFlowLayout = needsSimplifiedNormalFlowLayout() && !selfNeedsLayout() && !normalChildNeedsLayout();

    while (o) {
        // Don't mark the outermost object of an unrooted subtree. That object will be 
        // marked when the subtree is added to the document.
        RenderObject* container = o->container();
        if (!container && !o->isRenderView())
            return;
        if (!last->isText() && (last->style()->position() == FixedPosition || last->style()->position() == AbsolutePosition)) {
            bool willSkipRelativelyPositionedInlines = !o->isRenderBlock();
            while (o && !o->isRenderBlock()) // Skip relatively positioned inlines and get to the enclosing RenderBlock.
                o = o->container();
            if (!o || o->m_posChildNeedsLayout)
                return;
            if (willSkipRelativelyPositionedInlines)
                container = o->container();
            o->m_posChildNeedsLayout = true;
            simplifiedNormalFlowLayout = true;
            ASSERT(!o->isSetNeedsLayoutForbidden());
        } else if (simplifiedNormalFlowLayout) {
            if (o->m_needsSimplifiedNormalFlowLayout)
                return;
            o->m_needsSimplifiedNormalFlowLayout = true;
            ASSERT(!o->isSetNeedsLayoutForbidden());
        } else {
            if (o->m_normalChildNeedsLayout)
                return;
            o->m_normalChildNeedsLayout = true;
            ASSERT(!o->isSetNeedsLayoutForbidden());
        }

        if (o == newRoot)
            return;

        last = o;
        if (scheduleRelayout && objectIsRelayoutBoundary(last))
            break;
        o = container;
    }

    if (scheduleRelayout)
        last->scheduleRelayout();
}

inline bool RenderObject::preservesNewline() const
{
#if ENABLE(SVG)
    if (isSVGInlineText())
        return false;
#endif
        
    return style()->preserveNewline();
}

inline void makeMatrixRenderable(TransformationMatrix& matrix, bool has3DRendering)
{
#if !ENABLE(3D_RENDERING)
    UNUSED_PARAM(has3DRendering);
    matrix.makeAffine();
#else
    if (!has3DRendering)
        matrix.makeAffine();
#endif
}

inline int adjustForAbsoluteZoom(int value, RenderObject* renderer)
{
    return adjustForAbsoluteZoom(value, renderer->style());
}

inline FloatPoint adjustFloatPointForAbsoluteZoom(const FloatPoint& point, RenderObject* renderer)
{
    // The result here is in floats, so we don't need the truncation hack from the integer version above.
    float zoomFactor = renderer->style()->effectiveZoom();
    if (zoomFactor == 1)
        return point;
    return FloatPoint(point.x() / zoomFactor, point.y() / zoomFactor);
}

inline void adjustFloatQuadForAbsoluteZoom(FloatQuad& quad, RenderObject* renderer)
{
    quad.setP1(adjustFloatPointForAbsoluteZoom(quad.p1(), renderer));
    quad.setP2(adjustFloatPointForAbsoluteZoom(quad.p2(), renderer));
    quad.setP3(adjustFloatPointForAbsoluteZoom(quad.p3(), renderer));
    quad.setP4(adjustFloatPointForAbsoluteZoom(quad.p4(), renderer));
}

inline void adjustFloatRectForAbsoluteZoom(FloatRect& rect, RenderObject* renderer)
{
    RenderStyle* style = renderer->style();
    rect.setX(adjustFloatForAbsoluteZoom(rect.x(), style));
    rect.setY(adjustFloatForAbsoluteZoom(rect.y(), style));
    rect.setWidth(adjustFloatForAbsoluteZoom(rect.width(), style));
    rect.setHeight(adjustFloatForAbsoluteZoom(rect.height(), style));
}

inline FloatPoint adjustFloatPointForPageScale(const FloatPoint& point, float pageScale)
{
    if (pageScale == 1)
        return point;
    return FloatPoint(point.x() / pageScale, point.y() / pageScale);
}

inline void adjustFloatQuadForPageScale(FloatQuad& quad, float pageScale)
{
    if (pageScale == 1)
        return;
    quad.setP1(adjustFloatPointForPageScale(quad.p1(), pageScale));
    quad.setP2(adjustFloatPointForPageScale(quad.p2(), pageScale));
    quad.setP3(adjustFloatPointForPageScale(quad.p3(), pageScale));
    quad.setP4(adjustFloatPointForPageScale(quad.p4(), pageScale));
}

inline void adjustFloatRectForPageScale(FloatRect& rect, float pageScale)
{
    if (pageScale == 1)
        return;
    rect.setX(rect.x() / pageScale);
    rect.setY(rect.y() / pageScale);
    rect.setWidth(rect.width() / pageScale);
    rect.setHeight(rect.height() / pageScale);
}

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showTree(const WebCore::RenderObject*);
void showLineTree(const WebCore::RenderObject*);
void showRenderTree(const WebCore::RenderObject* object1);
// We don't make object2 an optional parameter so that showRenderTree
// can be called from gdb easily.
void showRenderTree(const WebCore::RenderObject* object1, const WebCore::RenderObject* object2);
#endif

#endif // RenderObject_h
