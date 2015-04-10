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

#ifndef UpdateAtlas_h
#define UpdateAtlas_h

#include "AreaAllocator.h"
#include "CoordinatedSurface.h"
#include "IntSize.h"

#if USE(COORDINATED_GRAPHICS)

namespace WebCore {
class GraphicsContext;
class IntPoint;

class UpdateAtlas {
    WTF_MAKE_NONCOPYABLE(UpdateAtlas);
public:
    class Client {
    public:
        virtual void createUpdateAtlas(uint32_t /* id */, PassRefPtr<CoordinatedSurface>) = 0;
        virtual void removeUpdateAtlas(uint32_t /* id */) = 0;
    };

    UpdateAtlas(Client*, int dimension, CoordinatedSurface::Flags);
    ~UpdateAtlas();

    inline IntSize size() const { return m_surface->size(); }

    // Returns false if there is no available buffer.
    bool paintOnAvailableBuffer(const IntSize&, uint32_t& atlasID, IntPoint& offset, CoordinatedSurface::Client*);
    void didSwapBuffers();
    bool supportsAlpha() const { return m_surface->supportsAlpha(); }

    void addTimeInactive(double seconds)
    {
        ASSERT(!isInUse());
        m_inactivityInSeconds += seconds;
    }
    bool isInactive() const
    {
        const double inactiveSecondsTolerance = 3;
        return m_inactivityInSeconds > inactiveSecondsTolerance;
    }
    bool isInUse() const { return m_areaAllocator; }

private:
    void buildLayoutIfNeeded();

private:
    Client* m_client;
    OwnPtr<GeneralAreaAllocator> m_areaAllocator;
    RefPtr<CoordinatedSurface> m_surface;
    double m_inactivityInSeconds;
    uint32_t m_ID;
};

} // namespace WebCore
#endif // USE(COORDINATED_GRAPHICS)
#endif // UpdateAtlas_h
