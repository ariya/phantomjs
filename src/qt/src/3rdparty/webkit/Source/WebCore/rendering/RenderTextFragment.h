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

#ifndef RenderTextFragment_h
#define RenderTextFragment_h

#include "RenderText.h"

namespace WebCore {

// Used to represent a text substring of an element, e.g., for text runs that are split because of
// first letter and that must therefore have different styles (and positions in the render tree).
// We cache offsets so that text transformations can be applied in such a way that we can recover
// the original unaltered string from our corresponding DOM node.
class RenderTextFragment : public RenderText {
public:
    RenderTextFragment(Node*, StringImpl*, int startOffset, int length);
    RenderTextFragment(Node*, StringImpl*);
    virtual ~RenderTextFragment();

    virtual bool isTextFragment() const { return true; }

    virtual void destroy();

    unsigned start() const { return m_start; }
    unsigned end() const { return m_end; }

    RenderObject* firstLetter() const { return m_firstLetter; }
    void setFirstLetter(RenderObject* firstLetter) { m_firstLetter = firstLetter; }

    StringImpl* contentString() const { return m_contentString.get(); }
    virtual PassRefPtr<StringImpl> originalText() const;

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual void setTextInternal(PassRefPtr<StringImpl>);
    virtual UChar previousCharacter() const;
    RenderBlock* blockForAccompanyingFirstLetter() const;

    unsigned m_start;
    unsigned m_end;
    RefPtr<StringImpl> m_contentString;
    RenderObject* m_firstLetter;
};

inline RenderTextFragment* toRenderTextFragment(RenderObject* object)
{ 
    ASSERT(!object || toRenderText(object)->isTextFragment());
    return static_cast<RenderTextFragment*>(object);
}

inline const RenderTextFragment* toRenderTextFragment(const RenderObject* object)
{ 
    ASSERT(!object || toRenderText(object)->isTextFragment());
    return static_cast<const RenderTextFragment*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTextFragment(const RenderTextFragment*);

} // namespace WebCore

#endif // RenderTextFragment_h
