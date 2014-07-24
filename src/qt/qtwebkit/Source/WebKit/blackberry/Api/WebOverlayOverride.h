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

#ifndef WebOverlayOverride_h
#define WebOverlayOverride_h

#include "BlackBerryGlobal.h"

#include <BlackBerryPlatformPrimitives.h>

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

class WebAnimation;
class WebOverlayPrivate;

/**
 * Compositing thread only
 *
 * Note that the WebAnimation class is not thread safe, but
 * reentrant, and have to be created on the thread where they'll be used.
 */
class BLACKBERRY_EXPORT WebOverlayOverride {
public:
    // Don't use this, call WebOverlay::override() instead
    WebOverlayOverride(WebOverlayPrivate*);
    ~WebOverlayOverride();

    void setPosition(const Platform::FloatPoint&);
    void setAnchorPoint(const Platform::FloatPoint&);
    void setSize(const Platform::FloatSize&);
    void setTransform(const Platform::TransformationMatrix&);
    void setOpacity(float);

    void addAnimation(const WebAnimation&);
    void removeAnimation(const BlackBerry::Platform::String& name);

private:
    friend class WebOverlayPrivate;

    // Disable copy constructor and operator=
    WebOverlayOverride(const WebOverlayOverride&);
    WebOverlayOverride& operator=(const WebOverlayOverride&);

    WebOverlayPrivate* d;
    bool m_owned;
};

}
}

#endif // WebOverlayOverride_h
