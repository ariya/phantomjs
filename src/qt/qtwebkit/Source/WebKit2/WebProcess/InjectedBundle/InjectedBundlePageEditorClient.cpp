/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InjectedBundlePageEditorClient.h"

#include "ImmutableArray.h"
#include "InjectedBundleNodeHandle.h"
#include "InjectedBundleRangeHandle.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WKString.h"
#include "WebData.h"
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

bool InjectedBundlePageEditorClient::shouldBeginEditing(WebPage* page, Range* range)
{
    if (m_client.shouldBeginEditing) {
        RefPtr<InjectedBundleRangeHandle> rangeHandle = InjectedBundleRangeHandle::getOrCreate(range);
        return m_client.shouldBeginEditing(toAPI(page), toAPI(rangeHandle.get()), m_client.clientInfo);
    }
    return true;
}

bool InjectedBundlePageEditorClient::shouldEndEditing(WebPage* page, Range* range)
{
    if (m_client.shouldEndEditing) {
        RefPtr<InjectedBundleRangeHandle> rangeHandle = InjectedBundleRangeHandle::getOrCreate(range);
        return m_client.shouldEndEditing(toAPI(page), toAPI(rangeHandle.get()), m_client.clientInfo);
    }
    return true;
}

bool InjectedBundlePageEditorClient::shouldInsertNode(WebPage* page, Node* node, Range* rangeToReplace, EditorInsertAction action)
{
    if (m_client.shouldInsertNode) {
        RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(node);
        RefPtr<InjectedBundleRangeHandle> rangeToReplaceHandle = InjectedBundleRangeHandle::getOrCreate(rangeToReplace);
        return m_client.shouldInsertNode(toAPI(page), toAPI(nodeHandle.get()), toAPI(rangeToReplaceHandle.get()), toAPI(action), m_client.clientInfo);
    }
    return true;
}

bool InjectedBundlePageEditorClient::shouldInsertText(WebPage* page, StringImpl* text, Range* rangeToReplace, EditorInsertAction action)
{
    if (m_client.shouldInsertText) {
        RefPtr<InjectedBundleRangeHandle> rangeToReplaceHandle = InjectedBundleRangeHandle::getOrCreate(rangeToReplace);
        return m_client.shouldInsertText(toAPI(page), toAPI(text), toAPI(rangeToReplaceHandle.get()), toAPI(action), m_client.clientInfo);
    }
    return true;
}

bool InjectedBundlePageEditorClient::shouldDeleteRange(WebPage* page, Range* range)
{
    if (m_client.shouldDeleteRange) {
        RefPtr<InjectedBundleRangeHandle> rangeHandle = InjectedBundleRangeHandle::getOrCreate(range);
        return m_client.shouldDeleteRange(toAPI(page), toAPI(rangeHandle.get()), m_client.clientInfo);
    }
    return true;
}

bool InjectedBundlePageEditorClient::shouldChangeSelectedRange(WebPage* page, Range* fromRange, Range* toRange, EAffinity affinity, bool stillSelecting)
{
    if (m_client.shouldChangeSelectedRange) {
        RefPtr<InjectedBundleRangeHandle> fromRangeHandle = InjectedBundleRangeHandle::getOrCreate(fromRange);
        RefPtr<InjectedBundleRangeHandle> toRangeHandle = InjectedBundleRangeHandle::getOrCreate(toRange);
        return m_client.shouldChangeSelectedRange(toAPI(page), toAPI(fromRangeHandle.get()), toAPI(toRangeHandle.get()), toAPI(affinity), stillSelecting, m_client.clientInfo);
    }
    return true;
}

bool InjectedBundlePageEditorClient::shouldApplyStyle(WebPage* page, CSSStyleDeclaration* style, Range* range)
{
    if (m_client.shouldApplyStyle) {
        RefPtr<InjectedBundleRangeHandle> rangeHandle = InjectedBundleRangeHandle::getOrCreate(range);
        return m_client.shouldApplyStyle(toAPI(page), toAPI(style), toAPI(rangeHandle.get()), m_client.clientInfo);
    }
    return true;
}

void InjectedBundlePageEditorClient::didBeginEditing(WebPage* page, StringImpl* notificationName)
{
    if (m_client.didBeginEditing)
        m_client.didBeginEditing(toAPI(page), toAPI(notificationName), m_client.clientInfo);
}

void InjectedBundlePageEditorClient::didEndEditing(WebPage* page, StringImpl* notificationName)
{
    if (m_client.didEndEditing)
        m_client.didEndEditing(toAPI(page), toAPI(notificationName), m_client.clientInfo);
}

void InjectedBundlePageEditorClient::didChange(WebPage* page, StringImpl* notificationName)
{
    if (m_client.didChange)
        m_client.didChange(toAPI(page), toAPI(notificationName), m_client.clientInfo);
}

void InjectedBundlePageEditorClient::didChangeSelection(WebPage* page, StringImpl* notificationName)
{
    if (m_client.didChangeSelection)
        m_client.didChangeSelection(toAPI(page), toAPI(notificationName), m_client.clientInfo);
}

void InjectedBundlePageEditorClient::willWriteToPasteboard(WebPage* page, Range* range)
{
    if (m_client.willWriteToPasteboard) {
        RefPtr<InjectedBundleRangeHandle> rangeHandle = InjectedBundleRangeHandle::getOrCreate(range);
        m_client.willWriteToPasteboard(toAPI(page), toAPI(rangeHandle.get()), m_client.clientInfo);
    }
}

void InjectedBundlePageEditorClient::getPasteboardDataForRange(WebPage* page, Range* range, Vector<String>& pasteboardTypes, Vector<RefPtr<SharedBuffer> >& pasteboardData)
{
    if (m_client.getPasteboardDataForRange) {
        RefPtr<InjectedBundleRangeHandle> rangeHandle = InjectedBundleRangeHandle::getOrCreate(range);
        WKArrayRef types = 0;
        WKArrayRef data = 0;
        m_client.getPasteboardDataForRange(toAPI(page), toAPI(rangeHandle.get()), &types, &data, m_client.clientInfo);
        RefPtr<ImmutableArray> typesArray = adoptRef(toImpl(types));
        RefPtr<ImmutableArray> dataArray = adoptRef(toImpl(data));

        pasteboardTypes.clear();
        pasteboardData.clear();

        if (!typesArray || !dataArray)
            return;

        ASSERT(typesArray->size() == dataArray->size());

        size_t size = typesArray->size();
        for (size_t i = 0; i < size; ++i) {
            WebString* type = typesArray->at<WebString>(i);
            if (type)
                pasteboardTypes.append(type->string());
        }

        size = dataArray->size();
        for (size_t i = 0; i < size; ++i) {
            WebData* item = dataArray->at<WebData>(i);
            if (item) {
                RefPtr<SharedBuffer> buffer = SharedBuffer::create(item->bytes(), item->size());
                pasteboardData.append(buffer);
            }
        }
    }
}

void InjectedBundlePageEditorClient::didWriteToPasteboard(WebPage* page)
{
    if (m_client.didWriteToPasteboard)
        m_client.didWriteToPasteboard(toAPI(page), m_client.clientInfo);
}

} // namespace WebKit
