/*
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2009 Diego Escalante Urrelo <diegoe@gnome.org>
 *  Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 *  Copyright (C) 2009, 2010 Igalia S.L.
 *  Copyright (C) 2010, Martin Robinson <mrobinson@webkit.org>
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

#include "config.h"
#include "EditorClientGtk.h"

#include "DataObjectGtk.h"
#include "DumpRenderTreeSupportGtk.h"
#include "Editor.h"
#include "EventNames.h"
#include "FocusController.h"
#include "Frame.h"
#include <glib.h>
#include "KeyboardEvent.h"
#include "markup.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PasteboardHelper.h"
#include "PlatformKeyboardEvent.h"
#include "Settings.h"
#include "StylePropertySet.h"
#include "UndoStep.h"
#include "WebKitDOMCSSStyleDeclarationPrivate.h"
#include "WebKitDOMHTMLElementPrivate.h"
#include "WebKitDOMNodePrivate.h"
#include "WebKitDOMRangePrivate.h"
#include "WindowsKeyboardCodes.h"
#include "webkitglobals.h"
#include "webkitglobalsprivate.h"
#include "webkitmarshal.h"
#include "webkitspellchecker.h"
#include "webkitwebsettingsprivate.h"
#include "webkitwebviewprivate.h"
#include <wtf/text/CString.h>

// Arbitrary depth limit for the undo stack, to keep it from using
// unbounded memory.  This is the maximum number of distinct undoable
// actions -- unbroken stretches of typed characters are coalesced
// into a single action.
#define maximumUndoStackDepth 1000

using namespace WebCore;

namespace WebKit {

void EditorClient::willSetInputMethodState()
{
}

void EditorClient::setInputMethodState(bool active)
{
    m_webView->priv->imFilter.setEnabled(active);
}

bool EditorClient::shouldShowUnicodeMenu()
{
    if (gtk_major_version > 2 || (gtk_major_version == 2 && gtk_minor_version >= 10)) {
        GtkSettings* settings = gtk_widget_get_settings(GTK_WIDGET(m_webView));
        if (!settings)
            return true;

        gboolean enabled;
        g_object_get(settings, "gtk-show-unicode-menu", &enabled, NULL);
        return enabled;
    }

    return true;
}

bool EditorClient::shouldDeleteRange(Range* range)
{
    gboolean accept = TRUE;
    GRefPtr<WebKitDOMRange> kitRange(adoptGRef(kit(range)));
    g_signal_emit_by_name(m_webView, "should-delete-range", kitRange.get(), &accept);
    return accept;
}

bool EditorClient::isContinuousSpellCheckingEnabled()
{
    WebKitWebSettings* settings = webkit_web_view_get_settings(m_webView);

    gboolean enabled;
    g_object_get(settings, "enable-spell-checking", &enabled, NULL);

    return enabled;
}

bool EditorClient::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

int EditorClient::spellCheckerDocumentTag()
{
    notImplemented();
    return 0;
}

bool EditorClient::shouldBeginEditing(WebCore::Range* range)
{
    gboolean accept = TRUE;
    GRefPtr<WebKitDOMRange> kitRange(adoptGRef(kit(range)));
    g_signal_emit_by_name(m_webView, "should-begin-editing", kitRange.get(), &accept);
    return accept;
}

bool EditorClient::shouldEndEditing(WebCore::Range* range)
{
    gboolean accept = TRUE;
    GRefPtr<WebKitDOMRange> kitRange(adoptGRef(kit(range)));
    g_signal_emit_by_name(m_webView, "should-end-editing", kitRange.get(), &accept);
    return accept;
}

static WebKitInsertAction kit(EditorInsertAction action)
{
    switch (action) {
    case EditorInsertActionTyped:
        return WEBKIT_INSERT_ACTION_TYPED;
    case EditorInsertActionPasted:
        return WEBKIT_INSERT_ACTION_PASTED;
    case EditorInsertActionDropped:
        return WEBKIT_INSERT_ACTION_DROPPED;
    }
    ASSERT_NOT_REACHED();
    return WEBKIT_INSERT_ACTION_TYPED;
}

bool EditorClient::shouldInsertText(const String& string, Range* range, EditorInsertAction action)
{
    gboolean accept = TRUE;
    GRefPtr<WebKitDOMRange> kitRange(adoptGRef(kit(range)));
    g_signal_emit_by_name(m_webView, "should-insert-text", string.utf8().data(), kitRange.get(), kit(action), &accept);
    return accept;
}

static WebKitSelectionAffinity kit(EAffinity affinity)
{
    switch (affinity) {
    case UPSTREAM:
        return WEBKIT_SELECTION_AFFINITY_UPSTREAM;
    case DOWNSTREAM:
        return WEBKIT_SELECTION_AFFINITY_DOWNSTREAM;
    }
    ASSERT_NOT_REACHED();
    return WEBKIT_SELECTION_AFFINITY_UPSTREAM;
}

bool EditorClient::shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity affinity, bool stillSelecting)
{
    gboolean accept = TRUE;
    GRefPtr<WebKitDOMRange> kitFromRange(fromRange ? adoptGRef(kit(fromRange)) : 0);
    GRefPtr<WebKitDOMRange> kitToRange(toRange ? adoptGRef(kit(toRange)) : 0);
    g_signal_emit_by_name(m_webView, "should-change-selected-range", kitFromRange.get(), kitToRange.get(),
                          kit(affinity), stillSelecting, &accept);
    return accept;
}

bool EditorClient::shouldApplyStyle(WebCore::StylePropertySet* set, WebCore::Range* range)
{
    gboolean accept = TRUE;
    GRefPtr<WebKitDOMCSSStyleDeclaration> kitDeclaration(kit(set->mutableCopy()->ensureCSSStyleDeclaration()));
    GRefPtr<WebKitDOMRange> kitRange(adoptGRef(kit(range)));
    g_signal_emit_by_name(m_webView, "should-apply-style", kitDeclaration.get(), kitRange.get(), &accept);
    return accept;
}

bool EditorClient::shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*)
{
    notImplemented();
    return true;
}

void EditorClient::didBeginEditing()
{
    g_signal_emit_by_name(m_webView, "editing-began");
}

void EditorClient::respondToChangedContents()
{
    g_signal_emit_by_name(m_webView, "user-changed-contents");
}

static WebKitWebView* viewSettingClipboard = 0;
static void collapseSelection(GtkClipboard* clipboard, WebKitWebView* webView)
{
    if (viewSettingClipboard && viewSettingClipboard == webView)
        return;

    WebCore::Page* corePage = core(webView);
    if (!corePage || !corePage->focusController())
        return;

    Frame* frame = corePage->focusController()->focusedOrMainFrame();

    // Collapse the selection without clearing it
    ASSERT(frame);
    frame->selection()->setBase(frame->selection()->extent(), frame->selection()->affinity());
}

#if PLATFORM(X11)
static void setSelectionPrimaryClipboardIfNeeded(WebKitWebView* webView)
{
    if (!gtk_widget_has_screen(GTK_WIDGET(webView)))
        return;

    GtkClipboard* clipboard = gtk_widget_get_clipboard(GTK_WIDGET(webView), GDK_SELECTION_PRIMARY);
    DataObjectGtk* dataObject = DataObjectGtk::forClipboard(clipboard);
    WebCore::Page* corePage = core(webView);
    Frame* targetFrame = corePage->focusController()->focusedOrMainFrame();

    if (!targetFrame->selection()->isRange())
        return;

    dataObject->clearAll();
    dataObject->setRange(targetFrame->selection()->toNormalizedRange());

    viewSettingClipboard = webView;
    GClosure* callback = g_cclosure_new_object(G_CALLBACK(collapseSelection), G_OBJECT(webView));
    g_closure_set_marshal(callback, g_cclosure_marshal_VOID__VOID);
    PasteboardHelper::defaultPasteboardHelper()->writeClipboardContents(clipboard, PasteboardHelper::DoNotIncludeSmartPaste, callback);
    viewSettingClipboard = 0;
}
#endif

void EditorClient::respondToChangedSelection(Frame* frame)
{
    g_signal_emit_by_name(m_webView, "selection-changed");

    if (!frame)
        return;

#if PLATFORM(X11)
    setSelectionPrimaryClipboardIfNeeded(m_webView);
#endif

    if (frame->editor().cancelCompositionIfSelectionIsInvalid())
        m_webView->priv->imFilter.resetContext();
}

void EditorClient::didEndEditing()
{
    g_signal_emit_by_name(m_webView, "editing-ended");
}

void EditorClient::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void EditorClient::willWriteSelectionToPasteboard(WebCore::Range*)
{
}

void EditorClient::getClientPasteboardDataForRange(WebCore::Range*, Vector<String>&, Vector<RefPtr<WebCore::SharedBuffer> >&)
{
}

void EditorClient::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

void EditorClient::registerUndoStep(WTF::PassRefPtr<WebCore::UndoStep> step)
{
    if (undoStack.size() == maximumUndoStackDepth)
        undoStack.removeFirst();
    if (!m_isInRedo)
        redoStack.clear();
    undoStack.append(step);
}

void EditorClient::registerRedoStep(WTF::PassRefPtr<WebCore::UndoStep> step)
{
    redoStack.append(step);
}

void EditorClient::clearUndoRedoOperations()
{
    undoStack.clear();
    redoStack.clear();
}

bool EditorClient::canCopyCut(WebCore::Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool EditorClient::canPaste(WebCore::Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool EditorClient::canUndo() const
{
    return !undoStack.isEmpty();
}

bool EditorClient::canRedo() const
{
    return !redoStack.isEmpty();
}

void EditorClient::undo()
{
    if (canUndo()) {
        RefPtr<WebCore::UndoStep> step(*(--undoStack.end()));
        undoStack.remove(--undoStack.end());
        // unapply will call us back to push this command onto the redo stack.
        step->unapply();
    }
}

void EditorClient::redo()
{
    if (canRedo()) {
        RefPtr<WebCore::UndoStep> step(*(--redoStack.end()));
        redoStack.remove(--redoStack.end());

        ASSERT(!m_isInRedo);
        m_isInRedo = true;
        // reapply will call us back to push this command onto the undo stack.
        step->reapply();
        m_isInRedo = false;
    }
}

bool EditorClient::shouldInsertNode(Node* node, Range* range, EditorInsertAction action)
{
    gboolean accept = TRUE;
    GRefPtr<WebKitDOMRange> kitRange(adoptGRef(kit(range)));
    GRefPtr<WebKitDOMNode> kitNode(adoptGRef(kit(node)));
    g_signal_emit_by_name(m_webView, "should-insert-node", kitNode.get(), kitRange.get(), kit(action), &accept);
    return accept;
}

void EditorClient::pageDestroyed()
{
    delete this;
}

bool EditorClient::smartInsertDeleteEnabled()
{
    WebCore::Page* corePage = core(m_webView);
    if (!corePage)
        return false;
    return corePage->settings()->smartInsertDeleteEnabled();
}

bool EditorClient::isSelectTrailingWhitespaceEnabled()
{
    WebCore::Page* corePage = core(m_webView);
    if (!corePage)
        return false;
    return corePage->settings()->selectTrailingWhitespaceEnabled();
}

void EditorClient::toggleContinuousSpellChecking()
{
    WebKitWebSettings* settings = webkit_web_view_get_settings(m_webView);

    gboolean enabled;
    g_object_get(settings, "enable-spell-checking", &enabled, NULL);

    g_object_set(settings, "enable-spell-checking", !enabled, NULL);
}

void EditorClient::toggleGrammarChecking()
{
}

bool EditorClient::executePendingEditorCommands(Frame* frame, bool allowTextInsertion)
{
    Vector<Editor::Command> commands;
    for (size_t i = 0; i < m_pendingEditorCommands.size(); i++) {
        Editor::Command command = frame->editor().command(m_pendingEditorCommands.at(i).utf8().data());
        if (command.isTextInsertion() && !allowTextInsertion)
            return false;

        commands.append(command);
    }

    bool success = true;
    for (size_t i = 0; i < commands.size(); i++) {
        if (!commands.at(i).execute()) {
            success = false;
            break;
        }
    }

    m_pendingEditorCommands.clear();
    return success;
}

static bool keyboardEventHadCompositionResults(KeyboardEvent* event)
{
    if (event->type() != eventNames().keydownEvent)
        return false;

    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return false;

    return platformEvent->compositionResults().compositionUpdated();
}

void EditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return;

    if (keyboardEventHadCompositionResults(event))
        return;

    KeyBindingTranslator::EventType type = event->type() == eventNames().keydownEvent ?
        KeyBindingTranslator::KeyDown : KeyBindingTranslator::KeyPress;
    m_keyBindingTranslator.getEditorCommandsForKeyEvent(platformEvent->gdkEventKey(), type, m_pendingEditorCommands);
    if (m_pendingEditorCommands.size() > 0) {

        // During RawKeyDown events if an editor command will insert text, defer
        // the insertion until the keypress event. We want keydown to bubble up
        // through the DOM first.
        if (platformEvent->type() == PlatformEvent::RawKeyDown) {
            if (executePendingEditorCommands(frame, false))
                event->setDefaultHandled();

            return;
        }

        // Only allow text insertion commands if the current node is editable.
        if (executePendingEditorCommands(frame, frame->editor().canEdit())) {
            event->setDefaultHandled();
            return;
        }
        m_pendingEditorCommands.clear();
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

void EditorClient::handleInputMethodKeydown(KeyboardEvent* event)
{
    // Input method results are handled in handleKeyboardEvent, so that we can wait
    // to trigger composition updates until after the keydown event handler. This better
    // matches other browsers.
    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (platformEvent && platformEvent->compositionResults().compositionUpdated())
        event->preventDefault();
}

EditorClient::EditorClient(WebKitWebView* webView)
    : m_isInRedo(false)
#if ENABLE(SPELLCHECK)
    , m_textCheckerClient(WEBKIT_SPELL_CHECKER(webkit_get_text_checker()))
#endif
    , m_webView(webView)
{
}

EditorClient::~EditorClient()
{
}

void EditorClient::textFieldDidBeginEditing(Element*)
{
}

void EditorClient::textFieldDidEndEditing(Element*)
{
}

void EditorClient::textDidChangeInTextField(Element*)
{
}

bool EditorClient::doTextFieldCommandFromEvent(Element*, KeyboardEvent*)
{
    return false;
}

void EditorClient::textWillBeDeletedInTextField(Element*)
{
    notImplemented();
}

void EditorClient::textDidChangeInTextArea(Element*)
{
    notImplemented();
}

void EditorClient::updateSpellingUIWithGrammarString(const String&, const GrammarDetail&)
{
    notImplemented();
}

void EditorClient::updateSpellingUIWithMisspelledWord(const String&)
{
    notImplemented();
}

void EditorClient::showSpellingUI(bool)
{
    notImplemented();
}

bool EditorClient::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

bool EditorClient::supportsGlobalSelection()
{
#if PLATFORM(X11)
    return true;
#else
    return false;
#endif
}

}
