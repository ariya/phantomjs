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

#ifndef WebTapHighlight_h
#define WebTapHighlight_h

#include "BlackBerryGlobal.h"

#include <BlackBerryPlatformIntRectRegion.h>

namespace BlackBerry {
namespace WebKit {

class BLACKBERRY_EXPORT WebTapHighlight {
public:
    virtual ~WebTapHighlight() { }

    virtual void draw(const Platform::IntRectRegion&, int red, int green, int blue, int alpha, bool hideAfterScroll, bool isStartOfSelection = false) = 0;
    virtual void hide() = 0;

    virtual bool isVisible() const = 0;
    virtual bool shouldHideAfterScroll() const = 0;
};

} // namespace WebKit
} // namespace BlackBerry

#endif // WebTapHighlight_h
