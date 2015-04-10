/*
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DumpHistoryItem.h"

#include "DumpRenderTree.h"
#include "DumpRenderTreeChrome.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"
#include "ewk_private.h"
#include <EWebKit.h>
#include <algorithm>
#include <cstdio>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

Ewk_History_Item* prevTestBFItem = 0;
const unsigned historyItemIndent = 8;

static bool compareHistoryItemsByTarget(const Ewk_History_Item* item1, const Ewk_History_Item* item2)
{
    return WTF::codePointCompare(DumpRenderTreeSupportEfl::historyItemTarget(item1),
                                 DumpRenderTreeSupportEfl::historyItemTarget(item2)) < 1;
}

static void dumpHistoryItem(const Ewk_History_Item* item, int indent, bool current)
{
    ASSERT(item);
    int start = 0;
    if (current) {
        printf("curr->");
        start = 6;
    }
    for (int i = start; i < indent; i++)
        putchar(' ');

    // normalize file URLs.
    const char* uri = ewk_history_item_uri_get(item);
    if (!strncasecmp(uri, "file://", sizeof("file://") - 1)) {
        const char* pos = strstr(uri, "/LayoutTests/");
        if (!pos)
            return;

        printf("(file test):%s", pos + sizeof("/LayoutTests/") - 1);
    } else
        printf("%s", uri);

    const String target = DumpRenderTreeSupportEfl::historyItemTarget(item);
    if (!target.isEmpty())
        printf(" (in frame \"%s\")", target.utf8().data());
    if (DumpRenderTreeSupportEfl::isTargetItem(item))
        printf("  **nav target**");
    putchar('\n');

    HistoryItemChildrenVector children = DumpRenderTreeSupportEfl::childHistoryItems(item);

    // Must sort to eliminate arbitrary result ordering which defeats reproducible testing.
    std::stable_sort(children.begin(), children.end(), compareHistoryItemsByTarget);

    const size_t size = children.size();
    for (size_t i = 0; i < size; ++i)
        dumpHistoryItem(children[i], indent + 4, false);
}

static void dumpBackForwardListForWebView(Evas_Object* view)
{
    printf("\n============== Back Forward List ==============\n");

    const Ewk_History* history = ewk_view_history_get(view);

    // Print out all items in the list after prevTestBFItem, which was from the previous test
    // Gather items from the end of the list, the print them out from oldest to newest
    Eina_List* itemsToPrint = 0;
    void* historyItem;
    Eina_List* backwardList = ewk_history_back_list_get(history);
    EINA_LIST_FREE(backwardList, historyItem) {
        if (historyItem == prevTestBFItem) {
            eina_list_free(backwardList);
            break;
        }
        itemsToPrint = eina_list_append(itemsToPrint, historyItem);
    }

    const Ewk_History_Item* currentItem = ewk_history_history_item_current_get(history);
    if (currentItem)
        itemsToPrint = eina_list_append(itemsToPrint, currentItem);

    Eina_List* forwardList = ewk_history_forward_list_get(history);
    EINA_LIST_FREE(forwardList, historyItem) {
        ASSERT(historyItem != prevTestBFItem);
        itemsToPrint = eina_list_append(itemsToPrint, historyItem);
    }

    EINA_LIST_FREE(itemsToPrint, historyItem) {
        dumpHistoryItem(static_cast<Ewk_History_Item*>(historyItem), historyItemIndent, historyItem == currentItem);
        ewk_history_item_free(static_cast<Ewk_History_Item*>(historyItem));
    }

    printf("===============================================\n");
}

void dumpBackForwardListForWebViews()
{
    // Dump the back forward list of the main WebView first
    dumpBackForwardListForWebView(browser->mainView());

    Vector<Evas_Object*>::const_iterator it = browser->extraViews().begin();
    for (; it != browser->extraViews().end(); ++it)
        dumpBackForwardListForWebView(*it);
}
