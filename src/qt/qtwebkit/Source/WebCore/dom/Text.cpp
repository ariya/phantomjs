/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "Text.h"

#include "ExceptionCode.h"
#include "ExceptionCodePlaceholder.h"
#include "NodeRenderingContext.h"
#include "RenderCombineText.h"
#include "RenderText.h"
#include "ScopedEventQueue.h"
#include "ShadowRoot.h"

#if ENABLE(SVG)
#include "RenderSVGInlineText.h"
#include "SVGNames.h"
#endif

#include "StyleInheritedData.h"
#include "StyleResolver.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace std;

namespace WebCore {

PassRefPtr<Text> Text::create(Document* document, const String& data)
{
    return adoptRef(new Text(document, data, CreateText));
}

PassRefPtr<Text> Text::createEditingText(Document* document, const String& data)
{
    return adoptRef(new Text(document, data, CreateEditingText));
}

PassRefPtr<Text> Text::splitText(unsigned offset, ExceptionCode& ec)
{
    ec = 0;

    // INDEX_SIZE_ERR: Raised if the specified offset is negative or greater than
    // the number of 16-bit units in data.
    if (offset > length()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }

    EventQueueScope scope;
    String oldStr = data();
    RefPtr<Text> newText = virtualCreate(oldStr.substring(offset));
    setDataWithoutUpdate(oldStr.substring(0, offset));

    dispatchModifiedEvent(oldStr);

    if (parentNode())
        parentNode()->insertBefore(newText.get(), nextSibling(), ec);
    if (ec)
        return 0;

    if (parentNode())
        document()->textNodeSplit(this);

    if (renderer())
        toRenderText(renderer())->setTextWithOffset(dataImpl(), 0, oldStr.length());

    return newText.release();
}

static const Text* earliestLogicallyAdjacentTextNode(const Text* t)
{
    const Node* n = t;
    while ((n = n->previousSibling())) {
        Node::NodeType type = n->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            t = static_cast<const Text*>(n);
            continue;
        }

        // We would need to visit EntityReference child text nodes if they existed
        ASSERT(type != Node::ENTITY_REFERENCE_NODE || !n->hasChildNodes());
        break;
    }
    return t;
}

static const Text* latestLogicallyAdjacentTextNode(const Text* t)
{
    const Node* n = t;
    while ((n = n->nextSibling())) {
        Node::NodeType type = n->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            t = static_cast<const Text*>(n);
            continue;
        }

        // We would need to visit EntityReference child text nodes if they existed
        ASSERT(type != Node::ENTITY_REFERENCE_NODE || !n->hasChildNodes());
        break;
    }
    return t;
}

String Text::wholeText() const
{
    const Text* startText = earliestLogicallyAdjacentTextNode(this);
    const Text* endText = latestLogicallyAdjacentTextNode(this);

    Node* onePastEndText = endText->nextSibling();
    unsigned resultLength = 0;
    for (const Node* n = startText; n != onePastEndText; n = n->nextSibling()) {
        if (!n->isTextNode())
            continue;
        const Text* t = static_cast<const Text*>(n);
        const String& data = t->data();
        if (std::numeric_limits<unsigned>::max() - data.length() < resultLength)
            CRASH();
        resultLength += data.length();
    }
    StringBuilder result;
    result.reserveCapacity(resultLength);
    for (const Node* n = startText; n != onePastEndText; n = n->nextSibling()) {
        if (!n->isTextNode())
            continue;
        const Text* t = static_cast<const Text*>(n);
        result.append(t->data());
    }
    ASSERT(result.length() == resultLength);

    return result.toString();
}

PassRefPtr<Text> Text::replaceWholeText(const String& newText, ExceptionCode&)
{
    // Remove all adjacent text nodes, and replace the contents of this one.

    // Protect startText and endText against mutation event handlers removing the last ref
    RefPtr<Text> startText = const_cast<Text*>(earliestLogicallyAdjacentTextNode(this));
    RefPtr<Text> endText = const_cast<Text*>(latestLogicallyAdjacentTextNode(this));

    RefPtr<Text> protectedThis(this); // Mutation event handlers could cause our last ref to go away
    RefPtr<ContainerNode> parent = parentNode(); // Protect against mutation handlers moving this node during traversal
    for (RefPtr<Node> n = startText; n && n != this && n->isTextNode() && n->parentNode() == parent;) {
        RefPtr<Node> nodeToRemove(n.release());
        n = nodeToRemove->nextSibling();
        parent->removeChild(nodeToRemove.get(), IGNORE_EXCEPTION);
    }

    if (this != endText) {
        Node* onePastEndText = endText->nextSibling();
        for (RefPtr<Node> n = nextSibling(); n && n != onePastEndText && n->isTextNode() && n->parentNode() == parent;) {
            RefPtr<Node> nodeToRemove(n.release());
            n = nodeToRemove->nextSibling();
            parent->removeChild(nodeToRemove.get(), IGNORE_EXCEPTION);
        }
    }

    if (newText.isEmpty()) {
        if (parent && parentNode() == parent)
            parent->removeChild(this, IGNORE_EXCEPTION);
        return 0;
    }

    setData(newText, IGNORE_EXCEPTION);
    return protectedThis.release();
}

String Text::nodeName() const
{
    return textAtom.string();
}

Node::NodeType Text::nodeType() const
{
    return TEXT_NODE;
}

PassRefPtr<Node> Text::cloneNode(bool /*deep*/)
{
    return create(document(), data());
}

bool Text::textRendererIsNeeded(const NodeRenderingContext& context)
{
    if (isEditingText())
        return true;

    if (!length())
        return false;

    if (context.style()->display() == NONE)
        return false;

    bool onlyWS = containsOnlyWhitespace();
    if (!onlyWS)
        return true;

    RenderObject* parent = context.parentRenderer();
    if (parent->isTable() || parent->isTableRow() || parent->isTableSection() || parent->isRenderTableCol() || parent->isFrameSet())
        return false;
    
    if (context.style()->preserveNewline()) // pre/pre-wrap/pre-line always make renderers.
        return true;
    
    RenderObject* prev = context.previousRenderer();
    if (prev && prev->isBR()) // <span><br/> <br/></span>
        return false;
        
    if (parent->isRenderInline()) {
        // <span><div/> <div/></span>
        if (prev && !prev->isInline())
            return false;
    } else {
        if (parent->isRenderBlock() && !parent->childrenInline() && (!prev || !prev->isInline()))
            return false;
        
        RenderObject* first = parent->firstChild();
        while (first && first->isFloatingOrOutOfFlowPositioned())
            first = first->nextSibling();
        RenderObject* next = context.nextRenderer();
        if (!first || next == first)
            // Whitespace at the start of a block just goes away.  Don't even
            // make a render object for this text.
            return false;
    }
    
    return true;
}

#if ENABLE(SVG)
static bool isSVGShadowText(Text* text)
{
    Node* parentNode = text->parentNode();
    return parentNode->isShadowRoot() && toShadowRoot(parentNode)->host()->hasTagName(SVGNames::trefTag);
}

static bool isSVGText(Text* text)
{
    Node* parentOrShadowHostNode = text->parentOrShadowHostNode();
    return parentOrShadowHostNode->isSVGElement() && !parentOrShadowHostNode->hasTagName(SVGNames::foreignObjectTag);
}
#endif

void Text::createTextRendererIfNeeded()
{
    NodeRenderingContext(this).createRendererForTextIfNeeded();
}

RenderText* Text::createTextRenderer(RenderArena* arena, RenderStyle* style)
{
#if ENABLE(SVG)
    if (isSVGText(this) || isSVGShadowText(this))
        return new (arena) RenderSVGInlineText(this, dataImpl());
#endif
    if (style->hasTextCombine())
        return new (arena) RenderCombineText(this, dataImpl());

    return new (arena) RenderText(this, dataImpl());
}

void Text::attach(const AttachContext& context)
{
    createTextRendererIfNeeded();
    CharacterData::attach(context);
}

void Text::recalcTextStyle(StyleChange change)
{
    RenderText* renderer = toRenderText(this->renderer());

    if (change != NoChange && renderer)
        renderer->setStyle(document()->ensureStyleResolver()->styleForText(this));

    if (needsStyleRecalc()) {
        if (renderer)
            renderer->setText(dataImpl());
        else
            reattach();
    }
    clearNeedsStyleRecalc();
}

void Text::updateTextRenderer(unsigned offsetOfReplacedData, unsigned lengthOfReplacedData)
{
    if (!attached())
        return;
    RenderText* textRenderer = toRenderText(renderer());
    if (!textRenderer) {
        reattach();
        return;
    }
    NodeRenderingContext renderingContext(this, textRenderer->style());
    if (!textRendererIsNeeded(renderingContext)) {
        reattach();
        return;
    }
    textRenderer->setTextWithOffset(dataImpl(), offsetOfReplacedData, lengthOfReplacedData);
}

bool Text::childTypeAllowed(NodeType) const
{
    return false;
}

PassRefPtr<Text> Text::virtualCreate(const String& data)
{
    return create(document(), data);
}

PassRefPtr<Text> Text::createWithLengthLimit(Document* document, const String& data, unsigned start, unsigned lengthLimit)
{
    unsigned dataLength = data.length();

    if (!start && dataLength <= lengthLimit)
        return create(document, data);

    RefPtr<Text> result = Text::create(document, String());
    result->parserAppendData(data, start, lengthLimit);

    return result;
}

#ifndef NDEBUG
void Text::formatForDebugger(char *buffer, unsigned length) const
{
    StringBuilder result;
    String s;

    result.append(nodeName());

    s = data();
    if (s.length() > 0) {
        if (result.length())
            result.appendLiteral("; ");
        result.appendLiteral("value=");
        result.append(s);
    }

    strncpy(buffer, result.toString().utf8().data(), length - 1);
}
#endif

} // namespace WebCore
