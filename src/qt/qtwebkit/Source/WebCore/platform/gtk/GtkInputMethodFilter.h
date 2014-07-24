/*
 * Copyright (C) 2012 Igalia S.L.
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

#ifndef GtkInputMethodFilter_h
#define GtkInputMethodFilter_h

#include "GRefPtrGtk.h"
#include "IntRect.h"
#include <gdk/gdk.h>
#include <wtf/text/WTFString.h>

typedef struct _GtkIMContext GtkIMContext;
typedef struct _GtkWidget GtkWidget;

namespace WebCore {

class GtkInputMethodFilter {
public:
    GtkInputMethodFilter();
    ~GtkInputMethodFilter();

    bool filterKeyEvent(GdkEventKey*);
    void notifyMouseButtonPress();
    void notifyFocusedIn();
    void notifyFocusedOut();
    void resetContext();
    void setEnabled(bool);

    void handleCommit(const char* compositionString);
    void handlePreeditChanged();
    void handlePreeditStart();
    void handlePreeditEnd();

    void confirmComposition();
    void cancelContextComposition();
    void updatePreedit();
    void setCursorRect(const IntRect& location);

    GtkIMContext* context() { return m_context.get(); }

    enum EventFakedForComposition {
        EventFaked,
        EventNotFaked
    };

protected:
    enum ResultsToSend {
        Preedit = 1 << 1,
        Composition = 1 << 2,
        PreeditAndComposition = Preedit + Composition
    };

    void setWidget(GtkWidget*);
    virtual bool canEdit() = 0;
    virtual bool sendSimpleKeyEvent(GdkEventKey*, WTF::String eventString = String(), EventFakedForComposition = EventNotFaked) = 0;
    virtual bool sendKeyEventWithCompositionResults(GdkEventKey*, ResultsToSend = PreeditAndComposition, EventFakedForComposition = EventNotFaked) = 0;
    virtual void confirmCompositionText(String composition) = 0;
    virtual void confirmCurrentComposition() = 0;
    virtual void cancelCurrentComposition() = 0;
    virtual void setPreedit(String, int cursorOffset) = 0;

    WTF::String m_confirmedComposition;
    WTF::String m_preedit;
    int m_cursorOffset;

private:
    void sendCompositionAndPreeditWithFakeKeyEvents(ResultsToSend);

    GRefPtr<GtkIMContext> m_context;
    GtkWidget* m_widget;
    bool m_enabled;
    bool m_composingTextCurrently;
    bool m_filteringKeyEvent;
    bool m_preeditChanged;
    bool m_preventNextCommit;
    bool m_justSentFakeKeyUp;
    unsigned int m_lastFilteredKeyPressCodeWithNoResults;
    IntPoint m_lastCareLocation;
};

} // namespace WebCore

#endif // GtkInputMethodFilter_h
