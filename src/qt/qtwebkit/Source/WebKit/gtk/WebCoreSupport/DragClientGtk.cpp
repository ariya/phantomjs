/*
 * Copyright (C) 2009, 2010 Igalia S.L.
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
#include "DragClientGtk.h"

#include "Clipboard.h"
#include "ClipboardUtilitiesGtk.h"
#include "DataObjectGtk.h"
#include "Document.h"
#include "DragController.h"
#include "Element.h"
#include "Frame.h"
#include "GOwnPtrGtk.h"
#include "GRefPtrGtk.h"
#include "GtkVersioning.h"
#include "NotImplemented.h"
#include "Pasteboard.h"
#include "PasteboardHelper.h"
#include "RenderObject.h"
#include "webkitwebframeprivate.h"
#include "webkitwebviewprivate.h"
#include "webkitwebview.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>

using namespace WebCore;

namespace WebKit {

DragClient::DragClient(WebKitWebView* webView)
    : m_webView(webView)
    , m_startPos(0, 0)
{
}

DragClient::~DragClient()
{
}

void DragClient::willPerformDragDestinationAction(DragDestinationAction, DragData*)
{
}

void DragClient::willPerformDragSourceAction(DragSourceAction, const IntPoint& startPos, Clipboard*)
{
    m_startPos = startPos;
}

DragDestinationAction DragClient::actionMaskForDrag(DragData*)
{
    notImplemented();
    return DragDestinationActionAny;
}

DragSourceAction DragClient::dragSourceActionMaskForPoint(const IntPoint&)
{
    notImplemented();
    return DragSourceActionAny;
}

void DragClient::startDrag(DragImageRef image, const IntPoint& dragImageOrigin, const IntPoint& eventPos, Clipboard* clipboard, Frame* frame, bool linkDrag)
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(kit(frame));
    RefPtr<DataObjectGtk> dataObject = clipboard->pasteboard().dataObject();
    GRefPtr<GtkTargetList> targetList = adoptGRef(PasteboardHelper::defaultPasteboardHelper()->targetListForDataObject(dataObject.get()));
    GOwnPtr<GdkEvent> currentEvent(gtk_get_current_event());

    GdkDragContext* context = gtk_drag_begin(GTK_WIDGET(m_webView), targetList.get(), dragOperationToGdkDragActions(clipboard->sourceOperation()), 1, currentEvent.get());
    webView->priv->dragAndDropHelper.startedDrag(context, dataObject.get());

    // A drag starting should prevent a double-click from happening. This might
    // happen if a drag is followed very quickly by another click (like in the DRT).
    webView->priv->clickCounter.reset();

    if (image) {
        m_dragIcon.setImage(image);
        m_dragIcon.useForDrag(context, IntPoint(eventPos - dragImageOrigin));
    } else
        gtk_drag_set_icon_default(context);
}

void DragClient::dragControllerDestroyed()
{
    delete this;
}
}
