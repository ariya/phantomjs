/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "FormDataList.h"

#include "LineEnding.h"

namespace WebCore {

FormDataList::FormDataList(const TextEncoding& c)
    : m_encoding(c)
{
}

void FormDataList::appendString(const String& s)
{
    CString cstr = m_encoding.encode(s.characters(), s.length(), EntitiesForUnencodables);
    m_items.append(normalizeLineEndingsToCRLF(cstr));
}

void FormDataList::appendString(const CString& s)
{
    m_items.append(s);
}

void FormDataList::appendBlob(PassRefPtr<Blob> blob, const String& filename)
{
    m_items.append(Item(blob, filename));
}

} // namespace
