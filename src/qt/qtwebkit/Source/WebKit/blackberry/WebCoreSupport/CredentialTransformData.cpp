/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

 /*
 * This methods are based on Chromium codes in
 * Source/WebKit/chromium/src/WebPasswordFormUtils.cpp
 */

#include "config.h"
#include "CredentialTransformData.h"

#include "HTMLInputElement.h"
#include "KURL.h"
#include <wtf/Vector.h>

namespace WebCore {

namespace {
// Maximum number of password fields we will observe before throwing our
// hands in the air and giving up with a given form.
static const size_t maxPasswords = 3;

// Helper method to clear url of unneeded parts.
KURL stripURL(const KURL& url)
{
    KURL strippedURL = url;
    strippedURL.setUser(String());
    strippedURL.setPass(String());
    strippedURL.setQuery(String());
    strippedURL.setFragmentIdentifier(String());
    return strippedURL;
}

} // namespace

CredentialTransformData::CredentialTransformData()
    : m_userNameElement(0)
    , m_passwordElement(0)
    , m_oldPasswordElement(0)
    , m_isValid(false)
{
}

CredentialTransformData::CredentialTransformData(HTMLFormElement* form, bool isForSaving)
    : m_userNameElement(0)
    , m_passwordElement(0)
    , m_oldPasswordElement(0)
    , m_isValid(false)
{
    ASSERT(form);

    // Get the document URL
    KURL fullOrigin(ParsedURLString, form->document()->documentURI());

    // Calculate the canonical action URL
    String action = form->action();
    if (action.isNull())
        action = ""; // missing 'action' attribute implies current URL
    KURL fullAction = form->document()->completeURL(action);
    if (!fullAction.isValid())
        return;

    if (!findPasswordFormFields(form))
        return;

    // Won't restore password if there're two password inputs on the page.
    if (!isForSaving && m_oldPasswordElement)
        return;

    KURL url = stripURL(fullOrigin);
    m_action = stripURL(fullAction);
    m_protectionSpace = ProtectionSpace(url.host(), url.port(), ProtectionSpaceServerHTTP, "Form", ProtectionSpaceAuthenticationSchemeHTMLForm);
    m_credential = Credential(m_userNameElement->value(), m_passwordElement->value(), CredentialPersistencePermanent);

    m_isValid = true;
}

CredentialTransformData::CredentialTransformData(const ProtectionSpace& protectionSpace, const Credential& credential)
    : m_protectionSpace(protectionSpace)
    , m_credential(credential)
    , m_userNameElement(0)
    , m_passwordElement(0)
    , m_oldPasswordElement(0)
    , m_isValid(true)
{
}

Credential CredentialTransformData::credential() const
{
    if (m_credential.isEmpty() && m_userNameElement && m_passwordElement)
        return m_credential = Credential(m_userNameElement->value(), m_passwordElement->value(), CredentialPersistencePermanent);
    return m_credential;
}

void CredentialTransformData::setCredential(const Credential& credential)
{
    if (!m_isValid)
        return;

    m_credential = credential;
    m_userNameElement->setValue(credential.user());
    m_userNameElement->setAutofilled();
    m_passwordElement->setValue(credential.password());
    m_passwordElement->setAutofilled();
}

bool CredentialTransformData::findPasswordFormFields(const HTMLFormElement* form)
{
    ASSERT(form);

    int firstPasswordIndex = 0;
    // First, find the password fields and activated submit button.
    const Vector<FormAssociatedElement*>& formElements = form->associatedElements();
    Vector<HTMLInputElement*> passwords;
    for (size_t i = 0; i < formElements.size(); i++) {
        if (!formElements[i]->isFormControlElement())
            continue;
        HTMLFormControlElement* formElement = static_cast<HTMLFormControlElement*>(formElements[i]);
        if (!isHTMLInputElement(formElement))
            continue;

        HTMLInputElement* inputElement = formElement->toInputElement();
        if (inputElement->isDisabledFormControl())
            continue;

        if ((passwords.size() < maxPasswords)
            && inputElement->isPasswordField()
            && inputElement->shouldAutocomplete()) {
            if (passwords.isEmpty())
                firstPasswordIndex = i;
            passwords.append(inputElement);
        }
    }

    if (!passwords.isEmpty()) {
        // Then, search backwards for the username field.
        for (int i = firstPasswordIndex - 1; i >= 0; i--) {
            if (!formElements[i]->isFormControlElement())
                continue;
            HTMLFormControlElement* formElement = static_cast<HTMLFormControlElement*>(formElements[i]);
            if (!isHTMLInputElement(formElement))
                continue;

            HTMLInputElement* inputElement = formElement->toInputElement();
            if (inputElement->isDisabledFormControl())
                continue;

            // Various input types such as text, url, email can be a username field.
            if ((inputElement->isTextField() && !inputElement->isPasswordField())
                && (inputElement->shouldAutocomplete())) {
                m_userNameElement = inputElement;
                break;
            }
        }
    }
    if (!m_userNameElement)
        return false;

    if (!locateSpecificPasswords(passwords))
        return false;
    return true;
}

// Helper method to determine which password is the main one, and which is
// an old password (e.g on a "make new password" form), if any.
bool CredentialTransformData::locateSpecificPasswords(const Vector<HTMLInputElement*>& passwords)
{
    switch (passwords.size()) {
    case 1:
        // Single password, easy.
        m_passwordElement = passwords[0];
        break;
    case 2:
        if (passwords[0]->value() == passwords[1]->value())
            // Treat two identical passwords as a single password.
            m_passwordElement = passwords[0];
        else {
            // Assume first is old password, second is new (no choice but to guess).
            m_oldPasswordElement = passwords[0];
            m_passwordElement = passwords[1];
        }
        break;
    case 3:
        if (passwords[0]->value() == passwords[1]->value()
            && passwords[0]->value() == passwords[2]->value()) {
            // All three passwords the same? Just treat as one and hope.
            m_passwordElement = passwords[0];
        } else if (passwords[0]->value() == passwords[1]->value()) {
            // Two the same and one different -> old password is duplicated one.
            m_oldPasswordElement = passwords[0];
            m_passwordElement = passwords[2];
        } else if (passwords[1]->value() == passwords[2]->value()) {
            m_oldPasswordElement = passwords[0];
            m_passwordElement = passwords[1];
        } else {
            // Three different passwords, or first and last match with middle
            // different. No idea which is which, so no luck.
            return false;
        }
        break;
    default:
        return false;
    }
    return true;
}

} // namespace WebCore
