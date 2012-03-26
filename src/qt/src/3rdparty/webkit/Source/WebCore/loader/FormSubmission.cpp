/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FormSubmission.h"

#include "DOMFormData.h"
#include "Document.h"
#include "Event.h"
#include "FormData.h"
#include "FormDataBuilder.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "TextEncoding.h"
#include <wtf/CurrentTime.h>
#include <wtf/RandomNumber.h>

namespace WebCore {

using namespace HTMLNames;

static int64_t generateFormDataIdentifier()
{
    // Initialize to the current time to reduce the likelihood of generating
    // identifiers that overlap with those from past/future browser sessions.
    static int64_t nextIdentifier = static_cast<int64_t>(currentTime() * 1000000.0);
    return ++nextIdentifier;
}

static void appendMailtoPostFormDataToURL(KURL& url, const FormData& data, const String& encodingType)
{
    String body = data.flattenToString();

    if (equalIgnoringCase(encodingType, "text/plain")) {
        // Convention seems to be to decode, and s/&/\r\n/. Also, spaces are encoded as %20.
        body = decodeURLEscapeSequences(body.replace('&', "\r\n").replace('+', ' ') + "\r\n");
    }

    Vector<char> bodyData;
    bodyData.append("body=", 5);
    FormDataBuilder::encodeStringAsFormData(bodyData, body.utf8());
    body = String(bodyData.data(), bodyData.size()).replace('+', "%20");

    String query = url.query();
    if (!query.isEmpty())
        query.append('&');
    query.append(body);
    url.setQuery(query);
}

void FormSubmission::Attributes::parseAction(const String& action)
{
    // FIXME: Can we parse into a KURL?
    m_action = stripLeadingAndTrailingHTMLSpaces(action);
}

void FormSubmission::Attributes::parseEncodingType(const String& type)
{
    if (type.contains("multipart", false) || type.contains("form-data", false)) {
        m_encodingType = "multipart/form-data";
        m_isMultiPartForm = true;
    } else if (type.contains("text", false) || type.contains("plain", false)) {
        m_encodingType = "text/plain";
        m_isMultiPartForm = false;
    } else {
        m_encodingType = "application/x-www-form-urlencoded";
        m_isMultiPartForm = false;
    }
}

void FormSubmission::Attributes::parseMethodType(const String& type)
{
    if (equalIgnoringCase(type, "post"))
        m_method = FormSubmission::PostMethod;
    else if (equalIgnoringCase(type, "get"))
        m_method = FormSubmission::GetMethod;
}

void FormSubmission::Attributes::copyFrom(const Attributes& other)
{
    m_method = other.m_method;
    m_isMultiPartForm = other.m_isMultiPartForm;

    m_action = other.m_action;
    m_target = other.m_target;
    m_encodingType = other.m_encodingType;
    m_acceptCharset = other.m_acceptCharset;
}

inline FormSubmission::FormSubmission(Method method, const KURL& action, const String& target, const String& contentType, PassRefPtr<FormState> state, PassRefPtr<FormData> data, const String& boundary, bool lockHistory, PassRefPtr<Event> event)
    : m_method(method)
    , m_action(action)
    , m_target(target)
    , m_contentType(contentType)
    , m_formState(state)
    , m_formData(data)
    , m_boundary(boundary)
    , m_lockHistory(lockHistory)
    , m_event(event)
{
}

PassRefPtr<FormSubmission> FormSubmission::create(HTMLFormElement* form, const Attributes& attributes, PassRefPtr<Event> event, bool lockHistory, FormSubmissionTrigger trigger)
{
    ASSERT(form);

    HTMLFormControlElement* submitButton = 0;
    if (event && event->target() && event->target()->toNode())
        submitButton = static_cast<HTMLFormControlElement*>(event->target()->toNode());

    FormSubmission::Attributes copiedAttributes;
    copiedAttributes.copyFrom(attributes);
    if (submitButton) {
        String attributeValue;
        if (!(attributeValue = submitButton->getAttribute(formactionAttr)).isNull())
            copiedAttributes.parseAction(attributeValue);
        if (!(attributeValue = submitButton->getAttribute(formenctypeAttr)).isNull())
            copiedAttributes.parseEncodingType(attributeValue);
        if (!(attributeValue = submitButton->getAttribute(formmethodAttr)).isNull())
            copiedAttributes.parseMethodType(attributeValue);
        if (!(attributeValue = submitButton->getAttribute(formtargetAttr)).isNull())
            copiedAttributes.setTarget(attributeValue);
    }
    
    Document* document = form->document();
    KURL actionURL = document->completeURL(copiedAttributes.action().isEmpty() ? document->url().string() : copiedAttributes.action());
    bool isMailtoForm = actionURL.protocolIs("mailto");
    bool isMultiPartForm = false;
    String encodingType = copiedAttributes.encodingType();

    if (copiedAttributes.method() == PostMethod) {
        isMultiPartForm = copiedAttributes.isMultiPartForm();
        if (isMultiPartForm && isMailtoForm) {
            encodingType = "application/x-www-form-urlencoded";
            isMultiPartForm = false;
        }
    }

    TextEncoding dataEncoding = isMailtoForm ? UTF8Encoding() : FormDataBuilder::encodingFromAcceptCharset(copiedAttributes.acceptCharset(), document);
    RefPtr<DOMFormData> domFormData = DOMFormData::create(dataEncoding.encodingForFormSubmission());
    Vector<pair<String, String> > formValues;

    for (unsigned i = 0; i < form->associatedElements().size(); ++i) {
        FormAssociatedElement* control = form->associatedElements()[i];
        HTMLElement* element = toHTMLElement(control);
        if (!element->disabled())
            control->appendFormData(*domFormData, isMultiPartForm);
        if (element->hasLocalName(inputTag)) {
            HTMLInputElement* input = static_cast<HTMLInputElement*>(control);
            if (input->isTextField()) {
                formValues.append(pair<String, String>(input->name().string(), input->value()));
                if (input->isSearchField())
                    input->addSearchResult();
            }
        }
    }

    RefPtr<FormData> formData;
    String boundary;

    if (isMultiPartForm) {
        formData = FormData::createMultiPart(*(static_cast<FormDataList*>(domFormData.get())), domFormData->encoding(), document);
        boundary = formData->boundary().data();
    } else {
        formData = FormData::create(*(static_cast<FormDataList*>(domFormData.get())), domFormData->encoding());
        if (copiedAttributes.method() == PostMethod && isMailtoForm) {
            // Convert the form data into a string that we put into the URL.
            appendMailtoPostFormDataToURL(actionURL, *formData, encodingType);
            formData = FormData::create();
        }
    }

    formData->setIdentifier(generateFormDataIdentifier());
    String targetOrBaseTarget = copiedAttributes.target().isEmpty() ? document->baseTarget() : copiedAttributes.target();
    RefPtr<FormState> formState = FormState::create(form, formValues, document->frame(), trigger);
    return adoptRef(new FormSubmission(copiedAttributes.method(), actionURL, targetOrBaseTarget, encodingType, formState.release(), formData.release(), boundary, lockHistory, event));
}

KURL FormSubmission::requestURL() const
{
    if (m_method == FormSubmission::PostMethod)
        return m_action;

    KURL requestURL(m_action);
    requestURL.setQuery(m_formData->flattenToString());    
    return requestURL;
}

void FormSubmission::populateFrameLoadRequest(FrameLoadRequest& frameRequest)
{
    if (!m_target.isEmpty())
        frameRequest.setFrameName(m_target);

    if (!m_referrer.isEmpty())
        frameRequest.resourceRequest().setHTTPReferrer(m_referrer);

    if (m_method == FormSubmission::PostMethod) {
        frameRequest.resourceRequest().setHTTPMethod("POST");
        frameRequest.resourceRequest().setHTTPBody(m_formData);

        // construct some user headers if necessary
        if (m_contentType.isNull() || m_contentType == "application/x-www-form-urlencoded")
            frameRequest.resourceRequest().setHTTPContentType(m_contentType);
        else // contentType must be "multipart/form-data"
            frameRequest.resourceRequest().setHTTPContentType(m_contentType + "; boundary=" + m_boundary);
    }

    frameRequest.resourceRequest().setURL(requestURL());
    FrameLoader::addHTTPOriginIfNeeded(frameRequest.resourceRequest(), m_origin);
}

}
