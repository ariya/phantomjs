/*
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RenderTreeAsText.h"

#include "Document.h"
#include "FlowThreadController.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "InlineTextBox.h"
#include "PrintContext.h"
#include "RenderBR.h"
#include "RenderDetailsMarker.h"
#include "RenderFileUploadControl.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListItem.h"
#include "RenderListMarker.h"
#include "RenderNamedFlowThread.h"
#include "RenderPart.h"
#include "RenderRegion.h"
#include "RenderTableCell.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "StylePropertySet.h"
#include <wtf/HexNumber.h>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

#if ENABLE(SVG)
#include "RenderSVGContainer.h"
#include "RenderSVGGradientStop.h"
#include "RenderSVGImage.h"
#include "RenderSVGInlineText.h"
#include "RenderSVGPath.h"
#include "RenderSVGRoot.h"
#include "RenderSVGText.h"
#include "SVGRenderTreeAsText.h"
#endif

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerBacking.h"
#endif

#if PLATFORM(QT)
#include <QVariant>
#endif

namespace WebCore {

using namespace HTMLNames;

static void writeLayers(TextStream&, const RenderLayer* rootLayer, RenderLayer*, const LayoutRect& paintDirtyRect, int indent = 0, RenderAsTextBehavior = RenderAsTextBehaviorNormal);

TextStream& operator<<(TextStream& ts, const IntRect& r)
{
    return ts << "at (" << r.x() << "," << r.y() << ") size " << r.width() << "x" << r.height();
}

TextStream& operator<<(TextStream& ts, const IntPoint& p)
{
    return ts << "(" << p.x() << "," << p.y() << ")";
}

TextStream& operator<<(TextStream& ts, const LayoutRect& r)
{
    // FIXME: These should be printed as floats. Keeping them ints for consistency with previous test expectations.
    return ts << pixelSnappedIntRect(r);
}

TextStream& operator<<(TextStream& ts, const LayoutPoint& p)
{
    // FIXME: These should be printed as floats. Keeping them ints for consistency with previous test expectations.
    return ts << "(" << p.x().toInt() << "," << p.y().toInt() << ")";
}

TextStream& operator<<(TextStream& ts, const FloatPoint& p)
{
    ts << "(" << TextStream::FormatNumberRespectingIntegers(p.x());
    ts << "," << TextStream::FormatNumberRespectingIntegers(p.y());
    ts << ")";
    return ts;
}

TextStream& operator<<(TextStream& ts, const FloatSize& s)
{
    ts << "width=" << TextStream::FormatNumberRespectingIntegers(s.width());
    ts << " height=" << TextStream::FormatNumberRespectingIntegers(s.height());
    return ts;
}

void writeIndent(TextStream& ts, int indent)
{
    for (int i = 0; i != indent; ++i)
        ts << "  ";
}

static void printBorderStyle(TextStream& ts, const EBorderStyle borderStyle)
{
    switch (borderStyle) {
        case BNONE:
            ts << "none";
            break;
        case BHIDDEN:
            ts << "hidden";
            break;
        case INSET:
            ts << "inset";
            break;
        case GROOVE:
            ts << "groove";
            break;
        case RIDGE:
            ts << "ridge";
            break;
        case OUTSET:
            ts << "outset";
            break;
        case DOTTED:
            ts << "dotted";
            break;
        case DASHED:
            ts << "dashed";
            break;
        case SOLID:
            ts << "solid";
            break;
        case DOUBLE:
            ts << "double";
            break;
    }

    ts << " ";
}

static String getTagName(Node* n)
{
    if (n->isDocumentNode())
        return "";
    if (n->nodeType() == Node::COMMENT_NODE)
        return "COMMENT";
    return n->nodeName();
}

static bool isEmptyOrUnstyledAppleStyleSpan(const Node* node)
{
    if (!node || !node->isHTMLElement() || !node->hasTagName(spanTag))
        return false;

    const HTMLElement* elem = toHTMLElement(node);
    if (elem->getAttribute(classAttr) != "Apple-style-span")
        return false;

    if (!node->hasChildNodes())
        return true;

    const StylePropertySet* inlineStyleDecl = elem->inlineStyle();
    return (!inlineStyleDecl || inlineStyleDecl->isEmpty());
}

String quoteAndEscapeNonPrintables(const String& s)
{
    StringBuilder result;
    result.append('"');
    for (unsigned i = 0; i != s.length(); ++i) {
        UChar c = s[i];
        if (c == '\\') {
            result.append('\\');
            result.append('\\');
        } else if (c == '"') {
            result.append('\\');
            result.append('"');
        } else if (c == '\n' || c == noBreakSpace)
            result.append(' ');
        else {
            if (c >= 0x20 && c < 0x7F)
                result.append(c);
            else {
                result.append('\\');
                result.append('x');
                result.append('{');
                appendUnsignedAsHex(c, result); 
                result.append('}');
            }
        }
    }
    result.append('"');
    return result.toString();
}

void RenderTreeAsText::writeRenderObject(TextStream& ts, const RenderObject& o, RenderAsTextBehavior behavior)
{
    ts << o.renderName();

    if (behavior & RenderAsTextShowAddresses)
        ts << " " << static_cast<const void*>(&o);

    if (o.style() && o.style()->zIndex())
        ts << " zI: " << o.style()->zIndex();

    if (o.node()) {
        String tagName = getTagName(o.node());
        // FIXME: Temporary hack to make tests pass by simulating the old generated content output.
        if (o.isPseudoElement() || (o.parent() && o.parent()->isPseudoElement()))
            tagName = emptyAtom;
        if (!tagName.isEmpty()) {
            ts << " {" << tagName << "}";
            // flag empty or unstyled AppleStyleSpan because we never
            // want to leave them in the DOM
            if (isEmptyOrUnstyledAppleStyleSpan(o.node()))
                ts << " *empty or unstyled AppleStyleSpan*";
        }
    }
    
    RenderBlock* cb = o.containingBlock();
    bool adjustForTableCells = cb ? cb->isTableCell() : false;

    LayoutRect r;
    if (o.isText()) {
        // FIXME: Would be better to dump the bounding box x and y rather than the first run's x and y, but that would involve updating
        // many test results.
        const RenderText& text = *toRenderText(&o);
        IntRect linesBox = text.linesBoundingBox();
        r = IntRect(text.firstRunX(), text.firstRunY(), linesBox.width(), linesBox.height());
        if (adjustForTableCells && !text.firstTextBox())
            adjustForTableCells = false;
    } else if (o.isRenderInline()) {
        // FIXME: Would be better not to just dump 0, 0 as the x and y here.
        const RenderInline& inlineFlow = *toRenderInline(&o);
        r = IntRect(0, 0, inlineFlow.linesBoundingBox().width(), inlineFlow.linesBoundingBox().height());
        adjustForTableCells = false;
    } else if (o.isTableCell()) {
        // FIXME: Deliberately dump the "inner" box of table cells, since that is what current results reflect.  We'd like
        // to clean up the results to dump both the outer box and the intrinsic padding so that both bits of information are
        // captured by the results.
        const RenderTableCell& cell = *toRenderTableCell(&o);
        r = LayoutRect(cell.x(), cell.y() + cell.intrinsicPaddingBefore(), cell.width(), cell.height() - cell.intrinsicPaddingBefore() - cell.intrinsicPaddingAfter());
    } else if (o.isBox())
        r = toRenderBox(&o)->frameRect();

    // FIXME: Temporary in order to ensure compatibility with existing layout test results.
    if (adjustForTableCells)
        r.move(0, -toRenderTableCell(o.containingBlock())->intrinsicPaddingBefore());

    // FIXME: Convert layout test results to report sub-pixel values, in the meantime using enclosingIntRect
    // for consistency with old results.
    ts << " " << enclosingIntRect(r);

    if (!(o.isText() && !o.isBR())) {
        if (o.isFileUploadControl())
            ts << " " << quoteAndEscapeNonPrintables(toRenderFileUploadControl(&o)->fileTextValue());

        if (o.parent()) {
            Color color = o.style()->visitedDependentColor(CSSPropertyColor);
            if (o.parent()->style()->visitedDependentColor(CSSPropertyColor) != color)
                ts << " [color=" << color.nameForRenderTreeAsText() << "]";

            // Do not dump invalid or transparent backgrounds, since that is the default.
            Color backgroundColor = o.style()->visitedDependentColor(CSSPropertyBackgroundColor);
            if (o.parent()->style()->visitedDependentColor(CSSPropertyBackgroundColor) != backgroundColor
                && backgroundColor.isValid() && backgroundColor.rgb())
                ts << " [bgcolor=" << backgroundColor.nameForRenderTreeAsText() << "]";
            
            Color textFillColor = o.style()->visitedDependentColor(CSSPropertyWebkitTextFillColor);
            if (o.parent()->style()->visitedDependentColor(CSSPropertyWebkitTextFillColor) != textFillColor
                && textFillColor.isValid() && textFillColor != color && textFillColor.rgb())
                ts << " [textFillColor=" << textFillColor.nameForRenderTreeAsText() << "]";

            Color textStrokeColor = o.style()->visitedDependentColor(CSSPropertyWebkitTextStrokeColor);
            if (o.parent()->style()->visitedDependentColor(CSSPropertyWebkitTextStrokeColor) != textStrokeColor
                && textStrokeColor.isValid() && textStrokeColor != color && textStrokeColor.rgb())
                ts << " [textStrokeColor=" << textStrokeColor.nameForRenderTreeAsText() << "]";

            if (o.parent()->style()->textStrokeWidth() != o.style()->textStrokeWidth() && o.style()->textStrokeWidth() > 0)
                ts << " [textStrokeWidth=" << o.style()->textStrokeWidth() << "]";
        }

        if (!o.isBoxModelObject())
            return;

        const RenderBoxModelObject& box = *toRenderBoxModelObject(&o);
        if (box.borderTop() || box.borderRight() || box.borderBottom() || box.borderLeft()) {
            ts << " [border:";

            BorderValue prevBorder = o.style()->borderTop();
            if (!box.borderTop())
                ts << " none";
            else {
                ts << " (" << box.borderTop() << "px ";
                printBorderStyle(ts, o.style()->borderTopStyle());
                Color col = o.style()->borderTopColor();
                if (!col.isValid())
                    col = o.style()->color();
                ts << col.nameForRenderTreeAsText() << ")";
            }

            if (o.style()->borderRight() != prevBorder) {
                prevBorder = o.style()->borderRight();
                if (!box.borderRight())
                    ts << " none";
                else {
                    ts << " (" << box.borderRight() << "px ";
                    printBorderStyle(ts, o.style()->borderRightStyle());
                    Color col = o.style()->borderRightColor();
                    if (!col.isValid())
                        col = o.style()->color();
                    ts << col.nameForRenderTreeAsText() << ")";
                }
            }

            if (o.style()->borderBottom() != prevBorder) {
                prevBorder = box.style()->borderBottom();
                if (!box.borderBottom())
                    ts << " none";
                else {
                    ts << " (" << box.borderBottom() << "px ";
                    printBorderStyle(ts, o.style()->borderBottomStyle());
                    Color col = o.style()->borderBottomColor();
                    if (!col.isValid())
                        col = o.style()->color();
                    ts << col.nameForRenderTreeAsText() << ")";
                }
            }

            if (o.style()->borderLeft() != prevBorder) {
                prevBorder = o.style()->borderLeft();
                if (!box.borderLeft())
                    ts << " none";
                else {
                    ts << " (" << box.borderLeft() << "px ";
                    printBorderStyle(ts, o.style()->borderLeftStyle());
                    Color col = o.style()->borderLeftColor();
                    if (!col.isValid())
                        col = o.style()->color();
                    ts << col.nameForRenderTreeAsText() << ")";
                }
            }

            ts << "]";
        }

#if ENABLE(MATHML)
        // We want to show any layout padding, both CSS padding and intrinsic padding, so we can't just check o.style()->hasPadding().
        if (o.isRenderMathMLBlock() && (box.paddingTop() || box.paddingRight() || box.paddingBottom() || box.paddingLeft())) {
            ts << " [";
            LayoutUnit cssTop = box.computedCSSPaddingTop();
            LayoutUnit cssRight = box.computedCSSPaddingRight();
            LayoutUnit cssBottom = box.computedCSSPaddingBottom();
            LayoutUnit cssLeft = box.computedCSSPaddingLeft();
            if (box.paddingTop() != cssTop || box.paddingRight() != cssRight || box.paddingBottom() != cssBottom || box.paddingLeft() != cssLeft) {
                ts << "intrinsic ";
                if (cssTop || cssRight || cssBottom || cssLeft)
                    ts << "+ CSS ";
            }
            ts << "padding: " << roundToInt(box.paddingTop()) << " " << roundToInt(box.paddingRight()) << " " << roundToInt(box.paddingBottom()) << " " << roundToInt(box.paddingLeft()) << "]";
        }
#endif
    }

    if (o.isTableCell()) {
        const RenderTableCell& c = *toRenderTableCell(&o);
        ts << " [r=" << c.rowIndex() << " c=" << c.col() << " rs=" << c.rowSpan() << " cs=" << c.colSpan() << "]";
    }

#if ENABLE(DETAILS_ELEMENT)
    if (o.isDetailsMarker()) {
        ts << ": ";
        switch (toRenderDetailsMarker(&o)->orientation()) {
        case RenderDetailsMarker::Left:
            ts << "left";
            break;
        case RenderDetailsMarker::Right:
            ts << "right";
            break;
        case RenderDetailsMarker::Up:
            ts << "up";
            break;
        case RenderDetailsMarker::Down:
            ts << "down";
            break;
        }
    }
#endif

    if (o.isListMarker()) {
        String text = toRenderListMarker(&o)->text();
        if (!text.isEmpty()) {
            if (text.length() != 1)
                text = quoteAndEscapeNonPrintables(text);
            else {
                switch (text[0]) {
                    case bullet:
                        text = "bullet";
                        break;
                    case blackSquare:
                        text = "black square";
                        break;
                    case whiteBullet:
                        text = "white bullet";
                        break;
                    default:
                        text = quoteAndEscapeNonPrintables(text);
                }
            }
            ts << ": " << text;
        }
    }
    
    if (behavior & RenderAsTextShowIDAndClass) {
        if (Element* element = o.node() && o.node()->isElementNode() ? toElement(o.node()) : 0) {
            if (element->hasID())
                ts << " id=\"" + element->getIdAttribute() + "\"";

            if (element->hasClass()) {
                ts << " class=\"";
                for (size_t i = 0; i < element->classNames().size(); ++i) {
                    if (i > 0)
                        ts << " ";
                    ts << element->classNames()[i];
                }
                ts << "\"";
            }
        }
    }
    
    if (behavior & RenderAsTextShowLayoutState) {
        bool needsLayout = o.selfNeedsLayout() || o.needsPositionedMovementLayout() || o.posChildNeedsLayout() || o.normalChildNeedsLayout();
        if (needsLayout)
            ts << " (needs layout:";
        
        bool havePrevious = false;
        if (o.selfNeedsLayout()) {
            ts << " self";
            havePrevious = true;
        }

        if (o.needsPositionedMovementLayout()) {
            if (havePrevious)
                ts << ",";
            havePrevious = true;
            ts << " positioned movement";
        }

        if (o.normalChildNeedsLayout()) {
            if (havePrevious)
                ts << ",";
            havePrevious = true;
            ts << " child";
        }

        if (o.posChildNeedsLayout()) {
            if (havePrevious)
                ts << ",";
            ts << " positioned child";
        }

        if (needsLayout)
            ts << ")";
    }

#if PLATFORM(QT)
    // Print attributes of embedded QWidgets. E.g. when the WebCore::Widget
    // is invisible the QWidget should be invisible too.
    if (o.isRenderPart()) {
        const RenderPart* part = toRenderPart(const_cast<RenderObject*>(&o));
        if (part->widget() && part->widget()->platformWidget()) {
            QObject* wid = part->widget()->platformWidget();

            ts << " [QT: ";
            ts << "geometry: {" << wid->property("geometry").toRect() << "} ";
            ts << "isHidden: " << !wid->property("isVisible").toBool() << " ";
            ts << "isSelfVisible: " << part->widget()->isSelfVisible() << " ";
            ts << "isParentVisible: " << part->widget()->isParentVisible() << " ] ";
        }
    }
#endif
}

static void writeTextRun(TextStream& ts, const RenderText& o, const InlineTextBox& run)
{
    // FIXME: For now use an "enclosingIntRect" model for x, y and logicalWidth, although this makes it harder
    // to detect any changes caused by the conversion to floating point. :(
    int x = run.x();
    int y = run.y();
    int logicalWidth = ceilf(run.left() + run.logicalWidth()) - x;

    // FIXME: Table cell adjustment is temporary until results can be updated.
    if (o.containingBlock()->isTableCell())
        y -= toRenderTableCell(o.containingBlock())->intrinsicPaddingBefore();
        
    ts << "text run at (" << x << "," << y << ") width " << logicalWidth;
    if (!run.isLeftToRightDirection() || run.dirOverride()) {
        ts << (!run.isLeftToRightDirection() ? " RTL" : " LTR");
        if (run.dirOverride())
            ts << " override";
    }
    ts << ": "
        << quoteAndEscapeNonPrintables(String(o.text()).substring(run.start(), run.len()));
    if (run.hasHyphen())
        ts << " + hyphen string " << quoteAndEscapeNonPrintables(o.style()->hyphenString());
    ts << "\n";
}

void write(TextStream& ts, const RenderObject& o, int indent, RenderAsTextBehavior behavior)
{
#if ENABLE(SVG)
    if (o.isSVGShape()) {
        write(ts, *toRenderSVGShape(&o), indent);
        return;
    }
    if (o.isSVGGradientStop()) {
        writeSVGGradientStop(ts, *toRenderSVGGradientStop(&o), indent);
        return;
    }
    if (o.isSVGResourceContainer()) {
        writeSVGResourceContainer(ts, o, indent);
        return;
    }
    if (o.isSVGContainer()) {
        writeSVGContainer(ts, o, indent);
        return;
    }
    if (o.isSVGRoot()) {
        write(ts, *toRenderSVGRoot(&o), indent);
        return;
    }
    if (o.isSVGText()) {
        writeSVGText(ts, *toRenderSVGText(&o), indent);
        return;
    }
    if (o.isSVGInlineText()) {
        writeSVGInlineText(ts, *toRenderSVGInlineText(&o), indent);
        return;
    }
    if (o.isSVGImage()) {
        writeSVGImage(ts, *toRenderSVGImage(&o), indent);
        return;
    }
#endif

    writeIndent(ts, indent);

    RenderTreeAsText::writeRenderObject(ts, o, behavior);
    ts << "\n";

    if (o.isText() && !o.isBR()) {
        const RenderText& text = *toRenderText(&o);
        for (InlineTextBox* box = text.firstTextBox(); box; box = box->nextTextBox()) {
            writeIndent(ts, indent + 1);
            writeTextRun(ts, text, *box);
        }
    }

    for (RenderObject* child = o.firstChild(); child; child = child->nextSibling()) {
        if (child->hasLayer())
            continue;
        write(ts, *child, indent + 1, behavior);
    }

    if (o.isWidget()) {
        Widget* widget = toRenderWidget(&o)->widget();
        if (widget && widget->isFrameView()) {
            FrameView* view = toFrameView(widget);
            RenderView* root = view->frame()->contentRenderer();
            if (root) {
                view->layout();
                RenderLayer* l = root->layer();
                if (l)
                    writeLayers(ts, l, l, l->rect(), indent + 1, behavior);
            }
        }
    }
}

enum LayerPaintPhase {
    LayerPaintPhaseAll = 0,
    LayerPaintPhaseBackground = -1,
    LayerPaintPhaseForeground = 1
};

static void write(TextStream& ts, RenderLayer& l,
                  const LayoutRect& layerBounds, const LayoutRect& backgroundClipRect, const LayoutRect& clipRect, const LayoutRect& outlineClipRect,
                  LayerPaintPhase paintPhase = LayerPaintPhaseAll, int indent = 0, RenderAsTextBehavior behavior = RenderAsTextBehaviorNormal)
{
    IntRect adjustedLayoutBounds = pixelSnappedIntRect(layerBounds);
    IntRect adjustedBackgroundClipRect = pixelSnappedIntRect(backgroundClipRect);
    IntRect adjustedClipRect = pixelSnappedIntRect(clipRect);
    IntRect adjustedOutlineClipRect = pixelSnappedIntRect(outlineClipRect);

    writeIndent(ts, indent);

    ts << "layer ";
    
    if (behavior & RenderAsTextShowAddresses)
        ts << static_cast<const void*>(&l) << " ";
      
    ts << adjustedLayoutBounds;

    if (!adjustedLayoutBounds.isEmpty()) {
        if (!adjustedBackgroundClipRect.contains(adjustedLayoutBounds))
            ts << " backgroundClip " << adjustedBackgroundClipRect;
        if (!adjustedClipRect.contains(adjustedLayoutBounds))
            ts << " clip " << adjustedClipRect;
        if (!adjustedOutlineClipRect.contains(adjustedLayoutBounds))
            ts << " outlineClip " << adjustedOutlineClipRect;
    }

    if (l.renderer()->hasOverflowClip()) {
        if (l.scrollXOffset())
            ts << " scrollX " << l.scrollXOffset();
        if (l.scrollYOffset())
            ts << " scrollY " << l.scrollYOffset();
        if (l.renderBox() && l.renderBox()->pixelSnappedClientWidth() != l.scrollWidth())
            ts << " scrollWidth " << l.scrollWidth();
        if (l.renderBox() && l.renderBox()->pixelSnappedClientHeight() != l.scrollHeight())
            ts << " scrollHeight " << l.scrollHeight();
    }

    if (paintPhase == LayerPaintPhaseBackground)
        ts << " layerType: background only";
    else if (paintPhase == LayerPaintPhaseForeground)
        ts << " layerType: foreground only";
    
#if USE(ACCELERATED_COMPOSITING)
    if (behavior & RenderAsTextShowCompositedLayers) {
        if (l.isComposited())
            ts << " (composited, bounds=" << l.backing()->compositedBounds() << ", drawsContent=" << l.backing()->graphicsLayer()->drawsContent() << ", paints into ancestor=" << l.backing()->paintsIntoCompositedAncestor() << ")";
    }
#else
    UNUSED_PARAM(behavior);
#endif
    
    ts << "\n";

    if (paintPhase != LayerPaintPhaseBackground)
        write(ts, *l.renderer(), indent + 1, behavior);
}

static void writeRenderRegionList(const RenderRegionList& flowThreadRegionList, TextStream& ts, int indent)
{
    for (RenderRegionList::const_iterator itRR = flowThreadRegionList.begin(); itRR != flowThreadRegionList.end(); ++itRR) {
        RenderRegion* renderRegion = *itRR;
        writeIndent(ts, indent + 2);
        ts << "RenderRegion";
        if (renderRegion->generatingNode()) {
            String tagName = getTagName(renderRegion->generatingNode());
            if (!tagName.isEmpty())
                ts << " {" << tagName << "}";
            if (renderRegion->generatingNode()->isElementNode() && toElement(renderRegion->generatingNode())->hasID()) {
                Element* element = toElement(renderRegion->generatingNode());
                ts << " #" << element->idForStyleResolution();
            }
            if (renderRegion->hasCustomRegionStyle())
                ts << " region style: 1";
            if (renderRegion->hasAutoLogicalHeight())
                ts << " hasAutoLogicalHeight";
        }
        if (!renderRegion->isValid())
            ts << " invalid";
        ts << "\n";
    }
}

static void writeRenderNamedFlowThreads(TextStream& ts, RenderView* renderView, const RenderLayer* rootLayer,
                        const LayoutRect& paintRect, int indent, RenderAsTextBehavior behavior)
{
    if (!renderView->hasRenderNamedFlowThreads())
        return;

    const RenderNamedFlowThreadList* list = renderView->flowThreadController()->renderNamedFlowThreadList();

    writeIndent(ts, indent);
    ts << "Flow Threads\n";

    for (RenderNamedFlowThreadList::const_iterator iter = list->begin(); iter != list->end(); ++iter) {
        const RenderNamedFlowThread* renderFlowThread = *iter;

        writeIndent(ts, indent + 1);
        ts << "Thread with flow-name '" << renderFlowThread->flowThreadName() << "'\n";

        RenderLayer* layer = renderFlowThread->layer();
        writeLayers(ts, rootLayer, layer, paintRect, indent + 2, behavior);

        // Display the valid and invalid render regions attached to this flow thread.
        const RenderRegionList& validRegionsList = renderFlowThread->renderRegionList();
        const RenderRegionList& invalidRegionsList = renderFlowThread->invalidRenderRegionList();
        if (!validRegionsList.isEmpty() || !invalidRegionsList.isEmpty()) {
            writeIndent(ts, indent + 1);
            ts << "Regions for flow '"<< renderFlowThread->flowThreadName() << "'\n";
            writeRenderRegionList(validRegionsList, ts, indent);
            writeRenderRegionList(invalidRegionsList, ts, indent);
        }
    }
}

static LayoutSize maxLayoutOverflow(const RenderBox* box)
{
    LayoutRect overflowRect = box->layoutOverflowRect();
    return LayoutSize(overflowRect.maxX(), overflowRect.maxY());
}

static void writeLayers(TextStream& ts, const RenderLayer* rootLayer, RenderLayer* l,
                        const LayoutRect& paintRect, int indent, RenderAsTextBehavior behavior)
{
    // FIXME: Apply overflow to the root layer to not break every test.  Complete hack.  Sigh.
    LayoutRect paintDirtyRect(paintRect);
    if (rootLayer == l) {
        paintDirtyRect.setWidth(max<LayoutUnit>(paintDirtyRect.width(), rootLayer->renderBox()->layoutOverflowRect().maxX()));
        paintDirtyRect.setHeight(max<LayoutUnit>(paintDirtyRect.height(), rootLayer->renderBox()->layoutOverflowRect().maxY()));
        l->setSize(l->size().expandedTo(pixelSnappedIntSize(maxLayoutOverflow(l->renderBox()), LayoutPoint(0, 0))));
    }
    
    // Calculate the clip rects we should use.
    LayoutRect layerBounds;
    ClipRect damageRect, clipRectToApply, outlineRect;
    l->calculateRects(RenderLayer::ClipRectsContext(rootLayer, 0, TemporaryClipRects), paintDirtyRect, layerBounds, damageRect, clipRectToApply, outlineRect);

    // Ensure our lists are up-to-date.
    l->updateLayerListsIfNeeded();

    bool shouldPaint = (behavior & RenderAsTextShowAllLayers) ? true : l->intersectsDamageRect(layerBounds, damageRect.rect(), rootLayer);
    Vector<RenderLayer*>* negList = l->negZOrderList();
    bool paintsBackgroundSeparately = negList && negList->size() > 0;
    if (shouldPaint && paintsBackgroundSeparately)
        write(ts, *l, layerBounds, damageRect.rect(), clipRectToApply.rect(), outlineRect.rect(), LayerPaintPhaseBackground, indent, behavior);

    if (negList) {
        int currIndent = indent;
        if (behavior & RenderAsTextShowLayerNesting) {
            writeIndent(ts, indent);
            ts << " negative z-order list(" << negList->size() << ")\n";
            ++currIndent;
        }
        for (unsigned i = 0; i != negList->size(); ++i)
            writeLayers(ts, rootLayer, negList->at(i), paintDirtyRect, currIndent, behavior);
    }

    if (shouldPaint)
        write(ts, *l, layerBounds, damageRect.rect(), clipRectToApply.rect(), outlineRect.rect(), paintsBackgroundSeparately ? LayerPaintPhaseForeground : LayerPaintPhaseAll, indent, behavior);

    if (Vector<RenderLayer*>* normalFlowList = l->normalFlowList()) {
        int currIndent = indent;
        if (behavior & RenderAsTextShowLayerNesting) {
            writeIndent(ts, indent);
            ts << " normal flow list(" << normalFlowList->size() << ")\n";
            ++currIndent;
        }
        for (unsigned i = 0; i != normalFlowList->size(); ++i)
            writeLayers(ts, rootLayer, normalFlowList->at(i), paintDirtyRect, currIndent, behavior);
    }

    if (Vector<RenderLayer*>* posList = l->posZOrderList()) {
        size_t layerCount = 0;
        for (unsigned i = 0; i != posList->size(); ++i)
            if (!posList->at(i)->isOutOfFlowRenderFlowThread())
                ++layerCount;
        if (layerCount) {
            int currIndent = indent;
            if (behavior & RenderAsTextShowLayerNesting) {
                writeIndent(ts, indent);
                ts << " positive z-order list(" << layerCount << ")\n";
                ++currIndent;
            }
            for (unsigned i = 0; i != posList->size(); ++i) {
                if (!posList->at(i)->isOutOfFlowRenderFlowThread())
                    writeLayers(ts, rootLayer, posList->at(i), paintDirtyRect, currIndent, behavior);
            }
        }
    }
    
    // Altough the RenderFlowThread requires a layer, it is not collected by its parent,
    // so we have to treat it as a special case.
    if (l->renderer()->isRenderView()) {
        RenderView* renderView = toRenderView(l->renderer());
        writeRenderNamedFlowThreads(ts, renderView, rootLayer, paintDirtyRect, indent, behavior);
    }
}

static String nodePosition(Node* node)
{
    StringBuilder result;

    Element* body = node->document()->body();
    Node* parent;
    for (Node* n = node; n; n = parent) {
        parent = n->parentOrShadowHostNode();
        if (n != node)
            result.appendLiteral(" of ");
        if (parent) {
            if (body && n == body) {
                // We don't care what offset body may be in the document.
                result.appendLiteral("body");
                break;
            }
            if (n->isShadowRoot()) {
                result.append('{');
                result.append(getTagName(n));
                result.append('}');
            } else {
                result.appendLiteral("child ");
                result.appendNumber(n->nodeIndex());
                result.appendLiteral(" {");
                result.append(getTagName(n));
                result.append('}');
            }
        } else
            result.appendLiteral("document");
    }

    return result.toString();
}

static void writeSelection(TextStream& ts, const RenderObject* o)
{
    Node* n = o->node();
    if (!n || !n->isDocumentNode())
        return;

    Document* doc = toDocument(n);
    Frame* frame = doc->frame();
    if (!frame)
        return;

    VisibleSelection selection = frame->selection()->selection();
    if (selection.isCaret()) {
        ts << "caret: position " << selection.start().deprecatedEditingOffset() << " of " << nodePosition(selection.start().deprecatedNode());
        if (selection.affinity() == UPSTREAM)
            ts << " (upstream affinity)";
        ts << "\n";
    } else if (selection.isRange())
        ts << "selection start: position " << selection.start().deprecatedEditingOffset() << " of " << nodePosition(selection.start().deprecatedNode()) << "\n"
           << "selection end:   position " << selection.end().deprecatedEditingOffset() << " of " << nodePosition(selection.end().deprecatedNode()) << "\n";
}

static String externalRepresentation(RenderBox* renderer, RenderAsTextBehavior behavior)
{
    TextStream ts;
    if (!renderer->hasLayer())
        return ts.release();
        
    RenderLayer* layer = renderer->layer();
    writeLayers(ts, layer, layer, layer->rect(), 0, behavior);
    writeSelection(ts, renderer);
    return ts.release();
}

String externalRepresentation(Frame* frame, RenderAsTextBehavior behavior)
{
    RenderObject* renderer = frame->contentRenderer();
    if (!renderer || !renderer->isBox())
        return String();

    PrintContext printContext(frame);
    if (behavior & RenderAsTextPrintingMode)
        printContext.begin(toRenderBox(renderer)->width());
    if (!(behavior & RenderAsTextDontUpdateLayout))
        frame->document()->updateLayout();

    return externalRepresentation(toRenderBox(renderer), behavior);
}

String externalRepresentation(Element* element, RenderAsTextBehavior behavior)
{
    RenderObject* renderer = element->renderer();
    if (!renderer || !renderer->isBox())
        return String();
    // Doesn't support printing mode.
    ASSERT(!(behavior & RenderAsTextPrintingMode));
    if (!(behavior & RenderAsTextDontUpdateLayout) && element->document())
        element->document()->updateLayout();
    
    return externalRepresentation(toRenderBox(renderer), behavior | RenderAsTextShowAllLayers);
}

static void writeCounterValuesFromChildren(TextStream& stream, RenderObject* parent, bool& isFirstCounter)
{
    for (RenderObject* child = parent->firstChild(); child; child = child->nextSibling()) {
        if (child->isCounter()) {
            if (!isFirstCounter)
                stream << " ";
            isFirstCounter = false;
            String str(toRenderText(child)->text());
            stream << str;
        }
    }
}

String counterValueForElement(Element* element)
{
    // Make sure the element is not freed during the layout.
    RefPtr<Element> elementRef(element);
    element->document()->updateLayout();
    TextStream stream;
    bool isFirstCounter = true;
    // The counter renderers should be children of :before or :after pseudo-elements.
    if (RenderObject* before = element->pseudoElementRenderer(BEFORE))
        writeCounterValuesFromChildren(stream, before, isFirstCounter);
    if (RenderObject* after = element->pseudoElementRenderer(AFTER))
        writeCounterValuesFromChildren(stream, after, isFirstCounter);
    return stream.release();
}

String markerTextForListItem(Element* element)
{
    // Make sure the element is not freed during the layout.
    RefPtr<Element> elementRef(element);
    element->document()->updateLayout();

    RenderObject* renderer = element->renderer();
    if (!renderer || !renderer->isListItem())
        return String();

    return toRenderListItem(renderer)->markerText();
}

} // namespace WebCore
