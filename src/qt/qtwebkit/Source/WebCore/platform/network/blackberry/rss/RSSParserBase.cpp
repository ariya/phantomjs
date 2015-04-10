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
#include "RSSParserBase.h"

#include "libxml/parser.h"
#include "libxml/xmlwriter.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

RSSEnclosure::RSSEnclosure()
    : m_typeInEnum(TypeUnknown)
{ }

RSSEnclosure::Type RSSEnclosure::typeInEnum()
{
    if (TypeUnknown != m_typeInEnum)
        return m_typeInEnum;

    size_t div = m_type.find('/');
    if (WTF::notFound == div || !div) {
        m_typeInEnum = TypeUnsupported;
        return m_typeInEnum;
    }

    String base = m_type.substring(0, div).lower();
    if (base == "image")
        m_typeInEnum = TypeImage;
    else if (base == "audio")
        m_typeInEnum = TypeAudio;
    else if (base == "video")
        m_typeInEnum = TypeVideo;
    else if (base == "application")
        m_typeInEnum = TypeApplication;
    else
        m_typeInEnum = TypeUnsupported;

    return m_typeInEnum;
}

String RSSEnclosure::suggestedName() const
{
    size_t start = m_url.reverseFind('/');
    if (WTF::notFound == start)
        start = 0;
    else
        ++start;

    return m_url.substring(start);
}

RSSFeed::RSSFeed()
{
}

RSSFeed::~RSSFeed()
{
    clear();
}

void RSSFeed::clear()
{
    deleteAllValues(m_items);
    m_items.clear();
}

RSSItem::RSSItem()
    : m_enclosure(0)
{
}

RSSItem::~RSSItem()
{
    clear();
}

void RSSItem::clear()
{
    delete m_enclosure;
}

RSSParserBase::RSSParserBase()
    : m_root(0)
{
}

RSSParserBase::~RSSParserBase()
{
    delete m_root;
}

String textFromXMLAttr(xmlAttr* attr)
{
    if (!attr)
        return emptyString();

    StringBuilder text;

    for (xmlNode* node = attr->children; node; node = node->next) {
        if (node->type == XML_TEXT_NODE)
            text.append(reinterpret_cast<const char*>(node->content));
    }

    return text.toString().stripWhiteSpace();
}

String textFromXMLNode(xmlNode* node)
{
    if (!node)
        return emptyString();

    StringBuilder text;

    for (node = node->children; node; node = node->next) {
        if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE))
            text.append(reinterpret_cast<const char*>(node->content));
    }

    return text.toString().stripWhiteSpace();
}

} // namespace WebCore
