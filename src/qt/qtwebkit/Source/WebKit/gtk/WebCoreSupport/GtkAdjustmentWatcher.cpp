/*
 *  Copyright (C) 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GtkAdjustmentWatcher.h"

#include "Frame.h"
#include "FrameView.h"
#include "Page.h"
#include "Scrollbar.h"
#include "webkitwebviewprivate.h"
#include <gtk/gtk.h>

using namespace WebCore;

namespace WebKit {

GtkAdjustmentWatcher::GtkAdjustmentWatcher(WebKitWebView* webView)
    : m_webView(webView)
    , m_scrollbarsDisabled(false)
    , m_handlingGtkAdjustmentChange(false)
    , m_updateAdjustmentCallbackId(0)
{
}

GtkAdjustmentWatcher::~GtkAdjustmentWatcher()
{
    if (m_updateAdjustmentCallbackId)
        g_source_remove(m_updateAdjustmentCallbackId);
}

static void updateAdjustmentFromScrollbar(GtkAdjustment* adjustment, Scrollbar* scrollbar)
{
    if (!adjustment)
        return;
    if (!scrollbar) {
        gtk_adjustment_configure(adjustment, 0, 0, 0, 0, 0, 0); // These are the settings which remove the scrollbar.
        return;
    }
    gtk_adjustment_configure(adjustment, scrollbar->value(), 0, scrollbar->totalSize(),
                             scrollbar->lineStep(), scrollbar->pageStep(), scrollbar->visibleSize());
}

void GtkAdjustmentWatcher::updateAdjustmentsFromScrollbars()
{
    if (m_scrollbarsDisabled)
        return;
    if (m_handlingGtkAdjustmentChange)
        return;
    if (!core(m_webView) || !core(m_webView)->mainFrame())
        return;

    FrameView* frameView = core(m_webView)->mainFrame()->view();
    updateAdjustmentFromScrollbar(m_horizontalAdjustment.get(), frameView->horizontalScrollbar());
    updateAdjustmentFromScrollbar(m_verticalAdjustment.get(), frameView->verticalScrollbar());
    if (m_updateAdjustmentCallbackId) {
        g_source_remove(m_updateAdjustmentCallbackId);
        m_updateAdjustmentCallbackId = 0;
    }
}

gboolean GtkAdjustmentWatcher::updateAdjustmentCallback(GtkAdjustmentWatcher* watcher)
{
    watcher->m_updateAdjustmentCallbackId = 0;
    watcher->updateAdjustmentsFromScrollbars();
    return FALSE;
}

void GtkAdjustmentWatcher::updateAdjustmentsFromScrollbarsLater() const
{
    // We've already scheduled an update. No need to schedule another.
    if (m_updateAdjustmentCallbackId || m_scrollbarsDisabled)
        return;

    // The fact that this method was called means that we need to update the scrollbars, but at the
    // time of invocation they are not updated to reflect the scroll yet. We set a short timeout
    // here, which means that they will be updated as soon as WebKit returns to the main loop.
    m_updateAdjustmentCallbackId = g_timeout_add(0, reinterpret_cast<GSourceFunc>(updateAdjustmentCallback),
                                                 const_cast<void*>(static_cast<const void*>(this)));
}

static void adjustmentValueChangedCallback(GtkAdjustment* adjustment, GtkAdjustmentWatcher* watcher)
{
    watcher->adjustmentValueChanged(adjustment);
}

static void setAdjustment(GtkAdjustmentWatcher* watcher, GRefPtr<GtkAdjustment>& adjustmentMember, GtkAdjustment* newAdjustment)
{
    if (adjustmentMember) {
        g_signal_handlers_disconnect_by_func(adjustmentMember.get(),
                                             reinterpret_cast<void*>(adjustmentValueChangedCallback), watcher);
    }

    adjustmentMember = newAdjustment;
    if (newAdjustment)
        g_signal_connect(newAdjustment, "value-changed", G_CALLBACK(adjustmentValueChangedCallback), watcher);
}

void GtkAdjustmentWatcher::setHorizontalAdjustment(GtkAdjustment* newAdjustment)
{
    setAdjustment(this, m_horizontalAdjustment, newAdjustment);
}

void GtkAdjustmentWatcher::setVerticalAdjustment(GtkAdjustment* newAdjustment)
{
    setAdjustment(this, m_verticalAdjustment, newAdjustment);
}

void GtkAdjustmentWatcher::adjustmentValueChanged(GtkAdjustment* adjustment)
{
    FrameView* frameView = core(m_webView)->mainFrame()->view();
    Scrollbar* scrollbar = (adjustment == m_horizontalAdjustment.get()) ? 
        frameView->horizontalScrollbar() : frameView->verticalScrollbar();
    if (!scrollbar)
        return;

    int newValue = static_cast<int>(gtk_adjustment_get_value(adjustment));
    if (newValue != scrollbar->value()) {
        m_handlingGtkAdjustmentChange = true;
        frameView->scrollToOffsetWithoutAnimation(scrollbar->orientation(), newValue);
        m_handlingGtkAdjustmentChange = false;
    }
}

void GtkAdjustmentWatcher::disableAllScrollbars()
{
    updateAdjustmentFromScrollbar(m_horizontalAdjustment.get(), 0);
    updateAdjustmentFromScrollbar(m_verticalAdjustment.get(), 0);
    m_scrollbarsDisabled = true;
}

void GtkAdjustmentWatcher::enableAllScrollbars()
{
    m_scrollbarsDisabled = false;
    updateAdjustmentsFromScrollbars();
}

} // namespace WebKit

