/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#ifndef CredentialBackingStore_h
#define CredentialBackingStore_h

#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)

#include "Credential.h"
#include "SQLiteDatabase.h"

#include <BlackBerryPlatformMisc.h>
#include <wtf/OwnPtr.h>

namespace BlackBerry {
namespace Platform {
class CertMgrWrapper;
}
}

namespace WebCore {

class ProtectionSpace;

class CredentialBackingStore {
public:
    friend CredentialBackingStore& credentialBackingStore();

    ~CredentialBackingStore();
    bool open(const String& dbPath);
    bool addLogin(const ProtectionSpace&, const Credential&);
    bool updateLogin(const ProtectionSpace&, const Credential&);
    bool hasLogin(const ProtectionSpace&);
    Credential getLogin(const ProtectionSpace&);
    bool removeLogin(const ProtectionSpace&, const String& username);
    bool addNeverRemember(const ProtectionSpace&);
    bool hasNeverRemember(const ProtectionSpace&);
    bool removeNeverRemember(const ProtectionSpace&);
    bool clearLogins();
    bool clearNeverRemember();

private:
    CredentialBackingStore();
    String encryptedString(const String& plainText) const;
    String decryptedString(const String& cipherText) const;

    BlackBerry::Platform::CertMgrWrapper* certMgrWrapper();

    SQLiteDatabase m_database;
    OwnPtr<SQLiteStatement> m_addLoginStatement;
    OwnPtr<SQLiteStatement> m_updateLoginStatement;
    OwnPtr<SQLiteStatement> m_hasLoginStatement;
    OwnPtr<SQLiteStatement> m_getLoginStatement;
    OwnPtr<SQLiteStatement> m_removeLoginStatement;
    OwnPtr<SQLiteStatement> m_addNeverRememberStatement;
    OwnPtr<SQLiteStatement> m_hasNeverRememberStatement;
    OwnPtr<SQLiteStatement> m_removeNeverRememberStatement;

    OwnPtr<BlackBerry::Platform::CertMgrWrapper> m_certMgrWrapper;

    DISABLE_COPY(CredentialBackingStore)
};

CredentialBackingStore& credentialBackingStore();

} // namespace WebCore

#endif // ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)

#endif // CredentialBackingStore_h
