/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
#include "CredentialManager.h"

#include "CredentialBackingStore.h"
#include "CredentialTransformData.h"
#include "HTMLFormElement.h"
#include "KURL.h"
#include "Logging.h"
#include "PageClientBlackBerry.h"

#include <BlackBerryPlatformString.h>

namespace WebCore {

CredentialManager& credentialManager()
{
    static CredentialManager *credentialManager = 0;
    if (!credentialManager)
        credentialManager = new CredentialManager();
    return *credentialManager;
}

void CredentialManager::autofillAuthenticationChallenge(const ProtectionSpace& protectionSpace, BlackBerry::Platform::String& username, BlackBerry::Platform::String& password)
{
    if (credentialBackingStore().hasNeverRemember(protectionSpace))
        return;

    Credential savedCredential = credentialBackingStore().getLogin(protectionSpace);
    username = savedCredential.user();
    password = savedCredential.password();
}

void CredentialManager::autofillPasswordForms(PassRefPtr<HTMLCollection> docForms)
{
    ASSERT(docForms);

    RefPtr<HTMLCollection> forms = docForms;
    size_t sourceLength = forms->length();
    for (size_t i = 0; i < sourceLength; ++i) {
        Node* node = forms->item(i);
        if (node && node->isHTMLElement()) {
            CredentialTransformData data(toHTMLFormElement(node));
            if (!data.isValid() || !credentialBackingStore().hasLogin(data.protectionSpace()))
                continue;
            Credential savedCredential = credentialBackingStore().getLogin(data.protectionSpace());
            data.setCredential(savedCredential);
        }
    }
}

void CredentialManager::saveCredentialIfConfirmed(PageClientBlackBerry* pageClient, const CredentialTransformData& data)
{
    ASSERT(pageClient);

    if (!data.isValid() || data.credential().isEmpty() || credentialBackingStore().hasNeverRemember(data.protectionSpace()))
        return;

    Credential savedCredential = credentialBackingStore().getLogin(data.protectionSpace());
    if (savedCredential == data.credential())
        return;

    PageClientBlackBerry::SaveCredentialType type = pageClient->notifyShouldSaveCredential(savedCredential.isEmpty());
    if (type == PageClientBlackBerry::SaveCredentialYes) {
        if (savedCredential.isEmpty())
            credentialBackingStore().addLogin(data.protectionSpace(), data.credential());
        else
            credentialBackingStore().updateLogin(data.protectionSpace(), data.credential());
    } else if (type == PageClientBlackBerry::SaveCredentialNeverForThisSite) {
        credentialBackingStore().addNeverRemember(data.protectionSpace());
        credentialBackingStore().removeLogin(data.protectionSpace(), data.credential().user());
    }
}

void CredentialManager::clearCredentials()
{
    credentialBackingStore().clearLogins();
}

void CredentialManager::clearNeverRememberSites()
{
    credentialBackingStore().clearNeverRemember();
}

} // namespace WebCore

#endif // ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
