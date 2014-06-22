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

#ifndef WebAnimation_h
#define WebAnimation_h

#include "BlackBerryGlobal.h"

#include <BlackBerryPlatformPrimitives.h>

namespace BlackBerry {
namespace WebKit {

class WebAnimationPrivate;

/**
 * Represents an animation running on an overlay.
 *
 * WebAnimation is not thread safe, but it is reentrant. This means that
 * instances can be created on different threads, but must be used on the
 * thread where they were created.
 */
class BLACKBERRY_EXPORT WebAnimation {
public:
    static WebAnimation fadeAnimation(const BlackBerry::Platform::String& name, float from, float to, double duration);
    static WebAnimation shrinkAnimation(const BlackBerry::Platform::String& name, float from, float to, double duration);

    WebAnimation();
    WebAnimation(const WebAnimation&);
    ~WebAnimation();

    WebAnimation& operator=(const WebAnimation&);

    BlackBerry::Platform::String name() const;

protected:
    friend class WebOverlay;
    friend class WebOverlayOverride;
    friend class WebOverlayPrivate;

    WebAnimationPrivate* d;
};

}
}

#endif // WebAnimation_h
