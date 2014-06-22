/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
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
#include "BackingStoreClient.h"

#include "BackingStore.h"
#include "BackingStore_p.h"
#include "FloatPoint.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "Page.h"
#include "RenderBox.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformViewportAccessor.h>

// FIXME: Leaving the below lines commented out as a reference for us to soon be sure if we need these
// methods and class variables be moved from WebPage to BackingStoreClient.
// Notification methods that deliver changes to the real geometry of the device as specified above.
// void notifyTransformChanged();
// void notifyTransformedContentsSizeChanged();
// void notifyTransformedScrollChanged();
// m_overflowExceedsContentsSize = true;
// haspendingscrollevent

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

static inline IntSize pointToSize(const IntPoint& point)
{
    return IntSize(point.x(), point.y());
}

BackingStoreClient* BackingStoreClient::create(Frame* frame, Frame* parentFrame, WebPage* parentPage)
{
    ASSERT_UNUSED(parentFrame, !parentFrame);
    BackingStoreClient* it = new BackingStoreClient(frame, parentPage);
    return it;
}

BackingStoreClient::BackingStoreClient(Frame* frame, WebPage* parentPage)
    : m_frame(frame)
    , m_webPage(parentPage)
    , m_backingStore(0)
    , m_isClientGeneratedScroll(false)
    , m_isScrollNotificationSuppressed(false)
{
    m_backingStore = new BackingStore(m_webPage, this);
}

BackingStoreClient::~BackingStoreClient()
{
    delete m_backingStore;
    m_backingStore = 0;
    m_frame = 0;
}

IntPoint BackingStoreClient::scrollPosition() const
{
    ASSERT(m_frame);
    if (!m_frame->view())
        return IntPoint();

    return m_frame->view()->scrollPosition() - pointToSize(m_frame->view()->minimumScrollPosition());
}

IntPoint BackingStoreClient::transformedScrollPosition() const
{
    return m_webPage->webkitThreadViewportAccessor()->pixelScrollPosition();
}

void BackingStoreClient::setScrollPosition(const IntPoint& pos)
{
    ASSERT(m_frame);
    if (!m_frame->view())
        return;

    if (pos == scrollPosition())
        return;

    // We set a flag here to note that this scroll operation was originated
    // within the BlackBerry-specific layer of WebKit and not by WebCore.
    // This flag is checked in checkOriginOfCurrentScrollOperation() to decide
    // whether to notify the client of the current scroll operation. This is
    // why it is important that all scroll operations that originate within
    // BlackBerry-specific code are encapsulated here and that callers of this
    // method also directly or indirectly call notifyTransformedScrollChanged().
    m_isScrollNotificationSuppressed = true;
    m_frame->view()->setScrollPosition(pos + pointToSize(m_frame->view()->minimumScrollPosition()));
    m_isScrollNotificationSuppressed = false;
}

IntPoint BackingStoreClient::maximumScrollPosition() const
{
    ASSERT(m_frame);
    if (!m_frame->view())
        return IntPoint();

    return m_frame->view()->maximumScrollPosition() - pointToSize(m_frame->view()->minimumScrollPosition());
}

IntPoint BackingStoreClient::transformedMaximumScrollPosition() const
{
    return m_webPage->webkitThreadViewportAccessor()->roundToPixelFromDocumentContents(WebCore::FloatPoint(maximumScrollPosition()));
}

IntSize BackingStoreClient::actualVisibleSize() const
{
    return m_webPage->webkitThreadViewportAccessor()->documentViewportSize();
}

IntSize BackingStoreClient::transformedActualVisibleSize() const
{
    ASSERT(isMainFrame());
    return m_webPage->webkitThreadViewportAccessor()->pixelViewportSize();
}

IntSize BackingStoreClient::viewportSize() const
{
    ASSERT(m_frame);
    if (!m_frame->view())
        return IntSize();

    ASSERT(isMainFrame());
    return m_webPage->d->viewportSize();
}

IntSize BackingStoreClient::transformedViewportSize() const
{
    ASSERT(m_frame);
    if (!m_frame->view())
        return IntSize();

    ASSERT(isMainFrame());
    return m_webPage->d->transformedViewportSize();
}

IntRect BackingStoreClient::visibleContentsRect() const
{
    ASSERT(m_frame);
    if (!m_frame->view())
        return IntRect();

    IntRect visibleContentRect = m_frame->view()->visibleContentRect();
    ASSERT(isMainFrame());
    return visibleContentRect;
}

IntSize BackingStoreClient::contentsSize() const
{
    ASSERT(m_frame);
    if (!m_frame->view())
        return IntSize();

    return m_frame->view()->contentsSize();
}

WebPagePrivate::LoadState BackingStoreClient::loadState() const
{
    // FIXME: Does it need to call WebPage's?
    return m_webPage->d->loadState();
}

bool BackingStoreClient::isLoading() const
{
    // FIXME: Does it need to call WebPage's?
    return m_webPage->d->isLoading();
}

bool BackingStoreClient::isFocused() const
{
    return m_frame && m_frame->page() && m_frame->page()->focusController()
        && m_frame->page()->focusController()->focusedFrame() == m_frame;
}

bool BackingStoreClient::isClientGeneratedScroll() const
{
    return m_isClientGeneratedScroll;
}

void BackingStoreClient::setIsClientGeneratedScroll(bool flag)
{
    m_isClientGeneratedScroll = flag;
}

bool BackingStoreClient::isScrollNotificationSuppressed() const
{
    return m_isScrollNotificationSuppressed;
}

void BackingStoreClient::setIsScrollNotificationSuppressed(bool flag)
{
    m_isScrollNotificationSuppressed = flag;
}

void BackingStoreClient::checkOriginOfCurrentScrollOperation()
{
    // This is called via ChromeClientBlackBerry::scroll in order to check the origin
    // of the current scroll operation to decide whether to notify the client.
    // If the current scroll operation was initiated internally by WebCore itself
    // either via JavaScript, back/forward or otherwise then we need to go ahead
    // and notify the client of this change.
    if (isScrollNotificationSuppressed())
        return;

    ASSERT(isMainFrame());
    m_webPage->d->notifyTransformedScrollChanged();
}

}
}
