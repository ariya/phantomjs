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

#ifndef TiledBackingStoreClient_h
#define TiledBackingStoreClient_h

namespace WebCore {

#if ENABLE(TILED_BACKING_STORE)
class TiledBackingStoreClient {
public:
    virtual void tiledBackingStorePaintBegin() = 0;
    virtual void tiledBackingStorePaint(GraphicsContext*, const IntRect&) = 0;
    virtual void tiledBackingStorePaintEnd(const Vector<IntRect>& paintedArea) = 0;
    virtual IntRect tiledBackingStoreContentsRect() = 0;
    virtual IntRect tiledBackingStoreVisibleRect() = 0;
    virtual Color tiledBackingStoreBackgroundColor() const = 0;
};

#else
class TiledBackingStoreClient {};
#endif

}

#endif
