/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef SurfaceUpdateInfo_h
#define SurfaceUpdateInfo_h

#if USE(COORDINATED_GRAPHICS)

#include "IntRect.h"

namespace WebCore {

class SurfaceUpdateInfo {

public:
    SurfaceUpdateInfo() { }

    // The rect to be updated.
    IntRect updateRect;

    // The page scale factor used to render this update.
    float scaleFactor;

    // The id of the update atlas including the shareable bitmap containing the updates.
    uint32_t atlasID;

    // The offset in the bitmap where the rendered contents are.
    IntPoint surfaceOffset;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

#endif // SurfaceUpdateInfo_h
