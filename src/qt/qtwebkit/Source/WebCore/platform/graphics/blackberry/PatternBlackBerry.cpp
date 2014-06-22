/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "Pattern.h"

#include "AffineTransform.h"
#include "Image.h"
#include "NativeImagePtr.h"
#include "Path.h"

#include <BlackBerryPlatformGraphicsContext.h>

using namespace std;

namespace WebCore {

void Pattern::platformDestroy()
{
    delete m_pattern;
    m_pattern = 0;
}

PlatformPatternPtr Pattern::platformPattern(const AffineTransform&)
{
    if (m_pattern)
        return m_pattern;

    const NativeImagePtr image = m_tileImage->nativeImageForCurrentFrame();

    m_pattern = BlackBerry::Platform::Graphics::Pattern::create();

    m_pattern->setImage(image, m_repeatX, m_repeatY);
    m_pattern->setLocalMatrix(reinterpret_cast<const double*>(&m_patternSpaceTransformation));

    return m_pattern;
}

void Pattern::setPlatformPatternSpaceTransform()
{
    if (m_pattern)
        m_pattern->setLocalMatrix(reinterpret_cast<const double*>(&m_patternSpaceTransformation));
}

} // namespace WebCore
