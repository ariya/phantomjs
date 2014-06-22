/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 */

#include "config.h"

#include "BackingStoreVisualizationViewportAccessor.h"

#include "BackingStore_p.h"

#include <BlackBerryPlatformPrimitives.h>
#include <algorithm>

using BlackBerry::Platform::FloatPoint;
using BlackBerry::Platform::IntPoint;
using BlackBerry::Platform::IntSize;
using BlackBerry::Platform::ViewportAccessor;

namespace BlackBerry {
namespace WebKit {

BackingStoreVisualizationViewportAccessor::BackingStoreVisualizationViewportAccessor(ViewportAccessor* originalAccessor, BackingStorePrivate* backingStorePrivate)
    : m_originalAccessor(originalAccessor)
    , m_backingStorePrivate(backingStorePrivate)
{
}

IntSize BackingStoreVisualizationViewportAccessor::pixelContentsSize() const
{
    return roundToPixelFromDocumentContents(documentContentsRect()).size();
}

IntSize BackingStoreVisualizationViewportAccessor::documentContentsSize() const
{
    return m_originalAccessor->documentContentsSize();
}

IntPoint BackingStoreVisualizationViewportAccessor::pixelScrollPosition() const
{
    return roundToPixelFromDocumentContents(
        toDocumentContents(state()->backingStoreOffset(), state()->scale()));
}

IntPoint BackingStoreVisualizationViewportAccessor::documentScrollPosition() const
{
    return roundToDocumentContents(state()->backingStoreOffset(), state()->scale());
}

IntSize BackingStoreVisualizationViewportAccessor::pixelViewportSize() const
{
    return m_originalAccessor->pixelViewportSize();
}

IntSize BackingStoreVisualizationViewportAccessor::documentViewportSize() const
{
    return roundToDocumentFromPixelContents(pixelViewportRect()).size();
}

IntPoint BackingStoreVisualizationViewportAccessor::destinationSurfaceOffset() const
{
    return m_originalAccessor->destinationSurfaceOffset();
}

double BackingStoreVisualizationViewportAccessor::scale() const
{
    return state()->scale() / std::max(state()->numberOfTilesWide(), state()->numberOfTilesHigh());
}

BackingStoreGeometry* BackingStoreVisualizationViewportAccessor::state() const
{
    return m_backingStorePrivate->frontState();
}

} // namespace WebKit
} // namespace BlackBerry
