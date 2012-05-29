/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2009 Michelangelo De Simone <micdesim@gmail.com>
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "BaseTextInputType.h"

#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "RegularExpression.h"

namespace WebCore {

using namespace HTMLNames;

bool BaseTextInputType::isTextType() const
{
    return true;
}

bool BaseTextInputType::patternMismatch(const String& value) const
{
    const AtomicString& pattern = element()->fastGetAttribute(patternAttr);
    // Empty values can't be mismatched
    if (pattern.isEmpty() || value.isEmpty())
        return false;
    int matchLength = 0;
    int valueLength = value.length();
    int matchOffset = RegularExpression(pattern, TextCaseSensitive).match(value, 0, &matchLength);
    return matchOffset || matchLength != valueLength;
}

} // namespace WebCore
