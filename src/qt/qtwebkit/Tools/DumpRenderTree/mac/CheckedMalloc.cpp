/*
 * Copyright (C) 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
 *           (C) 2007 Eric Seidel <eric@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h" 
#import "CheckedMalloc.h"

#import <mach/mach_init.h>
#import <mach/mach_vm.h>
#import <mach/vm_region.h>
#import <malloc/malloc.h>
#import <unistd.h>

static void* (*savedMalloc)(malloc_zone_t*, size_t);
static void* (*savedRealloc)(malloc_zone_t*, void*, size_t);

static void* checkedMalloc(malloc_zone_t* zone, size_t size)
{
    if (size >= 0x10000000)
        return 0;
    return savedMalloc(zone, size);
}

static void* checkedRealloc(malloc_zone_t* zone, void* ptr, size_t size)
{
    if (size >= 0x10000000)
        return 0;
    return savedRealloc(zone, ptr, size);
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
static vm_prot_t protectionOfRegion(mach_vm_address_t address)
{
    mach_vm_size_t regionSize = 0;
    vm_region_basic_info_64 regionInfo;
    mach_msg_type_number_t regionInfoCount = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t objectName;
    if (mach_vm_region(mach_task_self(), &address, &regionSize, VM_REGION_BASIC_INFO_64, (vm_region_info_t)&regionInfo, &regionInfoCount, &objectName))
        CRASH();
    return regionInfo.protection;
}
#endif

void makeLargeMallocFailSilently()
{
    malloc_zone_t* zone = malloc_default_zone();

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    mach_vm_address_t pageStart = reinterpret_cast<vm_address_t>(zone) & static_cast<vm_size_t>(~(getpagesize() - 1));
    vm_prot_t initialProtection = protectionOfRegion(pageStart);

    vm_size_t len = reinterpret_cast<vm_address_t>(zone) - pageStart + sizeof(malloc_zone_t);
    if (mach_vm_protect(mach_task_self(), pageStart, len, 0, initialProtection | VM_PROT_WRITE))
        CRASH();
#endif

    savedMalloc = zone->malloc;
    savedRealloc = zone->realloc;
    zone->malloc = checkedMalloc;
    zone->realloc = checkedRealloc;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    if (mach_vm_protect(mach_task_self(), pageStart, len, 0, initialProtection))
        CRASH();
#endif
}
