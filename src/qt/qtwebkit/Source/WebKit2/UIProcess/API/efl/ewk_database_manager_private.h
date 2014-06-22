/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ewk_database_manager_private_h
#define ewk_database_manager_private_h

#include "WKDatabaseManager.h"
#include "WKRetainPtr.h"
#include "ewk_security_origin_private.h"
#include <WebKit2/WKBase.h>
#include <wtf/PassOwnPtr.h>

using namespace WebKit;

class EwkDatabaseManager {
public:
    static PassOwnPtr<EwkDatabaseManager> create(WKDatabaseManagerRef databaseManager)
    {
        ASSERT(databaseManager);
        return adoptPtr(new EwkDatabaseManager(databaseManager));
    }

    Eina_List* createOriginList(WKArrayRef wkList) const;
    void getDatabaseOrigins(WKDatabaseManagerGetDatabaseOriginsFunction callback, void* context) const;

private:
    explicit EwkDatabaseManager(WKDatabaseManagerRef);

    WKRetainPtr<WKDatabaseManagerRef> m_databaseManager;
    mutable HashMap<WKSecurityOriginRef, RefPtr<EwkSecurityOrigin> > m_wrapperCache;
};
#endif // ewk_database_manager_private_h
