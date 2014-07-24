/*
 * Copyright (C) 2010 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EditingCallbacks.h"

#include "DumpRenderTree.h"
#include "TestRunner.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

static CString dumpNodePath(WebKitDOMNode* node)
{
    GOwnPtr<gchar> nodeName(webkit_dom_node_get_node_name(node));
    GString* path = g_string_new(nodeName.get());

    WebKitDOMNode* parent = webkit_dom_node_get_parent_node(node);
    while (parent) {
        GOwnPtr<gchar> parentName(webkit_dom_node_get_node_name(parent));

        g_string_append(path, " > ");
        g_string_append(path, parentName.get());
        parent = webkit_dom_node_get_parent_node(parent);
    }

    GOwnPtr<gchar> pathBuffer(g_string_free(path, FALSE));
    return pathBuffer.get();
}

static CString dumpRange(WebKitDOMRange* range)
{
    if (!range)
        return "(null)";

    GOwnPtr<gchar> dump(g_strdup_printf("range from %li of %s to %li of %s",
        webkit_dom_range_get_start_offset(range, 0),
        dumpNodePath(webkit_dom_range_get_start_container(range, 0)).data(),
        webkit_dom_range_get_end_offset(range, 0),
        dumpNodePath(webkit_dom_range_get_end_container(range, 0)).data()));

    return dump.get();
}

static const char* insertActionString(WebKitInsertAction action)
{
    switch (action) {
    case WEBKIT_INSERT_ACTION_TYPED:
        return "WebViewInsertActionTyped";
    case WEBKIT_INSERT_ACTION_PASTED:
        return "WebViewInsertActionPasted";
    case WEBKIT_INSERT_ACTION_DROPPED:
        return "WebViewInsertActionDropped";
    }
    ASSERT_NOT_REACHED();
    return "WebViewInsertActionTyped";
}

static const char* selectionAffinityString(WebKitSelectionAffinity affinity)
{
    switch (affinity) {
    case WEBKIT_SELECTION_AFFINITY_UPSTREAM:
        return "NSSelectionAffinityUpstream";
    case WEBKIT_SELECTION_AFFINITY_DOWNSTREAM:
        return "NSSelectionAffinityDownstream";
    }
    ASSERT_NOT_REACHED();
    return "NSSelectionAffinityUpstream";
}

gboolean shouldBeginEditing(WebKitWebView* webView, WebKitDOMRange* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldBeginEditingInDOMRange:%s\n", dumpRange(range).data());
    return TRUE;
}

gboolean shouldEndEditing(WebKitWebView* webView, WebKitDOMRange* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldEndEditingInDOMRange:%s\n", dumpRange(range).data());
    return TRUE;
}

gboolean shouldInsertNode(WebKitWebView* webView, WebKitDOMNode* node, WebKitDOMRange* range, WebKitInsertAction action)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldInsertNode:%s replacingDOMRange:%s givenAction:%s\n",
               dumpNodePath(node).data(), dumpRange(range).data(), insertActionString(action));
    }
    return TRUE;
}

gboolean shouldInsertText(WebKitWebView* webView, const gchar* text, WebKitDOMRange* range, WebKitInsertAction action)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldInsertText:%s replacingDOMRange:%s givenAction:%s\n",
               text, dumpRange(range).data(), insertActionString(action));
    }
    return TRUE;
}

gboolean shouldDeleteRange(WebKitWebView* webView, WebKitDOMRange* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldDeleteDOMRange:%s\n", dumpRange(range).data());
    return TRUE;
}

gboolean shouldShowDeleteInterfaceForElement(WebKitWebView* webView, WebKitDOMHTMLElement* element)
{
    return FALSE;
}

gboolean shouldChangeSelectedRange(WebKitWebView* webView, WebKitDOMRange* fromRange, WebKitDOMRange* toRange, WebKitSelectionAffinity affinity, gboolean stillSelecting)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldChangeSelectedDOMRange:%s toDOMRange:%s affinity:%s stillSelecting:%s\n",
               dumpRange(fromRange).data(), dumpRange(toRange).data(), selectionAffinityString(affinity),
               stillSelecting ? "TRUE" : "FALSE");
    }
    return TRUE;
}

gboolean shouldApplyStyle(WebKitWebView* webView, WebKitDOMCSSStyleDeclaration* style, WebKitDOMRange* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        GOwnPtr<gchar> styleText(webkit_dom_css_style_declaration_get_css_text(style));
        printf("EDITING DELEGATE: shouldApplyStyle:%s toElementsInDOMRange:%s\n",
               styleText.get(), dumpRange(range).data());
    }
    return TRUE;
}

void editingBegan(WebKitWebView*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidBeginEditing:WebViewDidBeginEditingNotification\n");
}

void userChangedContents(WebKitWebView*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChange:WebViewDidChangeNotification\n");
}

void editingEnded(WebKitWebView*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidEndEditing:WebViewDidEndEditingNotification\n");
}

void selectionChanged(WebKitWebView*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChangeSelection:WebViewDidChangeSelectionNotification\n");
}

void connectEditingCallbacks(WebKitWebView* webView)
{
    g_object_connect(G_OBJECT(webView),
                     "signal::should-begin-editing", shouldBeginEditing, 0,
                     "signal::should-end-editing", shouldEndEditing, 0,
                     "signal::should-insert-node", shouldInsertNode, 0,
                     "signal::should-insert-text", shouldInsertText, 0,
                     "signal::should-delete-range", shouldDeleteRange, 0,
                     "signal::should-show-delete-interface-for-element", shouldShowDeleteInterfaceForElement, 0,
                     "signal::should-change-selected-range", shouldChangeSelectedRange, 0,
                     "signal::should-apply-style", shouldApplyStyle, 0,
                     "signal::editing-began", editingBegan, 0,
                     "signal::user-changed-contents", userChangedContents, 0,
                     "signal::editing-ended", editingEnded, 0,
                     "signal::selection-changed", selectionChanged, 0,
                     NULL);
}

