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

#ifndef CredentialTransformData_h
#define CredentialTransformData_h

#include "Credential.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "KURL.h"
#include "ProtectionSpace.h"

#include <wtf/RefPtr.h>

namespace WebCore {

struct CredentialTransformData {
    // If the provided form is suitable for password completion, isValid() will
    // return true;
    CredentialTransformData();
    CredentialTransformData(HTMLFormElement*, bool isForSaving = false);
    CredentialTransformData(const ProtectionSpace&, const Credential&);

    // If creation failed, return false.
    bool isValid() const { return m_isValid; }

    ProtectionSpace protectionSpace() const { return m_protectionSpace; }
    Credential credential() const;
    void setCredential(const Credential&);

private:
    bool findPasswordFormFields(const HTMLFormElement*);
    bool locateSpecificPasswords(const Vector<HTMLInputElement*>& passwords);

    KURL m_action;
    ProtectionSpace m_protectionSpace;
    mutable Credential m_credential;
    RefPtr<HTMLInputElement> m_userNameElement;
    RefPtr<HTMLInputElement> m_passwordElement;
    RefPtr<HTMLInputElement> m_oldPasswordElement;
    bool m_isValid;
};

} // namespace WebCore

#endif
