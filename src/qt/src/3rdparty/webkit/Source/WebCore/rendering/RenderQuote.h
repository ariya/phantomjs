/*
 * Copyright (C) 2011 Nokia Inc. All rights reserved.
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

#ifndef RenderQuote_h
#define RenderQuote_h

#include "RenderStyleConstants.h"
#include "RenderText.h"

namespace WebCore {

class RenderQuote : public RenderText {
public:
    RenderQuote(Document*, const QuoteType);
    virtual ~RenderQuote();

    static void rendererSubtreeAttached(RenderObject*);
    static void rendererRemovedFromTree(RenderObject*);
protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
private:
    virtual const char* renderName() const;
    virtual bool isQuote() const { return true; };
    virtual PassRefPtr<StringImpl> originalText() const;
    virtual void computePreferredLogicalWidths(float leadWidth);
    QuoteType m_type;
    int m_depth;
    RenderQuote* m_next;
    RenderQuote* m_previous;
    void placeQuote();
};

inline RenderQuote* toRenderQuote(RenderObject* object)
{
    ASSERT(!object || object->isQuote());
    return static_cast<RenderQuote*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderQuote(const RenderQuote*);

} // namespace WebCore

#endif // RenderQuote_h
