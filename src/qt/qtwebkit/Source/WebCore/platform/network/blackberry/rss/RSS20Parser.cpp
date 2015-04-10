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
#include "RSS20Parser.h"

#include "BlackBerryPlatformAssert.h"
#include "libxml/parser.h"
#include "libxml/xmlwriter.h"

namespace WebCore {

RSS20Parser::RSS20Parser()
{
}

bool RSS20Parser::parseBuffer(const char* buffer, int length, const char* url, const char* encoding)
{
    return parseXmlDoc(xmlReadMemory(buffer, length, url, encoding, XML_PARSE_NOBLANKS | XML_PARSE_NONET));
}

bool RSS20Parser::parseXmlDoc(xmlDocPtr doc)
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

        if (name == "rss") {
            xmlNode* channel = node->children;
            if (channel->type == XML_ELEMENT_NODE) {
                name = reinterpret_cast<const char*>(channel->name);
                name.makeLower();
                if (name == "channel")
                    m_root = parseFeed(channel->children);
            }
            break;
        }
    }

    xmlFreeDoc(doc);

    return m_root;
}

bool RSS20Parser::parseItemBaseAttribute(RSSItemBase* item, const String& name, xmlNode* node)
{
    if (name == "link")
        item->m_link = textFromXMLNode(node);
    else if (name == "title")
        item->m_title = textFromXMLNode(node);
    else if (name == "description")
        item->m_description = textFromXMLNode(node);
    else if (name == "pubdate")
        item->m_pubDate = textFromXMLNode(node);
    else
        return false;

    return true;
}

RSSItem* RSS20Parser::parseItem(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSItem* item = new RSSItem();

    for (; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        if (parseItemBaseAttribute(item, name, node))
            continue;

        if (name == "author")
            item->m_author = textFromXMLNode(node);
        else if (name == "creator") {
            if (item->m_author.isEmpty())
                item->m_author = textFromXMLNode(node);
        } else if (name == "category")
            item->m_categories.append(textFromXMLNode(node));
        else if (name == "comments")
            item->m_comments = textFromXMLNode(node);
        else if (name == "enclosure") {
            // Right now we assume there is only one enclosure per item, and we handle only
            // the first enclosure if there are multiple ones.
            // Reference: http://www.rssboard.org/rss-profile#element-channel-item-enclosure
            if (!item->m_enclosure)
                item->m_enclosure = parseEnclosure(node);
        }
    }

    return item;
}

RSSFeed* RSS20Parser::parseFeed(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSFeed* feed = new RSSFeed();

    for (; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        if (parseItemBaseAttribute(feed, name, node))
            continue;

        if (name == "item")
            feed->m_items.append(parseItem(node->children));
        else if (name == "language")
            feed->m_language = textFromXMLNode(node);
        else if (name == "ttl")
            feed->m_ttl = textFromXMLNode(node);
    }

    return feed;
}

RSSEnclosure* RSS20Parser::parseEnclosure(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSEnclosure* enclosure = new RSSEnclosure();

    for (xmlAttr* attr = node->properties; attr; attr = attr->next) {
        String name(reinterpret_cast<const char*>(attr->name));
        name.makeLower();

        if (name == "url")
            enclosure->m_url = textFromXMLAttr(attr);
        else if (name == "type")
            enclosure->m_type = textFromXMLAttr(attr);
        else if (name == "length")
            enclosure->m_length = textFromXMLAttr(attr);
    }

    return enclosure;
}

} // namespace WebCore
