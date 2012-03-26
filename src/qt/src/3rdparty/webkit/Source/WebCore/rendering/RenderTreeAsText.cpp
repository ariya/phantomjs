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

#include "CSSMutableStyleDeclaration.h"
#include "Document.h"
#include "Frame.h"
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
#include "RenderPart.h"
#include "RenderTableCell.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "SelectionController.h"
#include <wtf/HexNumber.h>
#include <wtf/UnusedParam.h>
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
#include <QWidget>
#endif

namespace WebCore {

using namespace HTMLNames;

static void writeLayers(TextStream&, const RenderLayer* rootLayer, RenderLayer*, const IntRect& paintDirtyRect, int indent = 0, RenderAsTextBehavior behavior = RenderAsTextBehaviorNormal);

bool hasFractions(double val)
{
    static const double s_epsilon = 0.0001;
    int ival = static_cast<int>(val);
    double dval = static_cast<double>(ival);
    return fabs(val - dval) > s_epsilon;
}

TextStream& operator<<(TextStream& ts, const IntRect& r)
{
    return ts << "at (" << r.x() << "," << r.y() << ") size " << r.width() << "x" << r.height();
}

TextStream& operator<<(TextStream& ts, const IntPoint& p)
{
    return ts << "(" << p.x() << "," << p.y() << ")";
}

TextStream& operator<<(TextStream& ts, const FloatPoint& p)
{
    ts << "(";    
    if (hasFractions(p.x()))
        ts << p.x();
    else 
        ts << int(p.x());    
    ts << ",";
    if (hasFractions(p.y())) 
        ts << p.y();
    else 
        ts << int(p.y());    
    return ts << ")";
}

TextStream& operator<<(TextStream& ts, const FloatSize& s)
{
    ts << "width=";
    if (hasFractions(s.width()))
        ts << s.width();
    else
        ts << int(s.width());
    ts << " height=";
    if (hasFractions(s.height())) 
        ts << s.height();
    else
        ts << int(s.height());
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
    if (n->isCommentNode())
        return "COMMENT";
    return n->nodeName();
}

static bool isEmptyOrUnstyledAppleStyleSpan(const Node* node)
{
    if (!node || !node->isHTMLElement() || !node->hasTagName(spanTag))
        return false;

    const HTMLElement* elem = static_cast<const HTMLElement*>(node);
    if (elem->getAttribute(classAttr) != "Apple-style-span")
        return false;

    if (!node->hasChildNodes())
        return true;

    CSSMutableStyleDeclaration* inlineStyleDecl = elem->inlineStyleDecl();
    return (!inlineStyleDecl || inlineStyleDecl->length() == 0);
}

String quoteAndEscapeNonPrintables(const String& s)
{
    Vector<UChar> result;
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
    return String::adopt(result);
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
        if (!tagName.isEmpty()) {
            ts << " {" << tagName << "}";
            // flag empty or unstyled AppleStyleSpan because we never
            // want to leave them in the DOM
            if (isEmptyOrUnstyledAppleStyleSpan(o.node()))
                ts << " *empty or unstyled AppleStyleSpan*";
        }
    }

    bool adjustForTableCells = o.containingBlock()->isTableCell();

    IntRect r;
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
        r = IntRect(cell.x(), cell.y() + cell.intrinsicPaddingBefore(), cell.width(), cell.height() - cell.intrinsicPaddingBefore() - cell.intrinsicPaddingAfter());
    } else if (o.isBox())
        r = toRenderBox(&o)->frameRect();

    // FIXME: Temporary in order to ensure compatibility with existing layout test results.
    if (adjustForTableCells)
        r.move(0, -toRenderTableCell(o.containingBlock())->intrinsicPaddingBefore());

    ts << " " << r;

    if (!(o.isText() && !o.isBR())) {
        if (o.isFileUploadControl())
            ts << " " << quoteAndEscapeNonPrintables(toRenderFileUploadControl(&o)->fileTextValue());

        if (o.parent() && (o.parent()->style()->color() != o.style()->color()))
            ts << " [color=" << o.style()->color().nameForRenderTreeAsText() << "]";

        if (o.parent() && (o.parent()->style()->backgroundColor() != o.style()->backgroundColor()) &&
            o.style()->backgroundColor().isValid() && o.style()->backgroundColor().rgb())
            // Do not dump invalid or transparent backgrounds, since that is the default.
            ts << " [bgcolor=" << o.style()->backgroundColor().nameForRenderTreeAsText() << "]";
        
        if (o.parent() && (o.parent()->style()->textFillColor() != o.style()->textFillColor()) &&
            o.style()->textFillColor().isValid() && o.style()->textFillColor() != o.style()->color() &&
            o.style()->textFillColor().rgb())
            ts << " [textFillColor=" << o.style()->textFillColor().nameForRenderTreeAsText() << "]";

        if (o.parent() && (o.parent()->style()->textStrokeColor() != o.style()->textStrokeColor()) &&
            o.style()->textStrokeColor().isValid() && o.style()->textStrokeColor() != o.style()->color() &&
            o.style()->textStrokeColor().rgb())
            ts << " [textStrokeColor=" << o.style()->textStrokeColor().nameForRenderTreeAsText() << "]";

        if (o.parent() && (o.parent()->style()->textStrokeWidth() != o.style()->textStrokeWidth()) &&
            o.style()->textStrokeWidth() > 0)
            ts << " [textStrokeWidth=" << o.style()->textStrokeWidth() << "]";

        if (!o.isBoxModelObject())
            return;

        const RenderBoxModelObject& box = *toRenderBoxModelObject(&o);
        if (box.borderTop() || box.borderRight() || box.borderBottom() || box.borderLeft()) {
            ts << " [border:";

            BorderValue prevBorder;
            if (o.style()->borderTop() != prevBorder) {
                prevBorder = o.style()->borderTop();
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
    }

    if (o.isTableCell()) {
        const RenderTableCell& c = *toRenderTableCell(&o);
        ts << " [r=" << c.row() << " c=" << c.col() << " rs=" << c.rowSpan() << " cs=" << c.colSpan() << "]";
    }

#if ENABLE(DETAILS)
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
        if (Node* node = o.node()) {
            if (node->hasID())
                ts << " id=\"" + static_cast<Element*>(node)->getIdAttribute() + "\"";

            if (node->hasClass()) {
                StyledElement* styledElement = static_cast<StyledElement*>(node);
                String classes;
                for (size_t i = 0; i < styledElement->classNames().size(); ++i) {
                    if (i > 0)
                        classes += " ";
                    classes += styledElement->classNames()[i];
                }
                ts << " class=\"" + classes + "\"";
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
            QWidget* wid = part->widget()->platformWidget();

            ts << " [QT: ";
            ts << "geometry: {" << wid->geometry() << "} ";
            ts << "isHidden: " << wid->isHidden() << " ";
            ts << "isSelfVisible: " << part->widget()->isSelfVisible() << " ";
            ts << "isParentVisible: " << part->widget()->isParentVisible() << " ";
            ts << "mask: {" << wid->mask().boundingRect() << "} ] ";
        }
    }
#endif
}

static void writeTextRun(TextStream& ts, const RenderText& o, const InlineTextBox& run)
{
    // FIXME: For now use an "enclosingIntRect" model for x, y and logicalWidth, although this makes it harder
    // to detect any changes caused by the conversion to floating point. :(
    int x = run.m_x;
    int y = run.m_y;
    int logicalWidth = ceilf(run.m_x + run.m_logicalWidth) - x;

    // FIXME: Table cell adjustment is temporary until results can be updated.
    if (o.containingBlock()->isTableCell())
        y -= toRenderTableCell(o.containingBlock())->intrinsicPaddingBefore();
        
    ts << "text run at (" << x << "," << y << ") width " << logicalWidth;
    if (!run.isLeftToRightDirection() || run.m_dirOverride) {
        ts << (!run.isLeftToRightDirection() ? " RTL" : " LTR");
        if (run.m_dirOverride)
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
    if (o.isSVGPath()) {
        write(ts, *toRenderSVGPath(&o), indent);
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
        writeSVGText(ts, *toRenderBlock(&o), indent);
        return;
    }
    if (o.isSVGInlineText()) {
        writeSVGInlineText(ts, *toRenderText(&o), indent);
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
            FrameView* view = static_cast<FrameView*>(widget);
            RenderView* root = view->frame()->contentRenderer();
            if (root) {
                view->layout();
                RenderLayer* l = root->layer();
                if (l)
                    writeLayers(ts, l, l, IntRect(l->x(), l->y(), l->width(), l->height()), indent + 1, behavior);
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
                  const IntRect& layerBounds, const IntRect& backgroundClipRect, const IntRect& clipRect, const IntRect& outlineClipRect,
                  LayerPaintPhase paintPhase = LayerPaintPhaseAll, int indent = 0, RenderAsTextBehavior behavior = RenderAsTextBehaviorNormal)
{
    writeIndent(ts, indent);

    ts << "layer ";
    
    if (behavior & RenderAsTextShowAddresses)
        ts << static_cast<const void*>(&l) << " ";
      
    ts << layerBounds;

    if (!layerBounds.isEmpty()) {
        if (!backgroundClipRect.contains(layerBounds))
            ts << " backgroundClip " << backgroundClipRect;
        if (!clipRect.contains(layerBounds))
            ts << " clip " << clipRect;
        if (!outlineClipRect.contains(layerBounds))
            ts << " outlineClip " << outlineClipRect;
    }

    if (l.renderer()->hasOverflowClip()) {
        if (l.scrollXOffset())
            ts << " scrollX " << l.scrollXOffset();
        if (l.scrollYOffset())
            ts << " scrollY " << l.scrollYOffset();
        if (l.renderBox() && l.renderBox()->clientWidth() != l.scrollWidth())
            ts << " scrollWidth " << l.scrollWidth();
        if (l.renderBox() && l.renderBox()->clientHeight() != l.scrollHeight())
            ts << " scrollHeight " << l.scrollHeight();
    }

    if (paintPhase == LayerPaintPhaseBackground)
        ts << " layerType: background only";
    else if (paintPhase == LayerPaintPhaseForeground)
        ts << " layerType: foreground only";
    
#if USE(ACCELERATED_COMPOSITING)
    if (behavior & RenderAsTextShowCompositedLayers) {
        if (l.isComposited())
            ts << " (composited, bounds " << l.backing()->compositedBounds() << ")";
    }
#else
    UNUSED_PARAM(behavior);
#endif
    
    ts << "\n";

    if (paintPhase != LayerPaintPhaseBackground)
        write(ts, *l.renderer(), indent + 1, behavior);
}

static void writeLayers(TextStream& ts, const RenderLayer* rootLayer, RenderLayer* l,
                        const IntRect& paintRect, int indent, RenderAsTextBehavior behavior)
{
    // FIXME: Apply overflow to the root layer to not break every test.  Complete hack.  Sigh.
    IntRect paintDirtyRect(paintRect);
    if (rootLayer == l) {
        paintDirtyRect.setWidth(max(paintDirtyRect.width(), rootLayer->renderBox()->maxXLayoutOverflow()));
        paintDirtyRect.setHeight(max(paintDirtyRect.height(), rootLayer->renderBox()->maxYLayoutOverflow()));
        l->setWidth(max(l->width(), l->renderBox()->maxXLayoutOverflow()));
        l->setHeight(max(l->height(), l->renderBox()->maxYLayoutOverflow()));
    }
    
    // Calculate the clip rects we should use.
    IntRect layerBounds, damageRect, clipRectToApply, outlineRect;
    l->calculateRects(rootLayer, paintDirtyRect, layerBounds, damageRect, clipRectToApply, outlineRect, true);

    // Ensure our lists are up-to-date.
    l->updateZOrderLists();
    l->updateNormalFlowList();

    bool shouldPaint = (behavior & RenderAsTextShowAllLayers) ? true : l->intersectsDamageRect(layerBounds, damageRect, rootLayer);
    Vector<RenderLayer*>* negList = l->negZOrderList();
    bool paintsBackgroundSeparately = negList && negList->size() > 0;
    if (shouldPaint && paintsBackgroundSeparately)
        write(ts, *l, layerBounds, damageRect, clipRectToApply, outlineRect, LayerPaintPhaseBackground, indent, behavior);

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
        write(ts, *l, layerBounds, damageRect, clipRectToApply, outlineRect, paintsBackgroundSeparately ? LayerPaintPhaseForeground : LayerPaintPhaseAll, indent, behavior);

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
        int currIndent = indent;
        if (behavior & RenderAsTextShowLayerNesting) {
            writeIndent(ts, indent);
            ts << " positive z-order list(" << posList->size() << ")\n";
            ++currIndent;
        }
        for (unsigned i = 0; i != posList->size(); ++i)
            writeLayers(ts, rootLayer, posList->at(i), paintDirtyRect, currIndent, behavior);
    }
}

static String nodePosition(Node* node)
{
    String result;

    Element* body = node->document()->body();
    Node* parent;
    for (Node* n = node; n; n = parent) {
        parent = n->parentOrHostNode();
        if (n != node)
            result += " of ";
        if (parent) {
            if (body && n == body) {
                // We don't care what offset body may be in the document.
                result += "body";
                break;
            }
            if (n->isShadowBoundary())
                result += "{" + getTagName(n) + "}";
            else
                result += "child " + String::number(n->nodeIndex()) + " {" + getTagName(n) + "}";
        } else
            result += "document";
    }

    return result;
}

static void writeSelection(TextStream& ts, const RenderObject* o)
{
    Node* n = o->node();
    if (!n || !n->isDocumentNode())
        return;

    Document* doc = static_cast<Document*>(n);
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

String externalRepresentation(Frame* frame, RenderAsTextBehavior behavior)
{
    PrintContext printContext(frame);
    if (behavior & RenderAsTextPrintingMode) {
        if (!frame->contentRenderer())
            return String();
        printContext.begin(frame->contentRenderer()->width());
    }

    if (!(behavior & RenderAsTextDontUpdateLayout))
        frame->document()->updateLayout();

    RenderObject* o = frame->contentRenderer();
    if (!o)
        return String();

    TextStream ts;
    if (o->hasLayer()) {
        RenderLayer* l = toRenderBox(o)->layer();
        writeLayers(ts, l, l, IntRect(l->x(), l->y(), l->width(), l->height()), 0, behavior);
        writeSelection(ts, o);
    }
    return ts.release();
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
    if (RenderObject* renderer = element->renderer()) {
        if (RenderObject* pseudoElement = renderer->beforePseudoElementRenderer())
            writeCounterValuesFromChildren(stream, pseudoElement, isFirstCounter);
        if (RenderObject* pseudoElement = renderer->afterPseudoElementRenderer())
            writeCounterValuesFromChildren(stream, pseudoElement, isFirstCounter);
    }
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
