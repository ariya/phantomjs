/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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

#ifndef PointLightSource_h
#define PointLightSource_h

#if ENABLE(FILTERS)
#include "LightSource.h"

namespace WebCore {

class PointLightSource : public LightSource {
public:
    static PassRefPtr<PointLightSource> create(const FloatPoint3D& position)
    {
        return adoptRef(new PointLightSource(position));
    }

    const FloatPoint3D& position() const { return m_position; }
    virtual bool setX(float) OVERRIDE;
    virtual bool setY(float) OVERRIDE;
    virtual bool setZ(float) OVERRIDE;

    virtual void initPaintingData(PaintingData&);
    virtual void updatePaintingData(PaintingData&, int x, int y, float z);

    virtual TextStream& externalRepresentation(TextStream&) const;

private:
    PointLightSource(const FloatPoint3D& position)
        : LightSource(LS_POINT)
        , m_position(position)
    {
    }

    FloatPoint3D m_position;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // PointLightSource_h
