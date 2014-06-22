/*
 * Copyright (C) 2011 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef GtkAdjustmentWatcher_h
#define GtkAdjustmentWatcher_h

#include "GRefPtrGtk.h"

typedef struct _WebKitWebView WebKitWebView;

namespace WebKit {

class GtkAdjustmentWatcher {
public:
    GtkAdjustmentWatcher(WebKitWebView*);
    ~GtkAdjustmentWatcher();

    void setHorizontalAdjustment(GtkAdjustment*);
    void setVerticalAdjustment(GtkAdjustment*);
    GtkAdjustment* horizontalAdjustment() { return m_horizontalAdjustment.get(); };
    GtkAdjustment* verticalAdjustment() { return m_verticalAdjustment.get(); };
    void adjustmentValueChanged(GtkAdjustment*);
    void updateAdjustmentsFromScrollbars();
    void updateAdjustmentsFromScrollbarsLater() const;
    void disableAllScrollbars();
    void enableAllScrollbars();
    bool scrollbarsDisabled() const { return m_scrollbarsDisabled; };

private:
    static gboolean updateAdjustmentCallback(GtkAdjustmentWatcher*);

    WebKitWebView* m_webView;
    GRefPtr<GtkAdjustment> m_horizontalAdjustment;
    GRefPtr<GtkAdjustment> m_verticalAdjustment;
    bool m_scrollbarsDisabled;
    bool m_handlingGtkAdjustmentChange;
    mutable unsigned m_updateAdjustmentCallbackId;
};

}

#endif // GtkAdjustmentWatcher_h

