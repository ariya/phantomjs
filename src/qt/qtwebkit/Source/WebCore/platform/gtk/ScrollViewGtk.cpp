/*
 * Copyright (C) 2006, 2007, 2008 Apple Computer, Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008, 2010 Collabora Ltd.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollView.h"

#include "HostWindow.h"
#include "MainFrameScrollbarGtk.h"
#include "ScrollbarTheme.h"
#include <gtk/gtk.h>

using namespace std;

namespace WebCore {

static bool shouldCreateMainFrameScrollbar(const ScrollView* scrollView)
{
    // Interior frame ScrollViews never have MainFrameScrollbars.
    if (scrollView->parent())
        return false;

    // If we don't have a host window or a containing widget (ala WebKit2).
    HostWindow* hostWindow = scrollView->hostWindow();
    if (!hostWindow || !hostWindow->platformPageClient())
        return false;

    gboolean selfScrolling = FALSE;
    g_object_get(hostWindow->platformPageClient(), "self-scrolling", &selfScrolling, NULL);
    return !selfScrolling;
}

PassRefPtr<Scrollbar> ScrollView::createScrollbar(ScrollbarOrientation orientation)
{
    if (shouldCreateMainFrameScrollbar(this))
        return MainFrameScrollbarGtk::create(this, orientation);
    return Scrollbar::createNativeScrollbar(this, orientation, RegularScrollbar);
}

IntRect ScrollView::visibleContentRect(VisibleContentRectIncludesScrollbars scrollbarInclusion) const
{
    bool includeScrollbars = scrollbarInclusion == IncludeScrollbars;

    // If we are an interior frame scrollbar or are in some sort of transition
    // state, just calculate our size based on what the GTK+ theme says the
    // scrollbar width should be.
    if (!shouldCreateMainFrameScrollbar(this)) {
        return IntRect(IntPoint(m_scrollOffset.width(), m_scrollOffset.height()),
                       IntSize(max(0, width() - (verticalScrollbar() && !includeScrollbars ? verticalScrollbar()->width() : 0)),
                               max(0, height() - (horizontalScrollbar() && !includeScrollbars ? horizontalScrollbar()->height() : 0))));
    }

    // We don't have a parent, so we are the main frame and thus have
    // a parent widget which we can use to measure the visible region.
    GtkWidget* measuredWidget = hostWindow()->platformPageClient();
    GtkWidget* parentWidget = gtk_widget_get_parent(measuredWidget);

    // We may not be in a widget that displays scrollbars, but we may
    // have other kinds of decoration that make us smaller.
    if (parentWidget && includeScrollbars)
        measuredWidget = parentWidget;

    GtkAllocation allocation;
    gtk_widget_get_allocation(measuredWidget, &allocation);
    return IntRect(IntPoint(m_scrollOffset.width(), m_scrollOffset.height()),
                   IntSize(allocation.width, allocation.height));
}

void ScrollView::setScrollbarModes(ScrollbarMode horizontalMode, ScrollbarMode verticalMode, bool horizontalLock, bool verticalLock)
{
    // FIXME: Restructure the ScrollView abstraction so that we do not have to
    // copy this verbatim from ScrollView.cpp. Until then, we should make sure this
    // is kept in sync.
    bool needsUpdate = false;

    if (horizontalMode != horizontalScrollbarMode() && !m_horizontalScrollbarLock) {
        m_horizontalScrollbarMode = horizontalMode;
        needsUpdate = true;
    }

    if (verticalMode != verticalScrollbarMode() && !m_verticalScrollbarLock) {
        m_verticalScrollbarMode = verticalMode;
        needsUpdate = true;
    }

    if (horizontalLock)
        setHorizontalScrollbarLock();

    if (verticalLock)
        setVerticalScrollbarLock();

    if (needsUpdate)
        updateScrollbars(scrollOffset());

    // We don't need to report policy changes on ScrollView's unless this
    // one has an adjustment attached and it is a main frame.
    if (parent() || !isFrameView())
        return;

    // For frames that do have adjustments attached, we want to report
    // policy changes, so that they may be applied to the widget to
    // which the WebView's container (e.g. GtkScrolledWindow).
    if (hostWindow())
        hostWindow()->scrollbarsModeDidChange();
}

}

