/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2012, 2013 Company 100, Inc.
    Copyright (C) 2012, 2013 basysKom GmbH

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

#ifndef TextureMapperFPSCounter_h
#define TextureMapperFPSCounter_h

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#include "FloatPoint.h"
#include "TransformationMatrix.h"
#include <wtf/Noncopyable.h>

namespace WebCore {
class TextureMapper;

class TextureMapperFPSCounter {
    WTF_MAKE_NONCOPYABLE(TextureMapperFPSCounter);
    WTF_MAKE_FAST_ALLOCATED;
public:
    TextureMapperFPSCounter();
    void updateFPSAndDisplay(TextureMapper*, const FloatPoint& = FloatPoint::zero(), const TransformationMatrix& = TransformationMatrix());

private:
    bool m_isShowingFPS;
    double m_fpsInterval;
    double m_fpsTimestamp;
    int m_lastFPS;
    int m_frameCount;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // TextureMapperFPSCounter_h


