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

#ifndef GtkDragAndDropHelper_h
#define GtkDragAndDropHelper_h

#include "DataObjectGtk.h"
#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/CString.h>

namespace WebCore {

struct DroppingContext;
class DragData;

typedef void (*DragExitedCallback)(GtkWidget*, DragData*, bool dropHappened);

class GtkDragAndDropHelper {
    WTF_MAKE_NONCOPYABLE(GtkDragAndDropHelper);
    WTF_MAKE_FAST_ALLOCATED;

public:
    GtkDragAndDropHelper()
        : m_widget(0)
    {
    }
    ~GtkDragAndDropHelper();

    void setWidget(GtkWidget* widget) { m_widget = widget; }
    bool handleDragEnd(GdkDragContext*);
    void handleGetDragData(GdkDragContext*, GtkSelectionData*, guint info);
    void handleDragLeave(GdkDragContext*, DragExitedCallback);
    void handleDragLeaveLater(DroppingContext*);
    DataObjectGtk* handleDragMotion(GdkDragContext*, const IntPoint&, unsigned time);
    DataObjectGtk* handleDragDataReceived(GdkDragContext*, GtkSelectionData*, unsigned info, IntPoint&);
    DataObjectGtk* handleDragDrop(GdkDragContext*);
    void startedDrag(GdkDragContext*, DataObjectGtk*);

private:
    GtkWidget* m_widget;
    HashMap<GdkDragContext*, DroppingContext*> m_droppingContexts;
    HashMap<GdkDragContext*, RefPtr<DataObjectGtk> > m_draggingDataObjects;
};

}

#endif // DataObjectGtk_h
