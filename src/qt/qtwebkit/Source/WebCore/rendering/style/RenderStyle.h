/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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

#ifndef RenderStyle_h
#define RenderStyle_h

#include "AnimationList.h"
#include "BorderValue.h"
#include "CSSLineBoxContainValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "Color.h"
#include "ColorSpace.h"
#include "CounterDirectives.h"
#include "DataRef.h"
#include "FontBaseline.h"
#include "FontDescription.h"
#include "GraphicsTypes.h"
#include "LayoutBoxExtent.h"
#include "Length.h"
#include "LengthBox.h"
#include "LengthFunctions.h"
#include "LengthSize.h"
#include "LineClampValue.h"
#include "NinePieceImage.h"
#include "OutlineValue.h"
#include "Pagination.h"
#include "RenderStyleConstants.h"
#include "RoundedRect.h"
#include "ShadowData.h"
#include "ShapeValue.h"
#include "StyleBackgroundData.h"
#include "StyleBoxData.h"
#include "StyleDeprecatedFlexibleBoxData.h"
#include "StyleFlexibleBoxData.h"
#include "StyleGridData.h"
#include "StyleGridItemData.h"
#include "StyleMarqueeData.h"
#include "StyleMultiColData.h"
#include "StyleRareInheritedData.h"
#include "StyleRareNonInheritedData.h"
#include "StyleReflection.h"
#include "StyleSurroundData.h"
#include "StyleTransformData.h"
#include "StyleVisualData.h"
#include "TextDirection.h"
#include "ThemeTypes.h"
#include "TransformOperations.h"
#include "UnicodeBidi.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

#if ENABLE(CSS_FILTERS)
#include "StyleFilterData.h"
#endif

#if ENABLE(DASHBOARD_SUPPORT)
#include "StyleDashboardRegion.h"
#endif

#if ENABLE(SVG)
#include "SVGPaint.h"
#include "SVGRenderStyle.h"
#endif

template<typename T, typename U> inline bool compareEqual(const T& t, const U& u) { return t == static_cast<T>(u); }

#define SET_VAR(group, variable, value) \
    if (!compareEqual(group->variable, value)) \
        group.access()->variable = value

#define SET_BORDERVALUE_COLOR(group, variable, value) \
    if (!compareEqual(group->variable.color(), value)) \
        group.access()->variable.setColor(value)

namespace WebCore {

using std::max;

#if ENABLE(CSS_FILTERS)
class FilterOperations;
#endif

class BorderData;
class CounterContent;
class CursorList;
class Font;
class FontMetrics;
class IntRect;
class Pair;
class ShadowData;
class StyleImage;
class StyleInheritedData;
class StyleResolver;
class TransformationMatrix;

class ContentData;

typedef Vector<RefPtr<RenderStyle>, 4> PseudoStyleCache;

class RenderStyle: public RefCounted<RenderStyle> {
    friend class CSSPropertyAnimation; // Used by CSS animations. We can't allow them to animate based off visited colors.
    friend class ApplyStyleCommand; // Editing has to only reveal unvisited info.
    friend class DeprecatedStyleBuilder; // Sets members directly.
    friend class EditingStyle; // Editing has to only reveal unvisited info.
    friend class ComputedStyleExtractor; // Ignores visited styles, so needs to be able to see unvisited info.
    friend class PropertyWrapperMaybeInvalidColor; // Used by CSS animations. We can't allow them to animate based off visited colors.
    friend class RenderSVGResource; // FIXME: Needs to alter the visited state by hand. Should clean the SVG code up and move it into RenderStyle perhaps.
    friend class RenderTreeAsText; // FIXME: Only needed so the render tree can keep lying and dump the wrong colors.  Rebaselining would allow this to be yanked.
    friend class StyleResolver; // Sets members directly.
protected:

    // non-inherited attributes
    DataRef<StyleBoxData> m_box;
    DataRef<StyleVisualData> visual;
    DataRef<StyleBackgroundData> m_background;
    DataRef<StyleSurroundData> surround;
    DataRef<StyleRareNonInheritedData> rareNonInheritedData;

    // inherited attributes
    DataRef<StyleRareInheritedData> rareInheritedData;
    DataRef<StyleInheritedData> inherited;

    // list of associated pseudo styles
    OwnPtr<PseudoStyleCache> m_cachedPseudoStyles;

#if ENABLE(SVG)
    DataRef<SVGRenderStyle> m_svgStyle;
#endif

// !START SYNC!: Keep this in sync with the copy constructor in RenderStyle.cpp and implicitlyInherited() in StyleResolver.cpp

    // inherit
    struct InheritedFlags {
        bool operator==(const InheritedFlags& other) const
        {
            return (_empty_cells == other._empty_cells)
                && (_caption_side == other._caption_side)
                && (_list_style_type == other._list_style_type)
                && (_list_style_position == other._list_style_position)
                && (_visibility == other._visibility)
                && (_text_align == other._text_align)
                && (_text_transform == other._text_transform)
                && (_text_decorations == other._text_decorations)
                && (_cursor_style == other._cursor_style)
#if ENABLE(CURSOR_VISIBILITY)
                && (m_cursorVisibility == other.m_cursorVisibility)
#endif
                && (_direction == other._direction)
                && (_white_space == other._white_space)
                && (_border_collapse == other._border_collapse)
                && (_box_direction == other._box_direction)
                && (m_rtlOrdering == other.m_rtlOrdering)
                && (m_printColorAdjust == other.m_printColorAdjust)
                && (_pointerEvents == other._pointerEvents)
                && (_insideLink == other._insideLink)
                && (m_writingMode == other.m_writingMode);
        }

        bool operator!=(const InheritedFlags& other) const { return !(*this == other); }

        unsigned _empty_cells : 1; // EEmptyCell
        unsigned _caption_side : 2; // ECaptionSide
        unsigned _list_style_type : 7; // EListStyleType
        unsigned _list_style_position : 1; // EListStylePosition
        unsigned _visibility : 2; // EVisibility
        unsigned _text_align : 4; // ETextAlign
        unsigned _text_transform : 2; // ETextTransform
        unsigned _text_decorations : TextDecorationBits;
        unsigned _cursor_style : 6; // ECursor
#if ENABLE(CURSOR_VISIBILITY)
        unsigned m_cursorVisibility : 1; // CursorVisibility
#endif
        unsigned _direction : 1; // TextDirection
        unsigned _white_space : 3; // EWhiteSpace
        // 32 bits
        unsigned _border_collapse : 1; // EBorderCollapse
        unsigned _box_direction : 1; // EBoxDirection (CSS3 box_direction property, flexible box layout module)

        // non CSS2 inherited
        unsigned m_rtlOrdering : 1; // Order
        unsigned m_printColorAdjust : PrintColorAdjustBits;
        unsigned _pointerEvents : 4; // EPointerEvents
        unsigned _insideLink : 2; // EInsideLink
        // 43 bits

        // CSS Text Layout Module Level 3: Vertical writing support
        unsigned m_writingMode : 2; // WritingMode
        // 45 bits
    } inherited_flags;

// don't inherit
    struct NonInheritedFlags {
        bool operator==(const NonInheritedFlags& other) const
        {
            return _effectiveDisplay == other._effectiveDisplay
                && _originalDisplay == other._originalDisplay
                && _overflowX == other._overflowX
                && _overflowY == other._overflowY
                && _vertical_align == other._vertical_align
                && _clear == other._clear
                && _position == other._position
                && _floating == other._floating
                && _table_layout == other._table_layout
                && _page_break_before == other._page_break_before
                && _page_break_after == other._page_break_after
                && _page_break_inside == other._page_break_inside
                && _styleType == other._styleType
                && _affectedByHover == other._affectedByHover
                && _affectedByActive == other._affectedByActive
                && _affectedByDrag == other._affectedByDrag
                && _pseudoBits == other._pseudoBits
                && _unicodeBidi == other._unicodeBidi
                && explicitInheritance == other.explicitInheritance
                && unique == other.unique
                && emptyState == other.emptyState
                && firstChildState == other.firstChildState
                && lastChildState == other.lastChildState
                && _isLink == other._isLink;
        }

        bool operator!=(const NonInheritedFlags& other) const { return !(*this == other); }

        unsigned _effectiveDisplay : 5; // EDisplay
        unsigned _originalDisplay : 5; // EDisplay
        unsigned _overflowX : 3; // EOverflow
        unsigned _overflowY : 3; // EOverflow
        unsigned _vertical_align : 4; // EVerticalAlign
        unsigned _clear : 2; // EClear
        unsigned _position : 3; // EPosition
        unsigned _floating : 2; // EFloat
        unsigned _table_layout : 1; // ETableLayout

        unsigned _unicodeBidi : 3; // EUnicodeBidi
        // 31 bits
        unsigned _page_break_before : 2; // EPageBreak
        unsigned _page_break_after : 2; // EPageBreak
        unsigned _page_break_inside : 2; // EPageBreak

        unsigned _styleType : 6; // PseudoId
        unsigned _pseudoBits : 7;
        unsigned explicitInheritance : 1; // Explicitly inherits a non-inherited property
        unsigned unique : 1; // Style can not be shared.
        unsigned emptyState : 1;
        unsigned firstChildState : 1;
        unsigned lastChildState : 1;

        bool affectedByHover() const { return _affectedByHover; }
        void setAffectedByHover(bool value) { _affectedByHover = value; }
        bool affectedByActive() const { return _affectedByActive; }
        void setAffectedByActive(bool value) { _affectedByActive = value; }
        bool affectedByDrag() const { return _affectedByDrag; }
        void setAffectedByDrag(bool value) { _affectedByDrag = value; }
        bool isLink() const { return _isLink; }
        void setIsLink(bool value) { _isLink = value; }
    private:
        unsigned _affectedByHover : 1;
        unsigned _affectedByActive : 1;
        unsigned _affectedByDrag : 1;
        unsigned _isLink : 1;
        // If you add more style bits here, you will also need to update RenderStyle::copyNonInheritedFrom()
        // 59 bits
    } noninherited_flags;

// !END SYNC!

protected:
    void setBitDefaults()
    {
        inherited_flags._empty_cells = initialEmptyCells();
        inherited_flags._caption_side = initialCaptionSide();
        inherited_flags._list_style_type = initialListStyleType();
        inherited_flags._list_style_position = initialListStylePosition();
        inherited_flags._visibility = initialVisibility();
        inherited_flags._text_align = initialTextAlign();
        inherited_flags._text_transform = initialTextTransform();
        inherited_flags._text_decorations = initialTextDecoration();
        inherited_flags._cursor_style = initialCursor();
#if ENABLE(CURSOR_VISIBILITY)
        inherited_flags.m_cursorVisibility = initialCursorVisibility();
#endif
        inherited_flags._direction = initialDirection();
        inherited_flags._white_space = initialWhiteSpace();
        inherited_flags._border_collapse = initialBorderCollapse();
        inherited_flags.m_rtlOrdering = initialRTLOrdering();
        inherited_flags._box_direction = initialBoxDirection();
        inherited_flags.m_printColorAdjust = initialPrintColorAdjust();
        inherited_flags._pointerEvents = initialPointerEvents();
        inherited_flags._insideLink = NotInsideLink;
        inherited_flags.m_writingMode = initialWritingMode();

        noninherited_flags._effectiveDisplay = noninherited_flags._originalDisplay = initialDisplay();
        noninherited_flags._overflowX = initialOverflowX();
        noninherited_flags._overflowY = initialOverflowY();
        noninherited_flags._vertical_align = initialVerticalAlign();
        noninherited_flags._clear = initialClear();
        noninherited_flags._position = initialPosition();
        noninherited_flags._floating = initialFloating();
        noninherited_flags._table_layout = initialTableLayout();
        noninherited_flags._unicodeBidi = initialUnicodeBidi();
        noninherited_flags._page_break_before = initialPageBreak();
        noninherited_flags._page_break_after = initialPageBreak();
        noninherited_flags._page_break_inside = initialPageBreak();
        noninherited_flags._styleType = NOPSEUDO;
        noninherited_flags._pseudoBits = 0;
        noninherited_flags.explicitInheritance = false;
        noninherited_flags.unique = false;
        noninherited_flags.emptyState = false;
        noninherited_flags.firstChildState = false;
        noninherited_flags.lastChildState = false;
        noninherited_flags.setAffectedByHover(false);
        noninherited_flags.setAffectedByActive(false);
        noninherited_flags.setAffectedByDrag(false);
        noninherited_flags.setIsLink(false);
    }

private:
    ALWAYS_INLINE RenderStyle();
    // used to create the default style.
    ALWAYS_INLINE RenderStyle(bool);
    ALWAYS_INLINE RenderStyle(const RenderStyle&);

public:
    static PassRefPtr<RenderStyle> create();
    static PassRefPtr<RenderStyle> createDefaultStyle();
    static PassRefPtr<RenderStyle> createAnonymousStyleWithDisplay(const RenderStyle* parentStyle, EDisplay);
    static PassRefPtr<RenderStyle> clone(const RenderStyle*);

    enum IsAtShadowBoundary {
        AtShadowBoundary,
        NotAtShadowBoundary,
    };

    void inheritFrom(const RenderStyle* inheritParent, IsAtShadowBoundary = NotAtShadowBoundary);
    void copyNonInheritedFrom(const RenderStyle*);

    PseudoId styleType() const { return static_cast<PseudoId>(noninherited_flags._styleType); }
    void setStyleType(PseudoId styleType) { noninherited_flags._styleType = styleType; }

    RenderStyle* getCachedPseudoStyle(PseudoId) const;
    RenderStyle* addCachedPseudoStyle(PassRefPtr<RenderStyle>);
    void removeCachedPseudoStyle(PseudoId);

    const PseudoStyleCache* cachedPseudoStyles() const { return m_cachedPseudoStyles.get(); }

#if ENABLE(CSS_VARIABLES)
    void setVariable(const AtomicString& name, const String& value) { rareInheritedData.access()->m_variables.access()->setVariable(name, value); }
    const HashMap<AtomicString, String>* variables() { return &(rareInheritedData->m_variables->m_data); }
#endif

    bool affectedByHover() const { return noninherited_flags.affectedByHover(); }
    bool affectedByActive() const { return noninherited_flags.affectedByActive(); }
    bool affectedByDrag() const { return noninherited_flags.affectedByDrag(); }

    void setAffectedByHover() { noninherited_flags.setAffectedByHover(true); }
    void setAffectedByActive() { noninherited_flags.setAffectedByActive(true); }
    void setAffectedByDrag() { noninherited_flags.setAffectedByDrag(true); }

    void setColumnStylesFromPaginationMode(const Pagination::Mode&);
    
    bool operator==(const RenderStyle& other) const;
    bool operator!=(const RenderStyle& other) const { return !(*this == other); }
    bool isFloating() const { return noninherited_flags._floating != NoFloat; }
    bool hasMargin() const { return surround->margin.nonZero(); }
    bool hasBorder() const { return surround->border.hasBorder(); }
    bool hasPadding() const { return surround->padding.nonZero(); }
    bool hasOffset() const { return surround->offset.nonZero(); }
    bool hasMarginBeforeQuirk() const { return marginBefore().quirk(); }
    bool hasMarginAfterQuirk() const { return marginAfter().quirk(); }

    bool hasBackgroundImage() const { return m_background->background().hasImage(); }
    bool hasFixedBackgroundImage() const { return m_background->background().hasFixedImage(); }
    
    bool hasEntirelyFixedBackground() const;
    
    bool hasAppearance() const { return appearance() != NoControlPart; }

    bool hasBackground() const
    {
        Color color = visitedDependentColor(CSSPropertyBackgroundColor);
        if (color.isValid() && color.alpha())
            return true;
        return hasBackgroundImage();
    }
    
    LayoutBoxExtent imageOutsets(const NinePieceImage&) const;
    bool hasBorderImageOutsets() const
    {
        return borderImage().hasImage() && borderImage().outset().nonZero();
    }
    LayoutBoxExtent borderImageOutsets() const
    {
        return imageOutsets(borderImage());
    }

    LayoutBoxExtent maskBoxImageOutsets() const
    {
        return imageOutsets(maskBoxImage());
    }

#if ENABLE(CSS_FILTERS)
    bool hasFilterOutsets() const { return hasFilter() && filter().hasOutsets(); }
    FilterOutsets filterOutsets() const { return hasFilter() ? filter().outsets() : FilterOutsets(); }
#endif

    Order rtlOrdering() const { return static_cast<Order>(inherited_flags.m_rtlOrdering); }
    void setRTLOrdering(Order o) { inherited_flags.m_rtlOrdering = o; }

    bool isStyleAvailable() const;

    bool hasAnyPublicPseudoStyles() const;
    bool hasPseudoStyle(PseudoId pseudo) const;
    void setHasPseudoStyle(PseudoId pseudo);
    bool hasUniquePseudoStyle() const;

    // attribute getter methods

    EDisplay display() const { return static_cast<EDisplay>(noninherited_flags._effectiveDisplay); }
    EDisplay originalDisplay() const { return static_cast<EDisplay>(noninherited_flags._originalDisplay); }

    Length left() const { return surround->offset.left(); }
    Length right() const { return surround->offset.right(); }
    Length top() const { return surround->offset.top(); }
    Length bottom() const { return surround->offset.bottom(); }

    // Accessors for positioned object edges that take into account writing mode.
    Length logicalLeft() const { return surround->offset.logicalLeft(writingMode()); }
    Length logicalRight() const { return surround->offset.logicalRight(writingMode()); }
    Length logicalTop() const { return surround->offset.before(writingMode()); }
    Length logicalBottom() const { return surround->offset.after(writingMode()); }

    // Whether or not a positioned element requires normal flow x/y to be computed
    // to determine its position.
    bool hasAutoLeftAndRight() const { return left().isAuto() && right().isAuto(); }
    bool hasAutoTopAndBottom() const { return top().isAuto() && bottom().isAuto(); }
    bool hasStaticInlinePosition(bool horizontal) const { return horizontal ? hasAutoLeftAndRight() : hasAutoTopAndBottom(); }
    bool hasStaticBlockPosition(bool horizontal) const { return horizontal ? hasAutoTopAndBottom() : hasAutoLeftAndRight(); }

    EPosition position() const { return static_cast<EPosition>(noninherited_flags._position); }
    bool hasOutOfFlowPosition() const { return position() == AbsolutePosition || position() == FixedPosition; }
    bool hasInFlowPosition() const { return position() == RelativePosition || position() == StickyPosition; }
    bool hasViewportConstrainedPosition() const { return position() == FixedPosition || position() == StickyPosition; }
    EFloat floating() const { return static_cast<EFloat>(noninherited_flags._floating); }

    Length width() const { return m_box->width(); }
    Length height() const { return m_box->height(); }
    Length minWidth() const { return m_box->minWidth(); }
    Length maxWidth() const { return m_box->maxWidth(); }
    Length minHeight() const { return m_box->minHeight(); }
    Length maxHeight() const { return m_box->maxHeight(); }
    
    Length logicalWidth() const { return isHorizontalWritingMode() ? width() : height(); }
    Length logicalHeight() const { return isHorizontalWritingMode() ? height() : width(); }
    Length logicalMinWidth() const { return isHorizontalWritingMode() ? minWidth() : minHeight(); }
    Length logicalMaxWidth() const { return isHorizontalWritingMode() ? maxWidth() : maxHeight(); }
    Length logicalMinHeight() const { return isHorizontalWritingMode() ? minHeight() : minWidth(); }
    Length logicalMaxHeight() const { return isHorizontalWritingMode() ? maxHeight() : maxWidth(); }

    const BorderData& border() const { return surround->border; }
    const BorderValue& borderLeft() const { return surround->border.left(); }
    const BorderValue& borderRight() const { return surround->border.right(); }
    const BorderValue& borderTop() const { return surround->border.top(); }
    const BorderValue& borderBottom() const { return surround->border.bottom(); }

    const BorderValue& borderBefore() const;
    const BorderValue& borderAfter() const;
    const BorderValue& borderStart() const;
    const BorderValue& borderEnd() const;

    const NinePieceImage& borderImage() const { return surround->border.image(); }
    StyleImage* borderImageSource() const { return surround->border.image().image(); }
    LengthBox borderImageSlices() const { return surround->border.image().imageSlices(); }
    LengthBox borderImageWidth() const { return surround->border.image().borderSlices(); }
    LengthBox borderImageOutset() const { return surround->border.image().outset(); }

    LengthSize borderTopLeftRadius() const { return surround->border.topLeft(); }
    LengthSize borderTopRightRadius() const { return surround->border.topRight(); }
    LengthSize borderBottomLeftRadius() const { return surround->border.bottomLeft(); }
    LengthSize borderBottomRightRadius() const { return surround->border.bottomRight(); }
    bool hasBorderRadius() const { return surround->border.hasBorderRadius(); }

    unsigned borderLeftWidth() const { return surround->border.borderLeftWidth(); }
    EBorderStyle borderLeftStyle() const { return surround->border.left().style(); }
    bool borderLeftIsTransparent() const { return surround->border.left().isTransparent(); }
    unsigned borderRightWidth() const { return surround->border.borderRightWidth(); }
    EBorderStyle borderRightStyle() const { return surround->border.right().style(); }
    bool borderRightIsTransparent() const { return surround->border.right().isTransparent(); }
    unsigned borderTopWidth() const { return surround->border.borderTopWidth(); }
    EBorderStyle borderTopStyle() const { return surround->border.top().style(); }
    bool borderTopIsTransparent() const { return surround->border.top().isTransparent(); }
    unsigned borderBottomWidth() const { return surround->border.borderBottomWidth(); }
    EBorderStyle borderBottomStyle() const { return surround->border.bottom().style(); }
    bool borderBottomIsTransparent() const { return surround->border.bottom().isTransparent(); }
    
    unsigned short borderBeforeWidth() const;
    unsigned short borderAfterWidth() const;
    unsigned short borderStartWidth() const;
    unsigned short borderEndWidth() const;

    unsigned short outlineSize() const { return max(0, outlineWidth() + outlineOffset()); }
    unsigned short outlineWidth() const
    {
        if (m_background->outline().style() == BNONE)
            return 0;
        return m_background->outline().width();
    }
    bool hasOutline() const { return outlineWidth() > 0 && outlineStyle() > BHIDDEN; }
    EBorderStyle outlineStyle() const { return m_background->outline().style(); }
    OutlineIsAuto outlineStyleIsAuto() const { return static_cast<OutlineIsAuto>(m_background->outline().isAuto()); }
    
    EOverflow overflowX() const { return static_cast<EOverflow>(noninherited_flags._overflowX); }
    EOverflow overflowY() const { return static_cast<EOverflow>(noninherited_flags._overflowY); }

    EVisibility visibility() const { return static_cast<EVisibility>(inherited_flags._visibility); }
    EVerticalAlign verticalAlign() const { return static_cast<EVerticalAlign>(noninherited_flags._vertical_align); }
    Length verticalAlignLength() const { return m_box->verticalAlign(); }

    Length clipLeft() const { return visual->clip.left(); }
    Length clipRight() const { return visual->clip.right(); }
    Length clipTop() const { return visual->clip.top(); }
    Length clipBottom() const { return visual->clip.bottom(); }
    LengthBox clip() const { return visual->clip; }
    bool hasClip() const { return visual->hasClip; }

    EUnicodeBidi unicodeBidi() const { return static_cast<EUnicodeBidi>(noninherited_flags._unicodeBidi); }

    EClear clear() const { return static_cast<EClear>(noninherited_flags._clear); }
    ETableLayout tableLayout() const { return static_cast<ETableLayout>(noninherited_flags._table_layout); }

    const Font& font() const;
    const FontMetrics& fontMetrics() const;
    const FontDescription& fontDescription() const;
    float specifiedFontSize() const;
    float computedFontSize() const;
    int fontSize() const;

#if ENABLE(TEXT_AUTOSIZING)
    float textAutosizingMultiplier() const { return visual->m_textAutosizingMultiplier; }
#endif

    Length textIndent() const { return rareInheritedData->indent; }
#if ENABLE(CSS3_TEXT)
    TextIndentLine textIndentLine() const { return static_cast<TextIndentLine>(rareInheritedData->m_textIndentLine); }
    TextIndentType textIndentType() const { return static_cast<TextIndentType>(rareInheritedData->m_textIndentType); }
#endif
    ETextAlign textAlign() const { return static_cast<ETextAlign>(inherited_flags._text_align); }
    ETextTransform textTransform() const { return static_cast<ETextTransform>(inherited_flags._text_transform); }
    TextDecoration textDecorationsInEffect() const { return static_cast<TextDecoration>(inherited_flags._text_decorations); }
    TextDecoration textDecoration() const { return static_cast<TextDecoration>(visual->textDecoration); }
#if ENABLE(CSS3_TEXT)
    TextDecorationStyle textDecorationStyle() const { return static_cast<TextDecorationStyle>(rareNonInheritedData->m_textDecorationStyle); }
    TextAlignLast textAlignLast() const { return static_cast<TextAlignLast>(rareInheritedData->m_textAlignLast); }
    TextJustify textJustify() const { return static_cast<TextJustify>(rareInheritedData->m_textJustify); }
    TextUnderlinePosition textUnderlinePosition() const { return static_cast<TextUnderlinePosition>(rareInheritedData->m_textUnderlinePosition); }
#else
    TextDecorationStyle textDecorationStyle() const { return TextDecorationStyleSolid; }
#endif // CSS3_TEXT
    int wordSpacing() const;
    int letterSpacing() const;

    float zoom() const { return visual->m_zoom; }
    float effectiveZoom() const { return rareInheritedData->m_effectiveZoom; }

    TextDirection direction() const { return static_cast<TextDirection>(inherited_flags._direction); }
    bool isLeftToRightDirection() const { return direction() == LTR; }

    Length specifiedLineHeight() const;
    Length lineHeight() const;
    int computedLineHeight(RenderView* = 0) const;

    EWhiteSpace whiteSpace() const { return static_cast<EWhiteSpace>(inherited_flags._white_space); }
    static bool autoWrap(EWhiteSpace ws)
    {
        // Nowrap and pre don't automatically wrap.
        return ws != NOWRAP && ws != PRE;
    }

    bool autoWrap() const
    {
        return autoWrap(whiteSpace());
    }

    static bool preserveNewline(EWhiteSpace ws)
    {
        // Normal and nowrap do not preserve newlines.
        return ws != NORMAL && ws != NOWRAP;
    }

    bool preserveNewline() const
    {
        return preserveNewline(whiteSpace());
    }

    static bool collapseWhiteSpace(EWhiteSpace ws)
    {
        // Pre and prewrap do not collapse whitespace.
        return ws != PRE && ws != PRE_WRAP;
    }

    bool collapseWhiteSpace() const
    {
        return collapseWhiteSpace(whiteSpace());
    }

    bool isCollapsibleWhiteSpace(UChar c) const
    {
        switch (c) {
            case ' ':
            case '\t':
                return collapseWhiteSpace();
            case '\n':
                return !preserveNewline();
        }
        return false;
    }

    bool breakOnlyAfterWhiteSpace() const
    {
        return whiteSpace() == PRE_WRAP || lineBreak() == LineBreakAfterWhiteSpace;
    }

    bool breakWords() const
    {
        return wordBreak() == BreakWordBreak || overflowWrap() == BreakOverflowWrap;
    }

    EFillRepeat backgroundRepeatX() const { return static_cast<EFillRepeat>(m_background->background().repeatX()); }
    EFillRepeat backgroundRepeatY() const { return static_cast<EFillRepeat>(m_background->background().repeatY()); }
    CompositeOperator backgroundComposite() const { return static_cast<CompositeOperator>(m_background->background().composite()); }
    EFillAttachment backgroundAttachment() const { return static_cast<EFillAttachment>(m_background->background().attachment()); }
    EFillBox backgroundClip() const { return static_cast<EFillBox>(m_background->background().clip()); }
    EFillBox backgroundOrigin() const { return static_cast<EFillBox>(m_background->background().origin()); }
    Length backgroundXPosition() const { return m_background->background().xPosition(); }
    Length backgroundYPosition() const { return m_background->background().yPosition(); }
    EFillSizeType backgroundSizeType() const { return m_background->background().sizeType(); }
    LengthSize backgroundSizeLength() const { return m_background->background().sizeLength(); }
    FillLayer* accessBackgroundLayers() { return &(m_background.access()->m_background); }
    const FillLayer* backgroundLayers() const { return &(m_background->background()); }

    StyleImage* maskImage() const { return rareNonInheritedData->m_mask.image(); }
    EFillRepeat maskRepeatX() const { return static_cast<EFillRepeat>(rareNonInheritedData->m_mask.repeatX()); }
    EFillRepeat maskRepeatY() const { return static_cast<EFillRepeat>(rareNonInheritedData->m_mask.repeatY()); }
    CompositeOperator maskComposite() const { return static_cast<CompositeOperator>(rareNonInheritedData->m_mask.composite()); }
    EFillBox maskClip() const { return static_cast<EFillBox>(rareNonInheritedData->m_mask.clip()); }
    EFillBox maskOrigin() const { return static_cast<EFillBox>(rareNonInheritedData->m_mask.origin()); }
    Length maskXPosition() const { return rareNonInheritedData->m_mask.xPosition(); }
    Length maskYPosition() const { return rareNonInheritedData->m_mask.yPosition(); }
    EFillSizeType maskSizeType() const { return rareNonInheritedData->m_mask.sizeType(); }
    LengthSize maskSizeLength() const { return rareNonInheritedData->m_mask.sizeLength(); }
    FillLayer* accessMaskLayers() { return &(rareNonInheritedData.access()->m_mask); }
    const FillLayer* maskLayers() const { return &(rareNonInheritedData->m_mask); }
    const NinePieceImage& maskBoxImage() const { return rareNonInheritedData->m_maskBoxImage; }
    StyleImage* maskBoxImageSource() const { return rareNonInheritedData->m_maskBoxImage.image(); }
 
    EBorderCollapse borderCollapse() const { return static_cast<EBorderCollapse>(inherited_flags._border_collapse); }
    short horizontalBorderSpacing() const;
    short verticalBorderSpacing() const;
    EEmptyCell emptyCells() const { return static_cast<EEmptyCell>(inherited_flags._empty_cells); }
    ECaptionSide captionSide() const { return static_cast<ECaptionSide>(inherited_flags._caption_side); }

    EListStyleType listStyleType() const { return static_cast<EListStyleType>(inherited_flags._list_style_type); }
    StyleImage* listStyleImage() const;
    EListStylePosition listStylePosition() const { return static_cast<EListStylePosition>(inherited_flags._list_style_position); }

    Length marginTop() const { return surround->margin.top(); }
    Length marginBottom() const { return surround->margin.bottom(); }
    Length marginLeft() const { return surround->margin.left(); }
    Length marginRight() const { return surround->margin.right(); }
    Length marginBefore() const { return surround->margin.before(writingMode()); }
    Length marginAfter() const { return surround->margin.after(writingMode()); }
    Length marginStart() const { return surround->margin.start(writingMode(), direction()); }
    Length marginEnd() const { return surround->margin.end(writingMode(), direction()); }
    Length marginStartUsing(const RenderStyle* otherStyle) const { return surround->margin.start(otherStyle->writingMode(), otherStyle->direction()); }
    Length marginEndUsing(const RenderStyle* otherStyle) const { return surround->margin.end(otherStyle->writingMode(), otherStyle->direction()); }
    Length marginBeforeUsing(const RenderStyle* otherStyle) const { return surround->margin.before(otherStyle->writingMode()); }
    Length marginAfterUsing(const RenderStyle* otherStyle) const { return surround->margin.after(otherStyle->writingMode()); }

    LengthBox paddingBox() const { return surround->padding; }
    Length paddingTop() const { return surround->padding.top(); }
    Length paddingBottom() const { return surround->padding.bottom(); }
    Length paddingLeft() const { return surround->padding.left(); }
    Length paddingRight() const { return surround->padding.right(); }
    Length paddingBefore() const { return surround->padding.before(writingMode()); }
    Length paddingAfter() const { return surround->padding.after(writingMode()); }
    Length paddingStart() const { return surround->padding.start(writingMode(), direction()); }
    Length paddingEnd() const { return surround->padding.end(writingMode(), direction()); }

    ECursor cursor() const { return static_cast<ECursor>(inherited_flags._cursor_style); }
#if ENABLE(CURSOR_VISIBILITY)
    CursorVisibility cursorVisibility() const { return static_cast<CursorVisibility>(inherited_flags.m_cursorVisibility); }
#endif

    CursorList* cursors() const { return rareInheritedData->cursorData.get(); }

    EInsideLink insideLink() const { return static_cast<EInsideLink>(inherited_flags._insideLink); }
    bool isLink() const { return noninherited_flags.isLink(); }

    short widows() const { return rareInheritedData->widows; }
    short orphans() const { return rareInheritedData->orphans; }
    bool hasAutoWidows() const { return rareInheritedData->m_hasAutoWidows; }
    bool hasAutoOrphans() const { return rareInheritedData->m_hasAutoOrphans; }
    EPageBreak pageBreakInside() const { return static_cast<EPageBreak>(noninherited_flags._page_break_inside); }
    EPageBreak pageBreakBefore() const { return static_cast<EPageBreak>(noninherited_flags._page_break_before); }
    EPageBreak pageBreakAfter() const { return static_cast<EPageBreak>(noninherited_flags._page_break_after); }

    // CSS3 Getter Methods

    int outlineOffset() const
    {
        if (m_background->outline().style() == BNONE)
            return 0;
        return m_background->outline().offset();
    }

    const ShadowData* textShadow() const { return rareInheritedData->textShadow.get(); }
    void getTextShadowExtent(LayoutUnit& top, LayoutUnit& right, LayoutUnit& bottom, LayoutUnit& left) const { getShadowExtent(textShadow(), top, right, bottom, left); }
    void getTextShadowHorizontalExtent(LayoutUnit& left, LayoutUnit& right) const { getShadowHorizontalExtent(textShadow(), left, right); }
    void getTextShadowVerticalExtent(LayoutUnit& top, LayoutUnit& bottom) const { getShadowVerticalExtent(textShadow(), top, bottom); }
    void getTextShadowInlineDirectionExtent(LayoutUnit& logicalLeft, LayoutUnit& logicalRight) { getShadowInlineDirectionExtent(textShadow(), logicalLeft, logicalRight); }
    void getTextShadowBlockDirectionExtent(LayoutUnit& logicalTop, LayoutUnit& logicalBottom) { getShadowBlockDirectionExtent(textShadow(), logicalTop, logicalBottom); }

    float textStrokeWidth() const { return rareInheritedData->textStrokeWidth; }
    ColorSpace colorSpace() const { return static_cast<ColorSpace>(rareInheritedData->colorSpace); }
    float opacity() const { return rareNonInheritedData->opacity; }
    ControlPart appearance() const { return static_cast<ControlPart>(rareNonInheritedData->m_appearance); }
    // aspect ratio convenience method
    bool hasAspectRatio() const { return rareNonInheritedData->m_hasAspectRatio; }
    float aspectRatio() const { return aspectRatioNumerator() / aspectRatioDenominator(); }
    float aspectRatioDenominator() const { return rareNonInheritedData->m_aspectRatioDenominator; }
    float aspectRatioNumerator() const { return rareNonInheritedData->m_aspectRatioNumerator; }
    EBoxAlignment boxAlign() const { return static_cast<EBoxAlignment>(rareNonInheritedData->m_deprecatedFlexibleBox->align); }
    EBoxDirection boxDirection() const { return static_cast<EBoxDirection>(inherited_flags._box_direction); }
    float boxFlex() const { return rareNonInheritedData->m_deprecatedFlexibleBox->flex; }
    unsigned int boxFlexGroup() const { return rareNonInheritedData->m_deprecatedFlexibleBox->flex_group; }
    EBoxLines boxLines() const { return static_cast<EBoxLines>(rareNonInheritedData->m_deprecatedFlexibleBox->lines); }
    unsigned int boxOrdinalGroup() const { return rareNonInheritedData->m_deprecatedFlexibleBox->ordinal_group; }
    EBoxOrient boxOrient() const { return static_cast<EBoxOrient>(rareNonInheritedData->m_deprecatedFlexibleBox->orient); }
    EBoxPack boxPack() const { return static_cast<EBoxPack>(rareNonInheritedData->m_deprecatedFlexibleBox->pack); }

    int order() const { return rareNonInheritedData->m_order; }
    float flexGrow() const { return rareNonInheritedData->m_flexibleBox->m_flexGrow; }
    float flexShrink() const { return rareNonInheritedData->m_flexibleBox->m_flexShrink; }
    Length flexBasis() const { return rareNonInheritedData->m_flexibleBox->m_flexBasis; }
    EAlignContent alignContent() const { return static_cast<EAlignContent>(rareNonInheritedData->m_alignContent); }
    EAlignItems alignItems() const { return static_cast<EAlignItems>(rareNonInheritedData->m_alignItems); }
    EAlignItems alignSelf() const { return static_cast<EAlignItems>(rareNonInheritedData->m_alignSelf); }
    EFlexDirection flexDirection() const { return static_cast<EFlexDirection>(rareNonInheritedData->m_flexibleBox->m_flexDirection); }
    bool isColumnFlexDirection() const { return flexDirection() == FlowColumn || flexDirection() == FlowColumnReverse; }
    bool isReverseFlexDirection() const { return flexDirection() == FlowRowReverse || flexDirection() == FlowColumnReverse; }
    EFlexWrap flexWrap() const { return static_cast<EFlexWrap>(rareNonInheritedData->m_flexibleBox->m_flexWrap); }
    EJustifyContent justifyContent() const { return static_cast<EJustifyContent>(rareNonInheritedData->m_justifyContent); }

    const Vector<GridTrackSize>& gridColumns() const { return rareNonInheritedData->m_grid->m_gridColumns; }
    const Vector<GridTrackSize>& gridRows() const { return rareNonInheritedData->m_grid->m_gridRows; }
    GridAutoFlow gridAutoFlow() const { return rareNonInheritedData->m_grid->m_gridAutoFlow; }
    const GridTrackSize& gridAutoColumns() const { return rareNonInheritedData->m_grid->m_gridAutoColumns; }
    const GridTrackSize& gridAutoRows() const { return rareNonInheritedData->m_grid->m_gridAutoRows; }

    const GridPosition& gridItemStart() const { return rareNonInheritedData->m_gridItem->m_gridStart; }
    const GridPosition& gridItemEnd() const { return rareNonInheritedData->m_gridItem->m_gridEnd; }
    const GridPosition& gridItemBefore() const { return rareNonInheritedData->m_gridItem->m_gridBefore; }
    const GridPosition& gridItemAfter() const { return rareNonInheritedData->m_gridItem->m_gridAfter; }

    const ShadowData* boxShadow() const { return rareNonInheritedData->m_boxShadow.get(); }
    void getBoxShadowExtent(LayoutUnit& top, LayoutUnit& right, LayoutUnit& bottom, LayoutUnit& left) const { getShadowExtent(boxShadow(), top, right, bottom, left); }
    LayoutBoxExtent getBoxShadowInsetExtent() const { return getShadowInsetExtent(boxShadow()); }
    void getBoxShadowHorizontalExtent(LayoutUnit& left, LayoutUnit& right) const { getShadowHorizontalExtent(boxShadow(), left, right); }
    void getBoxShadowVerticalExtent(LayoutUnit& top, LayoutUnit& bottom) const { getShadowVerticalExtent(boxShadow(), top, bottom); }
    void getBoxShadowInlineDirectionExtent(LayoutUnit& logicalLeft, LayoutUnit& logicalRight) { getShadowInlineDirectionExtent(boxShadow(), logicalLeft, logicalRight); }
    void getBoxShadowBlockDirectionExtent(LayoutUnit& logicalTop, LayoutUnit& logicalBottom) { getShadowBlockDirectionExtent(boxShadow(), logicalTop, logicalBottom); }

#if ENABLE(CSS_BOX_DECORATION_BREAK)
    EBoxDecorationBreak boxDecorationBreak() const { return m_box->boxDecorationBreak(); }
#endif
    StyleReflection* boxReflect() const { return rareNonInheritedData->m_boxReflect.get(); }
    EBoxSizing boxSizing() const { return m_box->boxSizing(); }
    Length marqueeIncrement() const { return rareNonInheritedData->m_marquee->increment; }
    int marqueeSpeed() const { return rareNonInheritedData->m_marquee->speed; }
    int marqueeLoopCount() const { return rareNonInheritedData->m_marquee->loops; }
    EMarqueeBehavior marqueeBehavior() const { return static_cast<EMarqueeBehavior>(rareNonInheritedData->m_marquee->behavior); }
    EMarqueeDirection marqueeDirection() const { return static_cast<EMarqueeDirection>(rareNonInheritedData->m_marquee->direction); }
    EUserModify userModify() const { return static_cast<EUserModify>(rareInheritedData->userModify); }
    EUserDrag userDrag() const { return static_cast<EUserDrag>(rareNonInheritedData->userDrag); }
    EUserSelect userSelect() const { return static_cast<EUserSelect>(rareInheritedData->userSelect); }
    TextOverflow textOverflow() const { return static_cast<TextOverflow>(rareNonInheritedData->textOverflow); }
    EMarginCollapse marginBeforeCollapse() const { return static_cast<EMarginCollapse>(rareNonInheritedData->marginBeforeCollapse); }
    EMarginCollapse marginAfterCollapse() const { return static_cast<EMarginCollapse>(rareNonInheritedData->marginAfterCollapse); }
    EWordBreak wordBreak() const { return static_cast<EWordBreak>(rareInheritedData->wordBreak); }
    EOverflowWrap overflowWrap() const { return static_cast<EOverflowWrap>(rareInheritedData->overflowWrap); }
    ENBSPMode nbspMode() const { return static_cast<ENBSPMode>(rareInheritedData->nbspMode); }
    LineBreak lineBreak() const { return static_cast<LineBreak>(rareInheritedData->lineBreak); }
    const AtomicString& highlight() const { return rareInheritedData->highlight; }
    Hyphens hyphens() const { return static_cast<Hyphens>(rareInheritedData->hyphens); }
    short hyphenationLimitBefore() const { return rareInheritedData->hyphenationLimitBefore; }
    short hyphenationLimitAfter() const { return rareInheritedData->hyphenationLimitAfter; }
    short hyphenationLimitLines() const { return rareInheritedData->hyphenationLimitLines; }
    const AtomicString& hyphenationString() const { return rareInheritedData->hyphenationString; }
    const AtomicString& locale() const { return rareInheritedData->locale; }
    EBorderFit borderFit() const { return static_cast<EBorderFit>(rareNonInheritedData->m_borderFit); }
    EResize resize() const { return static_cast<EResize>(rareInheritedData->resize); }
    ColumnAxis columnAxis() const { return static_cast<ColumnAxis>(rareNonInheritedData->m_multiCol->m_axis); }
    bool hasInlineColumnAxis() const {
        ColumnAxis axis = columnAxis();
        return axis == AutoColumnAxis || isHorizontalWritingMode() == (axis == HorizontalColumnAxis);
    }
    ColumnProgression columnProgression() const { return static_cast<ColumnProgression>(rareNonInheritedData->m_multiCol->m_progression); }
    float columnWidth() const { return rareNonInheritedData->m_multiCol->m_width; }
    bool hasAutoColumnWidth() const { return rareNonInheritedData->m_multiCol->m_autoWidth; }
    unsigned short columnCount() const { return rareNonInheritedData->m_multiCol->m_count; }
    bool hasAutoColumnCount() const { return rareNonInheritedData->m_multiCol->m_autoCount; }
    bool specifiesColumns() const { return !hasAutoColumnCount() || !hasAutoColumnWidth() || !hasInlineColumnAxis(); }
    float columnGap() const { return rareNonInheritedData->m_multiCol->m_gap; }
    bool hasNormalColumnGap() const { return rareNonInheritedData->m_multiCol->m_normalGap; }
    EBorderStyle columnRuleStyle() const { return rareNonInheritedData->m_multiCol->m_rule.style(); }
    unsigned short columnRuleWidth() const { return rareNonInheritedData->m_multiCol->ruleWidth(); }
    bool columnRuleIsTransparent() const { return rareNonInheritedData->m_multiCol->m_rule.isTransparent(); }
    ColumnSpan columnSpan() const { return static_cast<ColumnSpan>(rareNonInheritedData->m_multiCol->m_columnSpan); }
    EPageBreak columnBreakBefore() const { return static_cast<EPageBreak>(rareNonInheritedData->m_multiCol->m_breakBefore); }
    EPageBreak columnBreakInside() const { return static_cast<EPageBreak>(rareNonInheritedData->m_multiCol->m_breakInside); }
    EPageBreak columnBreakAfter() const { return static_cast<EPageBreak>(rareNonInheritedData->m_multiCol->m_breakAfter); }
    EPageBreak regionBreakBefore() const { return static_cast<EPageBreak>(rareNonInheritedData->m_regionBreakBefore); }
    EPageBreak regionBreakInside() const { return static_cast<EPageBreak>(rareNonInheritedData->m_regionBreakInside); }
    EPageBreak regionBreakAfter() const { return static_cast<EPageBreak>(rareNonInheritedData->m_regionBreakAfter); }
    const TransformOperations& transform() const { return rareNonInheritedData->m_transform->m_operations; }
    Length transformOriginX() const { return rareNonInheritedData->m_transform->m_x; }
    Length transformOriginY() const { return rareNonInheritedData->m_transform->m_y; }
    float transformOriginZ() const { return rareNonInheritedData->m_transform->m_z; }
    bool hasTransform() const { return !rareNonInheritedData->m_transform->m_operations.operations().isEmpty(); }

    TextEmphasisFill textEmphasisFill() const { return static_cast<TextEmphasisFill>(rareInheritedData->textEmphasisFill); }
    TextEmphasisMark textEmphasisMark() const;
    const AtomicString& textEmphasisCustomMark() const { return rareInheritedData->textEmphasisCustomMark; }
    TextEmphasisPosition textEmphasisPosition() const { return static_cast<TextEmphasisPosition>(rareInheritedData->textEmphasisPosition); }
    const AtomicString& textEmphasisMarkString() const;

    RubyPosition rubyPosition() const { return static_cast<RubyPosition>(rareInheritedData->m_rubyPosition); }

    TextOrientation textOrientation() const { return static_cast<TextOrientation>(rareInheritedData->m_textOrientation); }
    
    // Return true if any transform related property (currently transform, transformStyle3D or perspective) 
    // indicates that we are transforming
    bool hasTransformRelatedProperty() const { return hasTransform() || preserves3D() || hasPerspective(); }

    enum ApplyTransformOrigin { IncludeTransformOrigin, ExcludeTransformOrigin };
    void applyTransform(TransformationMatrix&, const LayoutSize& borderBoxSize, ApplyTransformOrigin = IncludeTransformOrigin) const;
    void applyTransform(TransformationMatrix&, const FloatRect& boundingBox, ApplyTransformOrigin = IncludeTransformOrigin) const;
    void setPageScaleTransform(float);

    bool hasMask() const { return rareNonInheritedData->m_mask.hasImage() || rareNonInheritedData->m_maskBoxImage.hasImage(); }

    TextCombine textCombine() const { return static_cast<TextCombine>(rareNonInheritedData->m_textCombine); }
    bool hasTextCombine() const { return textCombine() != TextCombineNone; }

    unsigned tabSize() const { return rareInheritedData->m_tabSize; }

    // End CSS3 Getters

    const AtomicString& flowThread() const { return rareNonInheritedData->m_flowThread; }
    const AtomicString& regionThread() const { return rareNonInheritedData->m_regionThread; }
    RegionFragment regionFragment() const { return static_cast<RegionFragment>(rareNonInheritedData->m_regionFragment); }

    const AtomicString& lineGrid() const { return rareInheritedData->m_lineGrid; }
    LineSnap lineSnap() const { return static_cast<LineSnap>(rareInheritedData->m_lineSnap); }
    LineAlign lineAlign() const { return static_cast<LineAlign>(rareInheritedData->m_lineAlign); }

    WrapFlow wrapFlow() const { return static_cast<WrapFlow>(rareNonInheritedData->m_wrapFlow); }
    WrapThrough wrapThrough() const { return static_cast<WrapThrough>(rareNonInheritedData->m_wrapThrough); }

    // Apple-specific property getter methods
    EPointerEvents pointerEvents() const { return static_cast<EPointerEvents>(inherited_flags._pointerEvents); }
    const AnimationList* animations() const { return rareNonInheritedData->m_animations.get(); }
    const AnimationList* transitions() const { return rareNonInheritedData->m_transitions.get(); }

    AnimationList* accessAnimations();
    AnimationList* accessTransitions();

    bool hasAnimations() const { return rareNonInheritedData->m_animations && rareNonInheritedData->m_animations->size() > 0; }
    bool hasTransitions() const { return rareNonInheritedData->m_transitions && rareNonInheritedData->m_transitions->size() > 0; }

    // return the first found Animation (including 'all' transitions)
    const Animation* transitionForProperty(CSSPropertyID) const;

    ETransformStyle3D transformStyle3D() const { return static_cast<ETransformStyle3D>(rareNonInheritedData->m_transformStyle3D); }
    bool preserves3D() const { return rareNonInheritedData->m_transformStyle3D == TransformStyle3DPreserve3D; }

    EBackfaceVisibility backfaceVisibility() const { return static_cast<EBackfaceVisibility>(rareNonInheritedData->m_backfaceVisibility); }
    float perspective() const { return rareNonInheritedData->m_perspective; }
    bool hasPerspective() const { return rareNonInheritedData->m_perspective > 0; }
    Length perspectiveOriginX() const { return rareNonInheritedData->m_perspectiveOriginX; }
    Length perspectiveOriginY() const { return rareNonInheritedData->m_perspectiveOriginY; }
    LengthSize pageSize() const { return rareNonInheritedData->m_pageSize; }
    PageSizeType pageSizeType() const { return static_cast<PageSizeType>(rareNonInheritedData->m_pageSizeType); }
    
#if USE(ACCELERATED_COMPOSITING)
    // When set, this ensures that styles compare as different. Used during accelerated animations.
    bool isRunningAcceleratedAnimation() const { return rareNonInheritedData->m_runningAcceleratedAnimation; }
#endif

    LineBoxContain lineBoxContain() const { return rareInheritedData->m_lineBoxContain; }
    const LineClampValue& lineClamp() const { return rareNonInheritedData->lineClamp; }
#if ENABLE(TOUCH_EVENTS)
    Color tapHighlightColor() const { return rareInheritedData->tapHighlightColor; }
#endif
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    bool useTouchOverflowScrolling() const { return rareInheritedData->useTouchOverflowScrolling; }
#endif
    ETextSecurity textSecurity() const { return static_cast<ETextSecurity>(rareInheritedData->textSecurity); }

    WritingMode writingMode() const { return static_cast<WritingMode>(inherited_flags.m_writingMode); }
    bool isHorizontalWritingMode() const { return WebCore::isHorizontalWritingMode(writingMode()); }
    bool isFlippedLinesWritingMode() const { return WebCore::isFlippedLinesWritingMode(writingMode()); }
    bool isFlippedBlocksWritingMode() const { return WebCore::isFlippedBlocksWritingMode(writingMode()); }

#if ENABLE(CSS_IMAGE_ORIENTATION)
    ImageOrientationEnum imageOrientation() const { return static_cast<ImageOrientationEnum>(rareInheritedData->m_imageOrientation); }
#endif

    EImageRendering imageRendering() const { return static_cast<EImageRendering>(rareInheritedData->m_imageRendering); }

#if ENABLE(CSS_IMAGE_RESOLUTION)
    ImageResolutionSource imageResolutionSource() const { return static_cast<ImageResolutionSource>(rareInheritedData->m_imageResolutionSource); }
    ImageResolutionSnap imageResolutionSnap() const { return static_cast<ImageResolutionSnap>(rareInheritedData->m_imageResolutionSnap); }
    float imageResolution() const { return rareInheritedData->m_imageResolution; }
#endif
    
    ESpeak speak() const { return static_cast<ESpeak>(rareInheritedData->speak); }

#if ENABLE(CSS_FILTERS)
    FilterOperations& mutableFilter() { return rareNonInheritedData.access()->m_filter.access()->m_operations; }
    const FilterOperations& filter() const { return rareNonInheritedData->m_filter->m_operations; }
    bool hasFilter() const { return !rareNonInheritedData->m_filter->m_operations.operations().isEmpty(); }
#else
    bool hasFilter() const { return false; }
#endif

#if ENABLE(CSS_COMPOSITING)
    BlendMode blendMode() const { return static_cast<BlendMode>(rareNonInheritedData->m_effectiveBlendMode); }
    void setBlendMode(BlendMode v) { rareNonInheritedData.access()->m_effectiveBlendMode = v; }
    bool hasBlendMode() const { return static_cast<BlendMode>(rareNonInheritedData->m_effectiveBlendMode) != BlendModeNormal; }
#else
    bool hasBlendMode() const { return false; }
#endif
 
#if USE(RTL_SCROLLBAR)
    bool shouldPlaceBlockDirectionScrollbarOnLogicalLeft() const { return !isLeftToRightDirection() && isHorizontalWritingMode(); }
#else
    bool shouldPlaceBlockDirectionScrollbarOnLogicalLeft() const { return false; }
#endif
        
// attribute setter methods

    void setDisplay(EDisplay v) { noninherited_flags._effectiveDisplay = v; }
    void setOriginalDisplay(EDisplay v) { noninherited_flags._originalDisplay = v; }
    void setPosition(EPosition v) { noninherited_flags._position = v; }
    void setFloating(EFloat v) { noninherited_flags._floating = v; }

    void setLeft(Length v) { SET_VAR(surround, offset.m_left, v); }
    void setRight(Length v) { SET_VAR(surround, offset.m_right, v); }
    void setTop(Length v) { SET_VAR(surround, offset.m_top, v); }
    void setBottom(Length v) { SET_VAR(surround, offset.m_bottom, v); }

    void setWidth(Length v) { SET_VAR(m_box, m_width, v); }
    void setHeight(Length v) { SET_VAR(m_box, m_height, v); }

    void setLogicalWidth(Length v)
    {
        if (isHorizontalWritingMode()) {
            SET_VAR(m_box, m_width, v);
        } else {
            SET_VAR(m_box, m_height, v);
        }
    }

    void setLogicalHeight(Length v)
    {
        if (isHorizontalWritingMode()) {
            SET_VAR(m_box, m_height, v);
        } else {
            SET_VAR(m_box, m_width, v);
        }
    }

    void setMinWidth(Length v) { SET_VAR(m_box, m_minWidth, v); }
    void setMaxWidth(Length v) { SET_VAR(m_box, m_maxWidth, v); }
    void setMinHeight(Length v) { SET_VAR(m_box, m_minHeight, v); }
    void setMaxHeight(Length v) { SET_VAR(m_box, m_maxHeight, v); }

#if ENABLE(DASHBOARD_SUPPORT)
    Vector<StyleDashboardRegion> dashboardRegions() const { return rareNonInheritedData->m_dashboardRegions; }
    void setDashboardRegions(Vector<StyleDashboardRegion> regions) { SET_VAR(rareNonInheritedData, m_dashboardRegions, regions); }

    void setDashboardRegion(int type, const String& label, Length t, Length r, Length b, Length l, bool append)
    {
        StyleDashboardRegion region;
        region.label = label;
        region.offset.m_top = t;
        region.offset.m_right = r;
        region.offset.m_bottom = b;
        region.offset.m_left = l;
        region.type = type;
        if (!append)
            rareNonInheritedData.access()->m_dashboardRegions.clear();
        rareNonInheritedData.access()->m_dashboardRegions.append(region);
    }
#endif

#if ENABLE(DRAGGABLE_REGION)
    DraggableRegionMode getDraggableRegionMode() const { return rareNonInheritedData->m_draggableRegionMode; }
    void setDraggableRegionMode(DraggableRegionMode v) { SET_VAR(rareNonInheritedData, m_draggableRegionMode, v); }
#endif

    void resetBorder() { resetBorderImage(); resetBorderTop(); resetBorderRight(); resetBorderBottom(); resetBorderLeft(); resetBorderRadius(); }
    void resetBorderTop() { SET_VAR(surround, border.m_top, BorderValue()); }
    void resetBorderRight() { SET_VAR(surround, border.m_right, BorderValue()); }
    void resetBorderBottom() { SET_VAR(surround, border.m_bottom, BorderValue()); }
    void resetBorderLeft() { SET_VAR(surround, border.m_left, BorderValue()); }
    void resetBorderImage() { SET_VAR(surround, border.m_image, NinePieceImage()); }
    void resetBorderRadius() { resetBorderTopLeftRadius(); resetBorderTopRightRadius(); resetBorderBottomLeftRadius(); resetBorderBottomRightRadius(); }
    void resetBorderTopLeftRadius() { SET_VAR(surround, border.m_topLeft, initialBorderRadius()); }
    void resetBorderTopRightRadius() { SET_VAR(surround, border.m_topRight, initialBorderRadius()); }
    void resetBorderBottomLeftRadius() { SET_VAR(surround, border.m_bottomLeft, initialBorderRadius()); }
    void resetBorderBottomRightRadius() { SET_VAR(surround, border.m_bottomRight, initialBorderRadius()); }

    void setBackgroundColor(const Color& v) { SET_VAR(m_background, m_color, v); }

    void setBackgroundXPosition(Length length) { SET_VAR(m_background, m_background.m_xPosition, length); }
    void setBackgroundYPosition(Length length) { SET_VAR(m_background, m_background.m_yPosition, length); }
    void setBackgroundSize(EFillSizeType b) { SET_VAR(m_background, m_background.m_sizeType, b); }
    void setBackgroundSizeLength(LengthSize s) { SET_VAR(m_background, m_background.m_sizeLength, s); }
    
    void setBorderImage(const NinePieceImage& b) { SET_VAR(surround, border.m_image, b); }
    void setBorderImageSource(PassRefPtr<StyleImage>);
    void setBorderImageSlices(LengthBox);
    void setBorderImageWidth(LengthBox);
    void setBorderImageOutset(LengthBox);

    void setBorderTopLeftRadius(LengthSize s) { SET_VAR(surround, border.m_topLeft, s); }
    void setBorderTopRightRadius(LengthSize s) { SET_VAR(surround, border.m_topRight, s); }
    void setBorderBottomLeftRadius(LengthSize s) { SET_VAR(surround, border.m_bottomLeft, s); }
    void setBorderBottomRightRadius(LengthSize s) { SET_VAR(surround, border.m_bottomRight, s); }

    void setBorderRadius(LengthSize s)
    {
        setBorderTopLeftRadius(s);
        setBorderTopRightRadius(s);
        setBorderBottomLeftRadius(s);
        setBorderBottomRightRadius(s);
    }
    void setBorderRadius(const IntSize& s)
    {
        setBorderRadius(LengthSize(Length(s.width(), Fixed), Length(s.height(), Fixed)));
    }
    
    RoundedRect getRoundedBorderFor(const LayoutRect& borderRect, RenderView* = 0, bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true) const;
    RoundedRect getRoundedInnerBorderFor(const LayoutRect& borderRect, bool includeLogicalLeftEdge = true, bool includeLogicalRightEdge = true) const;

    RoundedRect getRoundedInnerBorderFor(const LayoutRect& borderRect,
        int topWidth, int bottomWidth, int leftWidth, int rightWidth, bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const;

    void setBorderLeftWidth(unsigned v) { SET_VAR(surround, border.m_left.m_width, v); }
    void setBorderLeftStyle(EBorderStyle v) { SET_VAR(surround, border.m_left.m_style, v); }
    void setBorderLeftColor(const Color& v) { SET_BORDERVALUE_COLOR(surround, border.m_left, v); }
    void setBorderRightWidth(unsigned v) { SET_VAR(surround, border.m_right.m_width, v); }
    void setBorderRightStyle(EBorderStyle v) { SET_VAR(surround, border.m_right.m_style, v); }
    void setBorderRightColor(const Color& v) { SET_BORDERVALUE_COLOR(surround, border.m_right, v); }
    void setBorderTopWidth(unsigned v) { SET_VAR(surround, border.m_top.m_width, v); }
    void setBorderTopStyle(EBorderStyle v) { SET_VAR(surround, border.m_top.m_style, v); }
    void setBorderTopColor(const Color& v) { SET_BORDERVALUE_COLOR(surround, border.m_top, v); }
    void setBorderBottomWidth(unsigned v) { SET_VAR(surround, border.m_bottom.m_width, v); }
    void setBorderBottomStyle(EBorderStyle v) { SET_VAR(surround, border.m_bottom.m_style, v); }
    void setBorderBottomColor(const Color& v) { SET_BORDERVALUE_COLOR(surround, border.m_bottom, v); }

    void setOutlineWidth(unsigned short v) { SET_VAR(m_background, m_outline.m_width, v); }
    void setOutlineStyleIsAuto(OutlineIsAuto isAuto) { SET_VAR(m_background, m_outline.m_isAuto, isAuto); }
    void setOutlineStyle(EBorderStyle v) { SET_VAR(m_background, m_outline.m_style, v); }
    void setOutlineColor(const Color& v) { SET_BORDERVALUE_COLOR(m_background, m_outline, v); }

    void setOverflowX(EOverflow v) { noninherited_flags._overflowX = v; }
    void setOverflowY(EOverflow v) { noninherited_flags._overflowY = v; }
    void setVisibility(EVisibility v) { inherited_flags._visibility = v; }
    void setVerticalAlign(EVerticalAlign v) { noninherited_flags._vertical_align = v; }
    void setVerticalAlignLength(Length length) { setVerticalAlign(LENGTH); SET_VAR(m_box, m_verticalAlign, length); }

    void setHasClip(bool b = true) { SET_VAR(visual, hasClip, b); }
    void setClipLeft(Length v) { SET_VAR(visual, clip.m_left, v); }
    void setClipRight(Length v) { SET_VAR(visual, clip.m_right, v); }
    void setClipTop(Length v) { SET_VAR(visual, clip.m_top, v); }
    void setClipBottom(Length v) { SET_VAR(visual, clip.m_bottom, v); }
    void setClip(Length top, Length right, Length bottom, Length left);
    void setClip(LengthBox box) { SET_VAR(visual, clip, box); }

    void setUnicodeBidi(EUnicodeBidi b) { noninherited_flags._unicodeBidi = b; }

    void setClear(EClear v) { noninherited_flags._clear = v; }
    void setTableLayout(ETableLayout v) { noninherited_flags._table_layout = v; }

    bool setFontDescription(const FontDescription&);
    // Only used for blending font sizes when animating, for MathML anonymous blocks, and for text autosizing.
    void setFontSize(float);

#if ENABLE(TEXT_AUTOSIZING)
    void setTextAutosizingMultiplier(float v)
    {
        SET_VAR(visual, m_textAutosizingMultiplier, v);
        setFontSize(fontDescription().specifiedSize());
    }
#endif

    void setColor(const Color&);
    void setTextIndent(Length v) { SET_VAR(rareInheritedData, indent, v); }
#if ENABLE(CSS3_TEXT)
    void setTextIndentLine(TextIndentLine v) { SET_VAR(rareInheritedData, m_textIndentLine, v); }
    void setTextIndentType(TextIndentType v) { SET_VAR(rareInheritedData, m_textIndentType, v); }
#endif
    void setTextAlign(ETextAlign v) { inherited_flags._text_align = v; }
    void setTextTransform(ETextTransform v) { inherited_flags._text_transform = v; }
    void addToTextDecorationsInEffect(TextDecoration v) { inherited_flags._text_decorations |= v; }
    void setTextDecorationsInEffect(TextDecoration v) { inherited_flags._text_decorations = v; }
    void setTextDecoration(TextDecoration v) { SET_VAR(visual, textDecoration, v); }
#if ENABLE(CSS3_TEXT)
    void setTextDecorationStyle(TextDecorationStyle v) { SET_VAR(rareNonInheritedData, m_textDecorationStyle, v); }
    void setTextAlignLast(TextAlignLast v) { SET_VAR(rareInheritedData, m_textAlignLast, v); }
    void setTextJustify(TextJustify v) { SET_VAR(rareInheritedData, m_textJustify, v); }
    void setTextUnderlinePosition(TextUnderlinePosition v) { SET_VAR(rareInheritedData, m_textUnderlinePosition, v); }
#endif // CSS3_TEXT
    void setDirection(TextDirection v) { inherited_flags._direction = v; }
    void setLineHeight(Length specifiedLineHeight);
    bool setZoom(float);
    void setZoomWithoutReturnValue(float f) { setZoom(f); }
    bool setEffectiveZoom(float);

#if ENABLE(CSS_IMAGE_ORIENTATION)
    void setImageOrientation(ImageOrientationEnum v) { SET_VAR(rareInheritedData, m_imageOrientation, static_cast<int>(v)); }
#endif

    void setImageRendering(EImageRendering v) { SET_VAR(rareInheritedData, m_imageRendering, v); }

#if ENABLE(CSS_IMAGE_RESOLUTION)
    void setImageResolutionSource(ImageResolutionSource v) { SET_VAR(rareInheritedData, m_imageResolutionSource, v); }
    void setImageResolutionSnap(ImageResolutionSnap v) { SET_VAR(rareInheritedData, m_imageResolutionSnap, v); }
    void setImageResolution(float f) { SET_VAR(rareInheritedData, m_imageResolution, f); }
#endif

    void setWhiteSpace(EWhiteSpace v) { inherited_flags._white_space = v; }

    void setWordSpacing(int);
    void setLetterSpacing(int);

    void clearBackgroundLayers() { m_background.access()->m_background = FillLayer(BackgroundFillLayer); }
    void inheritBackgroundLayers(const FillLayer& parent) { m_background.access()->m_background = parent; }

    void adjustBackgroundLayers()
    {
        if (backgroundLayers()->next()) {
            accessBackgroundLayers()->cullEmptyLayers();
            accessBackgroundLayers()->fillUnsetProperties();
        }
    }

    void clearMaskLayers() { rareNonInheritedData.access()->m_mask = FillLayer(MaskFillLayer); }
    void inheritMaskLayers(const FillLayer& parent) { rareNonInheritedData.access()->m_mask = parent; }

    void adjustMaskLayers()
    {
        if (maskLayers()->next()) {
            accessMaskLayers()->cullEmptyLayers();
            accessMaskLayers()->fillUnsetProperties();
        }
    }

    void setMaskImage(PassRefPtr<StyleImage> v) { rareNonInheritedData.access()->m_mask.setImage(v); }

    void setMaskBoxImage(const NinePieceImage& b) { SET_VAR(rareNonInheritedData, m_maskBoxImage, b); }
    void setMaskBoxImageSource(PassRefPtr<StyleImage> v) { rareNonInheritedData.access()->m_maskBoxImage.setImage(v); }
    void setMaskXPosition(Length length) { SET_VAR(rareNonInheritedData, m_mask.m_xPosition, length); }
    void setMaskYPosition(Length length) { SET_VAR(rareNonInheritedData, m_mask.m_yPosition, length); }
    void setMaskSize(LengthSize s) { SET_VAR(rareNonInheritedData, m_mask.m_sizeLength, s); }

    void setBorderCollapse(EBorderCollapse collapse) { inherited_flags._border_collapse = collapse; }
    void setHorizontalBorderSpacing(short);
    void setVerticalBorderSpacing(short);
    void setEmptyCells(EEmptyCell v) { inherited_flags._empty_cells = v; }
    void setCaptionSide(ECaptionSide v) { inherited_flags._caption_side = v; }

    void setHasAspectRatio(bool b) { SET_VAR(rareNonInheritedData, m_hasAspectRatio, b); }
    void setAspectRatioDenominator(float v) { SET_VAR(rareNonInheritedData, m_aspectRatioDenominator, v); }
    void setAspectRatioNumerator(float v) { SET_VAR(rareNonInheritedData, m_aspectRatioNumerator, v); }

    void setListStyleType(EListStyleType v) { inherited_flags._list_style_type = v; }
    void setListStyleImage(PassRefPtr<StyleImage>);
    void setListStylePosition(EListStylePosition v) { inherited_flags._list_style_position = v; }

    void resetMargin() { SET_VAR(surround, margin, LengthBox(Fixed)); }
    void setMarginTop(Length v) { SET_VAR(surround, margin.m_top, v); }
    void setMarginBottom(Length v) { SET_VAR(surround, margin.m_bottom, v); }
    void setMarginLeft(Length v) { SET_VAR(surround, margin.m_left, v); }
    void setMarginRight(Length v) { SET_VAR(surround, margin.m_right, v); }
    void setMarginStart(Length);
    void setMarginEnd(Length);

    void resetPadding() { SET_VAR(surround, padding, LengthBox(Auto)); }
    void setPaddingBox(const LengthBox& b) { SET_VAR(surround, padding, b); }
    void setPaddingTop(Length v) { SET_VAR(surround, padding.m_top, v); }
    void setPaddingBottom(Length v) { SET_VAR(surround, padding.m_bottom, v); }
    void setPaddingLeft(Length v) { SET_VAR(surround, padding.m_left, v); }
    void setPaddingRight(Length v) { SET_VAR(surround, padding.m_right, v); }

    void setCursor(ECursor c) { inherited_flags._cursor_style = c; }
    void addCursor(PassRefPtr<StyleImage>, const IntPoint& hotSpot = IntPoint());
    void setCursorList(PassRefPtr<CursorList>);
    void clearCursorList();

#if ENABLE(CURSOR_VISIBILITY)
    void setCursorVisibility(CursorVisibility c) { inherited_flags.m_cursorVisibility = c; }
#endif

    void setInsideLink(EInsideLink insideLink) { inherited_flags._insideLink = insideLink; }
    void setIsLink(bool b) { noninherited_flags.setIsLink(b); }

    PrintColorAdjust printColorAdjust() const { return static_cast<PrintColorAdjust>(inherited_flags.m_printColorAdjust); }
    void setPrintColorAdjust(PrintColorAdjust value) { inherited_flags.m_printColorAdjust = value; }

    bool hasAutoZIndex() const { return m_box->hasAutoZIndex(); }
    void setHasAutoZIndex() { SET_VAR(m_box, m_hasAutoZIndex, true); SET_VAR(m_box, m_zIndex, 0); }
    int zIndex() const { return m_box->zIndex(); }
    void setZIndex(int v) { SET_VAR(m_box, m_hasAutoZIndex, false); SET_VAR(m_box, m_zIndex, v); }

    void setHasAutoWidows() { SET_VAR(rareInheritedData, m_hasAutoWidows, true); SET_VAR(rareInheritedData, widows, initialWidows()); }
    void setWidows(short w) { SET_VAR(rareInheritedData, m_hasAutoWidows, false); SET_VAR(rareInheritedData, widows, w); }

    void setHasAutoOrphans() { SET_VAR(rareInheritedData, m_hasAutoOrphans, true); SET_VAR(rareInheritedData, orphans, initialOrphans()); }
    void setOrphans(short o) { SET_VAR(rareInheritedData, m_hasAutoOrphans, false); SET_VAR(rareInheritedData, orphans, o); }

    // For valid values of page-break-inside see http://www.w3.org/TR/CSS21/page.html#page-break-props
    void setPageBreakInside(EPageBreak b) { ASSERT(b == PBAUTO || b == PBAVOID); noninherited_flags._page_break_inside = b; }
    void setPageBreakBefore(EPageBreak b) { noninherited_flags._page_break_before = b; }
    void setPageBreakAfter(EPageBreak b) { noninherited_flags._page_break_after = b; }

    // CSS3 Setters
    void setOutlineOffset(int v) { SET_VAR(m_background, m_outline.m_offset, v); }
    void setTextShadow(PassOwnPtr<ShadowData>, bool add = false);
    void setTextStrokeColor(const Color& c) { SET_VAR(rareInheritedData, textStrokeColor, c); }
    void setTextStrokeWidth(float w) { SET_VAR(rareInheritedData, textStrokeWidth, w); }
    void setTextFillColor(const Color& c) { SET_VAR(rareInheritedData, textFillColor, c); }
    void setColorSpace(ColorSpace space) { SET_VAR(rareInheritedData, colorSpace, space); }
    void setOpacity(float f) { float v = clampTo<float>(f, 0, 1); SET_VAR(rareNonInheritedData, opacity, v); }
    void setAppearance(ControlPart a) { SET_VAR(rareNonInheritedData, m_appearance, a); }
    // For valid values of box-align see http://www.w3.org/TR/2009/WD-css3-flexbox-20090723/#alignment
    void setBoxAlign(EBoxAlignment a) { SET_VAR(rareNonInheritedData.access()->m_deprecatedFlexibleBox, align, a); }
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    void setBoxDecorationBreak(EBoxDecorationBreak b) { SET_VAR(m_box, m_boxDecorationBreak, b); }
#endif
    void setBoxDirection(EBoxDirection d) { inherited_flags._box_direction = d; }
    void setBoxFlex(float f) { SET_VAR(rareNonInheritedData.access()->m_deprecatedFlexibleBox, flex, f); }
    void setBoxFlexGroup(unsigned int fg) { SET_VAR(rareNonInheritedData.access()->m_deprecatedFlexibleBox, flex_group, fg); }
    void setBoxLines(EBoxLines l) { SET_VAR(rareNonInheritedData.access()->m_deprecatedFlexibleBox, lines, l); }
    void setBoxOrdinalGroup(unsigned int og) { SET_VAR(rareNonInheritedData.access()->m_deprecatedFlexibleBox, ordinal_group, og); }
    void setBoxOrient(EBoxOrient o) { SET_VAR(rareNonInheritedData.access()->m_deprecatedFlexibleBox, orient, o); }
    void setBoxPack(EBoxPack p) { SET_VAR(rareNonInheritedData.access()->m_deprecatedFlexibleBox, pack, p); }
    void setBoxShadow(PassOwnPtr<ShadowData>, bool add = false);
    void setBoxReflect(PassRefPtr<StyleReflection> reflect) { if (rareNonInheritedData->m_boxReflect != reflect) rareNonInheritedData.access()->m_boxReflect = reflect; }
    void setBoxSizing(EBoxSizing s) { SET_VAR(m_box, m_boxSizing, s); }
    void setFlexGrow(float f) { SET_VAR(rareNonInheritedData.access()->m_flexibleBox, m_flexGrow, f); }
    void setFlexShrink(float f) { SET_VAR(rareNonInheritedData.access()->m_flexibleBox, m_flexShrink, f); }
    void setFlexBasis(Length length) { SET_VAR(rareNonInheritedData.access()->m_flexibleBox, m_flexBasis, length); }
    void setOrder(int o) { SET_VAR(rareNonInheritedData, m_order, o); }
    void setAlignContent(EAlignContent p) { SET_VAR(rareNonInheritedData, m_alignContent, p); }
    void setAlignItems(EAlignItems a) { SET_VAR(rareNonInheritedData, m_alignItems, a); }
    void setAlignSelf(EAlignItems a) { SET_VAR(rareNonInheritedData, m_alignSelf, a); }
    void setFlexDirection(EFlexDirection direction) { SET_VAR(rareNonInheritedData.access()->m_flexibleBox, m_flexDirection, direction); }
    void setFlexWrap(EFlexWrap w) { SET_VAR(rareNonInheritedData.access()->m_flexibleBox, m_flexWrap, w); }
    void setJustifyContent(EJustifyContent p) { SET_VAR(rareNonInheritedData, m_justifyContent, p); }
    void setGridAutoColumns(const GridTrackSize& length) { SET_VAR(rareNonInheritedData.access()->m_grid, m_gridAutoColumns, length); }
    void setGridAutoRows(const GridTrackSize& length) { SET_VAR(rareNonInheritedData.access()->m_grid, m_gridAutoRows, length); }
    void setGridColumns(const Vector<GridTrackSize>& lengths) { SET_VAR(rareNonInheritedData.access()->m_grid, m_gridColumns, lengths); }
    void setGridRows(const Vector<GridTrackSize>& lengths) { SET_VAR(rareNonInheritedData.access()->m_grid, m_gridRows, lengths); }
    void setGridAutoFlow(GridAutoFlow flow) { SET_VAR(rareNonInheritedData.access()->m_grid, m_gridAutoFlow, flow); }
    void setGridItemStart(const GridPosition& startPosition) { SET_VAR(rareNonInheritedData.access()->m_gridItem, m_gridStart, startPosition); }
    void setGridItemEnd(const GridPosition& endPosition) { SET_VAR(rareNonInheritedData.access()->m_gridItem, m_gridEnd, endPosition); }
    void setGridItemBefore(const GridPosition& beforePosition) { SET_VAR(rareNonInheritedData.access()->m_gridItem, m_gridBefore, beforePosition); }
    void setGridItemAfter(const GridPosition& afterPosition) { SET_VAR(rareNonInheritedData.access()->m_gridItem, m_gridAfter, afterPosition); }

    void setMarqueeIncrement(Length f) { SET_VAR(rareNonInheritedData.access()->m_marquee, increment, f); }
    void setMarqueeSpeed(int f) { SET_VAR(rareNonInheritedData.access()->m_marquee, speed, f); }
    void setMarqueeDirection(EMarqueeDirection d) { SET_VAR(rareNonInheritedData.access()->m_marquee, direction, d); }
    void setMarqueeBehavior(EMarqueeBehavior b) { SET_VAR(rareNonInheritedData.access()->m_marquee, behavior, b); }
    void setMarqueeLoopCount(int i) { SET_VAR(rareNonInheritedData.access()->m_marquee, loops, i); }
    void setUserModify(EUserModify u) { SET_VAR(rareInheritedData, userModify, u); }
    void setUserDrag(EUserDrag d) { SET_VAR(rareNonInheritedData, userDrag, d); }
    void setUserSelect(EUserSelect s) { SET_VAR(rareInheritedData, userSelect, s); }
    void setTextOverflow(TextOverflow overflow) { SET_VAR(rareNonInheritedData, textOverflow, overflow); }
    void setMarginBeforeCollapse(EMarginCollapse c) { SET_VAR(rareNonInheritedData, marginBeforeCollapse, c); }
    void setMarginAfterCollapse(EMarginCollapse c) { SET_VAR(rareNonInheritedData, marginAfterCollapse, c); }
    void setWordBreak(EWordBreak b) { SET_VAR(rareInheritedData, wordBreak, b); }
    void setOverflowWrap(EOverflowWrap b) { SET_VAR(rareInheritedData, overflowWrap, b); }
    void setNBSPMode(ENBSPMode b) { SET_VAR(rareInheritedData, nbspMode, b); }
    void setLineBreak(LineBreak b) { SET_VAR(rareInheritedData, lineBreak, b); }
    void setHighlight(const AtomicString& h) { SET_VAR(rareInheritedData, highlight, h); }
    void setHyphens(Hyphens h) { SET_VAR(rareInheritedData, hyphens, h); }
    void setHyphenationLimitBefore(short limit) { SET_VAR(rareInheritedData, hyphenationLimitBefore, limit); }
    void setHyphenationLimitAfter(short limit) { SET_VAR(rareInheritedData, hyphenationLimitAfter, limit); }
    void setHyphenationLimitLines(short limit) { SET_VAR(rareInheritedData, hyphenationLimitLines, limit); }
    void setHyphenationString(const AtomicString& h) { SET_VAR(rareInheritedData, hyphenationString, h); }
    void setLocale(const AtomicString& locale) { SET_VAR(rareInheritedData, locale, locale); }
    void setBorderFit(EBorderFit b) { SET_VAR(rareNonInheritedData, m_borderFit, b); }
    void setResize(EResize r) { SET_VAR(rareInheritedData, resize, r); }
    void setColumnAxis(ColumnAxis axis) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_axis, axis); }
    void setColumnProgression(ColumnProgression progression) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_progression, progression); }
    void setColumnWidth(float f) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_autoWidth, false); SET_VAR(rareNonInheritedData.access()->m_multiCol, m_width, f); }
    void setHasAutoColumnWidth() { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_autoWidth, true); SET_VAR(rareNonInheritedData.access()->m_multiCol, m_width, 0); }
    void setColumnCount(unsigned short c) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_autoCount, false); SET_VAR(rareNonInheritedData.access()->m_multiCol, m_count, c); }
    void setHasAutoColumnCount() { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_autoCount, true); SET_VAR(rareNonInheritedData.access()->m_multiCol, m_count, 0); }
    void setColumnGap(float f) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_normalGap, false); SET_VAR(rareNonInheritedData.access()->m_multiCol, m_gap, f); }
    void setHasNormalColumnGap() { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_normalGap, true); SET_VAR(rareNonInheritedData.access()->m_multiCol, m_gap, 0); }
    void setColumnRuleColor(const Color& c) { SET_BORDERVALUE_COLOR(rareNonInheritedData.access()->m_multiCol, m_rule, c); }
    void setColumnRuleStyle(EBorderStyle b) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_rule.m_style, b); }
    void setColumnRuleWidth(unsigned short w) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_rule.m_width, w); }
    void resetColumnRule() { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_rule, BorderValue()); }
    void setColumnSpan(ColumnSpan columnSpan) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_columnSpan, columnSpan); }
    void setColumnBreakBefore(EPageBreak p) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_breakBefore, p); }
    // For valid values of column-break-inside see http://www.w3.org/TR/css3-multicol/#break-before-break-after-break-inside
    void setColumnBreakInside(EPageBreak p) { ASSERT(p == PBAUTO || p == PBAVOID); SET_VAR(rareNonInheritedData.access()->m_multiCol, m_breakInside, p); }
    void setColumnBreakAfter(EPageBreak p) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_breakAfter, p); }
    void setRegionBreakBefore(EPageBreak p) { SET_VAR(rareNonInheritedData, m_regionBreakBefore, p); }
    void setRegionBreakInside(EPageBreak p) { ASSERT(p == PBAUTO || p == PBAVOID); SET_VAR(rareNonInheritedData, m_regionBreakInside, p); }
    void setRegionBreakAfter(EPageBreak p) { SET_VAR(rareNonInheritedData, m_regionBreakAfter, p); }
    void inheritColumnPropertiesFrom(RenderStyle* parent) { rareNonInheritedData.access()->m_multiCol = parent->rareNonInheritedData->m_multiCol; }
    void setTransform(const TransformOperations& ops) { SET_VAR(rareNonInheritedData.access()->m_transform, m_operations, ops); }
    void setTransformOriginX(Length l) { SET_VAR(rareNonInheritedData.access()->m_transform, m_x, l); }
    void setTransformOriginY(Length l) { SET_VAR(rareNonInheritedData.access()->m_transform, m_y, l); }
    void setTransformOriginZ(float f) { SET_VAR(rareNonInheritedData.access()->m_transform, m_z, f); }
    void setSpeak(ESpeak s) { SET_VAR(rareInheritedData, speak, s); }
    void setTextCombine(TextCombine v) { SET_VAR(rareNonInheritedData, m_textCombine, v); }
#if ENABLE(CSS3_TEXT)
    void setTextDecorationColor(const Color& c) { SET_VAR(rareNonInheritedData, m_textDecorationColor, c); }
#endif // CSS3_TEXT
    void setTextEmphasisColor(const Color& c) { SET_VAR(rareInheritedData, textEmphasisColor, c); }
    void setTextEmphasisFill(TextEmphasisFill fill) { SET_VAR(rareInheritedData, textEmphasisFill, fill); }
    void setTextEmphasisMark(TextEmphasisMark mark) { SET_VAR(rareInheritedData, textEmphasisMark, mark); }
    void setTextEmphasisCustomMark(const AtomicString& mark) { SET_VAR(rareInheritedData, textEmphasisCustomMark, mark); }
    void setTextEmphasisPosition(TextEmphasisPosition position) { SET_VAR(rareInheritedData, textEmphasisPosition, position); }
    bool setTextOrientation(TextOrientation);

    void setRubyPosition(RubyPosition position) { SET_VAR(rareInheritedData, m_rubyPosition, position); }

#if ENABLE(CSS_FILTERS)
    void setFilter(const FilterOperations& ops) { SET_VAR(rareNonInheritedData.access()->m_filter, m_operations, ops); }
#endif

    void setTabSize(unsigned size) { SET_VAR(rareInheritedData, m_tabSize, size); }

    // End CSS3 Setters

    void setLineGrid(const AtomicString& lineGrid) { SET_VAR(rareInheritedData, m_lineGrid, lineGrid); }
    void setLineSnap(LineSnap lineSnap) { SET_VAR(rareInheritedData, m_lineSnap, lineSnap); }
    void setLineAlign(LineAlign lineAlign) { SET_VAR(rareInheritedData, m_lineAlign, lineAlign); }

    void setFlowThread(const AtomicString& flowThread) { SET_VAR(rareNonInheritedData, m_flowThread, flowThread); }
    void setRegionThread(const AtomicString& regionThread) { SET_VAR(rareNonInheritedData, m_regionThread, regionThread); }
    void setRegionFragment(RegionFragment regionFragment) { SET_VAR(rareNonInheritedData, m_regionFragment, regionFragment); }

    void setWrapFlow(WrapFlow wrapFlow) { SET_VAR(rareNonInheritedData, m_wrapFlow, wrapFlow); }
    void setWrapThrough(WrapThrough wrapThrough) { SET_VAR(rareNonInheritedData, m_wrapThrough, wrapThrough); }

    // Apple-specific property setters
    void setPointerEvents(EPointerEvents p) { inherited_flags._pointerEvents = p; }

    void clearAnimations()
    {
        rareNonInheritedData.access()->m_animations.clear();
    }

    void clearTransitions()
    {
        rareNonInheritedData.access()->m_transitions.clear();
    }

    void inheritAnimations(const AnimationList* parent) { rareNonInheritedData.access()->m_animations = parent ? adoptPtr(new AnimationList(*parent)) : nullptr; }
    void inheritTransitions(const AnimationList* parent) { rareNonInheritedData.access()->m_transitions = parent ? adoptPtr(new AnimationList(*parent)) : nullptr; }
    void adjustAnimations();
    void adjustTransitions();

    void setTransformStyle3D(ETransformStyle3D b) { SET_VAR(rareNonInheritedData, m_transformStyle3D, b); }
    void setBackfaceVisibility(EBackfaceVisibility b) { SET_VAR(rareNonInheritedData, m_backfaceVisibility, b); }
    void setPerspective(float p) { SET_VAR(rareNonInheritedData, m_perspective, p); }
    void setPerspectiveOriginX(Length l) { SET_VAR(rareNonInheritedData, m_perspectiveOriginX, l); }
    void setPerspectiveOriginY(Length l) { SET_VAR(rareNonInheritedData, m_perspectiveOriginY, l); }
    void setPageSize(LengthSize s) { SET_VAR(rareNonInheritedData, m_pageSize, s); }
    void setPageSizeType(PageSizeType t) { SET_VAR(rareNonInheritedData, m_pageSizeType, t); }
    void resetPageSizeType() { SET_VAR(rareNonInheritedData, m_pageSizeType, PAGE_SIZE_AUTO); }

#if USE(ACCELERATED_COMPOSITING)
    void setIsRunningAcceleratedAnimation(bool b = true) { SET_VAR(rareNonInheritedData, m_runningAcceleratedAnimation, b); }
#endif

    void setLineBoxContain(LineBoxContain c) { SET_VAR(rareInheritedData, m_lineBoxContain, c); }
    void setLineClamp(LineClampValue c) { SET_VAR(rareNonInheritedData, lineClamp, c); }
#if ENABLE(TOUCH_EVENTS)
    void setTapHighlightColor(const Color& c) { SET_VAR(rareInheritedData, tapHighlightColor, c); }
#endif
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    void setUseTouchOverflowScrolling(bool v) { SET_VAR(rareInheritedData, useTouchOverflowScrolling, v); }
#endif
    void setTextSecurity(ETextSecurity aTextSecurity) { SET_VAR(rareInheritedData, textSecurity, aTextSecurity); }

#if ENABLE(SVG)
    const SVGRenderStyle* svgStyle() const { return m_svgStyle.get(); }
    SVGRenderStyle* accessSVGStyle() { return m_svgStyle.access(); }

    const SVGPaint::SVGPaintType& fillPaintType() const { return svgStyle()->fillPaintType(); }
    Color fillPaintColor() const { return svgStyle()->fillPaintColor(); }
    void setFillPaintColor(const Color& c) { accessSVGStyle()->setFillPaint(SVGPaint::SVG_PAINTTYPE_RGBCOLOR, c, ""); }
    float fillOpacity() const { return svgStyle()->fillOpacity(); }
    void setFillOpacity(float f) { accessSVGStyle()->setFillOpacity(f); }

    const SVGPaint::SVGPaintType& strokePaintType() const { return svgStyle()->strokePaintType(); }
    Color strokePaintColor() const { return svgStyle()->strokePaintColor(); }
    void setStrokePaintColor(const Color& c) { accessSVGStyle()->setStrokePaint(SVGPaint::SVG_PAINTTYPE_RGBCOLOR, c, ""); }
    float strokeOpacity() const { return svgStyle()->strokeOpacity(); }
    void setStrokeOpacity(float f) { accessSVGStyle()->setStrokeOpacity(f); }
    SVGLength strokeWidth() const { return svgStyle()->strokeWidth(); }
    void setStrokeWidth(SVGLength w) { accessSVGStyle()->setStrokeWidth(w); }
    SVGLength strokeDashOffset() const { return svgStyle()->strokeDashOffset(); }
    void setStrokeDashOffset(SVGLength d) { accessSVGStyle()->setStrokeDashOffset(d); }
    float strokeMiterLimit() const { return svgStyle()->strokeMiterLimit(); }
    void setStrokeMiterLimit(float f) { accessSVGStyle()->setStrokeMiterLimit(f); }

    float floodOpacity() const { return svgStyle()->floodOpacity(); }
    void setFloodOpacity(float f) { accessSVGStyle()->setFloodOpacity(f); }

    float stopOpacity() const { return svgStyle()->stopOpacity(); }
    void setStopOpacity(float f) { accessSVGStyle()->setStopOpacity(f); }

    void setStopColor(const Color& c) { accessSVGStyle()->setStopColor(c); }
    void setFloodColor(const Color& c) { accessSVGStyle()->setFloodColor(c); }
    void setLightingColor(const Color& c) { accessSVGStyle()->setLightingColor(c); }

    SVGLength baselineShiftValue() const { return svgStyle()->baselineShiftValue(); }
    void setBaselineShiftValue(SVGLength s) { accessSVGStyle()->setBaselineShiftValue(s); }
    SVGLength kerning() const { return svgStyle()->kerning(); }
    void setKerning(SVGLength k) { accessSVGStyle()->setKerning(k); }
#endif

    void setShapeInside(PassRefPtr<ShapeValue> value)
    {
        if (rareNonInheritedData->m_shapeInside == value)
            return;
        rareNonInheritedData.access()->m_shapeInside = value;
    }
    ShapeValue* shapeInside() const { return rareNonInheritedData->m_shapeInside.get(); }
    ShapeValue* resolvedShapeInside() const
    {
        ShapeValue* shapeInside = this->shapeInside();
        if (shapeInside && shapeInside->type() == ShapeValue::Outside)
            return shapeOutside();
        return shapeInside;
    }

    void setShapeOutside(PassRefPtr<ShapeValue> value)
    {
        if (rareNonInheritedData->m_shapeOutside == value)
            return;
        rareNonInheritedData.access()->m_shapeOutside = value;
    }
    ShapeValue* shapeOutside() const { return rareNonInheritedData->m_shapeOutside.get(); }

    static ShapeValue* initialShapeInside();
    static ShapeValue* initialShapeOutside() { return 0; }

    void setClipPath(PassRefPtr<ClipPathOperation> operation)
    {
        if (rareNonInheritedData->m_clipPath != operation)
            rareNonInheritedData.access()->m_clipPath = operation;
    }
    ClipPathOperation* clipPath() const { return rareNonInheritedData->m_clipPath.get(); }

    static ClipPathOperation* initialClipPath() { return 0; }

    Length shapePadding() const { return rareNonInheritedData->m_shapePadding; }
    void setShapePadding(Length shapePadding) { SET_VAR(rareNonInheritedData, m_shapePadding, shapePadding); }
    static Length initialShapePadding() { return Length(0, Fixed); }

    Length shapeMargin() const { return rareNonInheritedData->m_shapeMargin; }
    void setShapeMargin(Length shapeMargin) { SET_VAR(rareNonInheritedData, m_shapeMargin, shapeMargin); }
    static Length initialShapeMargin() { return Length(0, Fixed); }

    bool hasContent() const { return contentData(); }
    const ContentData* contentData() const { return rareNonInheritedData->m_content.get(); }
    bool contentDataEquivalent(const RenderStyle* otherStyle) const { return const_cast<RenderStyle*>(this)->rareNonInheritedData->contentDataEquivalent(*const_cast<RenderStyle*>(otherStyle)->rareNonInheritedData); }
    void clearContent();
    void setContent(const String&, bool add = false);
    void setContent(PassRefPtr<StyleImage>, bool add = false);
    void setContent(PassOwnPtr<CounterContent>, bool add = false);
    void setContent(QuoteType, bool add = false);

    const CounterDirectiveMap* counterDirectives() const;
    CounterDirectiveMap& accessCounterDirectives();
    const CounterDirectives getCounterDirectives(const AtomicString& identifier) const;

    QuotesData* quotes() const { return rareInheritedData->quotes.get(); }
    void setQuotes(PassRefPtr<QuotesData>);

    const AtomicString& hyphenString() const;

    bool inheritedNotEqual(const RenderStyle*) const;
    bool inheritedDataShared(const RenderStyle*) const;

    StyleDifference diff(const RenderStyle*, unsigned& changedContextSensitiveProperties) const;
    bool diffRequiresRepaint(const RenderStyle*) const;

    bool isDisplayReplacedType() const { return isDisplayReplacedType(display()); }
    bool isDisplayInlineType() const { return isDisplayInlineType(display()); }
    bool isOriginalDisplayInlineType() const { return isDisplayInlineType(originalDisplay()); }
    bool isDisplayRegionType() const
    {
        return display() == BLOCK || display() == INLINE_BLOCK
            || display() == TABLE_CELL || display() == TABLE_CAPTION
            || display() == LIST_ITEM;
    }

    bool setWritingMode(WritingMode v)
    {
        if (v == writingMode())
            return false;

        inherited_flags.m_writingMode = v;
        return true;
    }

    // A unique style is one that has matches something that makes it impossible to share.
    bool unique() const { return noninherited_flags.unique; }
    void setUnique() { noninherited_flags.unique = true; }

    bool emptyState() const { return noninherited_flags.emptyState; }
    void setEmptyState(bool b) { setUnique(); noninherited_flags.emptyState = b; }
    bool firstChildState() const { return noninherited_flags.firstChildState; }
    void setFirstChildState() { setUnique(); noninherited_flags.firstChildState = true; }
    bool lastChildState() const { return noninherited_flags.lastChildState; }
    void setLastChildState() { setUnique(); noninherited_flags.lastChildState = true; }

    Color visitedDependentColor(int colorProperty) const;

    void setHasExplicitlyInheritedProperties() { noninherited_flags.explicitInheritance = true; }
    bool hasExplicitlyInheritedProperties() const { return noninherited_flags.explicitInheritance; }
    
    // Initial values for all the properties
    static EBorderCollapse initialBorderCollapse() { return BSEPARATE; }
    static EBorderStyle initialBorderStyle() { return BNONE; }
    static OutlineIsAuto initialOutlineStyleIsAuto() { return AUTO_OFF; }
    static NinePieceImage initialNinePieceImage() { return NinePieceImage(); }
    static LengthSize initialBorderRadius() { return LengthSize(Length(0, Fixed), Length(0, Fixed)); }
    static ECaptionSide initialCaptionSide() { return CAPTOP; }
    static EClear initialClear() { return CNONE; }
    static ColorSpace initialColorSpace() { return ColorSpaceDeviceRGB; }
    static ColumnAxis initialColumnAxis() { return AutoColumnAxis; }
    static ColumnProgression initialColumnProgression() { return NormalColumnProgression; }
    static TextDirection initialDirection() { return LTR; }
    static WritingMode initialWritingMode() { return TopToBottomWritingMode; }
    static TextCombine initialTextCombine() { return TextCombineNone; }
    static TextOrientation initialTextOrientation() { return TextOrientationVerticalRight; }
    static EDisplay initialDisplay() { return INLINE; }
    static EEmptyCell initialEmptyCells() { return SHOW; }
    static EFloat initialFloating() { return NoFloat; }
    static EListStylePosition initialListStylePosition() { return OUTSIDE; }
    static EListStyleType initialListStyleType() { return Disc; }
    static EOverflow initialOverflowX() { return OVISIBLE; }
    static EOverflow initialOverflowY() { return OVISIBLE; }
    static EPageBreak initialPageBreak() { return PBAUTO; }
    static EPosition initialPosition() { return StaticPosition; }
    static ETableLayout initialTableLayout() { return TAUTO; }
    static EUnicodeBidi initialUnicodeBidi() { return UBNormal; }
    static ETextTransform initialTextTransform() { return TTNONE; }
    static EVisibility initialVisibility() { return VISIBLE; }
    static EWhiteSpace initialWhiteSpace() { return NORMAL; }
    static short initialHorizontalBorderSpacing() { return 0; }
    static short initialVerticalBorderSpacing() { return 0; }
    static ECursor initialCursor() { return CURSOR_AUTO; }
#if ENABLE(CURSOR_VISIBILITY)
    static CursorVisibility initialCursorVisibility() { return CursorVisibilityAuto; }
#endif
    static Color initialColor() { return Color::black; }
    static StyleImage* initialListStyleImage() { return 0; }
    static unsigned initialBorderWidth() { return 3; }
    static unsigned short initialColumnRuleWidth() { return 3; }
    static unsigned short initialOutlineWidth() { return 3; }
    static int initialLetterWordSpacing() { return 0; }
    static Length initialSize() { return Length(); }
    static Length initialMinSize() { return Length(Fixed); }
    static Length initialMaxSize() { return Length(Undefined); }
    static Length initialOffset() { return Length(); }
    static Length initialMargin() { return Length(Fixed); }
    static Length initialPadding() { return Length(Fixed); }
    static Length initialTextIndent() { return Length(Fixed); }
#if ENABLE(CSS3_TEXT)
    static TextIndentLine initialTextIndentLine() { return TextIndentFirstLine; }
    static TextIndentType initialTextIndentType() { return TextIndentNormal; }
#endif
    static EVerticalAlign initialVerticalAlign() { return BASELINE; }
    static short initialWidows() { return 2; }
    static short initialOrphans() { return 2; }
    static Length initialLineHeight() { return Length(-100.0, Percent); }
    static ETextAlign initialTextAlign() { return TASTART; }
    static TextDecoration initialTextDecoration() { return TextDecorationNone; }
#if ENABLE(CSS3_TEXT)
    static TextDecorationStyle initialTextDecorationStyle() { return TextDecorationStyleSolid; }
    static TextAlignLast initialTextAlignLast() { return TextAlignLastAuto; }
    static TextJustify initialTextJustify() { return TextJustifyAuto; }
    static TextUnderlinePosition initialTextUnderlinePosition() { return TextUnderlinePositionAuto; }
#endif // CSS3_TEXT
    static float initialZoom() { return 1.0f; }
    static int initialOutlineOffset() { return 0; }
    static float initialOpacity() { return 1.0f; }
    static EBoxAlignment initialBoxAlign() { return BSTRETCH; }
    static EBoxDecorationBreak initialBoxDecorationBreak() { return DSLICE; }
    static EBoxDirection initialBoxDirection() { return BNORMAL; }
    static EBoxLines initialBoxLines() { return SINGLE; }
    static EBoxOrient initialBoxOrient() { return HORIZONTAL; }
    static EBoxPack initialBoxPack() { return Start; }
    static float initialBoxFlex() { return 0.0f; }
    static unsigned int initialBoxFlexGroup() { return 1; }
    static unsigned int initialBoxOrdinalGroup() { return 1; }
    static EBoxSizing initialBoxSizing() { return CONTENT_BOX; }
    static StyleReflection* initialBoxReflect() { return 0; }
    static float initialFlexGrow() { return 0; }
    static float initialFlexShrink() { return 1; }
    static Length initialFlexBasis() { return Length(Auto); }
    static int initialOrder() { return 0; }
    static EAlignContent initialAlignContent() { return AlignContentStretch; }
    static EAlignItems initialAlignItems() { return AlignStretch; }
    static EAlignItems initialAlignSelf() { return AlignAuto; }
    static EFlexDirection initialFlexDirection() { return FlowRow; }
    static EFlexWrap initialFlexWrap() { return FlexNoWrap; }
    static EJustifyContent initialJustifyContent() { return JustifyFlexStart; }
    static int initialMarqueeLoopCount() { return -1; }
    static int initialMarqueeSpeed() { return 85; }
    static Length initialMarqueeIncrement() { return Length(6, Fixed); }
    static EMarqueeBehavior initialMarqueeBehavior() { return MSCROLL; }
    static EMarqueeDirection initialMarqueeDirection() { return MAUTO; }
    static EUserModify initialUserModify() { return READ_ONLY; }
    static EUserDrag initialUserDrag() { return DRAG_AUTO; }
    static EUserSelect initialUserSelect() { return SELECT_TEXT; }
    static TextOverflow initialTextOverflow() { return TextOverflowClip; }
    static EMarginCollapse initialMarginBeforeCollapse() { return MCOLLAPSE; }
    static EMarginCollapse initialMarginAfterCollapse() { return MCOLLAPSE; }
    static EWordBreak initialWordBreak() { return NormalWordBreak; }
    static EOverflowWrap initialOverflowWrap() { return NormalOverflowWrap; }
    static ENBSPMode initialNBSPMode() { return NBNORMAL; }
    static LineBreak initialLineBreak() { return LineBreakAuto; }
    static const AtomicString& initialHighlight() { return nullAtom; }
    static ESpeak initialSpeak() { return SpeakNormal; }
    static Hyphens initialHyphens() { return HyphensManual; }
    static short initialHyphenationLimitBefore() { return -1; }
    static short initialHyphenationLimitAfter() { return -1; }
    static short initialHyphenationLimitLines() { return -1; }
    static const AtomicString& initialHyphenationString() { return nullAtom; }
    static const AtomicString& initialLocale() { return nullAtom; }
    static EBorderFit initialBorderFit() { return BorderFitBorder; }
    static EResize initialResize() { return RESIZE_NONE; }
    static ControlPart initialAppearance() { return NoControlPart; }
    static bool initialHasAspectRatio() { return false; }
    static float initialAspectRatioDenominator() { return 1; }
    static float initialAspectRatioNumerator() { return 1; }
    static Order initialRTLOrdering() { return LogicalOrder; }
    static float initialTextStrokeWidth() { return 0; }
    static unsigned short initialColumnCount() { return 1; }
    static ColumnSpan initialColumnSpan() { return ColumnSpanNone; }
    static const TransformOperations& initialTransform() { DEFINE_STATIC_LOCAL(TransformOperations, ops, ()); return ops; }
    static Length initialTransformOriginX() { return Length(50.0, Percent); }
    static Length initialTransformOriginY() { return Length(50.0, Percent); }
    static EPointerEvents initialPointerEvents() { return PE_AUTO; }
    static float initialTransformOriginZ() { return 0; }
    static ETransformStyle3D initialTransformStyle3D() { return TransformStyle3DFlat; }
    static EBackfaceVisibility initialBackfaceVisibility() { return BackfaceVisibilityVisible; }
    static float initialPerspective() { return 0; }
    static Length initialPerspectiveOriginX() { return Length(50.0, Percent); }
    static Length initialPerspectiveOriginY() { return Length(50.0, Percent); }
    static Color initialBackgroundColor() { return Color::transparent; }
    static Color initialTextEmphasisColor() { return TextEmphasisFillFilled; }
    static TextEmphasisFill initialTextEmphasisFill() { return TextEmphasisFillFilled; }
    static TextEmphasisMark initialTextEmphasisMark() { return TextEmphasisMarkNone; }
    static const AtomicString& initialTextEmphasisCustomMark() { return nullAtom; }
    static TextEmphasisPosition initialTextEmphasisPosition() { return TextEmphasisPositionOver; }
    static RubyPosition initialRubyPosition() { return RubyPositionBefore; }
    static LineBoxContain initialLineBoxContain() { return LineBoxContainBlock | LineBoxContainInline | LineBoxContainReplaced; }
    static ImageOrientationEnum initialImageOrientation() { return OriginTopLeft; }
    static EImageRendering initialImageRendering() { return ImageRenderingAuto; }
    static ImageResolutionSource initialImageResolutionSource() { return ImageResolutionSpecified; }
    static ImageResolutionSnap initialImageResolutionSnap() { return ImageResolutionNoSnap; }
    static float initialImageResolution() { return 1; }
    static StyleImage* initialBorderImageSource() { return 0; }
    static StyleImage* initialMaskBoxImageSource() { return 0; }
    static PrintColorAdjust initialPrintColorAdjust() { return PrintColorAdjustEconomy; }

    // The initial value is 'none' for grid tracks.
    static Vector<GridTrackSize> initialGridColumns() { return Vector<GridTrackSize>(); }
    static Vector<GridTrackSize> initialGridRows() { return Vector<GridTrackSize>(); }

    static GridAutoFlow initialGridAutoFlow() { return AutoFlowNone; }

    static GridTrackSize initialGridAutoColumns() { return GridTrackSize(Auto); }
    static GridTrackSize initialGridAutoRows() { return GridTrackSize(Auto); }

    // 'auto' is the default.
    static GridPosition initialGridPosition() { return GridPosition(); }

    static unsigned initialTabSize() { return 8; }

    static const AtomicString& initialLineGrid() { return nullAtom; }
    static LineSnap initialLineSnap() { return LineSnapNone; }
    static LineAlign initialLineAlign() { return LineAlignNone; }

    static const AtomicString& initialFlowThread() { return nullAtom; }
    static const AtomicString& initialRegionThread() { return nullAtom; }
    static RegionFragment initialRegionFragment() { return AutoRegionFragment; }

    static WrapFlow initialWrapFlow() { return WrapFlowAuto; }
    static WrapThrough initialWrapThrough() { return WrapThroughWrap; }

    // Keep these at the end.
    static LineClampValue initialLineClamp() { return LineClampValue(); }
    static ETextSecurity initialTextSecurity() { return TSNONE; }
#if ENABLE(TOUCH_EVENTS)
    static Color initialTapHighlightColor();
#endif
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    static bool initialUseTouchOverflowScrolling() { return false; }
#endif
#if ENABLE(DASHBOARD_SUPPORT)
    static const Vector<StyleDashboardRegion>& initialDashboardRegions();
    static const Vector<StyleDashboardRegion>& noneDashboardRegions();
#endif
#if ENABLE(CSS_FILTERS)
    static const FilterOperations& initialFilter() { DEFINE_STATIC_LOCAL(FilterOperations, ops, ()); return ops; }
#endif
#if ENABLE(CSS_COMPOSITING)
    static BlendMode initialBlendMode() { return BlendModeNormal; }
#endif

private:
    bool changeRequiresLayout(const RenderStyle*, unsigned& changedContextSensitiveProperties) const;
    bool changeRequiresPositionedLayoutOnly(const RenderStyle*, unsigned& changedContextSensitiveProperties) const;
    bool changeRequiresLayerRepaint(const RenderStyle*, unsigned& changedContextSensitiveProperties) const;
    bool changeRequiresRepaint(const RenderStyle*, unsigned& changedContextSensitiveProperties) const;
    bool changeRequiresRepaintIfText(const RenderStyle*, unsigned& changedContextSensitiveProperties) const;
    bool changeRequiresRecompositeLayer(const RenderStyle*, unsigned& changedContextSensitiveProperties) const;

    void setVisitedLinkColor(const Color&);
    void setVisitedLinkBackgroundColor(const Color& v) { SET_VAR(rareNonInheritedData, m_visitedLinkBackgroundColor, v); }
    void setVisitedLinkBorderLeftColor(const Color& v) { SET_VAR(rareNonInheritedData, m_visitedLinkBorderLeftColor, v); }
    void setVisitedLinkBorderRightColor(const Color& v) { SET_VAR(rareNonInheritedData, m_visitedLinkBorderRightColor, v); }
    void setVisitedLinkBorderBottomColor(const Color& v) { SET_VAR(rareNonInheritedData, m_visitedLinkBorderBottomColor, v); }
    void setVisitedLinkBorderTopColor(const Color& v) { SET_VAR(rareNonInheritedData, m_visitedLinkBorderTopColor, v); }
    void setVisitedLinkOutlineColor(const Color& v) { SET_VAR(rareNonInheritedData, m_visitedLinkOutlineColor, v); }
    void setVisitedLinkColumnRuleColor(const Color& v) { SET_VAR(rareNonInheritedData.access()->m_multiCol, m_visitedLinkColumnRuleColor, v); }
#if ENABLE(CSS3_TEXT)
    void setVisitedLinkTextDecorationColor(const Color& v) { SET_VAR(rareNonInheritedData, m_visitedLinkTextDecorationColor, v); }
#endif // CSS3_TEXT
    void setVisitedLinkTextEmphasisColor(const Color& v) { SET_VAR(rareInheritedData, visitedLinkTextEmphasisColor, v); }
    void setVisitedLinkTextFillColor(const Color& v) { SET_VAR(rareInheritedData, visitedLinkTextFillColor, v); }
    void setVisitedLinkTextStrokeColor(const Color& v) { SET_VAR(rareInheritedData, visitedLinkTextStrokeColor, v); }

    void inheritUnicodeBidiFrom(const RenderStyle* parent) { noninherited_flags._unicodeBidi = parent->noninherited_flags._unicodeBidi; }
    void getShadowExtent(const ShadowData*, LayoutUnit& top, LayoutUnit& right, LayoutUnit& bottom, LayoutUnit& left) const;
    LayoutBoxExtent getShadowInsetExtent(const ShadowData*) const;
    void getShadowHorizontalExtent(const ShadowData*, LayoutUnit& left, LayoutUnit& right) const;
    void getShadowVerticalExtent(const ShadowData*, LayoutUnit& top, LayoutUnit& bottom) const;
    void getShadowInlineDirectionExtent(const ShadowData* shadow, LayoutUnit& logicalLeft, LayoutUnit& logicalRight) const
    {
        return isHorizontalWritingMode() ? getShadowHorizontalExtent(shadow, logicalLeft, logicalRight) : getShadowVerticalExtent(shadow, logicalLeft, logicalRight);
    }
    void getShadowBlockDirectionExtent(const ShadowData* shadow, LayoutUnit& logicalTop, LayoutUnit& logicalBottom) const
    {
        return isHorizontalWritingMode() ? getShadowVerticalExtent(shadow, logicalTop, logicalBottom) : getShadowHorizontalExtent(shadow, logicalTop, logicalBottom);
    }

    bool isDisplayReplacedType(EDisplay display) const
    {
        return display == INLINE_BLOCK || display == INLINE_BOX || display == INLINE_FLEX
            || display == INLINE_TABLE || display == INLINE_GRID;
    }

    bool isDisplayInlineType(EDisplay display) const
    {
        return display == INLINE || isDisplayReplacedType(display);
    }

    // Color accessors are all private to make sure callers use visitedDependentColor instead to access them.
    Color invalidColor() const { static Color invalid; return invalid; }
    Color borderLeftColor() const { return surround->border.left().color(); }
    Color borderRightColor() const { return surround->border.right().color(); }
    Color borderTopColor() const { return surround->border.top().color(); }
    Color borderBottomColor() const { return surround->border.bottom().color(); }
    Color backgroundColor() const { return m_background->color(); }
    Color color() const;
    Color columnRuleColor() const { return rareNonInheritedData->m_multiCol->m_rule.color(); }
    Color outlineColor() const { return m_background->outline().color(); }
    Color textEmphasisColor() const { return rareInheritedData->textEmphasisColor; }
    Color textFillColor() const { return rareInheritedData->textFillColor; }
    Color textStrokeColor() const { return rareInheritedData->textStrokeColor; }
    Color visitedLinkColor() const;
    Color visitedLinkBackgroundColor() const { return rareNonInheritedData->m_visitedLinkBackgroundColor; }
    Color visitedLinkBorderLeftColor() const { return rareNonInheritedData->m_visitedLinkBorderLeftColor; }
    Color visitedLinkBorderRightColor() const { return rareNonInheritedData->m_visitedLinkBorderRightColor; }
    Color visitedLinkBorderBottomColor() const { return rareNonInheritedData->m_visitedLinkBorderBottomColor; }
    Color visitedLinkBorderTopColor() const { return rareNonInheritedData->m_visitedLinkBorderTopColor; }
    Color visitedLinkOutlineColor() const { return rareNonInheritedData->m_visitedLinkOutlineColor; }
    Color visitedLinkColumnRuleColor() const { return rareNonInheritedData->m_multiCol->m_visitedLinkColumnRuleColor; }
#if ENABLE(CSS3_TEXT)
    Color textDecorationColor() const { return rareNonInheritedData->m_textDecorationColor; }
    Color visitedLinkTextDecorationColor() const { return rareNonInheritedData->m_visitedLinkTextDecorationColor; }
#endif // CSS3_TEXT
    Color visitedLinkTextEmphasisColor() const { return rareInheritedData->visitedLinkTextEmphasisColor; }
    Color visitedLinkTextFillColor() const { return rareInheritedData->visitedLinkTextFillColor; }
    Color visitedLinkTextStrokeColor() const { return rareInheritedData->visitedLinkTextStrokeColor; }

    Color colorIncludingFallback(int colorProperty, bool visitedLink) const;

#if ENABLE(SVG)
    Color stopColor() const { return svgStyle()->stopColor(); }
    Color floodColor() const { return svgStyle()->floodColor(); }
    Color lightingColor() const { return svgStyle()->lightingColor(); }
#endif

    void appendContent(PassOwnPtr<ContentData>);
};

inline int adjustForAbsoluteZoom(int value, const RenderStyle* style)
{
    double zoomFactor = style->effectiveZoom();
    if (zoomFactor == 1)
        return value;
    // Needed because computeLengthInt truncates (rather than rounds) when scaling up.
    if (zoomFactor > 1) {
        if (value < 0)
            value--;
        else 
            value++;
    }

    return roundForImpreciseConversion<int>(value / zoomFactor);
}

inline float adjustFloatForAbsoluteZoom(float value, const RenderStyle* style)
{
    return value / style->effectiveZoom();
}

#if ENABLE(SUBPIXEL_LAYOUT)
inline LayoutUnit adjustLayoutUnitForAbsoluteZoom(LayoutUnit value, const RenderStyle* style)
{
    return value / style->effectiveZoom();
}
#endif

inline bool RenderStyle::setZoom(float f)
{
    if (compareEqual(visual->m_zoom, f))
        return false;
    visual.access()->m_zoom = f;
    setEffectiveZoom(effectiveZoom() * zoom());
    return true;
}

inline bool RenderStyle::setEffectiveZoom(float f)
{
    if (compareEqual(rareInheritedData->m_effectiveZoom, f))
        return false;
    rareInheritedData.access()->m_effectiveZoom = f;
    return true;
}

inline bool RenderStyle::setTextOrientation(TextOrientation textOrientation)
{
    if (compareEqual(rareInheritedData->m_textOrientation, textOrientation))
        return false;

    rareInheritedData.access()->m_textOrientation = textOrientation;
    return true;
}

} // namespace WebCore

#endif // RenderStyle_h
