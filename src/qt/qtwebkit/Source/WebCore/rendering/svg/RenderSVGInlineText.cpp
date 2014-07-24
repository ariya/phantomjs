/*
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 * Copyright (C) 2006 Apple Computer Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "RenderSVGInlineText.h"

#include "CSSFontSelector.h"
#include "FloatConversion.h"
#include "FloatQuad.h"
#include "RenderBlock.h"
#include "RenderSVGRoot.h"
#include "RenderSVGText.h"
#include "Settings.h"
#include "SVGInlineTextBox.h"
#include "SVGRenderingContext.h"
#include "SVGRootInlineBox.h"
#include "StyleResolver.h"
#include "VisiblePosition.h"

namespace WebCore {

static PassRefPtr<StringImpl> applySVGWhitespaceRules(PassRefPtr<StringImpl> string, bool preserveWhiteSpace)
{
    if (preserveWhiteSpace) {
        // Spec: When xml:space="preserve", the SVG user agent will do the following using a
        // copy of the original character data content. It will convert all newline and tab
        // characters into space characters. Then, it will draw all space characters, including
        // leading, trailing and multiple contiguous space characters.
        RefPtr<StringImpl> newString = string->replace('\t', ' ');
        newString = newString->replace('\n', ' ');
        newString = newString->replace('\r', ' ');
        return newString.release();
    }

    // Spec: When xml:space="default", the SVG user agent will do the following using a
    // copy of the original character data content. First, it will remove all newline
    // characters. Then it will convert all tab characters into space characters.
    // Then, it will strip off all leading and trailing space characters.
    // Then, all contiguous space characters will be consolidated.
    RefPtr<StringImpl> newString = string->replace('\n', StringImpl::empty());
    newString = newString->replace('\r', StringImpl::empty());
    newString = newString->replace('\t', ' ');
    return newString.release();
}

RenderSVGInlineText::RenderSVGInlineText(Node* n, PassRefPtr<StringImpl> string)
    : RenderText(n, applySVGWhitespaceRules(string, false))
    , m_scalingFactor(1)
    , m_layoutAttributes(this)
{
}

void RenderSVGInlineText::setTextInternal(PassRefPtr<StringImpl> text)
{
    RenderText::setTextInternal(text);
    if (RenderSVGText* textRenderer = RenderSVGText::locateRenderSVGTextAncestor(this))
        textRenderer->subtreeTextDidChange(this);
}

void RenderSVGInlineText::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderText::styleDidChange(diff, oldStyle);
    updateScaledFont();

    bool newPreserves = style() ? style()->whiteSpace() == PRE : false;
    bool oldPreserves = oldStyle ? oldStyle->whiteSpace() == PRE : false;
    if (oldPreserves && !newPreserves) {
        setText(applySVGWhitespaceRules(originalText(), false), true);
        return;
    }

    if (!oldPreserves && newPreserves) {
        setText(applySVGWhitespaceRules(originalText(), true), true);
        return;
    }

    if (diff != StyleDifferenceLayout)
        return;

    // The text metrics may be influenced by style changes.
    if (RenderSVGText* textRenderer = RenderSVGText::locateRenderSVGTextAncestor(this))
        textRenderer->subtreeStyleDidChange(this);
}

InlineTextBox* RenderSVGInlineText::createTextBox()
{
    InlineTextBox* box = new (renderArena()) SVGInlineTextBox(this);
    box->setHasVirtualLogicalHeight();
    return box;
}

LayoutRect RenderSVGInlineText::localCaretRect(InlineBox* box, int caretOffset, LayoutUnit*)
{
    if (!box || !box->isInlineTextBox())
        return LayoutRect();

    InlineTextBox* textBox = static_cast<InlineTextBox*>(box);
    if (static_cast<unsigned>(caretOffset) < textBox->start() || static_cast<unsigned>(caretOffset) > textBox->start() + textBox->len())
        return LayoutRect();

    // Use the edge of the selection rect to determine the caret rect.
    if (static_cast<unsigned>(caretOffset) < textBox->start() + textBox->len()) {
        LayoutRect rect = textBox->localSelectionRect(caretOffset, caretOffset + 1);
        LayoutUnit x = box->isLeftToRightDirection() ? rect.x() : rect.maxX();
        return LayoutRect(x, rect.y(), caretWidth, rect.height());
    }

    LayoutRect rect = textBox->localSelectionRect(caretOffset - 1, caretOffset);
    LayoutUnit x = box->isLeftToRightDirection() ? rect.maxX() : rect.x();
    return LayoutRect(x, rect.y(), caretWidth, rect.height());
}

FloatRect RenderSVGInlineText::floatLinesBoundingBox() const
{
    FloatRect boundingBox;
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
        boundingBox.unite(box->calculateBoundaries());
    return boundingBox;
}

IntRect RenderSVGInlineText::linesBoundingBox() const
{
    return enclosingIntRect(floatLinesBoundingBox());
}

bool RenderSVGInlineText::characterStartsNewTextChunk(int position) const
{
    ASSERT(position >= 0);
    ASSERT(position < static_cast<int>(textLength()));

    // Each <textPath> element starts a new text chunk, regardless of any x/y values.
    if (!position && parent()->isSVGTextPath() && !previousSibling())
        return true;

    const SVGCharacterDataMap::const_iterator it = m_layoutAttributes.characterDataMap().find(static_cast<unsigned>(position + 1));
    if (it == m_layoutAttributes.characterDataMap().end())
        return false;

    return it->value.x != SVGTextLayoutAttributes::emptyValue() || it->value.y != SVGTextLayoutAttributes::emptyValue();
}

VisiblePosition RenderSVGInlineText::positionForPoint(const LayoutPoint& point)
{
    if (!firstTextBox() || !textLength())
        return createVisiblePosition(0, DOWNSTREAM);

    float baseline = m_scaledFont.fontMetrics().floatAscent();

    RenderBlock* containingBlock = this->containingBlock();
    ASSERT(containingBlock);

    // Map local point to absolute point, as the character origins stored in the text fragments use absolute coordinates.
    FloatPoint absolutePoint(point);
    absolutePoint.moveBy(containingBlock->location());

    float closestDistance = std::numeric_limits<float>::max();
    float closestDistancePosition = 0;
    const SVGTextFragment* closestDistanceFragment = 0;
    SVGInlineTextBox* closestDistanceBox = 0;

    AffineTransform fragmentTransform;
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        if (!box->isSVGInlineTextBox())
            continue;

        SVGInlineTextBox* textBox = toSVGInlineTextBox(box);
        Vector<SVGTextFragment>& fragments = textBox->textFragments();

        unsigned textFragmentsSize = fragments.size();
        for (unsigned i = 0; i < textFragmentsSize; ++i) {
            const SVGTextFragment& fragment = fragments.at(i);
            FloatRect fragmentRect(fragment.x, fragment.y - baseline, fragment.width, fragment.height);
            fragment.buildFragmentTransform(fragmentTransform);
            if (!fragmentTransform.isIdentity())
                fragmentRect = fragmentTransform.mapRect(fragmentRect);

            float distance = powf(fragmentRect.x() - absolutePoint.x(), 2) +
                             powf(fragmentRect.y() + fragmentRect.height() / 2 - absolutePoint.y(), 2);

            if (distance < closestDistance) {
                closestDistance = distance;
                closestDistanceBox = textBox;
                closestDistanceFragment = &fragment;
                closestDistancePosition = fragmentRect.x();
            }
        }
    }

    if (!closestDistanceFragment)
        return createVisiblePosition(0, DOWNSTREAM);

    int offset = closestDistanceBox->offsetForPositionInFragment(*closestDistanceFragment, absolutePoint.x() - closestDistancePosition, true);
    return createVisiblePosition(offset + closestDistanceBox->start(), offset > 0 ? VP_UPSTREAM_IF_POSSIBLE : DOWNSTREAM);
}

void RenderSVGInlineText::updateScaledFont()
{
    computeNewScaledFontForStyle(this, style(), m_scalingFactor, m_scaledFont);
}

void RenderSVGInlineText::computeNewScaledFontForStyle(RenderObject* renderer, const RenderStyle* style, float& scalingFactor, Font& scaledFont)
{
    ASSERT(style);
    ASSERT(renderer);

    Document* document = renderer->document();
    ASSERT(document);
    
    StyleResolver* styleResolver = document->ensureStyleResolver();

    // Alter font-size to the right on-screen value to avoid scaling the glyphs themselves, except when GeometricPrecision is specified
    scalingFactor = SVGRenderingContext::calculateScreenFontSizeScalingFactor(renderer);
    if (scalingFactor == 1 || !scalingFactor || style->fontDescription().textRenderingMode() == GeometricPrecision) {
        scalingFactor = 1;
        scaledFont = style->font();
        return;
    }

    FontDescription fontDescription(style->fontDescription());

    // FIXME: We need to better handle the case when we compute very small fonts below (below 1pt).
    fontDescription.setComputedSize(StyleResolver::getComputedSizeFromSpecifiedSize(document, scalingFactor, fontDescription.isAbsoluteSize(), fontDescription.computedSize(), DoNotUseSmartMinimumForFontSize));

    scaledFont = Font(fontDescription, 0, 0);
    scaledFont.update(styleResolver->fontSelector());
}

}

#endif // ENABLE(SVG)
