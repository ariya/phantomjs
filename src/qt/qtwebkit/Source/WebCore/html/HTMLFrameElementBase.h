/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2004, 2006, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef HTMLFrameElementBase_h
#define HTMLFrameElementBase_h

#include "HTMLFrameOwnerElement.h"
#include "ScrollTypes.h"

namespace WebCore {

class HTMLFrameElementBase : public HTMLFrameOwnerElement {
public:
    KURL location() const;
    void setLocation(const String&);

    virtual ScrollbarMode scrollingMode() const { return m_scrolling; }
    
    int marginWidth() const { return m_marginWidth; }
    int marginHeight() const { return m_marginHeight; }

    int width();
    int height();

    virtual bool canContainRangeEndPoint() const { return false; }

protected:
    HTMLFrameElementBase(const QualifiedName&, Document*);

    bool isURLAllowed() const;

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void didNotifySubtreeInsertions(ContainerNode*) OVERRIDE;
    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;

private:
    virtual bool supportsFocus() const OVERRIDE;
    virtual void setFocus(bool) OVERRIDE;
    
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;
    virtual bool isHTMLContentAttribute(const Attribute&) const OVERRIDE;

    virtual bool isFrameElementBase() const { return true; }

    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    bool viewSourceMode() const { return m_viewSource; }

    void setNameAndOpenURL();
    void openURL(bool lockHistory = true, bool lockBackForwardList = true);

    AtomicString m_URL;
    AtomicString m_frameName;

    ScrollbarMode m_scrolling;

    int m_marginWidth;
    int m_marginHeight;

    bool m_viewSource;
};

inline HTMLFrameElementBase* toHTMLFrameElementBase(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->hasTagName(HTMLNames::frameTag) || node->hasTagName(HTMLNames::iframeTag));
    return static_cast<HTMLFrameElementBase*>(node);
}

} // namespace WebCore

#endif // HTMLFrameElementBase_h
