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

    String getParameter(const String&) const;

    String text() const;

    String toString() const;

    bool isLiveLink() const;

    bool hasRel(uint32_t relation) const;
    void setRel(const String&);

protected:
    HTMLAnchorElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);

private:
    virtual bool supportsFocus() const;
    virtual bool isMouseFocusable() const;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const;
    virtual void defaultEventHandler(Event*);
    virtual void setActive(bool active = true, bool pause = false);
    virtual void accessKeyAction(bool fullAction);
    virtual bool isURLAttribute(Attribute*) const;
    virtual bool canStartSelection() const;
    virtual String target() const;
    virtual short tabIndex() const;
    virtual bool draggable() const;

    void sendPings(const KURL& destinationURL);

    enum EventType {
        MouseEventWithoutShiftKey,
        MouseEventWithShiftKey,
        NonMouseEvent,
    };
    static EventType eventType(Event*);
    bool treatLinkAsLiveForEventType(EventType) const;

    RefPtr<Element> m_rootEditableElementForSelectionOnMouseDown;
    bool m_wasShiftKeyDownOnMouseDown;
    uint32_t m_linkRelations;
};

// Functions shared with the other anchor elements (i.e., SVG).

bool isEnterKeyKeydownEvent(Event*);
bool isMiddleMouseButtonEvent(Event*);
bool isLinkClick(Event*);
void handleLinkClick(Event*, Document*, const String& url, const String& target, bool hideReferrer = false);

} // namespace WebCore

#endif // HTMLAnchorElement_h
