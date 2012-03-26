/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006 Apple Computer, Inc.
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
#include "StyleSheet.h"

#include "MediaList.h"
#include "Node.h"

namespace WebCore {

StyleSheet::StyleSheet(StyleSheet* parentSheet, const String& originalURL, const KURL& finalURL)
    : StyleList(parentSheet)
    , m_parentNode(0)
    , m_originalURL(originalURL)
    , m_finalURL(finalURL)
    , m_disabled(false)
{
}

StyleSheet::StyleSheet(Node* parentNode, const String& originalURL, const KURL& finalURL)
    : StyleList(0)
    , m_parentNode(parentNode)
    , m_originalURL(originalURL)
    , m_finalURL(finalURL)
    , m_disabled(false)
{
}

StyleSheet::StyleSheet(StyleBase* owner, const String& originalURL, const KURL& finalURL)
    : StyleList(owner)
    , m_parentNode(0)
    , m_originalURL(originalURL)
    , m_finalURL(finalURL)
    , m_disabled(false)
{
}

StyleSheet::~StyleSheet()
{
    if (m_media)
        m_media->setParent(0);

    // For style rules outside the document, .parentStyleSheet can become null even if the style rule
    // is still observable from JavaScript. This matches the behavior of .parentNode for nodes, but
    // it's not ideal because it makes the CSSOM's behavior depend on the timing of garbage collection.
    for (unsigned i = 0; i < length(); ++i) {
        ASSERT(item(i)->parent() == this);
        item(i)->setParent(0);
    }
}

StyleSheet* StyleSheet::parentStyleSheet() const
{
    return (parent() && parent()->isStyleSheet()) ? static_cast<StyleSheet*>(parent()) : 0;
}

void StyleSheet::setMedia(PassRefPtr<MediaList> media)
{
    if (m_media)
        m_media->setParent(0);

    m_media = media;
    m_media->setParent(this);
}

KURL StyleSheet::completeURL(const String& url) const
{
    // Always return a null URL when passed a null string.
    // FIXME: Should we change the KURL constructor to have this behavior?
    // See also Document::completeURL(const String&)
    if (url.isNull())
        return KURL();
    return KURL(baseURL(), url);
}

}
