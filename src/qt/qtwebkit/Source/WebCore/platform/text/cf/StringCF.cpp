/**
 * Copyright (C) 2006, 2012 Apple Computer, Inc.
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

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#include <wtf/RetainPtr.h>

namespace WTF {

String::String(CFStringRef str)
{
    if (!str)
        return;

    CFIndex size = CFStringGetLength(str);
    if (size == 0)
        m_impl = StringImpl::empty();
    else {
        Vector<LChar, 1024> lcharBuffer(size);
        CFIndex usedBufLen;
        CFIndex convertedsize = CFStringGetBytes(str, CFRangeMake(0, size), kCFStringEncodingISOLatin1, 0, false, lcharBuffer.data(), size, &usedBufLen);
        if ((convertedsize == size) && (usedBufLen == size)) {
            m_impl = StringImpl::create(lcharBuffer.data(), size);
            return;
        }

        Vector<UChar, 1024> buffer(size);
        CFStringGetCharacters(str, CFRangeMake(0, size), (UniChar*)buffer.data());
        m_impl = StringImpl::create(buffer.data(), size);
    }
}

RetainPtr<CFStringRef> String::createCFString() const
{
    if (!m_impl)
        return CFSTR("");

    return m_impl->createCFString();
}

}

#endif // USE(CF)
