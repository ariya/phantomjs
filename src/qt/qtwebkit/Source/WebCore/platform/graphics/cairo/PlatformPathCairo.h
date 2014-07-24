/*
 *  Copyright (C) 2007 Alp Toker <alp.toker@collabora.co.uk>
 *  Copyright (C) 2010 Igalia S.L.
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

#ifndef PlatformPathCairo_h
#define PlatformPathCairo_h

#include "RefPtrCairo.h"

namespace WebCore {

// This is necessary since cairo_path_fixed_t isn't exposed in Cairo's public API.
class CairoPath {
public:
    CairoPath();

    ~CairoPath() {}

    cairo_t* context() { return m_cr.get(); }

private:
    RefPtr<cairo_t> m_cr;
};

} // namespace WebCore

#endif // PlatformPathCairo_h
