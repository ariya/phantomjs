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

#ifndef InjectedBundlePageFormClient_h
#define InjectedBundlePageFormClient_h

#include "APIClient.h"
#include "WKBundlePage.h"
#include <algorithm>
#include <wtf/Forward.h>
#include <wtf/Vector.h>

namespace WebCore {
class Element;
class HTMLFormElement;
class HTMLInputElement;
class HTMLTextAreaElement;
}

namespace WebKit {

class APIObject;
class ImmutableDictionary;
class WebFrame;
class WebPage;

class InjectedBundlePageFormClient : public APIClient<WKBundlePageFormClient, kWKBundlePageFormClientCurrentVersion> {
public:
    void didFocusTextField(WebPage*, WebCore::HTMLInputElement*, WebFrame*);
    void textFieldDidBeginEditing(WebPage*, WebCore::HTMLInputElement*, WebFrame*);
    void textFieldDidEndEditing(WebPage*, WebCore::HTMLInputElement*, WebFrame*);
    void textDidChangeInTextField(WebPage*, WebCore::HTMLInputElement*, WebFrame*);
    void textDidChangeInTextArea(WebPage*, WebCore::HTMLTextAreaElement*, WebFrame*);
    bool shouldPerformActionInTextField(WebPage*, WebCore::HTMLInputElement*, WKInputFieldActionType, WebFrame*);    
    void willSubmitForm(WebPage*, WebCore::HTMLFormElement*, WebFrame*, WebFrame* sourceFrame, const Vector<std::pair<String, String> >&, RefPtr<APIObject>& userData);
    void willSendSubmitEvent(WebPage*, WebCore::HTMLFormElement*, WebFrame*, WebFrame* sourceFrame, const Vector<std::pair<String, String> >&);
    void didAssociateFormControls(WebPage*, const Vector<RefPtr<WebCore::Element> >&);
    bool shouldNotifyOnFormChanges(WebPage*);
};

} // namespace WebKit

#endif // InjectedBundlePageFormClient_h
