/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLOptGroupElement_h
#define HTMLOptGroupElement_h

#include "HTMLElement.h"

namespace WebCore {
    
class HTMLSelectElement;

class HTMLOptGroupElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLOptGroupElement> create(const QualifiedName&, Document*);

    virtual bool isDisabledFormControl() const OVERRIDE;
    HTMLSelectElement* ownerSelectElement() const;
    
    String groupLabelText() const;

private:
    HTMLOptGroupElement(const QualifiedName&, Document*);

    virtual const AtomicString& formControlType() const;
    virtual bool isFocusable() const OVERRIDE;
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool rendererIsNeeded(const NodeRenderingContext&) { return false; }
    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual void accessKeyAction(bool sendMouseEvents);

    // <optgroup> never has a renderer so we manually manage a cached style.
    void updateNonRenderStyle();
    virtual RenderStyle* nonRendererStyle() const OVERRIDE;
    virtual PassRefPtr<RenderStyle> customStyleForRenderer() OVERRIDE;

    void recalcSelectOptions();

    RefPtr<RenderStyle> m_style;
};

inline bool isHTMLOptGroupElement(Node* node)
{
    return node->hasTagName(HTMLNames::optgroupTag);
}

inline bool isHTMLOptGroupElement(Element* element)
{
    return element->hasTagName(HTMLNames::optgroupTag);
}

inline HTMLOptGroupElement* toHTMLOptGroupElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLOptGroupElement(node));
    return static_cast<HTMLOptGroupElement*>(node);
}

} //namespace

#endif
