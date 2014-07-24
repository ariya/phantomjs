/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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

#include "config.h"
#include "WebViewTest.h"
#include <wtf/gobject/GRefPtr.h>

class EditorTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(EditorTest);

    static const unsigned int kClipboardWaitTimeout = 50;
    static const unsigned int kClipboardWaitMaxTries = 2;

    EditorTest()
        : m_clipboard(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD))
        , m_canExecuteEditingCommand(false)
        , m_triesCount(0)
    {
        gtk_clipboard_clear(m_clipboard);
    }

    static void canExecuteEditingCommandReadyCallback(GObject*, GAsyncResult* result, EditorTest* test)
    {
        GOwnPtr<GError> error;
        test->m_canExecuteEditingCommand = webkit_web_view_can_execute_editing_command_finish(test->m_webView, result, &error.outPtr());
        g_assert(!error.get());
        g_main_loop_quit(test->m_mainLoop);
    }

    bool canExecuteEditingCommand(const char* command)
    {
        m_canExecuteEditingCommand = false;
        webkit_web_view_can_execute_editing_command(m_webView, command, 0, reinterpret_cast<GAsyncReadyCallback>(canExecuteEditingCommandReadyCallback), this);
        g_main_loop_run(m_mainLoop);
        return m_canExecuteEditingCommand;
    }

    static gboolean waitForClipboardText(EditorTest* test)
    {
        test->m_triesCount++;
        if (gtk_clipboard_wait_is_text_available(test->m_clipboard) || test->m_triesCount > kClipboardWaitMaxTries) {
            g_main_loop_quit(test->m_mainLoop);
            return FALSE;
        }

        return TRUE;
    }

    void copyClipboard()
    {
        webkit_web_view_execute_editing_command(m_webView, WEBKIT_EDITING_COMMAND_COPY);
        // There's no way to know when the selection has been copied to
        // the clipboard, so use a timeout source to query the clipboard.
        m_triesCount = 0;
        g_timeout_add(kClipboardWaitTimeout, reinterpret_cast<GSourceFunc>(waitForClipboardText), this);
        g_main_loop_run(m_mainLoop);
    }

    GtkClipboard* m_clipboard;
    bool m_canExecuteEditingCommand;
    size_t m_triesCount;
};

static void testWebViewEditorCutCopyPasteNonEditable(EditorTest* test, gconstpointer)
{
    static const char* selectedSpanHTML = "<html><body contentEditable=\"false\">"
        "<span id=\"mainspan\">All work and no play <span id=\"subspan\">make Jack a dull</span> boy.</span>"
        "<script>document.getSelection().collapse();\n"
        "document.getSelection().selectAllChildren(document.getElementById('subspan'));\n"
        "</script></body></html>";

    // Nothing loaded yet.
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_CUT));
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_COPY));
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_PASTE));

    test->loadHtml(selectedSpanHTML, 0);
    test->waitUntilLoadFinished();

    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_COPY));
    // It's not possible to cut and paste when content is not editable
    // even if there's a selection.
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_CUT));
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_PASTE));

    test->copyClipboard();
    GOwnPtr<char> clipboardText(gtk_clipboard_wait_for_text(test->m_clipboard));
    g_assert_cmpstr(clipboardText.get(), ==, "make Jack a dull");
}

static void testWebViewEditorCutCopyPasteEditable(EditorTest* test, gconstpointer)
{
    static const char* selectedSpanHTML = "<html><body contentEditable=\"true\">"
        "<span id=\"mainspan\">All work and no play <span>make Jack a dull</span> boy.</span>"
        "<script>document.getSelection().collapse();\n"
        "document.getSelection().selectAllChildren(document.getElementById('mainspan'));\n"
        "</script></body></html>";

    // Nothing loaded yet.
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_CUT));
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_COPY));
    g_assert(!test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_PASTE));

    test->loadHtml(selectedSpanHTML, 0);
    test->waitUntilLoadFinished();

    // There's a selection.
    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_CUT));
    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_COPY));
    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_PASTE));

    test->copyClipboard();
    GOwnPtr<char> clipboardText(gtk_clipboard_wait_for_text(test->m_clipboard));
    g_assert_cmpstr(clipboardText.get(), ==, "All work and no play make Jack a dull boy.");
}

static void testWebViewEditorSelectAllNonEditable(EditorTest* test, gconstpointer)
{
    static const char* selectedSpanHTML = "<html><body contentEditable=\"false\">"
        "<span id=\"mainspan\">All work and no play <span id=\"subspan\">make Jack a dull</span> boy.</span>"
        "<script>document.getSelection().collapse();\n"
        "document.getSelection().selectAllChildren(document.getElementById('subspan'));\n"
        "</script></body></html>";

    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_SELECT_ALL));

    test->loadHtml(selectedSpanHTML, 0);
    test->waitUntilLoadFinished();

    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_SELECT_ALL));

    test->copyClipboard();
    GOwnPtr<char> clipboardText(gtk_clipboard_wait_for_text(test->m_clipboard));

    // Initially only the subspan is selected.
    g_assert_cmpstr(clipboardText.get(), ==, "make Jack a dull");

    webkit_web_view_execute_editing_command(test->m_webView, WEBKIT_EDITING_COMMAND_SELECT_ALL);
    test->copyClipboard();
    clipboardText.set(gtk_clipboard_wait_for_text(test->m_clipboard));

    // The mainspan should be selected after calling SELECT_ALL.
    g_assert_cmpstr(clipboardText.get(), ==, "All work and no play make Jack a dull boy.");
}

static void testWebViewEditorSelectAllEditable(EditorTest* test, gconstpointer)
{
    static const char* selectedSpanHTML = "<html><body contentEditable=\"true\">"
        "<span id=\"mainspan\">All work and no play <span id=\"subspan\">make Jack a dull</span> boy.</span>"
        "<script>document.getSelection().collapse();\n"
        "document.getSelection().selectAllChildren(document.getElementById('subspan'));\n"
        "</script></body></html>";

    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_SELECT_ALL));

    test->loadHtml(selectedSpanHTML, 0);
    test->waitUntilLoadFinished();

    g_assert(test->canExecuteEditingCommand(WEBKIT_EDITING_COMMAND_SELECT_ALL));

    test->copyClipboard();
    GOwnPtr<char> clipboardText(gtk_clipboard_wait_for_text(test->m_clipboard));

    // Initially only the subspan is selected.
    g_assert_cmpstr(clipboardText.get(), ==, "make Jack a dull");

    webkit_web_view_execute_editing_command(test->m_webView, WEBKIT_EDITING_COMMAND_SELECT_ALL);
    test->copyClipboard();
    clipboardText.set(gtk_clipboard_wait_for_text(test->m_clipboard));

    // The mainspan should be selected after calling SELECT_ALL.
    g_assert_cmpstr(clipboardText.get(), ==, "All work and no play make Jack a dull boy.");
}

void beforeAll()
{
    EditorTest::add("WebKitWebView", "cut-copy-paste/non-editable", testWebViewEditorCutCopyPasteNonEditable);
    EditorTest::add("WebKitWebView", "cut-copy-paste/editable", testWebViewEditorCutCopyPasteEditable);
    EditorTest::add("WebKitWebView", "select-all/non-editable", testWebViewEditorSelectAllNonEditable);
    EditorTest::add("WebKitWebView", "select-all/editable", testWebViewEditorSelectAllEditable);
}

void afterAll()
{
}
