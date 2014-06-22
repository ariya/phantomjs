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

#ifndef InjectedBundlePageEditorClient_h
#define InjectedBundlePageEditorClient_h

#include "APIClient.h"
#include "WKBundlePage.h"
#include <WebCore/EditorInsertAction.h>
#include <WebCore/SharedBuffer.h>
#include <WebCore/TextAffinity.h>
#include <wtf/Forward.h>

namespace WebCore {
    class CSSStyleDeclaration;
    class Node;
    class Range;
    class SharedBuffer;
}

namespace WebKit {

class WebFrame;
class WebPage;

class InjectedBundlePageEditorClient : public APIClient<WKBundlePageEditorClient, kWKBundlePageEditorClientCurrentVersion> {
public:
    bool shouldBeginEditing(WebPage*, WebCore::Range*);
    bool shouldEndEditing(WebPage*, WebCore::Range*);
    bool shouldInsertNode(WebPage*, WebCore::Node*, WebCore::Range* rangeToReplace, WebCore::EditorInsertAction);
    bool shouldInsertText(WebPage*, StringImpl*, WebCore::Range* rangeToReplace, WebCore::EditorInsertAction);
    bool shouldDeleteRange(WebPage*, WebCore::Range*);
    bool shouldChangeSelectedRange(WebPage*, WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity affinity, bool stillSelecting);
    bool shouldApplyStyle(WebPage*, WebCore::CSSStyleDeclaration*, WebCore::Range*);
    void didBeginEditing(WebPage*, StringImpl* notificationName);
    void didEndEditing(WebPage*, StringImpl* notificationName);
    void didChange(WebPage*, StringImpl* notificationName);
    void didChangeSelection(WebPage*, StringImpl* notificationName);
    void willWriteToPasteboard(WebPage*, WebCore::Range*);
    void getPasteboardDataForRange(WebPage*, WebCore::Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData);
    void didWriteToPasteboard(WebPage*);
};

} // namespace WebKit

#endif // InjectedBundlePageEditorClient_h
