/**
 * Copyright (C) 2006 Apple Computer, Inc.
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
#include <wtf/text/WTFString.h>

#include <CoreFoundation/CFString.h>

namespace WTF {

String::String(NSString* str)
{
    if (!str)
        return;

    CFIndex size = CFStringGetLength(reinterpret_cast<CFStringRef>(str));
    if (size == 0)
        m_impl = StringImpl::empty();
    else {
        Vector<LChar, 1024> lcharBuffer(size);
        CFIndex usedBufLen;
        CFIndex convertedsize = CFStringGetBytes(reinterpret_cast<CFStringRef>(str), CFRangeMake(0, size), kCFStringEncodingISOLatin1, 0, false, lcharBuffer.data(), size, &usedBufLen);
        if ((convertedsize == size) && (usedBufLen == size)) {
            m_impl = StringImpl::create(lcharBuffer.data(), size);
            return;
        }

        Vector<UChar, 1024> ucharBuffer(size);
        CFStringGetCharacters(reinterpret_cast<CFStringRef>(str), CFRangeMake(0, size), ucharBuffer.data());
        m_impl = StringImpl::create(ucharBuffer.data(), size);
    }
}

}
