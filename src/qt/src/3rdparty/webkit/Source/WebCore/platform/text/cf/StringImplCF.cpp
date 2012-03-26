/*
 * Copyright (C) 2006, 2009 Apple Inc. All rights reserved.
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
#include <wtf/text/StringImpl.h>

#if USE(CF)

#include <CoreFoundation/CoreFoundation.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

#if PLATFORM(MAC)
#include <objc/objc-auto.h>
#endif

namespace WTF {

namespace StringWrapperCFAllocator {

    static StringImpl* currentString;

    static const void* retain(const void* info)
    {
        return info;
    }

    static void release(const void*)
    {
        ASSERT_NOT_REACHED();
    }

    static CFStringRef copyDescription(const void*)
    {
        return CFSTR("WTF::String-based allocator");
    }

    static void* allocate(CFIndex size, CFOptionFlags, void*)
    {
        StringImpl* underlyingString = 0;
        if (isMainThread()) {
            underlyingString = currentString;
            if (underlyingString) {
                currentString = 0;
                underlyingString->ref(); // Balanced by call to deref in deallocate below.
            }
        }
        StringImpl** header = static_cast<StringImpl**>(fastMalloc(sizeof(StringImpl*) + size));
        *header = underlyingString;
        return header + 1;
    }

    static void* reallocate(void* pointer, CFIndex newSize, CFOptionFlags, void*)
    {
        size_t newAllocationSize = sizeof(StringImpl*) + newSize;
        StringImpl** header = static_cast<StringImpl**>(pointer) - 1;
        ASSERT(!*header);
        header = static_cast<StringImpl**>(fastRealloc(header, newAllocationSize));
        return header + 1;
    }

    static void deallocateOnMainThread(void* headerPointer)
    {
        StringImpl** header = static_cast<StringImpl**>(headerPointer);
        StringImpl* underlyingString = *header;
        ASSERT(underlyingString);
        underlyingString->deref(); // Balanced by call to ref in allocate above.
        fastFree(header);
    }

    static void deallocate(void* pointer, void*)
    {
        StringImpl** header = static_cast<StringImpl**>(pointer) - 1;
        StringImpl* underlyingString = *header;
        if (!underlyingString)
            fastFree(header);
        else {
            if (!isMainThread())
                callOnMainThread(deallocateOnMainThread, header);
            else {
                underlyingString->deref(); // Balanced by call to ref in allocate above.
                fastFree(header);
            }
        }
    }

    static CFIndex preferredSize(CFIndex size, CFOptionFlags, void*)
    {
        // FIXME: If FastMalloc provided a "good size" callback, we'd want to use it here.
        // Note that this optimization would help performance for strings created with the
        // allocator that are mutable, and those typically are only created by callers who
        // make a new string using the old string's allocator, such as some of the call
        // sites in CFURL.
        return size;
    }

    static CFAllocatorRef create()
    {
#if PLATFORM(MAC)
        // Since garbage collection isn't compatible with custom allocators, don't use this at all when garbage collection is active.
        if (objc_collectingEnabled())
            return 0;
#endif
        CFAllocatorContext context = { 0, 0, retain, release, copyDescription, allocate, reallocate, deallocate, preferredSize };
        return CFAllocatorCreate(0, &context);
    }

    static CFAllocatorRef allocator()
    {
        static CFAllocatorRef allocator = create();
        return allocator;
    }

}

CFStringRef StringImpl::createCFString()
{
    CFAllocatorRef allocator = (m_length && isMainThread()) ? StringWrapperCFAllocator::allocator() : 0;
    if (!allocator)
        return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar*>(m_data), m_length);

    // Put pointer to the StringImpl in a global so the allocator can store it with the CFString.
    ASSERT(!StringWrapperCFAllocator::currentString);
    StringWrapperCFAllocator::currentString = this;

    CFStringRef string = CFStringCreateWithCharactersNoCopy(allocator, reinterpret_cast<const UniChar*>(m_data), m_length, kCFAllocatorNull);

    // The allocator cleared the global when it read it, but also clear it here just in case.
    ASSERT(!StringWrapperCFAllocator::currentString);
    StringWrapperCFAllocator::currentString = 0;

    return string;
}

// On StringImpl creation we could check if the allocator is the StringWrapperCFAllocator.
// If it is, then we could find the original StringImpl and just return that. But to
// do that we'd have to compute the offset from CFStringRef to the allocated block;
// the CFStringRef is *not* at the start of an allocated block. Testing shows 1000x
// more calls to createCFString than calls to the create functions with the appropriate
// allocator, so it's probably not urgent optimize that case.

}

#endif // USE(CF)
