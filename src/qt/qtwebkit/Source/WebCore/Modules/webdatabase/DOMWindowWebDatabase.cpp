/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(SQL_DATABASE)

#include "DOMWindowWebDatabase.h"

#include "DOMWindow.h"
#include "Database.h"
#include "DatabaseCallback.h"
#include "DatabaseManager.h"
#include "Document.h"
#include "Frame.h"
#include "SecurityOrigin.h"

namespace WebCore {

PassRefPtr<Database> DOMWindowWebDatabase::openDatabase(DOMWindow* window, const String& name, const String& version, const String& displayName, unsigned long estimatedSize, PassRefPtr<DatabaseCallback> creationCallback, ExceptionCode& ec)
{
    if (!window->isCurrentlyDisplayedInFrame())
        return 0;

    RefPtr<Database> database = 0;
    DatabaseManager& dbManager = DatabaseManager::manager();
    DatabaseError error = DatabaseError::None;
    if (dbManager.isAvailable() && window->document()->securityOrigin()->canAccessDatabase(window->document()->topOrigin())) {
        database = dbManager.openDatabase(window->document(), name, version, displayName, estimatedSize, creationCallback, error);
        ASSERT(database || error != DatabaseError::None);
        ec = DatabaseManager::exceptionCodeForDatabaseError(error);
    } else
        ec = SECURITY_ERR;

    return database;
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
