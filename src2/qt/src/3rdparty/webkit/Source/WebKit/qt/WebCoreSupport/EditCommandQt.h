/*
    Copyright (C) 2007 Staikos Computing Services Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef EditCommandQt_h
#define EditCommandQt_h

#include <EditCommand.h>
#include <QUndoCommand>
#include <qglobal.h>

class EditCommandQt
#ifndef QT_NO_UNDOCOMMAND
    : public QUndoCommand
#endif
{
    public:
#ifndef QT_NO_UNDOCOMMAND
        EditCommandQt(WTF::RefPtr<WebCore::EditCommand> cmd, QUndoCommand *parent = 0);
#else
        EditCommandQt(WTF::RefPtr<WebCore::EditCommand> cmd);
#endif
        ~EditCommandQt();

        void redo();
        void undo();

    private:
        WTF::RefPtr<WebCore::EditCommand> m_cmd;
        bool m_first;
};

#endif

// vim: ts=4 sw=4 et
