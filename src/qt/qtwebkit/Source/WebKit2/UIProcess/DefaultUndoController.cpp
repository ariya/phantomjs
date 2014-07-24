/*
    Copyright (C) 2007 Staikos Computing Services Inc.
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2012 Samsung Electronics

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
#include "DefaultUndoController.h"

#include "WebEditCommandProxy.h"
#include <wtf/RefPtr.h>

namespace WebKit {

void DefaultUndoController::registerEditCommand(PassRefPtr<WebEditCommandProxy> command, WebPageProxy::UndoOrRedo undoOrRedo)
{
    if (undoOrRedo == WebPageProxy::Undo)
        m_undoStack.append(command);
    else
        m_redoStack.append(command);
}

void DefaultUndoController::clearAllEditCommands()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

bool DefaultUndoController::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    if (undoOrRedo == WebPageProxy::Undo)
        return !m_undoStack.isEmpty();

    return !m_redoStack.isEmpty();
}

void DefaultUndoController::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    RefPtr<WebEditCommandProxy> command;
    if (undoOrRedo == WebPageProxy::Undo) {
        command = m_undoStack.last();
        m_undoStack.removeLast();
        command->unapply();
    } else {
        command = m_redoStack.last();
        m_redoStack.removeLast();
        command->reapply();
    }
}

} // namespace WebKit
