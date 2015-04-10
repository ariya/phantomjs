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
    const AtomicString& rawPattern = element()->fastGetAttribute(patternAttr);
    if (rawPattern.isNull() || value.isEmpty() || !RegularExpression(rawPattern, TextCaseSensitive).isValid())
        return false;
    String pattern = "^(?:" + rawPattern + ")$";
    int matchLength = 0;
    int valueLength = value.length();
    int matchOffset = RegularExpression(pattern, TextCaseSensitive).match(value, 0, &matchLength);
    return matchOffset || matchLength != valueLength;
}

bool BaseTextInputType::supportsPlaceholder() const
{
    return true;
}

bool BaseTextInputType::supportsSelectionAPI() const
{
    return true;
}

} // namespace WebCore
