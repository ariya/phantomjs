/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
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
#include "StyleSheetList.h"

#include "CSSStyleSheet.h"
#include "Document.h"
#include "HTMLNames.h"
#include "HTMLStyleElement.h"
#include "PlatformString.h"

namespace WebCore {

using namespace HTMLNames;

StyleSheetList::StyleSheetList(Document* doc)
    : m_doc(doc)
{
}

StyleSheetList::~StyleSheetList()
{
}

void StyleSheetList::documentDestroyed()
{
    m_doc = 0;
}

unsigned StyleSheetList::length() const
{
    return m_sheets.size();
}

StyleSheet* StyleSheetList::item(unsigned index)
{
    return index < length() ? m_sheets[index].get() : 0;
}

HTMLStyleElement* StyleSheetList::getNamedItem(const String& name) const
{
    if (!m_doc)
        return 0;

    // IE also supports retrieving a stylesheet by name, using the name/id of the <style> tag
    // (this is consistent with all the other collections)
    // ### Bad implementation because returns a single element (are IDs always unique?)
    // and doesn't look for name attribute.
    // But unicity of stylesheet ids is good practice anyway ;)
    
    Element* element = m_doc->getElementById(name);
    if (element && element->hasTagName(styleTag))
        return static_cast<HTMLStyleElement*>(element);
    return 0;
}

} // namespace WebCore
