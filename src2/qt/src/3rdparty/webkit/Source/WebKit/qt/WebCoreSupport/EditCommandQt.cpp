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

#include "config.h"
#include "EditCommandQt.h"

using namespace WebCore;

#ifndef QT_NO_UNDOCOMMAND
EditCommandQt::EditCommandQt(WTF::RefPtr<EditCommand> cmd, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_cmd(cmd)
    , m_first(true)
{
}
#else
EditCommandQt::EditCommandQt(WTF::RefPtr<EditCommand> cmd)
    : m_cmd(cmd)
    , m_first(true)
{
}
#endif

EditCommandQt::~EditCommandQt()
{
}


void EditCommandQt::redo()
{
    if (m_first) {
        m_first = false;
        return;
    }
    if (m_cmd)
        m_cmd->reapply();
}


void EditCommandQt::undo()
{
    if (m_cmd)
        m_cmd->unapply();
}


// vim: ts=4 sw=4 et
