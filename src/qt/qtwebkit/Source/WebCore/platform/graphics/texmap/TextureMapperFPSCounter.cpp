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

#include "config.h"

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

#include "TextureMapperFPSCounter.h"

#include "TextureMapper.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

TextureMapperFPSCounter::TextureMapperFPSCounter()
    : m_isShowingFPS(false)
    , m_fpsInterval(0)
    , m_fpsTimestamp(0)
    , m_lastFPS(0)
    , m_frameCount(0)
{
    String showFPSEnvironment = getenv("WEBKIT_SHOW_FPS");
    bool ok = false;
    m_fpsInterval = showFPSEnvironment.toDouble(&ok);
    if (ok && m_fpsInterval) {
        m_isShowingFPS = true;
        m_fpsTimestamp = WTF::currentTime();
    }
}

void TextureMapperFPSCounter::updateFPSAndDisplay(TextureMapper* textureMapper, const FloatPoint& location, const TransformationMatrix& matrix)
{
    if (!m_isShowingFPS)
        return;

    m_frameCount++;
    double delta = WTF::currentTime() - m_fpsTimestamp;
    if (delta >= m_fpsInterval) {
        m_lastFPS = int(m_frameCount / delta);
        m_frameCount = 0;
        m_fpsTimestamp += delta;
    }

    textureMapper->drawNumber(m_lastFPS, Color::black, location, matrix);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
