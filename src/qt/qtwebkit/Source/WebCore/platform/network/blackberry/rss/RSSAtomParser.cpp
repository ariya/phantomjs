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

#include "config.h"
#include "RSSAtomParser.h"

#include "BlackBerryPlatformAssert.h"
#include "libxml/parser.h"
#include "libxml/xmlwriter.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static inline bool isRelativePath(const String& path)
{
    return !(path.startsWith("/") || path.find(":/") != WTF::notFound);
}

RSSAtomLink::Type RSSAtomLink::relType()
{
    if (m_typeInEnum != TypeUnknown)
        return m_typeInEnum;

    if (m_rel.isEmpty())
        m_typeInEnum = TypeAlternate;
    else {
        String lowrel = m_rel.lower();
        if (lowrel == "alternate")
            m_typeInEnum = TypeAlternate;
        else if (lowrel == "enclosure")
            m_typeInEnum = TypeEnclosure;
        else if (lowrel == "related")
            m_typeInEnum = TypeRelated;
        else if (lowrel == "self")
            m_typeInEnum = TypeSelf;
        else if (lowrel == "via")
            m_typeInEnum = TypeVia;
        else
            m_typeInEnum = TypeUnsupported;
    }

    return m_typeInEnum;
}

RSSAtomParser::RSSAtomParser()
{
}

bool RSSAtomParser::parseBuffer(const char* buffer, int length, const char* url, const char* encoding)
{
    m_url = KURL(blankURL(), url);
    return parseXmlDoc(xmlReadMemory(buffer, length, url, encoding, XML_PARSE_NOBLANKS | XML_PARSE_NONET));
}

bool RSSAtomParser::parseXmlDoc(xmlDocPtr doc)
{
    if (!doc)
        return false;

    xmlNode* node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        return false;
    }

    for (; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        if (name == "feed") {
            m_root = parseFeed(node->children);
            break;
        }
    }

    xmlFreeDoc(doc);
    return m_root;
}

bool RSSAtomParser::parseItemBaseAttribute(RSSItemBase* item, const String& name, xmlNode* node, const String& base)
{
    if (name == "title")
        item->m_title = textFromXMLNode(node);
    else if (name == "id")
        item->m_id = textFromXMLNode(node);
    else if (name == "author")
        item->m_author = parseAuthor(node);
    else if (name == "updated")
        item->m_updated = textFromXMLNode(node);
    else if (name == "content")
        item->m_description = parseContent(base, node);
    else if (name == "published")
        item->m_pubDate = textFromXMLNode(node);
    else
        return false;

    return true;
}

RSSItem* RSSAtomParser::parseItem(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSItem* item = new RSSItem();

    String base;
    for (xmlAttr* attr = node->properties; attr; attr = attr->next) {
        String name(reinterpret_cast<const char*>(attr->name));
        name.makeLower();
        if (name == "base")
            base = textFromXMLAttr(attr);
    }

    node = node->children;
    for (; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        if (parseItemBaseAttribute(item, name, node, base))
            continue;

        if (name == "link") {
            RSSAtomLink* link = parseLink(node);
            if (isRelativePath(link->m_href))
                link->m_href = base + "/" + link->m_href;

            switch (link->relType()) {
            case RSSAtomLink::TypeAlternate:
                item->m_link = link->m_href;
                break;
            case RSSAtomLink::TypeEnclosure:
                BLACKBERRY_ASSERT(!item->m_enclosure);
                if (!item->m_enclosure)
                    item->m_enclosure = enclosureFromLink(link);
                break;
            default:
                break;
            }
            delete link;
        } else if (name == "category")
            item->m_categories.append(parseCategory(node));
    }

    return item;
}

RSSFeed* RSSAtomParser::parseFeed(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSFeed* feed = new RSSFeed();

    for (; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        if (parseItemBaseAttribute(feed, name, node, emptyString()))
            continue;

        if (name == "entry")
            feed->m_items.append(parseItem(node));
        else if (name == "link") {
            RSSAtomLink* link = parseLink(node);
            if (link->relType() == RSSAtomLink::TypeAlternate)
                feed->m_link = link->m_href;
            delete link;
        }
    }

    return feed;
}

RSSAtomLink* RSSAtomParser::parseLink(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSAtomLink* link = new RSSAtomLink();

    for (xmlAttr* attr = node->properties; attr; attr = attr->next) {
        String name(reinterpret_cast<const char*>(attr->name));
        name.makeLower();

        if (name == "href")
            link->m_href = textFromXMLAttr(attr);
        else if (name == "rel")
            link->m_rel = textFromXMLAttr(attr);
        else if (name == "type")
            link->m_type = textFromXMLAttr(attr);
        else if (name == "hreflang")
            link->m_hreflang = textFromXMLAttr(attr);
        else if (name == "title")
            link->m_title = textFromXMLAttr(attr);
        else if (name == "length")
            link->m_length = textFromXMLAttr(attr);
    }

    return link;
}

RSSEnclosure* RSSAtomParser::enclosureFromLink(RSSAtomLink* link)
{
    BLACKBERRY_ASSERT(link);
    BLACKBERRY_ASSERT(link->relType() == RSSAtomLink::TypeEnclosure);

    RSSEnclosure* enclosure = new RSSEnclosure();

    enclosure->m_url = link->m_href;
    enclosure->m_type = link->m_type;
    enclosure->m_length = link->m_length;

    return enclosure;
}

String RSSAtomParser::parseContent(const String& base, xmlNode* node)
{
    // See: http://tools.ietf.org/html/rfc4287#page-16

    BLACKBERRY_ASSERT(node);
    // Why does Blackberry have its own RSS parser?

    String content;
    String type = "default";
    String src;
    for (xmlAttr* attr = node->properties; attr; attr = attr->next) {
        String name(reinterpret_cast<const char*>(attr->name));
        name.makeLower();

        if (name == "type")
            type = textFromXMLAttr(attr);
        else if (name == "src")
            src = textFromXMLAttr(attr);
    }

    if (!src.isEmpty()) {
        if (isRelativePath(src))
            src = base + "/" + src;
        StringBuilder builder;
        builder.appendLiteral("<a href=\"");
        builder.append(src + "\">" + src + "</a>");
        return builder.toString();
    }

    if (type == "text" || type.startsWith("text/"))
        content = textFromXMLNode(node);
    else if (type == "html")
        content = textFromXMLNode(node);
    else if (type == "xhtml") {
        xmlBufferPtr buffer = xmlBufferCreate();
        xmlNode * cur = node->children;
        if (cur && cur->type == XML_ELEMENT_NODE) {
            // Encoding of buffer is utf-8.
            xmlNodeDump(buffer, cur->doc, cur, 0, 0);
            StringBuilder builder;
            if (!base.isEmpty()) {
                builder.appendLiteral("<base href='");
                builder.append(m_url.baseAsString());
                builder.appendLiteral("/");
                builder.append(base);
                builder.appendLiteral("/' />");
            }
            builder.append((const char*)xmlBufferContent(buffer));
            content = builder.toString();
        }
        xmlBufferFree(buffer);
    } else if (type.endsWith("+xml") || type.endsWith("/xml"))
        // FIXME: see atom spec 4.1.3.3.4.
        content = textFromXMLNode(node);
    else
        content = textFromXMLNode(node);

    return content;
}

String RSSAtomParser::parseAuthor(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    String username;
    String email;

    for (node = node->children; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        if (name == "name")
            username = textFromXMLNode(node);
        else if (name == "email")
            email = textFromXMLNode(node);
    }

    if (!email.isEmpty()) {
        username = username + " (";
        username = username + email;
        username = username + ")";
    }

    return username;
}

String RSSAtomParser::parseCategory(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    String category;

    for (xmlAttr* attr = node->properties; attr; attr = attr->next) {
        String name(reinterpret_cast<const char*>(attr->name));
        name.makeLower();

        // If there's a label, we use it, if not, use term attribute, as label is
        // optional, but term is mandatory.
        if (name == "label") {
            category = textFromXMLAttr(attr);
            break;
        }

        if (name == "term")
            category = textFromXMLAttr(attr);
    }

    return category;
}

} // namespace WebCore
