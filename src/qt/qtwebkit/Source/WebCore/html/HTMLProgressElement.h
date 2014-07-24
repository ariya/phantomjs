/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef HTMLProgressElement_h
#define HTMLProgressElement_h

#if ENABLE(PROGRESS_ELEMENT)
#include "LabelableElement.h"

namespace WebCore {

class ProgressValueElement;
class RenderProgress;

class HTMLProgressElement FINAL : public LabelableElement {
public:
    static const double IndeterminatePosition;
    static const double InvalidPosition;

    static PassRefPtr<HTMLProgressElement> create(const QualifiedName&, Document*);

    double value() const;
    void setValue(double, ExceptionCode&);

    double max() const;
    void setMax(double, ExceptionCode&);

    double position() const;

    virtual bool canContainRangeEndPoint() const { return false; }

private:
    HTMLProgressElement(const QualifiedName&, Document*);
    virtual ~HTMLProgressElement();

    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }
    virtual bool shouldAppearIndeterminate() const OVERRIDE;
    virtual bool supportLabels() const OVERRIDE { return true; }

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual bool childShouldCreateRenderer(const NodeRenderingContext&) const OVERRIDE;
    RenderProgress* renderProgress() const;

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;

    void didElementStateChange();
    virtual void didAddUserAgentShadowRoot(ShadowRoot*) OVERRIDE;
    bool isDeterminate() const;

    ProgressValueElement* m_value;
};

inline bool isHTMLProgressElement(Node* node)
{
    ASSERT(node);
    return node->hasTagName(HTMLNames::progressTag);
}

inline HTMLProgressElement* toHTMLProgressElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLProgressElement(node));
    return static_cast<HTMLProgressElement*>(node);
}

} // namespace

#endif
#endif
