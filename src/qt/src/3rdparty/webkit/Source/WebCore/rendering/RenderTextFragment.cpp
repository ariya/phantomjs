/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "RenderTextFragment.h"

#include "RenderBlock.h"
#include "Text.h"

namespace WebCore {

RenderTextFragment::RenderTextFragment(Node* node, StringImpl* str, int startOffset, int length)
    : RenderText(node, str ? str->substring(startOffset, length) : PassRefPtr<StringImpl>(0))
    , m_start(startOffset)
    , m_end(length)
    , m_firstLetter(0)
{
}

RenderTextFragment::RenderTextFragment(Node* node, StringImpl* str)
    : RenderText(node, str)
    , m_start(0)
    , m_end(str ? str->length() : 0)
    , m_contentString(str)
    , m_firstLetter(0)
{
}

RenderTextFragment::~RenderTextFragment()
{
}

PassRefPtr<StringImpl> RenderTextFragment::originalText() const
{
    Node* e = node();
    RefPtr<StringImpl> result = ((e && e->isTextNode()) ? static_cast<Text*>(e)->dataImpl() : contentString());
    if (!result)
        return 0;
    return result->substring(start(), end());
}

void RenderTextFragment::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderText::styleDidChange(diff, oldStyle);

    if (RenderBlock* block = blockForAccompanyingFirstLetter()) {
        block->style()->removeCachedPseudoStyle(FIRST_LETTER);
        block->updateFirstLetter();
    }
}

void RenderTextFragment::destroy()
{
    if (m_firstLetter)
        m_firstLetter->destroy();
    RenderText::destroy();
}

void RenderTextFragment::setTextInternal(PassRefPtr<StringImpl> text)
{
    RenderText::setTextInternal(text);
    if (m_firstLetter) {
        ASSERT(!m_contentString);
        m_firstLetter->destroy();
        m_firstLetter = 0;
        m_start = 0;
        m_end = textLength();
        if (Node* t = node()) {
            ASSERT(!t->renderer());
            t->setRenderer(this);
        }
    }
}

UChar RenderTextFragment::previousCharacter() const
{
    if (start()) {
        Node* e = node();
        StringImpl*  original = ((e && e->isTextNode()) ? static_cast<Text*>(e)->dataImpl() : contentString());
        if (original && start() <= original->length())
            return (*original)[start() - 1];
    }

    return RenderText::previousCharacter();
}

RenderBlock* RenderTextFragment::blockForAccompanyingFirstLetter() const
{
    if (!m_firstLetter)
        return 0;
    for (RenderObject* block = m_firstLetter->parent(); block; block = block->parent()) {
        if (block->style()->hasPseudoStyle(FIRST_LETTER) && block->canHaveChildren() && block->isRenderBlock())
            return toRenderBlock(block);
    }
    return 0;
}

} // namespace WebCore
