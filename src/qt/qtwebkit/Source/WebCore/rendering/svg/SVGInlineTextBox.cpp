/**
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "SVGInlineTextBox.h"

#if ENABLE(SVG)
#include "FloatConversion.h"
#include "FontCache.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "InlineFlowBox.h"
#include "PointerEventsHitRules.h"
#include "RenderBlock.h"
#include "RenderSVGInlineText.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceSolidColor.h"
#include "SVGRenderingContext.h"
#include "SVGResourcesCache.h"
#include "SVGRootInlineBox.h"
#include "SVGTextRunRenderingContext.h"

using namespace std;

namespace WebCore {

struct ExpectedSVGInlineTextBoxSize : public InlineTextBox {
    float float1;
    uint32_t bitfields : 5;
    void* pointer;
    Vector<SVGTextFragment> vector;
};

COMPILE_ASSERT(sizeof(SVGInlineTextBox) == sizeof(ExpectedSVGInlineTextBoxSize), SVGInlineTextBox_is_not_of_expected_size);

SVGInlineTextBox::SVGInlineTextBox(RenderObject* object)
    : InlineTextBox(object)
    , m_logicalHeight(0)
    , m_paintingResourceMode(ApplyToDefaultMode)
    , m_startsNewTextChunk(false)
    , m_paintingResource(0)
{
}

void SVGInlineTextBox::dirtyLineBoxes()
{
    InlineTextBox::dirtyLineBoxes();

    // Clear the now stale text fragments
    clearTextFragments();
}

int SVGInlineTextBox::offsetForPosition(float, bool) const
{
    // SVG doesn't use the standard offset <-> position selection system, as it's not suitable for SVGs complex needs.
    // vertical text selection, inline boxes spanning multiple lines (contrary to HTML, etc.)
    ASSERT_NOT_REACHED();
    return 0;
}

int SVGInlineTextBox::offsetForPositionInFragment(const SVGTextFragment& fragment, float position, bool includePartialGlyphs) const
{
    RenderSVGInlineText* textRenderer = toRenderSVGInlineText(this->textRenderer());
    ASSERT(textRenderer);

    float scalingFactor = textRenderer->scalingFactor();
    ASSERT(scalingFactor);

    RenderStyle* style = textRenderer->style();
    ASSERT(style);

    TextRun textRun = constructTextRun(style, fragment);

    // Eventually handle lengthAdjust="spacingAndGlyphs".
    // FIXME: Handle vertical text.
    AffineTransform fragmentTransform;
    fragment.buildFragmentTransform(fragmentTransform);
    if (!fragmentTransform.isIdentity())
        textRun.setHorizontalGlyphStretch(narrowPrecisionToFloat(fragmentTransform.xScale()));

    return fragment.characterOffset - start() + textRenderer->scaledFont().offsetForPosition(textRun, position * scalingFactor, includePartialGlyphs);
}

float SVGInlineTextBox::positionForOffset(int) const
{
    // SVG doesn't use the offset <-> position selection system. 
    ASSERT_NOT_REACHED();
    return 0;
}

FloatRect SVGInlineTextBox::selectionRectForTextFragment(const SVGTextFragment& fragment, int startPosition, int endPosition, RenderStyle* style)
{
    ASSERT(startPosition < endPosition);
    ASSERT(style);

    FontCachePurgePreventer fontCachePurgePreventer;

    RenderSVGInlineText* textRenderer = toRenderSVGInlineText(this->textRenderer());
    ASSERT(textRenderer);

    float scalingFactor = textRenderer->scalingFactor();
    ASSERT(scalingFactor);

    const Font& scaledFont = textRenderer->scaledFont();
    const FontMetrics& scaledFontMetrics = scaledFont.fontMetrics();
    FloatPoint textOrigin(fragment.x, fragment.y);
    if (scalingFactor != 1)
        textOrigin.scale(scalingFactor, scalingFactor);

    textOrigin.move(0, -scaledFontMetrics.floatAscent());

    FloatRect selectionRect = scaledFont.selectionRectForText(constructTextRun(style, fragment), textOrigin, fragment.height * scalingFactor, startPosition, endPosition);
    if (scalingFactor == 1)
        return selectionRect;

    selectionRect.scale(1 / scalingFactor);
    return selectionRect;
}

LayoutRect SVGInlineTextBox::localSelectionRect(int startPosition, int endPosition)
{
    int boxStart = start();
    startPosition = max(startPosition - boxStart, 0);
    endPosition = min(endPosition - boxStart, static_cast<int>(len()));
    if (startPosition >= endPosition)
        return LayoutRect();

    RenderText* text = textRenderer();
    ASSERT(text);

    RenderStyle* style = text->style();
    ASSERT(style);

    AffineTransform fragmentTransform;
    FloatRect selectionRect;
    int fragmentStartPosition = 0;
    int fragmentEndPosition = 0;

    unsigned textFragmentsSize = m_textFragments.size();
    for (unsigned i = 0; i < textFragmentsSize; ++i) {
        const SVGTextFragment& fragment = m_textFragments.at(i);

        fragmentStartPosition = startPosition;
        fragmentEndPosition = endPosition;
        if (!mapStartEndPositionsIntoFragmentCoordinates(fragment, fragmentStartPosition, fragmentEndPosition))
            continue;

        FloatRect fragmentRect = selectionRectForTextFragment(fragment, fragmentStartPosition, fragmentEndPosition, style);
        fragment.buildFragmentTransform(fragmentTransform);
        if (!fragmentTransform.isIdentity())
            fragmentRect = fragmentTransform.mapRect(fragmentRect);

        selectionRect.unite(fragmentRect);
    }

    return enclosingIntRect(selectionRect);
}

static inline bool textShouldBePainted(RenderSVGInlineText* textRenderer)
{
    // Font::pixelSize(), returns FontDescription::computedPixelSize(), which returns "int(x + 0.5)".
    // If the absolute font size on screen is below x=0.5, don't render anything.
    return textRenderer->scaledFont().pixelSize();
}

void SVGInlineTextBox::paintSelectionBackground(PaintInfo& paintInfo)
{
    ASSERT(paintInfo.shouldPaintWithinRoot(renderer()));
    ASSERT(paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection);
    ASSERT(truncation() == cNoTruncation);

    if (renderer()->style()->visibility() != VISIBLE)
        return;

    RenderObject* parentRenderer = parent()->renderer();
    ASSERT(parentRenderer);
    ASSERT(!parentRenderer->document()->printing());

    // Determine whether or not we're selected.
    bool paintSelectedTextOnly = paintInfo.phase == PaintPhaseSelection;
    bool hasSelection = selectionState() != RenderObject::SelectionNone;
    if (!hasSelection || paintSelectedTextOnly)
        return;

    Color backgroundColor = renderer()->selectionBackgroundColor();
    if (!backgroundColor.isValid() || !backgroundColor.alpha())
        return;

    RenderSVGInlineText* textRenderer = toRenderSVGInlineText(this->textRenderer());
    ASSERT(textRenderer);
    if (!textShouldBePainted(textRenderer))
        return;

    RenderStyle* style = parentRenderer->style();
    ASSERT(style);

    RenderStyle* selectionStyle = style;
    if (hasSelection) {
        selectionStyle = parentRenderer->getCachedPseudoStyle(SELECTION);
        if (!selectionStyle)
            selectionStyle = style;
    }

    int startPosition, endPosition;
    selectionStartEnd(startPosition, endPosition);

    int fragmentStartPosition = 0;
    int fragmentEndPosition = 0;
    AffineTransform fragmentTransform;
    unsigned textFragmentsSize = m_textFragments.size();
    for (unsigned i = 0; i < textFragmentsSize; ++i) {
        SVGTextFragment& fragment = m_textFragments.at(i);
        ASSERT(!m_paintingResource);

        fragmentStartPosition = startPosition;
        fragmentEndPosition = endPosition;
        if (!mapStartEndPositionsIntoFragmentCoordinates(fragment, fragmentStartPosition, fragmentEndPosition))
            continue;

        GraphicsContextStateSaver stateSaver(*paintInfo.context);
        fragment.buildFragmentTransform(fragmentTransform);
        if (!fragmentTransform.isIdentity())
            paintInfo.context->concatCTM(fragmentTransform);

        paintInfo.context->setFillColor(backgroundColor, style->colorSpace());
        paintInfo.context->fillRect(selectionRectForTextFragment(fragment, fragmentStartPosition, fragmentEndPosition, style), backgroundColor, style->colorSpace());

        m_paintingResourceMode = ApplyToDefaultMode;
    }

    ASSERT(!m_paintingResource);
}

void SVGInlineTextBox::paint(PaintInfo& paintInfo, const LayoutPoint&, LayoutUnit, LayoutUnit)
{
    ASSERT(paintInfo.shouldPaintWithinRoot(renderer()));
    ASSERT(paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection);
    ASSERT(truncation() == cNoTruncation);

    if (renderer()->style()->visibility() != VISIBLE)
        return;

    // Note: We're explicitely not supporting composition & custom underlines and custom highlighters - unlike InlineTextBox.
    // If we ever need that for SVG, it's very easy to refactor and reuse the code.

    RenderObject* parentRenderer = parent()->renderer();
    ASSERT(parentRenderer);

    bool paintSelectedTextOnly = paintInfo.phase == PaintPhaseSelection;
    bool hasSelection = !parentRenderer->document()->printing() && selectionState() != RenderObject::SelectionNone;
    if (!hasSelection && paintSelectedTextOnly)
        return;

    RenderSVGInlineText* textRenderer = toRenderSVGInlineText(this->textRenderer());
    ASSERT(textRenderer);
    if (!textShouldBePainted(textRenderer))
        return;

    RenderStyle* style = parentRenderer->style();
    ASSERT(style);

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    bool hasFill = svgStyle->hasFill();
    bool hasVisibleStroke = svgStyle->hasVisibleStroke();

    RenderStyle* selectionStyle = style;
    if (hasSelection) {
        selectionStyle = parentRenderer->getCachedPseudoStyle(SELECTION);
        if (selectionStyle) {
            const SVGRenderStyle* svgSelectionStyle = selectionStyle->svgStyle();
            ASSERT(svgSelectionStyle);

            if (!hasFill)
                hasFill = svgSelectionStyle->hasFill();
            if (!hasVisibleStroke)
                hasVisibleStroke = svgSelectionStyle->hasVisibleStroke();
        } else
            selectionStyle = style;
    }

    if (textRenderer->frame() && textRenderer->frame()->view() && textRenderer->frame()->view()->paintBehavior() & PaintBehaviorRenderingSVGMask) {
        hasFill = true;
        hasVisibleStroke = false;
    }

    AffineTransform fragmentTransform;
    unsigned textFragmentsSize = m_textFragments.size();
    for (unsigned i = 0; i < textFragmentsSize; ++i) {
        SVGTextFragment& fragment = m_textFragments.at(i);
        ASSERT(!m_paintingResource);

        GraphicsContextStateSaver stateSaver(*paintInfo.context);
        fragment.buildFragmentTransform(fragmentTransform);
        if (!fragmentTransform.isIdentity())
            paintInfo.context->concatCTM(fragmentTransform);

        // Spec: All text decorations except line-through should be drawn before the text is filled and stroked; thus, the text is rendered on top of these decorations.
        int decorations = style->textDecorationsInEffect();
        if (decorations & TextDecorationUnderline)
            paintDecoration(paintInfo.context, TextDecorationUnderline, fragment);
        if (decorations & TextDecorationOverline)
            paintDecoration(paintInfo.context, TextDecorationOverline, fragment);

        // Fill text
        if (hasFill) {
            m_paintingResourceMode = ApplyToFillMode | ApplyToTextMode;
            paintText(paintInfo.context, style, selectionStyle, fragment, hasSelection, paintSelectedTextOnly);
        }

        // Stroke text
        if (hasVisibleStroke) {
            m_paintingResourceMode = ApplyToStrokeMode | ApplyToTextMode;
            paintText(paintInfo.context, style, selectionStyle, fragment, hasSelection, paintSelectedTextOnly);
        }

        // Spec: Line-through should be drawn after the text is filled and stroked; thus, the line-through is rendered on top of the text.
        if (decorations & TextDecorationLineThrough)
            paintDecoration(paintInfo.context, TextDecorationLineThrough, fragment);

        m_paintingResourceMode = ApplyToDefaultMode;
    }

    ASSERT(!m_paintingResource);
}

bool SVGInlineTextBox::acquirePaintingResource(GraphicsContext*& context, float scalingFactor, RenderObject* renderer, RenderStyle* style)
{
    ASSERT(scalingFactor);
    ASSERT(renderer);
    ASSERT(style);
    ASSERT(m_paintingResourceMode != ApplyToDefaultMode);

    Color fallbackColor;
    if (m_paintingResourceMode & ApplyToFillMode)
        m_paintingResource = RenderSVGResource::fillPaintingResource(renderer, style, fallbackColor);
    else if (m_paintingResourceMode & ApplyToStrokeMode)
        m_paintingResource = RenderSVGResource::strokePaintingResource(renderer, style, fallbackColor);
    else {
        // We're either called for stroking or filling.
        ASSERT_NOT_REACHED();
    }

    if (!m_paintingResource)
        return false;

    if (!m_paintingResource->applyResource(renderer, style, context, m_paintingResourceMode)) {
        if (fallbackColor.isValid()) {
            RenderSVGResourceSolidColor* fallbackResource = RenderSVGResource::sharedSolidPaintingResource();
            fallbackResource->setColor(fallbackColor);

            m_paintingResource = fallbackResource;
            m_paintingResource->applyResource(renderer, style, context, m_paintingResourceMode);
        }
    }

    if (scalingFactor != 1 && m_paintingResourceMode & ApplyToStrokeMode)
        context->setStrokeThickness(context->strokeThickness() * scalingFactor);

    return true;
}

void SVGInlineTextBox::releasePaintingResource(GraphicsContext*& context, const Path* path)
{
    ASSERT(m_paintingResource);

    RenderObject* parentRenderer = parent()->renderer();
    ASSERT(parentRenderer);

    m_paintingResource->postApplyResource(parentRenderer, context, m_paintingResourceMode, path, /*RenderSVGShape*/ 0);
    m_paintingResource = 0;
}

bool SVGInlineTextBox::prepareGraphicsContextForTextPainting(GraphicsContext*& context, float scalingFactor, TextRun& textRun, RenderStyle* style)
{
    bool acquiredResource = acquirePaintingResource(context, scalingFactor, parent()->renderer(), style);
    if (!acquiredResource)
        return false;

#if ENABLE(SVG_FONTS)
    // SVG Fonts need access to the painting resource used to draw the current text chunk.
    TextRun::RenderingContext* renderingContext = textRun.renderingContext();
    if (renderingContext)
        static_cast<SVGTextRunRenderingContext*>(renderingContext)->setActivePaintingResource(m_paintingResource);
#endif

    return true;
}

void SVGInlineTextBox::restoreGraphicsContextAfterTextPainting(GraphicsContext*& context, TextRun& textRun)
{
    releasePaintingResource(context, /* path */0);

#if ENABLE(SVG_FONTS)
    TextRun::RenderingContext* renderingContext = textRun.renderingContext();
    if (renderingContext)
        static_cast<SVGTextRunRenderingContext*>(renderingContext)->setActivePaintingResource(0);
#else
    UNUSED_PARAM(textRun);
#endif
}

TextRun SVGInlineTextBox::constructTextRun(RenderStyle* style, const SVGTextFragment& fragment) const
{
    ASSERT(style);
    ASSERT(textRenderer());

    RenderText* text = textRenderer();
    ASSERT(text);

    TextRun run(text->characters() + fragment.characterOffset
                , fragment.length
                , 0 /* xPos, only relevant with allowTabs=true */
                , 0 /* padding, only relevant for justified text, not relevant for SVG */
                , TextRun::AllowTrailingExpansion
                , direction()
                , dirOverride() || style->rtlOrdering() == VisualOrder /* directionalOverride */);

    if (textRunNeedsRenderingContext(style->font()))
        run.setRenderingContext(SVGTextRunRenderingContext::create(text));

    run.disableRoundingHacks();

    // We handle letter & word spacing ourselves.
    run.disableSpacing();

    // Propagate the maximum length of the characters buffer to the TextRun, even when we're only processing a substring.
    run.setCharactersLength(text->textLength() - fragment.characterOffset);
    ASSERT(run.charactersLength() >= run.length());
    return run;
}

bool SVGInlineTextBox::mapStartEndPositionsIntoFragmentCoordinates(const SVGTextFragment& fragment, int& startPosition, int& endPosition) const
{
    if (startPosition >= endPosition)
        return false;

    int offset = static_cast<int>(fragment.characterOffset) - start();
    int length = static_cast<int>(fragment.length);

    if (startPosition >= offset + length || endPosition <= offset)
        return false;

    if (startPosition < offset)
        startPosition = 0;
    else
        startPosition -= offset;

    if (endPosition > offset + length)
        endPosition = length;
    else {
        ASSERT(endPosition >= offset);
        endPosition -= offset;
    }

    ASSERT(startPosition < endPosition);
    return true;
}

static inline float positionOffsetForDecoration(TextDecoration decoration, const FontMetrics& fontMetrics, float thickness)
{
    // FIXME: For SVG Fonts we need to use the attributes defined in the <font-face> if specified.
    // Compatible with Batik/Opera.
    if (decoration == TextDecorationUnderline)
        return fontMetrics.floatAscent() + thickness * 1.5f;
    if (decoration == TextDecorationOverline)
        return thickness;
    if (decoration == TextDecorationLineThrough)
        return fontMetrics.floatAscent() * 5 / 8.0f;

    ASSERT_NOT_REACHED();
    return 0.0f;
}

static inline float thicknessForDecoration(TextDecoration, const Font& font)
{
    // FIXME: For SVG Fonts we need to use the attributes defined in the <font-face> if specified.
    // Compatible with Batik/Opera
    return font.size() / 20.0f;
}

static inline RenderObject* findRenderObjectDefininingTextDecoration(InlineFlowBox* parentBox)
{
    // Lookup first render object in parent hierarchy which has text-decoration set.
    RenderObject* renderer = 0;
    while (parentBox) {
        renderer = parentBox->renderer();

        if (renderer->style() && renderer->style()->textDecoration() != TextDecorationNone)
            break;

        parentBox = parentBox->parent();
    }

    ASSERT(renderer);
    return renderer;
}

void SVGInlineTextBox::paintDecoration(GraphicsContext* context, TextDecoration decoration, const SVGTextFragment& fragment)
{
    if (textRenderer()->style()->textDecorationsInEffect() == TextDecorationNone)
        return;

    // Find out which render style defined the text-decoration, as its fill/stroke properties have to be used for drawing instead of ours.
    RenderObject* decorationRenderer = findRenderObjectDefininingTextDecoration(parent());
    RenderStyle* decorationStyle = decorationRenderer->style();
    ASSERT(decorationStyle);

    if (decorationStyle->visibility() == HIDDEN)
        return;

    const SVGRenderStyle* svgDecorationStyle = decorationStyle->svgStyle();
    ASSERT(svgDecorationStyle);

    bool hasDecorationFill = svgDecorationStyle->hasFill();
    bool hasVisibleDecorationStroke = svgDecorationStyle->hasVisibleStroke();

    if (hasDecorationFill) {
        m_paintingResourceMode = ApplyToFillMode;
        paintDecorationWithStyle(context, decoration, fragment, decorationRenderer);
    }

    if (hasVisibleDecorationStroke) {
        m_paintingResourceMode = ApplyToStrokeMode;
        paintDecorationWithStyle(context, decoration, fragment, decorationRenderer);
    }
}

void SVGInlineTextBox::paintDecorationWithStyle(GraphicsContext* context, TextDecoration decoration, const SVGTextFragment& fragment, RenderObject* decorationRenderer)
{
    ASSERT(!m_paintingResource);
    ASSERT(m_paintingResourceMode != ApplyToDefaultMode);

    RenderStyle* decorationStyle = decorationRenderer->style();
    ASSERT(decorationStyle);

    float scalingFactor = 1;
    Font scaledFont;
    RenderSVGInlineText::computeNewScaledFontForStyle(decorationRenderer, decorationStyle, scalingFactor, scaledFont);
    ASSERT(scalingFactor);

    // The initial y value refers to overline position.
    float thickness = thicknessForDecoration(decoration, scaledFont);

    if (fragment.width <= 0 && thickness <= 0)
        return;

    FloatPoint decorationOrigin(fragment.x, fragment.y);
    float width = fragment.width;
    const FontMetrics& scaledFontMetrics = scaledFont.fontMetrics();

    GraphicsContextStateSaver stateSaver(*context);
    if (scalingFactor != 1) {
        width *= scalingFactor;
        decorationOrigin.scale(scalingFactor, scalingFactor);
        context->scale(FloatSize(1 / scalingFactor, 1 / scalingFactor));
    }

    decorationOrigin.move(0, -scaledFontMetrics.floatAscent() + positionOffsetForDecoration(decoration, scaledFontMetrics, thickness));

    Path path;
    path.addRect(FloatRect(decorationOrigin, FloatSize(width, thickness)));

    if (acquirePaintingResource(context, scalingFactor, decorationRenderer, decorationStyle))
        releasePaintingResource(context, &path);
}

void SVGInlineTextBox::paintTextWithShadows(GraphicsContext* context, RenderStyle* style, TextRun& textRun, const SVGTextFragment& fragment, int startPosition, int endPosition)
{
    RenderSVGInlineText* textRenderer = toRenderSVGInlineText(this->textRenderer());
    ASSERT(textRenderer);

    float scalingFactor = textRenderer->scalingFactor();
    ASSERT(scalingFactor);

    const Font& scaledFont = textRenderer->scaledFont();
    const ShadowData* shadow = style->textShadow();

    FloatPoint textOrigin(fragment.x, fragment.y);
    FloatSize textSize(fragment.width, fragment.height);

    if (scalingFactor != 1) {
        textOrigin.scale(scalingFactor, scalingFactor);
        textSize.scale(scalingFactor);
    }

    FloatRect shadowRect(FloatPoint(textOrigin.x(), textOrigin.y() - scaledFont.fontMetrics().floatAscent()), textSize);

    do {
        if (!prepareGraphicsContextForTextPainting(context, scalingFactor, textRun, style))
            break;

        FloatSize extraOffset;
        if (shadow)
            extraOffset = applyShadowToGraphicsContext(context, shadow, shadowRect, false /* stroked */, true /* opaque */, true /* horizontal */);

        context->save();
        context->scale(FloatSize(1 / scalingFactor, 1 / scalingFactor));

        scaledFont.drawText(context, textRun, textOrigin + extraOffset, startPosition, endPosition);

        context->restore();

        restoreGraphicsContextAfterTextPainting(context, textRun);

        if (!shadow)
            break;

        if (shadow->next())
            context->restore();
        else
            context->clearShadow();

        shadow = shadow->next();
    } while (shadow);
}

void SVGInlineTextBox::paintText(GraphicsContext* context, RenderStyle* style, RenderStyle* selectionStyle, const SVGTextFragment& fragment, bool hasSelection, bool paintSelectedTextOnly)
{
    ASSERT(style);
    ASSERT(selectionStyle);

    int startPosition = 0;
    int endPosition = 0;
    if (hasSelection) {
        selectionStartEnd(startPosition, endPosition);
        hasSelection = mapStartEndPositionsIntoFragmentCoordinates(fragment, startPosition, endPosition);
    }

    // Fast path if there is no selection, just draw the whole chunk part using the regular style
    TextRun textRun = constructTextRun(style, fragment);
    if (!hasSelection || startPosition >= endPosition) {
        paintTextWithShadows(context, style, textRun, fragment, 0, fragment.length);
        return;
    }

    // Eventually draw text using regular style until the start position of the selection
    if (startPosition > 0 && !paintSelectedTextOnly)
        paintTextWithShadows(context, style, textRun, fragment, 0, startPosition);

    // Draw text using selection style from the start to the end position of the selection
    if (style != selectionStyle)
        SVGResourcesCache::clientStyleChanged(parent()->renderer(), StyleDifferenceRepaint, selectionStyle);

    TextRun selectionTextRun = constructTextRun(selectionStyle, fragment);
    paintTextWithShadows(context, selectionStyle, textRun, fragment, startPosition, endPosition);

    if (style != selectionStyle)
        SVGResourcesCache::clientStyleChanged(parent()->renderer(), StyleDifferenceRepaint, style);

    // Eventually draw text using regular style from the end position of the selection to the end of the current chunk part
    if (endPosition < static_cast<int>(fragment.length) && !paintSelectedTextOnly)
        paintTextWithShadows(context, style, textRun, fragment, endPosition, fragment.length);
}

FloatRect SVGInlineTextBox::calculateBoundaries() const
{
    FloatRect textRect;

    RenderSVGInlineText* textRenderer = toRenderSVGInlineText(this->textRenderer());
    ASSERT(textRenderer);

    float scalingFactor = textRenderer->scalingFactor();
    ASSERT(scalingFactor);

    float baseline = textRenderer->scaledFont().fontMetrics().floatAscent() / scalingFactor;

    AffineTransform fragmentTransform;
    unsigned textFragmentsSize = m_textFragments.size();
    for (unsigned i = 0; i < textFragmentsSize; ++i) {
        const SVGTextFragment& fragment = m_textFragments.at(i);
        FloatRect fragmentRect(fragment.x, fragment.y - baseline, fragment.width, fragment.height);
        fragment.buildFragmentTransform(fragmentTransform);
        if (!fragmentTransform.isIdentity())
            fragmentRect = fragmentTransform.mapRect(fragmentRect);

        textRect.unite(fragmentRect);
    }

    return textRect;
}

bool SVGInlineTextBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit, LayoutUnit)
{
    // FIXME: integrate with InlineTextBox::nodeAtPoint better.
    ASSERT(!isLineBreak());

    PointerEventsHitRules hitRules(PointerEventsHitRules::SVG_TEXT_HITTESTING, request, renderer()->style()->pointerEvents());
    bool isVisible = renderer()->style()->visibility() == VISIBLE;
    if (isVisible || !hitRules.requireVisible) {
        if ((hitRules.canHitStroke && (renderer()->style()->svgStyle()->hasStroke() || !hitRules.requireStroke))
            || (hitRules.canHitFill && (renderer()->style()->svgStyle()->hasFill() || !hitRules.requireFill))) {
            FloatPoint boxOrigin(x(), y());
            boxOrigin.moveBy(accumulatedOffset);
            FloatRect rect(boxOrigin, size());
            if (locationInContainer.intersects(rect)) {
                renderer()->updateHitTestResult(result, locationInContainer.point() - toLayoutSize(accumulatedOffset));
                if (!result.addNodeToRectBasedTestResult(renderer()->node(), request, locationInContainer, rect))
                    return true;
             }
        }
    }
    return false;
}

} // namespace WebCore

#endif
