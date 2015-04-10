/*
 *  Copyright (C) 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "config.h"
#include "WebEditorClient.h"

#include "Frame.h"
#include "FrameDestructionObserver.h"
#include "PlatformKeyboardEvent.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/DataObjectGtk.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/PasteboardHelper.h>
#include <WebCore/WindowsKeyboardCodes.h>

using namespace WebCore;

namespace WebKit {

void WebEditorClient::getEditorCommandsForKeyEvent(const KeyboardEvent* event, Vector<WTF::String>& pendingEditorCommands)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    /* First try to interpret the command in the UI and get the commands.
       UI needs to receive event type because only knows current NativeWebKeyboardEvent.*/
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebPageProxy::GetEditorCommandsForKeyEvent(event->type()),
                                                Messages::WebPageProxy::GetEditorCommandsForKeyEvent::Reply(pendingEditorCommands),
                                                m_page->pageID(), CoreIPC::Connection::NoTimeout);
}

bool WebEditorClient::executePendingEditorCommands(Frame* frame, Vector<WTF::String> pendingEditorCommands, bool allowTextInsertion)
{
    Vector<Editor::Command> commands;
    for (size_t i = 0; i < pendingEditorCommands.size(); i++) {
        Editor::Command command = frame->editor().command(pendingEditorCommands.at(i).utf8().data());
        if (command.isTextInsertion() && !allowTextInsertion)
            return false;

        commands.append(command);
    }

    for (size_t i = 0; i < commands.size(); i++) {
        if (!commands.at(i).execute())
            return false;
    }

    return true;
}

void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return;

    // If this was an IME event don't do anything.
    if (platformEvent->windowsVirtualKeyCode() == VK_PROCESSKEY)
        return;

    Vector<WTF::String> pendingEditorCommands;
    getEditorCommandsForKeyEvent(event, pendingEditorCommands);
    if (!pendingEditorCommands.isEmpty()) {

        // During RawKeyDown events if an editor command will insert text, defer
        // the insertion until the keypress event. We want keydown to bubble up
        // through the DOM first.
        if (platformEvent->type() == PlatformEvent::RawKeyDown) {
            if (executePendingEditorCommands(frame, pendingEditorCommands, false))
                event->setDefaultHandled();

            return;
        }

        // Only allow text insertion commands if the current node is editable.
        if (executePendingEditorCommands(frame, pendingEditorCommands, frame->editor().canEdit())) {
            event->setDefaultHandled();
            return;
        }
    }

    // Don't allow text insertion for nodes that cannot edit.
    if (!frame->editor().canEdit())
        return;

    // This is just a normal text insertion, so wait to execute the insertion
    // until a keypress event happens. This will ensure that the insertion will not
    // be reflected in the contents of the field until the keyup DOM event.
    if (event->type() != eventNames().keypressEvent)
        return;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (event->charCode() < ' ')
        return;

    // Don't insert anything if a modifier is pressed
    if (platformEvent->ctrlKey() || platformEvent->altKey())
        return;

    if (frame->editor().insertText(platformEvent->text(), event))
        event->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent* event)
{
    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (platformEvent && platformEvent->windowsVirtualKeyCode() == VK_PROCESSKEY)
        event->preventDefault();
}

#if PLATFORM(X11)
class EditorClientFrameDestructionObserver : FrameDestructionObserver {
public:
    EditorClientFrameDestructionObserver(Frame* frame, GClosure* closure)
        : FrameDestructionObserver(frame)
        , m_closure(closure)
    {
        g_closure_add_finalize_notifier(m_closure, this, destroyOnClosureFinalization);
    }

    void frameDestroyed()
    {
        g_closure_invalidate(m_closure);
        FrameDestructionObserver::frameDestroyed();
    }
private:
    GClosure* m_closure;

    static void destroyOnClosureFinalization(gpointer data, GClosure* closure)
    {
        // Calling delete void* will free the memory but won't invoke
        // the destructor, something that is a must for us.
        EditorClientFrameDestructionObserver* observer = static_cast<EditorClientFrameDestructionObserver*>(data);
        delete observer;
    }
};

static Frame* frameSettingClipboard;

static void collapseSelection(GtkClipboard* clipboard, Frame* frame)
{
    if (frameSettingClipboard && frameSettingClipboard == frame)
        return;

    // Collapse the selection without clearing it.
    ASSERT(frame);
    frame->selection()->setBase(frame->selection()->extent(), frame->selection()->affinity());
}
#endif

void WebEditorClient::updateGlobalSelection(Frame* frame)
{
#if PLATFORM(X11)
    GtkClipboard* clipboard = PasteboardHelper::defaultPasteboardHelper()->getPrimarySelectionClipboard(frame);
    DataObjectGtk* dataObject = DataObjectGtk::forClipboard(clipboard);

    if (!frame->selection()->isRange())
        return;

    dataObject->clearAll();
    dataObject->setRange(frame->selection()->toNormalizedRange());

    frameSettingClipboard = frame;
    GClosure* callback = g_cclosure_new(G_CALLBACK(collapseSelection), frame, 0);
    // This observer will be self-destroyed on closure finalization,
    // that will happen either after closure execution or after
    // closure invalidation.
    new EditorClientFrameDestructionObserver(frame, callback);
    g_closure_set_marshal(callback, g_cclosure_marshal_VOID__VOID);
    PasteboardHelper::defaultPasteboardHelper()->writeClipboardContents(clipboard, PasteboardHelper::DoNotIncludeSmartPaste, callback);
    frameSettingClipboard = 0;
#endif
}

bool WebEditorClient::shouldShowUnicodeMenu()
{
    return true;
}

}
