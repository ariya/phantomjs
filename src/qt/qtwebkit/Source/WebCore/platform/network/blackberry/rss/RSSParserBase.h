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


#ifndef RSSParserBase_h
#define RSSParserBase_h

#include <BlackBerryPlatformMisc.h>
#include <libxml/tree.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class RSSItem;

class RSSEnclosure {
public:
    enum Type {
        TypeUnknown,
        TypeImage,
        TypeAudio,
        TypeVideo,
        TypeApplication,
        TypeUnsupported
    };

    RSSEnclosure();

    Type typeInEnum();
    String suggestedName() const;

    String m_url;
    String m_type;
    String m_length;

private:
    Type m_typeInEnum;
};

class RSSItemBase {
public:
    String m_title;
    String m_link;
    String m_description;
    String m_updated;
    String m_author;
    String m_pubDate;
    String m_id;
};

class RSSFeed: public RSSItemBase {
public:
    RSSFeed();
    ~RSSFeed();

    void clear();

    String m_language;
    String m_ttl;

    Vector<RSSItem*> m_items;
};

class RSSItem: public RSSItemBase {
public:
    RSSItem();
    ~RSSItem();

    void clear();

    String m_comments;
    Vector<String> m_categories;
    RSSEnclosure* m_enclosure;
private:
    DISABLE_COPY(RSSItem)
};

class RSSParserBase {
public:
    RSSParserBase();
    virtual ~RSSParserBase();

    virtual bool parseBuffer(const char* buffer, int length, const char* url, const char* encoding) = 0;

    RSSFeed* m_root;
private:
    DISABLE_COPY(RSSParserBase)
};

String textFromXMLAttr(xmlAttr*);
String textFromXMLNode(xmlNode*);

} // namespace WebCore

#endif // RSSParserBase_h
