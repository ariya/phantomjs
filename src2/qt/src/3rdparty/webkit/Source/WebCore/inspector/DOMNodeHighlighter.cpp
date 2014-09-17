/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMNodeHighlighter.h"

#if ENABLE(INSPECTOR)

#include "Element.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "Page.h"
#include "Range.h"
#include "RenderInline.h"
#include "Settings.h"
#include "StyledElement.h"
#include "TextRun.h"

namespace WebCore {

namespace {

Path quadToPath(const FloatQuad& quad)
{
    Path quadPath;
    quadPath.moveTo(quad.p1());
    quadPath.addLineTo(quad.p2());
    quadPath.addLineTo(quad.p3());
    quadPath.addLineTo(quad.p4());
    quadPath.closeSubpath();
    return quadPath;
}

void drawOutlinedQuad(GraphicsContext& context, const FloatQuad& quad, const Color& fillColor)
{
    static const int outlineThickness = 2;
    static const Color outlineColor(62, 86, 180, 228);

    Path quadPath = quadToPath(quad);

    // Clip out the quad, then draw with a 2px stroke to get a pixel
    // of outline (because inflating a quad is hard)
    {
        context.save();
        context.clipOut(quadPath);

        context.setStrokeThickness(outlineThickness);
        context.setStrokeColor(outlineColor, ColorSpaceDeviceRGB);
        context.strokePath(quadPath);

        context.restore();
    }

    // Now do the fill
    context.setFillColor(fillColor, ColorSpaceDeviceRGB);
    context.fillPath(quadPath);
}

void drawOutlinedQuadWithClip(GraphicsContext& context, const FloatQuad& quad, const FloatQuad& clipQuad, const Color& fillColor)
{
    context.save();
    Path clipQuadPath = quadToPath(clipQuad);
    context.clipOut(clipQuadPath);
    drawOutlinedQuad(context, quad, fillColor);
    context.restore();
}

void drawHighlightForBox(GraphicsContext& context, const FloatQuad& contentQuad, const FloatQuad& paddingQuad, const FloatQuad& borderQuad, const FloatQuad& marginQuad, DOMNodeHighlighter::HighlightMode mode)
{
    static const Color contentBoxColor(125, 173, 217, 128);
    static const Color paddingBoxColor(125, 173, 217, 160);
    static const Color borderBoxColor(125, 173, 217, 192);
    static const Color marginBoxColor(125, 173, 217, 228);

    FloatQuad clipQuad;
    if (mode == DOMNodeHighlighter::HighlightMargin || (mode == DOMNodeHighlighter::HighlightAll && marginQuad != borderQuad)) {
        drawOutlinedQuadWithClip(context, marginQuad, borderQuad, marginBoxColor);
        clipQuad = borderQuad;
    }
    if (mode == DOMNodeHighlighter::HighlightBorder || (mode == DOMNodeHighlighter::HighlightAll && borderQuad != paddingQuad)) {
        drawOutlinedQuadWithClip(context, borderQuad, paddingQuad, borderBoxColor);
        clipQuad = paddingQuad;
    }
    if (mode == DOMNodeHighlighter::HighlightPadding || (mode == DOMNodeHighlighter::HighlightAll && paddingQuad != contentQuad)) {
        drawOutlinedQuadWithClip(context, paddingQuad, contentQuad, paddingBoxColor);
        clipQuad = contentQuad;
    }
    if (mode == DOMNodeHighlighter::HighlightContent || mode == DOMNodeHighlighter::HighlightAll)
        drawOutlinedQuad(context, contentQuad, contentBoxColor);
    else
        drawOutlinedQuadWithClip(context, clipQuad, clipQuad, contentBoxColor);
}

void drawHighlightForLineBoxesOrSVGRenderer(GraphicsContext& context, const Vector<FloatQuad>& lineBoxQuads)
{
    static const Color lineBoxColor(125, 173, 217, 128);

    for (size_t i = 0; i < lineBoxQuads.size(); ++i)
        drawOutlinedQuad(context, lineBoxQuads[i], lineBoxColor);
}

inline IntSize frameToMainFrameOffset(Frame* frame)
{
    IntPoint mainFramePoint = frame->page()->mainFrame()->view()->windowToContents(frame->view()->contentsToWindow(IntPoint()));
    return mainFramePoint - IntPoint();
}

void drawElementTitle(GraphicsContext& context, Node* node, const IntRect& boundingBox, const IntRect& anchorBox, const FloatRect& overlayRect, WebCore::Settings* settings)
{
    static const int rectInflatePx = 4;
    static const int fontHeightPx = 12;
    static const int borderWidthPx = 1;
    static const Color tooltipBackgroundColor(255, 255, 194, 255);
    static const Color tooltipBorderColor(Color::black);
    static const Color tooltipFontColor(Color::black);

    Element* element = static_cast<Element*>(node);
    bool isXHTML = element->document()->isXHTMLDocument();
    String nodeTitle = isXHTML ? element->nodeName() : element->nodeName().lower();
    const AtomicString& idValue = element->getIdAttribute();
    if (!idValue.isNull() && !idValue.isEmpty()) {
        nodeTitle += "#";
        nodeTitle += idValue;
    }
    if (element->hasClass() && element->isStyledElement()) {
        const SpaceSplitString& classNamesString = static_cast<StyledElement*>(element)->classNames();
        size_t classNameCount = classNamesString.size();
        if (classNameCount) {
            HashSet<AtomicString> usedClassNames;
            for (size_t i = 0; i < classNameCount; ++i) {
                const AtomicString& className = classNamesString[i];
                if (usedClassNames.contains(className))
                    continue;
                usedClassNames.add(className);
                nodeTitle += ".";
                nodeTitle += className;
            }
        }
    }

    nodeTitle += " [";
    nodeTitle += String::number(boundingBox.width());
    nodeTitle.append(static_cast<UChar>(0x00D7)); // &times;
    nodeTitle += String::number(boundingBox.height());
    nodeTitle += "]";

    FontDescription desc;
    FontFamily family;
    family.setFamily(settings->fixedFontFamily());
    desc.setFamily(family);
    desc.setComputedSize(fontHeightPx);
    Font font = Font(desc, 0, 0);
    font.update(0);

    TextRun nodeTitleRun(nodeTitle);
    IntPoint titleBasePoint = IntPoint(anchorBox.x(), anchorBox.maxY() - 1);
    titleBasePoint.move(rectInflatePx, rectInflatePx);
    IntRect titleRect = enclosingIntRect(font.selectionRectForText(nodeTitleRun, titleBasePoint, fontHeightPx));
    titleRect.inflate(rectInflatePx);

    // The initial offsets needed to compensate for a 1px-thick border stroke (which is not a part of the rectangle).
    int dx = -borderWidthPx;
    int dy = borderWidthPx;

    // If the tip sticks beyond the right of overlayRect, right-align the tip with the said boundary.
    if (titleRect.maxX() > overlayRect.maxX())
        dx = overlayRect.maxX() - titleRect.maxX();

    // If the tip sticks beyond the left of overlayRect, left-align the tip with the said boundary.
    if (titleRect.x() + dx < overlayRect.x())
        dx = overlayRect.x() - titleRect.x() - borderWidthPx;

    // If the tip sticks beyond the bottom of overlayRect, show the tip at top of bounding box.
    if (titleRect.maxY() > overlayRect.maxY()) {
        dy = anchorBox.y() - titleRect.maxY() - borderWidthPx;
        // If the tip still sticks beyond the bottom of overlayRect, bottom-align the tip with the said boundary.
        if (titleRect.maxY() + dy > overlayRect.maxY())
            dy = overlayRect.maxY() - titleRect.maxY();
    }

    // If the tip sticks beyond the top of overlayRect, show the tip at top of overlayRect.
    if (titleRect.y() + dy < overlayRect.y())
        dy = overlayRect.y() - titleRect.y() + borderWidthPx;

    titleRect.move(dx, dy);
    context.setStrokeColor(tooltipBorderColor, ColorSpaceDeviceRGB);
    context.setStrokeThickness(borderWidthPx);
    context.setFillColor(tooltipBackgroundColor, ColorSpaceDeviceRGB);
    context.drawRect(titleRect);
    context.setFillColor(tooltipFontColor, ColorSpaceDeviceRGB);
    context.drawText(font, nodeTitleRun, IntPoint(titleRect.x() + rectInflatePx, titleRect.y() + font.fontMetrics().height()));
}

} // anonymous namespace

namespace DOMNodeHighlighter {

void DrawNodeHighlight(GraphicsContext& context, Node* node, HighlightMode mode)
{
    node->document()->updateLayoutIgnorePendingStylesheets();
    RenderObject* renderer = node->renderer();
    Frame* containingFrame = node->document()->frame();

    if (!renderer || !containingFrame)
        return;

    IntSize mainFrameOffset = frameToMainFrameOffset(containingFrame);
    IntRect boundingBox = renderer->absoluteBoundingBoxRect(true);

    boundingBox.move(mainFrameOffset);

    IntRect titleAnchorBox = boundingBox;

    FrameView* view = containingFrame->page()->mainFrame()->view();
    FloatRect overlayRect = view->visibleContentRect();
    if (!overlayRect.contains(boundingBox) && !boundingBox.contains(enclosingIntRect(overlayRect)))
        overlayRect = view->visibleContentRect();
    context.translate(-overlayRect.x(), -overlayRect.y());

    // RenderSVGRoot should be highlighted through the isBox() code path, all other SVG elements should just dump their absoluteQuads().
#if ENABLE(SVG)
    bool isSVGRenderer = renderer->node() && renderer->node()->isSVGElement() && !renderer->isSVGRoot();
#else
    bool isSVGRenderer = false;
#endif

    if (renderer->isBox() && !isSVGRenderer) {
        RenderBox* renderBox = toRenderBox(renderer);

        // RenderBox returns the "pure" content area box, exclusive of the scrollbars (if present), which also count towards the content area in CSS.
        IntRect contentBox = renderBox->contentBoxRect();
        contentBox.setWidth(contentBox.width() + renderBox->verticalScrollbarWidth());
        contentBox.setHeight(contentBox.height() + renderBox->horizontalScrollbarHeight());

        IntRect paddingBox(contentBox.x() - renderBox->paddingLeft(), contentBox.y() - renderBox->paddingTop(),
                           contentBox.width() + renderBox->paddingLeft() + renderBox->paddingRight(), contentBox.height() + renderBox->paddingTop() + renderBox->paddingBottom());
        IntRect borderBox(paddingBox.x() - renderBox->borderLeft(), paddingBox.y() - renderBox->borderTop(),
                          paddingBox.width() + renderBox->borderLeft() + renderBox->borderRight(), paddingBox.height() + renderBox->borderTop() + renderBox->borderBottom());
        IntRect marginBox(borderBox.x() - renderBox->marginLeft(), borderBox.y() - renderBox->marginTop(),
                          borderBox.width() + renderBox->marginLeft() + renderBox->marginRight(), borderBox.height() + renderBox->marginTop() + renderBox->marginBottom());


        FloatQuad absContentQuad = renderBox->localToAbsoluteQuad(FloatRect(contentBox));
        FloatQuad absPaddingQuad = renderBox->localToAbsoluteQuad(FloatRect(paddingBox));
        FloatQuad absBorderQuad = renderBox->localToAbsoluteQuad(FloatRect(borderBox));
        FloatQuad absMarginQuad = renderBox->localToAbsoluteQuad(FloatRect(marginBox));

        absContentQuad.move(mainFrameOffset);
        absPaddingQuad.move(mainFrameOffset);
        absBorderQuad.move(mainFrameOffset);
        absMarginQuad.move(mainFrameOffset);

        titleAnchorBox = absMarginQuad.enclosingBoundingBox();

        drawHighlightForBox(context, absContentQuad, absPaddingQuad, absBorderQuad, absMarginQuad, mode);
    } else if (renderer->isRenderInline() || isSVGRenderer) {
        // FIXME: We should show margins/padding/border for inlines.
        Vector<FloatQuad> lineBoxQuads;
        renderer->absoluteQuads(lineBoxQuads);
        for (unsigned i = 0; i < lineBoxQuads.size(); ++i)
            lineBoxQuads[i] += mainFrameOffset;

        drawHighlightForLineBoxesOrSVGRenderer(context, lineBoxQuads);
    }

    // Draw node title if necessary.

    if (!node->isElementNode())
        return;

    WebCore::Settings* settings = containingFrame->settings();
    if (mode == DOMNodeHighlighter::HighlightAll)
        drawElementTitle(context, node, boundingBox, titleAnchorBox, overlayRect, settings);
}

} // namespace DOMNodeHighlighter

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
