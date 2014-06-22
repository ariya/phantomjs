/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 */

#ifndef BackingStoreVisualizationViewportAccessor_h
#define BackingStoreVisualizationViewportAccessor_h

#include <BlackBerryPlatformViewportAccessor.h>

namespace BlackBerry {

namespace Platform {
class IntPoint;
class IntSize;
}

namespace WebKit {

class BackingStorePrivate;
class BackingStoreGeometry;

class BackingStoreVisualizationViewportAccessor : public Platform::ViewportAccessor {
public:
    BackingStoreVisualizationViewportAccessor(ViewportAccessor* originalAccessor, BackingStorePrivate*);
    virtual ~BackingStoreVisualizationViewportAccessor() { }

    virtual BlackBerry::Platform::IntSize documentContentsSize() const;
    virtual BlackBerry::Platform::IntSize pixelContentsSize() const;

    virtual BlackBerry::Platform::IntPoint documentScrollPosition() const;
    virtual BlackBerry::Platform::IntPoint pixelScrollPosition() const;

    virtual BlackBerry::Platform::IntSize documentViewportSize() const;
    virtual BlackBerry::Platform::IntSize pixelViewportSize() const;

    virtual BlackBerry::Platform::IntPoint destinationSurfaceOffset() const;

    virtual double scale() const;

private:
    BackingStoreGeometry* state() const;

    Platform::ViewportAccessor* m_originalAccessor;
    BackingStorePrivate* m_backingStorePrivate;
};

} // namespace WebKit
} // namespace BlackBerry

#endif
