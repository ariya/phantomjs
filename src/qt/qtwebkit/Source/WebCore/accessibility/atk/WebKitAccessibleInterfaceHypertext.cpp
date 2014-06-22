/*
 * Copyright (C) 2010, 2011, 2012 Igalia S.L.
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
 */

#include "config.h"
#include "WebKitAccessibleInterfaceHypertext.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkHypertext* hypertext)
{
    if (!WEBKIT_IS_ACCESSIBLE(hypertext))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(hypertext));
}

static AtkHyperlink* webkitAccessibleHypertextGetLink(AtkHypertext* hypertext, gint index)
{
    AccessibilityObject::AccessibilityChildrenVector children = core(hypertext)->children();
    if (index < 0 || static_cast<unsigned>(index) >= children.size())
        return 0;

    gint currentLink = -1;
    for (unsigned i = 0; i < children.size(); i++) {
        AccessibilityObject* coreChild = children.at(i).get();
        if (!coreChild->accessibilityIsIgnored()) {
            AtkObject* axObject = coreChild->wrapper();
            if (!axObject || !ATK_IS_HYPERLINK_IMPL(axObject))
                continue;

            currentLink++;
            if (index != currentLink)
                continue;

            return atk_hyperlink_impl_get_hyperlink(ATK_HYPERLINK_IMPL(axObject));
        }
    }

    return 0;
}

static gint webkitAccessibleHypertextGetNLinks(AtkHypertext* hypertext)
{
    AccessibilityObject::AccessibilityChildrenVector children = core(hypertext)->children();
    if (!children.size())
        return 0;

    gint linksFound = 0;
    for (size_t i = 0; i < children.size(); i++) {
        AccessibilityObject* coreChild = children.at(i).get();
        if (!coreChild->accessibilityIsIgnored()) {
            AtkObject* axObject = coreChild->wrapper();
            if (axObject && ATK_IS_HYPERLINK_IMPL(axObject))
                linksFound++;
        }
    }

    return linksFound;
}

static gint webkitAccessibleHypertextGetLinkIndex(AtkHypertext* hypertext, gint charIndex)
{
    size_t linksCount = webkitAccessibleHypertextGetNLinks(hypertext);
    if (!linksCount)
        return -1;

    for (size_t i = 0; i < linksCount; i++) {
        AtkHyperlink* hyperlink = ATK_HYPERLINK(webkitAccessibleHypertextGetLink(hypertext, i));
        gint startIndex = atk_hyperlink_get_start_index(hyperlink);
        gint endIndex = atk_hyperlink_get_end_index(hyperlink);

        // Check if the char index in the link's offset range
        if (startIndex <= charIndex && charIndex < endIndex)
            return i;
    }

    // Not found if reached
    return -1;
}

void webkitAccessibleHypertextInterfaceInit(AtkHypertextIface* iface)
{
    iface->get_link = webkitAccessibleHypertextGetLink;
    iface->get_n_links = webkitAccessibleHypertextGetNLinks;
    iface->get_link_index = webkitAccessibleHypertextGetLinkIndex;
}

#endif
