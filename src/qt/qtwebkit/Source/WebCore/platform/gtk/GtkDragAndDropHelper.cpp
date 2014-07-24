/*
 * Copyright (C) 2011, Igalia S.L.
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
#include "GtkDragAndDropHelper.h"

#include "ClipboardUtilitiesGtk.h"
#include "DragData.h"
#include "GtkUtilities.h"
#include "GtkVersioning.h"
#include "PasteboardHelper.h"
#include <gtk/gtk.h>

namespace WebCore {

struct DroppingContext {
    DroppingContext(GdkDragContext* gdkContext, const IntPoint& position)
        : gdkContext(gdkContext)
        , dataObject(DataObjectGtk::create())
        , lastMotionPosition(position)
        , dropHappened(false)
        , exitedCallback(0)
    {
    }

    GdkDragContext* gdkContext;
    RefPtr<WebCore::DataObjectGtk> dataObject;
    WebCore::IntPoint lastMotionPosition;
    int pendingDataRequests;
    bool dropHappened;
    DragExitedCallback exitedCallback;
};

typedef HashMap<GdkDragContext*, DroppingContext*> DroppingContextMap;
typedef HashMap<GdkDragContext*, RefPtr<DataObjectGtk> > DraggingDataObjectsMap;

GtkDragAndDropHelper::~GtkDragAndDropHelper()
{
    deleteAllValues(m_droppingContexts);
}

bool GtkDragAndDropHelper::handleDragEnd(GdkDragContext* dragContext)
{
    DraggingDataObjectsMap::iterator iterator = m_draggingDataObjects.find(dragContext);
    if (iterator == m_draggingDataObjects.end())
        return false;

    m_draggingDataObjects.remove(iterator);
    return true;
}

void GtkDragAndDropHelper::handleGetDragData(GdkDragContext* context, GtkSelectionData* selectionData, guint info)
{
    DraggingDataObjectsMap::iterator iterator = m_draggingDataObjects.find(context);
    if (iterator == m_draggingDataObjects.end())
        return;
    PasteboardHelper::defaultPasteboardHelper()->fillSelectionData(selectionData, info, iterator->value.get());
}

struct HandleDragLaterData {
    DroppingContext* context;
    GtkDragAndDropHelper* glue;
};

static gboolean handleDragLeaveLaterCallback(HandleDragLaterData* data)
{
    data->glue->handleDragLeaveLater(data->context);
    delete data;
    return FALSE;
}

void GtkDragAndDropHelper::handleDragLeaveLater(DroppingContext* context)
{
    DroppingContextMap::iterator iterator = m_droppingContexts.find(context->gdkContext);
    if (iterator == m_droppingContexts.end())
        return;

    // If the view doesn't know about the drag yet (there are still pending data)
    // requests, don't update it with information about the drag.
    if (context->pendingDataRequests)
        return;

    const IntPoint& position = context->lastMotionPosition;
    DragData dragData(context->dataObject.get(), position,
                      convertWidgetPointToScreenPoint(m_widget, position),
                      DragOperationNone);
    context->exitedCallback(m_widget, &dragData, context->dropHappened);

    m_droppingContexts.remove(iterator);
    delete context;
}

void GtkDragAndDropHelper::handleDragLeave(GdkDragContext* gdkContext, DragExitedCallback exitedCallback)
{
    DroppingContextMap::iterator iterator = m_droppingContexts.find(gdkContext);
    if (iterator == m_droppingContexts.end())
        return;

    // During a drop GTK+ will fire a drag-leave signal right before firing
    // the drag-drop signal. We want the actions for drag-leave to happen after
    // those for drag-drop, so schedule them to happen asynchronously here.
    HandleDragLaterData* data = new HandleDragLaterData;
    data->context = iterator->value;
    data->context->exitedCallback = exitedCallback;
    data->glue = this;
    g_timeout_add(0, reinterpret_cast<GSourceFunc>(handleDragLeaveLaterCallback), data);
}

static void queryNewDropContextData(DroppingContext* dropContext, GtkWidget* widget, guint time)
{
    GdkDragContext* gdkContext = dropContext->gdkContext;
    Vector<GdkAtom> acceptableTargets(PasteboardHelper::defaultPasteboardHelper()->dropAtomsForContext(widget, gdkContext));
    dropContext->pendingDataRequests = acceptableTargets.size();
    for (size_t i = 0; i < acceptableTargets.size(); i++)
        gtk_drag_get_data(widget, gdkContext, acceptableTargets.at(i), time);
}

DataObjectGtk* GtkDragAndDropHelper::handleDragMotion(GdkDragContext* context, const IntPoint& position, unsigned time)
{
    DroppingContext* droppingContext = 0;
    DroppingContextMap::iterator iterator = m_droppingContexts.find(context);
    if (iterator == m_droppingContexts.end()) {
        droppingContext = new DroppingContext(context, position);
        m_droppingContexts.set(context, droppingContext);
        queryNewDropContextData(droppingContext, m_widget, time);
    } else {
        droppingContext = iterator->value;
        droppingContext->lastMotionPosition = position;
    }

    // Don't send any drag information to WebCore until we've retrieved all
    // the data for this drag operation. Otherwise we'd have to block to wait
    // for the drag's data.
    ASSERT(droppingContext);
    if (droppingContext->pendingDataRequests > 0)
        return 0;

    return droppingContext->dataObject.get();
}

DataObjectGtk* GtkDragAndDropHelper::handleDragDataReceived(GdkDragContext* context, GtkSelectionData* selectionData, unsigned info, IntPoint& position)
{
    DroppingContextMap::iterator iterator = m_droppingContexts.find(context);
    if (iterator == m_droppingContexts.end())
        return 0;

    DroppingContext* droppingContext = iterator->value;
    droppingContext->pendingDataRequests--;
    PasteboardHelper::defaultPasteboardHelper()->fillDataObjectFromDropData(selectionData, info, droppingContext->dataObject.get());

    if (droppingContext->pendingDataRequests)
        return 0;

    // The coordinates passed to drag-data-received signal are sometimes
    // inaccurate in DRT, so use the coordinates of the last motion event.
    position = droppingContext->lastMotionPosition;

    // If there are no more pending requests, start sending dragging data to WebCore.
    return droppingContext->dataObject.get();
}

DataObjectGtk* GtkDragAndDropHelper::handleDragDrop(GdkDragContext* context)
{
    DroppingContextMap::iterator iterator = m_droppingContexts.find(context);
    if (iterator == m_droppingContexts.end())
        return 0;

    DroppingContext* droppingContext = iterator->value;
    droppingContext->dropHappened = true;

    return droppingContext->dataObject.get();
}

void GtkDragAndDropHelper::startedDrag(GdkDragContext* context, DataObjectGtk* dataObject)
{
    m_draggingDataObjects.set(context, dataObject);
}

} // namespace WebCore
