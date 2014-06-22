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

#ifndef CredentialManager_h
#define CredentialManager_h

#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)

#include "HTMLCollection.h"
#include <wtf/PassRefPtr.h>

class PageClientBlackBerry;

namespace BlackBerry {
namespace Platform {
class String;
}
}

namespace WebCore {
class KURL;
class ProtectionSpace;
class CredentialTransformData;

class CredentialManager {
public:
    void autofillAuthenticationChallenge(const ProtectionSpace&, BlackBerry::Platform::String& username, BlackBerry::Platform::String& password);
    void autofillPasswordForms(PassRefPtr<HTMLCollection> docForms);
    void saveCredentialIfConfirmed(PageClientBlackBerry*, const CredentialTransformData&);
    void clearCredentials();
    void clearNeverRememberSites();

private:
    friend CredentialManager& credentialManager();
    CredentialManager() { }
};

CredentialManager& credentialManager();

} // WebCore

#endif // ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)

#endif // CredentialManager_h
