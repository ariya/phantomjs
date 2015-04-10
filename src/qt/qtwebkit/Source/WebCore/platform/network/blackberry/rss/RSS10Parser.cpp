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
#include "RSS10Parser.h"

#include "BlackBerryPlatformAssert.h"
#include "libxml/parser.h"
#include "libxml/xmlwriter.h"

namespace WebCore {

RSS10Parser::RSS10Parser()
{
}

bool RSS10Parser::parseBuffer(const char* buffer, int length, const char* url, const char* encoding)
{
    return parseXmlDoc(xmlReadMemory(buffer, length, url, encoding, XML_PARSE_NOBLANKS | XML_PARSE_NONET));
}

bool RSS10Parser::parseXmlDoc(xmlDocPtr doc)
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

        if (name == "RDF") {
            xmlNode* childnode = node->children;
            for (; childnode; childnode = childnode->next) {
                if (childnode->type == XML_ELEMENT_NODE) {
                    name = String(reinterpret_cast<const char*>(childnode->name));
                    name.makeLower();
                    if (name == "channel") {
                        BLACKBERRY_ASSERT(!m_root);
                        if (!m_root)
                            m_root = parseFeed(childnode->children);
                    } else if (name == "item") {
                        BLACKBERRY_ASSERT(m_root);
                        if (m_root) {
                            RSSItem* item = parseItem(childnode->children);
                            if (item)
                                m_root->m_items.append(item);
                        }
                    }
                }
            }
        }
    }

    xmlFreeDoc(doc);

    return m_root;
}

bool RSS10Parser::parseItemBaseAttribute(RSSItemBase* item, const String& name, xmlNode* node)
{
    if (name == "link")
        item->m_link = textFromXMLNode(node);
    else if (name == "title")
        item->m_title = textFromXMLNode(node);
    else if (name == "description")
        item->m_description = textFromXMLNode(node);
    else
        return false;

    return true;
}

RSSItem* RSS10Parser::parseItem(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSItem* item = new RSSItem();

    for (; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        parseItemBaseAttribute(item, name, node);
    }

    return item;
}

RSSFeed* RSS10Parser::parseFeed(xmlNode* node)
{
    BLACKBERRY_ASSERT(node);

    RSSFeed* feed = new RSSFeed();

    for (; node; node = node->next) {
        String name(reinterpret_cast<const char*>(node->name));
        name.makeLower();

        parseItemBaseAttribute(feed, name, node);
    }

    return feed;
}

} // namespace WebCore
