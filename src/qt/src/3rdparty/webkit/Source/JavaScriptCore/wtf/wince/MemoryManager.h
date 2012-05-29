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

#pragma once

#include <winbase.h>

typedef struct HBITMAP__* HBITMAP;
typedef struct HDC__* HDC;
typedef void *HANDLE;
typedef struct tagBITMAPINFO BITMAPINFO;

namespace WTF {

    class MemoryManager {
    public:
        MemoryManager();
        ~MemoryManager();

        bool allocationCanFail() const { return m_allocationCanFail; }
        void setAllocationCanFail(bool c) { m_allocationCanFail = c; }

        static HBITMAP createCompatibleBitmap(HDC hdc, int width, int height);
        static HBITMAP createDIBSection(const BITMAPINFO* pbmi, void** ppvBits);
        static void* m_malloc(size_t size);
        static void* m_calloc(size_t num, size_t size);
        static void* m_realloc(void* p, size_t size);
        static void m_free(void*);
        static bool resizeMemory(void* p, size_t newSize);
        static void* allocate64kBlock();
        static void free64kBlock(void*);
        static bool onIdle(DWORD& timeLimitMs);
        static LPVOID virtualAlloc(LPVOID lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect);
        static BOOL virtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType);

    private:
        friend MemoryManager* memoryManager();

        bool m_allocationCanFail;
    };

    MemoryManager* memoryManager();

    class MemoryAllocationCanFail {
    public:
        MemoryAllocationCanFail() : m_old(memoryManager()->allocationCanFail()) { memoryManager()->setAllocationCanFail(true); }
        ~MemoryAllocationCanFail() { memoryManager()->setAllocationCanFail(m_old); }
    private:
        bool m_old;
    };

    class MemoryAllocationCannotFail {
    public:
        MemoryAllocationCannotFail() : m_old(memoryManager()->allocationCanFail()) { memoryManager()->setAllocationCanFail(false); }
        ~MemoryAllocationCannotFail() { memoryManager()->setAllocationCanFail(m_old); }
    private:
        bool m_old;
    };
}

using WTF::MemoryManager;
using WTF::memoryManager;
using WTF::MemoryAllocationCanFail;
using WTF::MemoryAllocationCannotFail;
