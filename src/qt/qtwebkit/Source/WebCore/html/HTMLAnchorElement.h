/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLAnchorElement_h
#define HTMLAnchorElement_h

#include "HTMLElement.h"
#include "HTMLNames.h"
#include "LinkHash.h"

namespace WebCore {

// Link relation bitmask values.
// FIXME: Uncomment as the various link relations are implemented.
enum {
//     RelationAlternate   = 0x00000001,
//     RelationArchives    = 0x00000002,
//     RelationAuthor      = 0x00000004,
//     RelationBoomark     = 0x00000008,
//     RelationExternal    = 0x00000010,
//     RelationFirst       = 0x00000020,
//     RelationHelp        = 0x00000040,
//     RelationIndex       = 0x00000080,
//     RelationLast        = 0x00000100,
//     RelationLicense     = 0x00000200,
//     RelationNext        = 0x00000400,
//     RelationNoFolow    = 0x00000800,
    RelationNoReferrer     = 0x00001000,
//     RelationPrev        = 0x00002000,
//     RelationSearch      = 0x00004000,
//     RelationSidebar     = 0x00008000,
//     RelationTag         = 0x00010000,
//     RelationUp          = 0x00020000,
};

class HTMLAnchorElement : public HTMLElement {
public:
    static PassRefPtr<HTMLAnchorElement> create(Document*);
    static PassRefPtr<HTMLAnchorElement> create(const QualifiedName&, Document*);

    virtual ~HTMLAnchorElement();

    KURL href() const;
    void setHref(const AtomicString&);

    const AtomicString& name() const;

    String hash() const;
    void setHash(const String&);

    String host() const;
    void setHost(const String&);

    String hostname() const;
    void setHostname(const String&);

    String pathname() const;
    void setPathname(const String&);

    String port() const;
    void setPort(const String&);

    String protocol() const;
    void setProtocol(const String&);

    String search() const;
    void setSearch(const String&);

    String origin() const;

    String text();

    String toString() const;

    bool isLiveLink() const;

    virtual bool willRespondToMouseClickEvents() OVERRIDE;

    bool hasRel(uint32_t relation) const;
    void setRel(const String&);
    
    LinkHash visitedLinkHash() const;
    void invalidateCachedVisitedLinkHash() { m_cachedVisitedLinkHash = 0; }

protected:
    HTMLAnchorElement(const QualifiedName&, Document*);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

private:
    virtual bool supportsFocus() const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const OVERRIDE;
    virtual void defaultEventHandler(Event*);
    virtual void setActive(bool active = true, bool pause = false) OVERRIDE FINAL;
    virtual void accessKeyAction(bool sendMouseEvents);
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;
    virtual bool canStartSelection() const;
    virtual String target() const;
    virtual short tabIndex() const OVERRIDE FINAL;
    virtual bool draggable() const;

    void sendPings(const KURL& destinationURL);

    void handleClick(Event*);

    enum EventType {
        MouseEventWithoutShiftKey,
        MouseEventWithShiftKey,
        NonMouseEvent,
    };
    static EventType eventType(Event*);
    bool treatLinkAsLiveForEventType(EventType) const;

#if ENABLE(MICRODATA)
    virtual String itemValueText() const OVERRIDE;
    virtual void setItemValueText(const String&, ExceptionCode&) OVERRIDE;
#endif

    Element* rootEditableElementForSelectionOnMouseDown() const;
    void setRootEditableElementForSelectionOnMouseDown(Element*);
    void clearRootEditableElementForSelectionOnMouseDown();

    bool m_hasRootEditableElementForSelectionOnMouseDown : 1;
    bool m_wasShiftKeyDownOnMouseDown : 1;
    uint32_t m_linkRelations : 30;
    mutable LinkHash m_cachedVisitedLinkHash;
};

inline LinkHash HTMLAnchorElement::visitedLinkHash() const
{
    if (!m_cachedVisitedLinkHash)
        m_cachedVisitedLinkHash = WebCore::visitedLinkHash(document()->baseURL(), fastGetAttribute(HTMLNames::hrefAttr));
    return m_cachedVisitedLinkHash; 
}

inline bool isHTMLAnchorElement(Node* node)
{
    return node->hasTagName(HTMLNames::aTag);
}

inline bool isHTMLAnchorElement(Element* element)
{
    return element->hasTagName(HTMLNames::aTag);
}

inline HTMLAnchorElement* toHTMLAnchorElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLAnchorElement(node));
    return static_cast<HTMLAnchorElement*>(node);
}

// Functions shared with the other anchor elements (i.e., SVG).

bool isEnterKeyKeydownEvent(Event*);
bool isLinkClick(Event*);
bool shouldProhibitLinks(Element*);

} // namespace WebCore

#endif // HTMLAnchorElement_h
