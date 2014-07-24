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

#ifndef DefaultUndoController_h
#define DefaultUndoController_h

#include "WebEditCommandProxy.h"
#include "WebPageProxy.h"

namespace WebKit {

class DefaultUndoController {
public:
    void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo);
    void clearAllEditCommands();
    bool canUndoRedo(WebPageProxy::UndoOrRedo);
    void executeUndoRedo(WebPageProxy::UndoOrRedo);

private:
    typedef Vector<RefPtr<WebEditCommandProxy> > CommandVector;
    CommandVector m_undoStack;
    CommandVector m_redoStack;
};

} // namespace WebKit

#endif // DefaultUndoController_h
