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
#include "RSSGenerator.h"

#include "LocalizeResource.h"
#include "RSSParserBase.h"

#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static const char* s_defaultFeedTitle = i18n("Untitled Feed");
static const char* s_defaultEntryTitle = i18n("Untitled Entry");

RSSGenerator::RSSGenerator()
{
}

RSSGenerator::~RSSGenerator()
{
}

String RSSGenerator::generateHtml(RSSFeed* feed)
{
    StringBuilder builder;
    builder.appendLiteral("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />");
    builder.appendLiteral("<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable= no\">");
    builder.appendLiteral("<title>");

    if (!feed->m_title.isEmpty())
        builder.append(feed->m_title);
    else if (!feed->m_link.isEmpty())
        builder.append(feed->m_link);
    else
        builder.append(s_defaultFeedTitle);

    builder.appendLiteral("</title></head><body>");

    builder.appendLiteral("<h2>");
    if (!feed->m_link.isEmpty()) {
        builder.appendLiteral("<a href=\"");
        builder.append(feed->m_link);
        builder.appendLiteral("\">");
    }
    if (!feed->m_title.isEmpty())
        builder.append(feed->m_title);
    else
        builder.append(s_defaultFeedTitle);

    if (!feed->m_link.isEmpty())
        builder.appendLiteral("</a>");

    builder.appendLiteral("</h2>");

    if (!feed->m_description.isEmpty()) {
        builder.appendLiteral("<p>");
        builder.append(feed->m_description);
        builder.appendLiteral("</p>");
    }

    for (unsigned i = 0; i < feed->m_items.size(); ++i) {
        RSSItem* item = feed->m_items[i];
        String articleName;
        builder.appendLiteral("<div id=\"");
        builder.append(articleName);
        builder.appendLiteral("\" class=\"article\">\n<a href=\"");
        if (!item->m_link.isEmpty())
            builder.append(item->m_link);
        builder.appendLiteral("\"><b>");
        if (!item->m_title.isEmpty())
            builder.append(item->m_title);
        else
            builder.append(s_defaultEntryTitle);
        builder.appendLiteral("</b></a>\n<br />");

        if (!item->m_author.isEmpty()) {
            builder.append(i18n("By"));
            builder.appendLiteral(" <b>");
            builder.append(item->m_author);
            builder.appendLiteral("</b> ");
        } else {
            if (!feed->m_author.isEmpty()) {
                builder.append(i18n("By"));
                builder.appendLiteral(" <b>");
                builder.append(feed->m_author);
                builder.appendLiteral("</b> ");
            }
        }

        if (!item->m_categories.isEmpty()) {
            if (!item->m_author.isEmpty())
                builder.append(i18n("under "));
            else
                builder.append(i18n("Under "));

            for (unsigned i = 0; i < item->m_categories.size() ; ++i) {
                builder.appendLiteral("<b>");
                builder.append(item->m_categories[i]);
                builder.appendLiteral("</b>");

                if (i < item->m_categories.size() - 1)
                    builder.appendLiteral(", ");
            }
        }

        builder.appendLiteral("<br />");
        if (!item->m_pubDate.isEmpty())
            builder.append(item->m_pubDate);

        builder.appendLiteral("<br />");
        if (!item->m_description.isEmpty())
            builder.append(item->m_description);
        builder.appendLiteral("<br />");

        if (item->m_enclosure) {
            builder.appendLiteral("<br />");
            builder.appendLiteral("<a href=\"");
            builder.append(item->m_enclosure->m_url);
            builder.appendLiteral("\">");
            builder.append(i18n("Embedded "));

            RSSEnclosure::Type type = item->m_enclosure->typeInEnum();
            switch (type) {
            case RSSEnclosure::TypeImage:
                builder.append(i18n("Image"));
                break;
            case RSSEnclosure::TypeAudio:
                builder.append(i18n("Audio"));
                break;
            case RSSEnclosure::TypeVideo:
                builder.append(i18n("Video"));
                break;
            case RSSEnclosure::TypeApplication:
            default:
                builder.append(i18n("Unknown Content"));
                break;
            }

            builder.append(i18n(" Source: "));
            builder.append(item->m_enclosure->suggestedName());
            builder.appendLiteral("</a><br /><br />");
        }

        builder.appendLiteral("<br /></div>\n");
    }

    builder.appendLiteral("</body></html>\n");

    return builder.toString();
}

} // namespace WebCore
