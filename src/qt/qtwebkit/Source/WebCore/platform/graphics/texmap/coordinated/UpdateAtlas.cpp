/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Company 100, Inc.

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
#include "UpdateAtlas.h"

#if USE(COORDINATED_GRAPHICS)

#include "CoordinatedGraphicsState.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include <wtf/MathExtras.h>

namespace WebCore {

class UpdateAtlasSurfaceClient : public CoordinatedSurface::Client {
public:
    UpdateAtlasSurfaceClient(CoordinatedSurface::Client* client, const IntSize& size, bool supportsAlpha)
        : m_client(client)
        , m_size(size)
        , m_supportsAlpha(supportsAlpha)
    {
    }

    virtual void paintToSurfaceContext(GraphicsContext* context) OVERRIDE
    {
        if (m_supportsAlpha) {
            context->setCompositeOperation(CompositeCopy);
            context->fillRect(IntRect(IntPoint::zero(), m_size), Color::transparent, ColorSpaceDeviceRGB);
            context->setCompositeOperation(CompositeSourceOver);
        }

        m_client->paintToSurfaceContext(context);
    }

private:
    CoordinatedSurface::Client* m_client;
    IntSize m_size;
    bool m_supportsAlpha;
};

UpdateAtlas::UpdateAtlas(Client* client, int dimension, CoordinatedSurface::Flags flags)
    : m_client(client)
    , m_inactivityInSeconds(0)
{
    static uint32_t nextID = 0;
    m_ID = ++nextID;
    IntSize size = nextPowerOfTwo(IntSize(dimension, dimension));
    m_surface = CoordinatedSurface::create(size, flags);

    m_client->createUpdateAtlas(m_ID, m_surface);
}

UpdateAtlas::~UpdateAtlas()
{
    if (m_surface)
        m_client->removeUpdateAtlas(m_ID);
}

void UpdateAtlas::buildLayoutIfNeeded()
{
    if (!m_areaAllocator) {
        m_areaAllocator = adoptPtr(new GeneralAreaAllocator(size()));
        m_areaAllocator->setMinimumAllocation(IntSize(32, 32));
    }
}

void UpdateAtlas::didSwapBuffers()
{
    m_areaAllocator.clear();
}


bool UpdateAtlas::paintOnAvailableBuffer(const IntSize& size, uint32_t& atlasID, IntPoint& offset, CoordinatedSurface::Client* client)
{
    m_inactivityInSeconds = 0;
    buildLayoutIfNeeded();
    IntRect rect = m_areaAllocator->allocate(size);

    // No available buffer was found.
    if (rect.isEmpty())
        return false;

    if (!m_surface)
        return false;

    atlasID = m_ID;

    // FIXME: Use tri-state buffers, to allow faster updates.
    offset = rect.location();

    UpdateAtlasSurfaceClient surfaceClient(client, size, supportsAlpha());
    m_surface->paintToSurface(rect, &surfaceClient);

    return true;
}

} // namespace WebCore
#endif // USE(COORDINATED_GRAPHICS)
