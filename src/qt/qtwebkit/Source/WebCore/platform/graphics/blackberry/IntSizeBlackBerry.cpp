/*
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
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
#include "IntSize.h"

#include <BlackBerryPlatformPrimitives.h>

namespace WebCore {

IntSize::IntSize(const BlackBerry::Platform::IntSize& size)
    : m_width(size.width())
    , m_height(size.height())
{
}

IntSize::operator BlackBerry::Platform::IntSize() const
{
    return BlackBerry::Platform::IntSize(m_width, m_height);
}

} // namespace WebCore
