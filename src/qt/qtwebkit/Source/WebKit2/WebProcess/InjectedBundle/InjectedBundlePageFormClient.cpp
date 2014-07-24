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
#include "InjectedBundlePageFormClient.h"

#include "ImmutableArray.h"
#include "ImmutableDictionary.h"
#include "InjectedBundleNodeHandle.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLTextAreaElement.h>

using namespace WebCore;

namespace WebKit {

void InjectedBundlePageFormClient::didFocusTextField(WebPage* page, HTMLInputElement* inputElement, WebFrame* frame)
{
    if (!m_client.didFocusTextField)
        return;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(inputElement);
    m_client.didFocusTextField(toAPI(page), toAPI(nodeHandle.get()), toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageFormClient::textFieldDidBeginEditing(WebPage* page, HTMLInputElement* inputElement, WebFrame* frame)
{
    if (!m_client.textFieldDidBeginEditing)
        return;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(inputElement);
    m_client.textFieldDidBeginEditing(toAPI(page), toAPI(nodeHandle.get()), toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageFormClient::textFieldDidEndEditing(WebPage* page, HTMLInputElement* inputElement, WebFrame* frame)
{
    if (!m_client.textFieldDidEndEditing)
        return;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(inputElement);
    m_client.textFieldDidEndEditing(toAPI(page), toAPI(nodeHandle.get()), toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageFormClient::textDidChangeInTextField(WebPage* page, HTMLInputElement* inputElement, WebFrame* frame)
{
    if (!m_client.textDidChangeInTextField)
        return;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(inputElement);
    m_client.textDidChangeInTextField(toAPI(page), toAPI(nodeHandle.get()), toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageFormClient::textDidChangeInTextArea(WebPage* page, HTMLTextAreaElement* textAreaElement, WebFrame* frame)
{
    if (!m_client.textDidChangeInTextArea)
        return;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(textAreaElement);
    m_client.textDidChangeInTextArea(toAPI(page), toAPI(nodeHandle.get()), toAPI(frame), m_client.clientInfo);
}

bool InjectedBundlePageFormClient::shouldPerformActionInTextField(WebPage* page, HTMLInputElement* inputElement, WKInputFieldActionType actionType, WebFrame* frame)
{
    if (!m_client.shouldPerformActionInTextField)
        return false;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(inputElement);
    return m_client.shouldPerformActionInTextField(toAPI(page), toAPI(nodeHandle.get()), actionType, toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageFormClient::willSendSubmitEvent(WebPage* page, HTMLFormElement* formElement, WebFrame* frame, WebFrame* sourceFrame, const Vector<std::pair<String, String> >& values)
{
    if (!m_client.willSendSubmitEvent)
        return;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(formElement);

    ImmutableDictionary::MapType map;
    for (size_t i = 0; i < values.size(); ++i)
        map.set(values[i].first, WebString::create(values[i].second));
    RefPtr<ImmutableDictionary> textFieldsMap = ImmutableDictionary::adopt(map);

    m_client.willSendSubmitEvent(toAPI(page), toAPI(nodeHandle.get()), toAPI(frame), toAPI(sourceFrame), toAPI(textFieldsMap.get()), m_client.clientInfo);
}

void InjectedBundlePageFormClient::willSubmitForm(WebPage* page, HTMLFormElement* formElement, WebFrame* frame, WebFrame* sourceFrame, const Vector<std::pair<String, String> >& values, RefPtr<APIObject>& userData)
{
    if (!m_client.willSubmitForm)
        return;

    RefPtr<InjectedBundleNodeHandle> nodeHandle = InjectedBundleNodeHandle::getOrCreate(formElement);

    ImmutableDictionary::MapType map;
    for (size_t i = 0; i < values.size(); ++i)
        map.set(values[i].first, WebString::create(values[i].second));
    RefPtr<ImmutableDictionary> textFieldsMap = ImmutableDictionary::adopt(map);

    WKTypeRef userDataToPass = 0;
    m_client.willSubmitForm(toAPI(page), toAPI(nodeHandle.get()), toAPI(frame), toAPI(sourceFrame), toAPI(textFieldsMap.get()), &userDataToPass, m_client.clientInfo);
    userData = adoptRef(toImpl(userDataToPass));
}

void InjectedBundlePageFormClient::didAssociateFormControls(WebPage* page, const Vector<RefPtr<WebCore::Element> >& elements)
{
    if (!m_client.didAssociateFormControls)
        return;

    size_t size = elements.size();

    Vector<RefPtr<APIObject> > elementHandles;
    elementHandles.reserveCapacity(size);

    for (size_t i = 0; i < size; ++i)
        elementHandles.uncheckedAppend(InjectedBundleNodeHandle::getOrCreate(elements[i].get()).get());

    m_client.didAssociateFormControls(toAPI(page), toAPI(ImmutableArray::adopt(elementHandles).get()), m_client.clientInfo);
}

bool InjectedBundlePageFormClient::shouldNotifyOnFormChanges(WebPage* page)
{
    if (!m_client.shouldNotifyOnFormChanges)
        return false;

    return m_client.shouldNotifyOnFormChanges(toAPI(page), m_client.clientInfo);
}

} // namespace WebKit
