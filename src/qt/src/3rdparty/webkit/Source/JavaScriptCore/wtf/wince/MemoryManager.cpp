/*
 * Copyright (C) 2008-2009 Torch Mobile Inc.
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
#include "MemoryManager.h"

#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strdup
#undef _strdup
#undef VirtualAlloc
#undef VirtualFree

#include <malloc.h>
#include <windows.h>

namespace WTF {

MemoryManager* memoryManager()
{
    static MemoryManager mm;
    return &mm;
}

MemoryManager::MemoryManager()
: m_allocationCanFail(false)
{
}

MemoryManager::~MemoryManager()
{
}

HBITMAP MemoryManager::createCompatibleBitmap(HDC hdc, int width, int height)
{
    return ::CreateCompatibleBitmap(hdc, width, height);
}

HBITMAP MemoryManager::createDIBSection(const BITMAPINFO* pbmi, void** ppvBits)
{
    return ::CreateDIBSection(0, pbmi, DIB_RGB_COLORS, ppvBits, 0, 0);
}

void* MemoryManager::m_malloc(size_t size)
{
    return malloc(size);
}

void* MemoryManager::m_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

void* MemoryManager::m_realloc(void* p, size_t size)
{
    return realloc(p, size);
}

void MemoryManager::m_free(void* p)
{
    return free(p);
}

bool MemoryManager::resizeMemory(void*, size_t)
{
    return false;
}

void* MemoryManager::allocate64kBlock()
{
    return VirtualAlloc(0, 65536, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void MemoryManager::free64kBlock(void* p)
{
    VirtualFree(p, 65536, MEM_RELEASE);
}

bool MemoryManager::onIdle(DWORD& timeLimitMs)
{
    return false;
}

LPVOID MemoryManager::virtualAlloc(LPVOID lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect)
{
    return ::VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}

BOOL MemoryManager::virtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType)
{
    return ::VirtualFree(lpAddress, dwSize, dwFreeType);
}


#if defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC

void *fastMalloc(size_t n) { return malloc(n); }
void *fastCalloc(size_t n_elements, size_t element_size) { return calloc(n_elements, element_size); }
void fastFree(void* p) { return free(p); }
void *fastRealloc(void* p, size_t n) { return realloc(p, n); }

#else

void *fastMalloc(size_t n) { return MemoryManager::m_malloc(n); }
void *fastCalloc(size_t n_elements, size_t element_size) { return MemoryManager::m_calloc(n_elements, element_size); }
void fastFree(void* p) { return MemoryManager::m_free(p); }
void *fastRealloc(void* p, size_t n) { return MemoryManager::m_realloc(p, n); }

#endif

#ifndef NDEBUG
void fastMallocForbid() {}
void fastMallocAllow() {}
#endif

void* fastZeroedMalloc(size_t n)
{
    void* p = fastMalloc(n);
    if (p)
        memset(p, 0, n);
    return p;
}

TryMallocReturnValue tryFastMalloc(size_t n)
{
    MemoryAllocationCanFail canFail;
    return fastMalloc(n);
}

TryMallocReturnValue tryFastZeroedMalloc(size_t n)
{
    MemoryAllocationCanFail canFail;
    return fastZeroedMalloc(n);
}

TryMallocReturnValue tryFastCalloc(size_t n_elements, size_t element_size)
{
    MemoryAllocationCanFail canFail;
    return fastCalloc(n_elements, element_size);
}

TryMallocReturnValue tryFastRealloc(void* p, size_t n)
{
    MemoryAllocationCanFail canFail;
    return fastRealloc(p, n);
}

char* fastStrDup(const char* str)
{
    return _strdup(str);
}

}