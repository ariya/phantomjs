/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010, 2012 Google Inc. All rights reserved.
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
#include "InputTypeNames.h"

namespace WebCore {

namespace InputTypeNames {

// The type names must be lowercased because they will be the return values of
// input.type and input.type must be lowercase according to DOM Level 2.

const AtomicString& button()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("button", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& checkbox()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("checkbox", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& color()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("color", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& date()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("date", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& datetime()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("datetime", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& datetimelocal()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("datetime-local", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& email()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("email", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& file()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("file", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& hidden()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("hidden", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& image()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("image", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& month()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("month", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& number()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("number", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& password()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("password", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& radio()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("radio", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& range()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("range", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& reset()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("reset", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& search()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("search", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& submit()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("submit", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& telephone()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("tel", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& text()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("text", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& time()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("time", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& url()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("url", AtomicString::ConstructFromLiteral));
    return name;
}

const AtomicString& week()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("week", AtomicString::ConstructFromLiteral));
    return name;
}

} // namespace WebCore::InputTypeNames

} // namespace WebCore
