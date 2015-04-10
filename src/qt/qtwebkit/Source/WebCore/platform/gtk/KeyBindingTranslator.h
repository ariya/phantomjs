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

#ifndef KeyBindingTranslator_h
#define KeyBindingTranslator_h

#include "GRefPtrGtk.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

typedef struct _GdkEventKey GdkEventKey;

namespace WebCore {

class KeyBindingTranslator {
public:
    KeyBindingTranslator();

    enum EventType { KeyDown, KeyPress };
    void getEditorCommandsForKeyEvent(GdkEventKey*, EventType, Vector<WTF::String>&);
    void addPendingEditorCommand(const char* command) { m_pendingEditorCommands.append(command); }

private:
    GRefPtr<GtkWidget> m_nativeWidget;
    Vector<WTF::String> m_pendingEditorCommands;
};

} // namespace WebCore

#endif


