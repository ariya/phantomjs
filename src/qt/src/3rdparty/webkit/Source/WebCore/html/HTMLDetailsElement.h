/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef HTMLDetailsElement_h
#define HTMLDetailsElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLDetailsElement : public HTMLElement {
public:
    static PassRefPtr<HTMLDetailsElement> create(const QualifiedName& tagName, Document* document);
    Node* mainSummary() const { return m_mainSummary; }
    void toggleOpen();

private:
    enum RefreshRenderer {
        RefreshRendererAllowed,
        RefreshRendererSupressed,
    };

    enum SummaryType {
        NoSummary,
        DefaultSummary,
        ForwardingSummary
    };

    HTMLDetailsElement(const QualifiedName&, Document*);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta);
    virtual void finishParsingChildren();

    void parseMappedAttribute(Attribute*);
    bool childShouldCreateRenderer(Node*) const;

    Node* ensureMainSummary();
    void refreshMainSummary(RefreshRenderer);
    void ensureShadowSubtreeOf(SummaryType);
    void createShadowSubtree();

    SummaryType m_summaryType;
    Node* m_mainSummary;
    bool m_isOpen;

};

} // namespace WebCore

#endif // HTMLDetailsElement_h
