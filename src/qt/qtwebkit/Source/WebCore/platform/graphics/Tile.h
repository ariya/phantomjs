/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 
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

#ifndef Tile_h
#define Tile_h

#if USE(TILED_BACKING_STORE)

#include "IntPoint.h"
#include "IntPointHash.h"
#include "IntRect.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class GraphicsContext;

class Tile : public RefCounted<Tile> {
public:
    typedef IntPoint Coordinate;

    virtual ~Tile() { }

    virtual bool isDirty() const = 0;
    virtual void invalidate(const IntRect&) = 0;
    virtual Vector<IntRect> updateBackBuffer() = 0;
    virtual void swapBackBufferToFront() = 0;
    virtual bool isReadyToPaint() const = 0;
    virtual void paint(GraphicsContext*, const IntRect&) = 0;

    virtual const Tile::Coordinate& coordinate() const = 0;
    virtual const IntRect& rect() const = 0;
    virtual void resize(const WebCore::IntSize&) = 0;
};

}
#endif
#endif
