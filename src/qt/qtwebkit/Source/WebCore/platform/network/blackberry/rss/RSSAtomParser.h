/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef RSSAtomParser_h
#define RSSAtomParser_h

#include "KURL.h"
#include "RSSParserBase.h"

namespace WebCore {

class RSSAtomLink {
public:
    enum Type {
        TypeUnknown,
        TypeAlternate,
        TypeRelated,
        TypeSelf,
        TypeEnclosure,
        TypeVia,
        TypeUnsupported
    };

    RSSAtomLink()
        : m_typeInEnum(TypeUnknown)
    { }

    Type relType();

    String m_rel;
    String m_href;
    String m_hreflang;
    String m_type;
    String m_title;
    String m_length;

private:
    Type m_typeInEnum;
};

class RSSAtomParser : public RSSParserBase {
public:
    RSSAtomParser();

    bool parseBuffer(const char* buffer, int length, const char* url, const char* encoding);

private:
    bool parseXmlDoc(xmlDocPtr);
    bool parseItemBaseAttribute(RSSItemBase*, const String& name, xmlNode*, const String& base);
    RSSItem* parseItem(xmlNode*);
    RSSFeed* parseFeed(xmlNode*);
    RSSAtomLink* parseLink(xmlNode*);
    RSSEnclosure* enclosureFromLink(RSSAtomLink*);

    String parseContent(const String& base, xmlNode*);
    String parseAuthor(xmlNode*);
    String parseCategory(xmlNode*);
    KURL m_url;
};

} // namespace WebCore

#endif // RSSAtomParser_h
