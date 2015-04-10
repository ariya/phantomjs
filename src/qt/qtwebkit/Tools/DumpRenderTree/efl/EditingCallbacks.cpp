/*
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2012 Samsung Electronics
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
#include "EditorClientEfl.h"
#include "EditorInsertAction.h"
#include "Node.h"
#include "Range.h"
#include "StylePropertySet.h"
#include "TestRunner.h"
#include "TextAffinity.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

static WTF::String dumpPath(WebCore::Node* node)
{
    WebCore::Node* parent = node->parentNode();
    WTF::String str = WTF::String::format("%s", node->nodeName().utf8().data());
    if (parent) {
        str.append(" > ");
        str.append(dumpPath(parent));
    }
    return str;
}

static WTF::String dumpRange(WebCore::Range* range)
{
    if (!range)
        return "(null)";
    return WTF::String::format("range from %d of %s to %d of %s", range->startOffset(), dumpPath(range->startContainer()).utf8().data(), range->endOffset(), dumpPath(range->endContainer()).utf8().data());
}

static const char* insertActionString(WebCore::EditorInsertAction action)
{
    switch (action) {
    case WebCore::EditorInsertActionTyped:
        return "WebViewInsertActionTyped";
    case WebCore::EditorInsertActionPasted:
        return "WebViewInsertActionPasted";
    case WebCore::EditorInsertActionDropped:
        return "WebViewInsertActionDropped";
    }
    ASSERT_NOT_REACHED();
    return "WebViewInsertActionTyped";
}

static const char* selectionAffinityString(WebCore::EAffinity affinity)
{
    switch (affinity) {
    case WebCore::UPSTREAM:
        return "NSSelectionAffinityUpstream";
    case WebCore::DOWNSTREAM:
        return "NSSelectionAffinityDownstream";
    }
    ASSERT_NOT_REACHED();
    return "NSSelectionAffinityUpstream";
}

void shouldBeginEditing(void*, Evas_Object*, void* eventInfo)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        WebCore::Range* range = static_cast<WebCore::Range*>(eventInfo);
        printf("EDITING DELEGATE: shouldBeginEditingInDOMRange:%s\n", dumpRange(range).utf8().data());
    }
}

void shouldEndEditing(void*, Evas_Object*, void* eventInfo)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        WebCore::Range* range = static_cast<WebCore::Range*>(eventInfo);
        printf("EDITING DELEGATE: shouldEndEditingInDOMRange:%s\n", dumpRange(range).utf8().data());
    }
}

void shouldInsertNode(void*, Evas_Object*, void* eventInfo)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        Ewk_Should_Insert_Node_Event* shouldInsertNodeEvent = static_cast<Ewk_Should_Insert_Node_Event*>(eventInfo);
        printf("EDITING DELEGATE: shouldInsertNode:%s replacingDOMRange:%s givenAction:%s\n",
               dumpPath(shouldInsertNodeEvent->node).utf8().data(), dumpRange(shouldInsertNodeEvent->range).utf8().data(),
               insertActionString(shouldInsertNodeEvent->action));
    }
}

void shouldInsertText(void*, Evas_Object*, void* eventInfo)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        Ewk_Should_Insert_Text_Event* shouldInsertTextEvent = static_cast<Ewk_Should_Insert_Text_Event*>(eventInfo);
        printf("EDITING DELEGATE: shouldInsertText:%s replacingDOMRange:%s givenAction:%s\n",
                shouldInsertTextEvent->text, dumpRange(shouldInsertTextEvent->range).utf8().data(), insertActionString(shouldInsertTextEvent->action));
    }
}

void shouldDeleteRange(void*, Evas_Object*, void* eventInfo)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        WebCore::Range* range = static_cast<WebCore::Range*>(eventInfo);
        printf("EDITING DELEGATE: shouldDeleteDOMRange:%s\n", dumpRange(range).utf8().data());
    }
}

void shouldChangeSelectedRange(void*, Evas_Object*, void* eventInfo)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        Ewk_Should_Change_Selected_Range_Event* shouldChangeSelectedRangeEvent = static_cast<Ewk_Should_Change_Selected_Range_Event*>(eventInfo);
        printf("EDITING DELEGATE: shouldChangeSelectedDOMRange:%s toDOMRange:%s affinity:%s stillSelecting:%s\n",
               dumpRange(shouldChangeSelectedRangeEvent->fromRange).utf8().data(), dumpRange(shouldChangeSelectedRangeEvent->toRange).utf8().data(),
               selectionAffinityString(shouldChangeSelectedRangeEvent->affinity), shouldChangeSelectedRangeEvent->stillSelecting ? "TRUE" : "FALSE");
    }
}

void shouldApplyStyle(void*, Evas_Object*, void* eventInfo)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        Ewk_Should_Apply_Style_Event* shouldApplyStyleEvent = static_cast<Ewk_Should_Apply_Style_Event*>(eventInfo);
        printf("EDITING DELEGATE: shouldApplyStyle:%s toElementsInDOMRange:%s\n",
                shouldApplyStyleEvent->style->asText().utf8().data(), dumpRange(shouldApplyStyleEvent->range).utf8().data());
    }
}

void editingBegan(void*, Evas_Object*, void*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidBeginEditing:WebViewDidBeginEditingNotification\n");
}

void userChangedContents(void*, Evas_Object*, void*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChange:WebViewDidChangeNotification\n");
}

void editingEnded(void*, Evas_Object*, void*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidEndEditing:WebViewDidEndEditingNotification\n");
}

void selectionChanged(void*, Evas_Object*, void*)
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChangeSelection:WebViewDidChangeSelectionNotification\n");
}

void connectEditingCallbacks(Evas_Object* webView)
{
    evas_object_smart_callback_add(webView, "editorclient,editing,begin", shouldBeginEditing, 0);
    evas_object_smart_callback_add(webView, "editorclient,editing,end", shouldEndEditing, 0);
    evas_object_smart_callback_add(webView, "editorclient,node,insert", shouldInsertNode, 0);
    evas_object_smart_callback_add(webView, "editorclient,text,insert", shouldInsertText, 0);
    evas_object_smart_callback_add(webView, "editorclient,range,delete", shouldDeleteRange, 0);
    evas_object_smart_callback_add(webView, "editorclient,selected,range,change", shouldChangeSelectedRange, 0);
    evas_object_smart_callback_add(webView, "editorclient,style,apply", shouldApplyStyle, 0);
    evas_object_smart_callback_add(webView, "editorclient,editing,began", editingBegan, 0);
    evas_object_smart_callback_add(webView, "editorclient,contents,changed", userChangedContents, 0);
    evas_object_smart_callback_add(webView, "editorclient,editing,ended", editingEnded, 0);
    evas_object_smart_callback_add(webView, "editorclient,selection,changed", selectionChanged, 0);
}
