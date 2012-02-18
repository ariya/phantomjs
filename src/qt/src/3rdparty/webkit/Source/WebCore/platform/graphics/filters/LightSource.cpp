/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2010 Zoltan Herczeg <zherczeg@webkit.org>
 * Copyright (C) 2011 Renata Hodovan <reni@webkit.org>, University of Szeged.
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

#include "config.h"

#if ENABLE(FILTERS)
#include "LightSource.h"

#include "DistantLightSource.h"
#include "PointLightSource.h"
#include "RenderTreeAsText.h"
#include "SpotLightSource.h"
#include <wtf/MathExtras.h>

namespace WebCore {

bool LightSource::setAzimuth(float azimuth)
{
    if (m_type == LS_DISTANT)
        return static_cast<DistantLightSource*>(this)->setAzimuth(azimuth);
    return false;
}

bool LightSource::setElevation(float elevation)
{
    if (m_type == LS_DISTANT)
        return static_cast<DistantLightSource*>(this)->setElevation(elevation);
    return false;
}

bool LightSource::setX(float x)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setX(x);
    if (m_type == LS_POINT)
        return static_cast<PointLightSource*>(this)->setX(x);
    return false;
}

bool LightSource::setY(float y)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setY(y);
    if (m_type == LS_POINT)
        return static_cast<PointLightSource*>(this)->setY(y);
    return false;
}

bool LightSource::setZ(float z)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setZ(z);
    if (m_type == LS_POINT)
        return static_cast<PointLightSource*>(this)->setZ(z);
    return false;
}

bool LightSource::setPointsAtX(float pointsAtX)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setPointsAtX(pointsAtX);
    return false;
}

bool LightSource::setPointsAtY(float pointsAtY)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setPointsAtY(pointsAtY);
    return false;
}

bool LightSource::setPointsAtZ(float pointsAtZ)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setPointsAtZ(pointsAtZ);
    return false;
}

bool LightSource::setSpecularExponent(float specularExponent)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setSpecularExponent(specularExponent);
    return false;
}

bool LightSource::setLimitingConeAngle(float limitingConeAngle)
{
    if (m_type == LS_SPOT)
        return static_cast<SpotLightSource*>(this)->setLimitingConeAngle(limitingConeAngle);
    return false;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
