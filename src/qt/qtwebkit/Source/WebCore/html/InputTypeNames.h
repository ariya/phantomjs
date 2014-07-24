/*
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


#ifndef InputTypeNames_h
#define InputTypeNames_h

#include <wtf/text/AtomicString.h>

namespace WebCore {

namespace InputTypeNames {

const AtomicString& button();
const AtomicString& checkbox();
const AtomicString& color();
const AtomicString& date();
const AtomicString& datetime();
const AtomicString& datetimelocal();
const AtomicString& email();
const AtomicString& file();
const AtomicString& hidden();
const AtomicString& image();
const AtomicString& month();
const AtomicString& number();
const AtomicString& password();
const AtomicString& radio();
const AtomicString& range();
const AtomicString& reset();
const AtomicString& search();
const AtomicString& submit();
const AtomicString& telephone();
const AtomicString& text();
const AtomicString& time();
const AtomicString& url();
const AtomicString& week();

}

} // namespace WebCore

#endif // InputTypeNames_h
