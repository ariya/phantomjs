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

#ifndef WebViewportArguments_h
#define WebViewportArguments_h

#include "BlackBerryGlobal.h"
#include "BlackBerryPlatformMisc.h"

// Not for public API purpose.
namespace WebCore {
class ViewportArguments;
}

namespace BlackBerry {
namespace WebKit {

class WebPage;

/**
 * A class designed to expose a meta viewport fallback.
 *
 * This class simply wraps a WebCore::ViewportArguments. It can be
 * instantiated by the WebPageClient and supplied to a WebPage to
 * provide a userViewportArguments object that can be used whenever
 * there is no meta viewport tag provided in any loaded html.
 */
class BLACKBERRY_EXPORT WebViewportArguments {
public:
    WebViewportArguments();
    ~WebViewportArguments();

    // This matches the enum found in WebCore::ViewportArguments
    enum {
        ValueAuto = -1,
        ValueDeviceWidth = -2,
        ValueDeviceHeight = -3,
    };

    float zoom() const;
    void setZoom(float);

    float minZoom() const;
    void setMinZoom(float);

    float maxZoom() const;
    void setMaxZoom(float);

    float width() const;
    void setWidth(float);

    float height() const;
    void setHeight(float);

    float devicePixelRatio() const;
    void setDevicePixelRatio(float);

    float userZoom() const;
    void setUserZoom(float);

    bool operator==(const WebViewportArguments &other);
    bool operator!=(const WebViewportArguments &other);

private:
    WebCore::ViewportArguments* d;

private:
    friend class WebPage;
    DISABLE_COPY(WebViewportArguments)
};

} // namespace WebKit
} // namespace BlackBerry

#endif // WebViewportArguments_h
