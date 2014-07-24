/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 */

#ifndef WebKitThreadViewportAccessor_h
#define WebKitThreadViewportAccessor_h

#include <BlackBerryPlatformViewportAccessor.h>

namespace BlackBerry {

namespace Platform {
class IntPoint;
class IntSize;
}

namespace WebKit {

class WebPagePrivate;

class WebKitThreadViewportAccessor : public Platform::ViewportAccessor {
public:
    WebKitThreadViewportAccessor(WebPagePrivate*);
    virtual ~WebKitThreadViewportAccessor() { }

    virtual BlackBerry::Platform::IntSize documentContentsSize() const;
    virtual BlackBerry::Platform::IntSize pixelContentsSize() const;

    virtual BlackBerry::Platform::IntPoint documentScrollPosition() const;
    virtual BlackBerry::Platform::IntPoint pixelScrollPosition() const;

    virtual BlackBerry::Platform::IntSize documentViewportSize() const;
    virtual BlackBerry::Platform::IntSize pixelViewportSize() const;

    virtual BlackBerry::Platform::IntPoint destinationSurfaceOffset() const;

    virtual double scale() const;

private:
    Platform::ViewportAccessor* m_originalAccessor;
    WebPagePrivate* m_webPagePrivate;
};

} // namespace WebKit
} // namespace BlackBerry

#endif
