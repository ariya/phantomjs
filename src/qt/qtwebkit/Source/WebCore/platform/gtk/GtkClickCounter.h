/*
 * Copyright (C) 2010, 2011 Igalia S.L.
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

#ifndef GtkClickCounter_h
#define GtkClickCounter_h

#include "IntPoint.h"

typedef struct _GdkEventButton GdkEventButton;

namespace WebCore {

class GtkClickCounter {
public:
    GtkClickCounter();
    void reset();
    bool shouldProcessButtonEvent(GdkEventButton*);
    int clickCountForGdkButtonEvent(GtkWidget*, GdkEventButton*);

private:
    int m_currentClickCount;
    IntPoint m_previousClickPoint;
    unsigned int m_previousClickButton;
    int m_previousClickTime;
};

} // namespace WebCore

#endif

