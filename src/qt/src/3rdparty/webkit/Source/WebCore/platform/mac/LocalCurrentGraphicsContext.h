/*
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
 */

#include <wtf/Noncopyable.h>

#ifdef __OBJC__
@class NSGraphicsContext;
#else
class NSGraphicsContext;
#endif

namespace WebCore {

class GraphicsContext;
    
// This class automatically saves and restores the current NSGraphicsContext for
// functions which call out into AppKit and rely on the currentContext being set
class LocalCurrentGraphicsContext {
    WTF_MAKE_NONCOPYABLE(LocalCurrentGraphicsContext);
public:
    LocalCurrentGraphicsContext(GraphicsContext* graphicsContext);
    ~LocalCurrentGraphicsContext();

private:
    GraphicsContext* m_savedGraphicsContext;
    NSGraphicsContext* m_savedNSGraphicsContext;
};

}
