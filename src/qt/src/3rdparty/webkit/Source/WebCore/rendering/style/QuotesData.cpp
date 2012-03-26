/**
 * Copyright (C) 2011 Nokia Inc.  All rights reserved.
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
#include "QuotesData.h"

namespace WebCore {

QuotesData* QuotesData::create(int stringCount)
{
    char* tmp = new char[sizeof(QuotesData)+sizeof(String)*stringCount];
    if (!tmp)
        return 0;
    new (tmp) QuotesData(stringCount);
    for (int i = 0; i < stringCount; ++i)
        new (tmp +sizeof(QuotesData) + sizeof(String)*i) String();
    return reinterpret_cast<QuotesData*>(tmp);
}

bool QuotesData::operator==(const QuotesData& other) const
{
    if (this == &other)
        return true;
    if (!&other || !this)
        return false;
    if (length != other.length)
        return false;
    const String* myData = data();
    const String* otherData = other.data();
    for (int i = length-1; i >= 0; --i)
        if (myData[i] != otherData[i])
            return false;
    return true;
}

QuotesData::~QuotesData()
{
    String* p = data();
    for (int i = 0; i < length; ++i)
        p[i].~String();
}

} // namespace WebCore
