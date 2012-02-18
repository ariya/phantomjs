/*
 * Copyright (C) 2003, 2009 Apple Inc. All rights reserved.
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
#include "EditingText.h"

#include "Document.h"

// FIXME: Does this really require a class? Perhaps instead any text node
// inside an editable element could have the "always create a renderer" behavior.

namespace WebCore {

inline EditingText::EditingText(Document* document, const String& data)
    : Text(document, data)
{
}

PassRefPtr<EditingText> EditingText::create(Document* document, const String& data)
{
    return adoptRef(new EditingText(document, data));
}

bool EditingText::rendererIsNeeded(RenderStyle*)
{
    return true;
}

} // namespace WebCore
