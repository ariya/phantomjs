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

#ifndef RSS20Parser_h
#define RSS20Parser_h

#include "RSSParserBase.h"

namespace WebCore {

class RSS20Parser : public RSSParserBase {
public:
    RSS20Parser();

    bool parseBuffer(const char* buffer, int length, const char* url, const char* encoding);

private:
    bool parseXmlDoc(xmlDocPtr);
    bool parseItemBaseAttribute(RSSItemBase*, const String& name, xmlNode*);
    RSSItem* parseItem(xmlNode*);
    RSSFeed* parseFeed(xmlNode*);
    RSSEnclosure* parseEnclosure(xmlNode*);
};

} // namespace WebCore

#endif // RSS20Parser_h
