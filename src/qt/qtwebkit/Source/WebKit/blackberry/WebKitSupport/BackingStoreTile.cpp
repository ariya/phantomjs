/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "BackingStoreTile.h"

#include "GraphicsContext.h"
#include "SurfacePool.h"

#include <BlackBerryPlatformGraphics.h>

namespace BlackBerry {
namespace WebKit {

Fence::~Fence()
{
    if (m_platformSync)
        SurfacePool::globalSurfacePool()->destroyPlatformSync(m_platformSync);
}

TileBuffer::TileBuffer(const Platform::IntSize& size)
    : m_lastRenderOrigin(-1, -1)
    , m_size(size)
    , m_fence(Fence::create())
    , m_nativeBuffer(0)
    , m_lastRenderScale(1.0)
    , m_backgroundPainted(false)
{
}

TileBuffer::~TileBuffer()
{
    Platform::Graphics::destroyBuffer(m_nativeBuffer);
}

Platform::IntSize TileBuffer::size() const
{
    return m_size;
}

Platform::IntRect TileBuffer::surfaceRect() const
{
    return Platform::IntRect(Platform::IntPoint::zero(), m_size);
}

Platform::IntRect TileBuffer::pixelContentsRect() const
{
    return Platform::IntRect(m_lastRenderOrigin, m_size);
}

bool TileBuffer::isRendered(double scale) const
{
    return isRendered(pixelContentsRect(), scale);
}

bool TileBuffer::isRendered(const Platform::IntRectRegion& pixelContentsRegion, double scale) const
{
    return m_lastRenderScale == scale && Platform::IntRectRegion::subtractRegions(pixelContentsRegion, m_renderedRegion).isEmpty();
}

void TileBuffer::clearRenderedRegion(const Platform::IntRectRegion& region)
{
    m_renderedRegion = Platform::IntRectRegion::subtractRegions(m_renderedRegion, region);
}

void TileBuffer::clearRenderedRegion()
{
    m_renderedRegion = Platform::IntRectRegion();
}

void TileBuffer::addRenderedRegion(const Platform::IntRectRegion& region)
{
    m_renderedRegion = Platform::IntRectRegion::unionRegions(region, m_renderedRegion);
}

Platform::IntRectRegion TileBuffer::renderedRegion() const
{
    return m_renderedRegion;
}

Platform::IntRectRegion TileBuffer::notRenderedRegion() const
{
    return Platform::IntRectRegion::subtractRegions(pixelContentsRect(), m_renderedRegion);
}

Platform::Graphics::Buffer* TileBuffer::nativeBuffer() const
{
    if (!m_nativeBuffer)
        m_nativeBuffer = Platform::Graphics::createBuffer(m_size, Platform::Graphics::BackedWhenNecessary);

    return m_nativeBuffer;
}

bool TileBuffer::wasNativeBufferCreated() const
{
    return static_cast<bool>(m_nativeBuffer);
}

void TileBuffer::paintBackground()
{
    m_backgroundPainted = true;

    Platform::Graphics::clearBuffer(nativeBuffer(), 0, 0, 0, 0);
}

}
}
