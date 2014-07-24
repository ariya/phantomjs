/*
 Copyright (C) 2012 Digia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#ifndef GraphicsSurfaceToken_h
#define GraphicsSurfaceToken_h

#include "GraphicsContext.h"
#include "IntRect.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if USE(GRAPHICS_SURFACE)

namespace WebCore {

struct GraphicsSurfaceToken {

#if OS(DARWIN)
    typedef mach_port_t BufferHandle;
#elif OS(LINUX)
    typedef uint32_t BufferHandle;
#elif OS(WINDOWS)
    typedef HANDLE BufferHandle;
#endif

#if HAVE(GLX)
    GraphicsSurfaceToken(uint32_t windowID = 0)
        : frontBufferHandle(windowID)
    { }

    bool operator!=(const GraphicsSurfaceToken &rhs) const
    {
        return frontBufferHandle != rhs.frontBufferHandle;
    }

    bool isValid() const
    {
        return frontBufferHandle;
    }

#endif

#if OS(DARWIN) || OS(WINDOWS)
    GraphicsSurfaceToken(BufferHandle frontBuffer = 0, BufferHandle backBuffer = 0)
        : frontBufferHandle(frontBuffer)
        , backBufferHandle(backBuffer)
    { }

    bool operator!=(const GraphicsSurfaceToken &rhs) const
    {
        return (frontBufferHandle != rhs.frontBufferHandle || backBufferHandle != rhs.backBufferHandle);
    }

    bool isValid() const
    {
        return frontBufferHandle && backBufferHandle;
    }

    BufferHandle backBufferHandle;
#endif

    BufferHandle frontBufferHandle;
};

}
#endif // USE(GRAPHICS_SURFACE)

#endif // GraphicsSurfaceToken_h
