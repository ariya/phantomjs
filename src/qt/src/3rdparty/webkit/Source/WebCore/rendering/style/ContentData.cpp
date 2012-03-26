/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "ContentData.h"

#include "StyleImage.h"
#include <wtf/text/StringImpl.h>

namespace WebCore {

void ContentData::clear()
{
    deleteContent();

    // Delete the singly-linked list without recursing.
    for (OwnPtr<ContentData> next = m_next.release(); next; next = next->m_next.release()) { }
}

// FIXME: Why isn't this just operator==?
// FIXME: This is not a good name for a boolean-returning function.
bool ContentData::dataEquivalent(const ContentData& other) const
{
    if (type() != other.type())
        return false;

    switch (type()) {
    case CONTENT_NONE:
        return true;
    case CONTENT_TEXT:
        return equal(text(), other.text());
    case CONTENT_OBJECT:
        return StyleImage::imagesEquivalent(image(), other.image());
    case CONTENT_COUNTER:
        return *counter() == *other.counter();
    case CONTENT_QUOTE:
        return quote() == other.quote();
    }

    ASSERT_NOT_REACHED();
    return false;
}

void ContentData::deleteContent()
{
    switch (m_type) {
    case CONTENT_NONE:
        break;
    case CONTENT_OBJECT:
        m_content.m_image->deref();
        break;
    case CONTENT_TEXT:
        m_content.m_text->deref();
        break;
    case CONTENT_COUNTER:
        delete m_content.m_counter;
        break;
    case CONTENT_QUOTE:
        break;
    }

    m_type = CONTENT_NONE;
}

} // namespace WebCore
