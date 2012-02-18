/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include "ChangeVersionWrapper.h"

#if ENABLE(DATABASE)
#include "Database.h"
#include "SQLError.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

ChangeVersionWrapper::ChangeVersionWrapper(const String& oldVersion, const String& newVersion)
    : m_oldVersion(oldVersion.crossThreadString())
    , m_newVersion(newVersion.crossThreadString())
{
}

bool ChangeVersionWrapper::performPreflight(SQLTransaction* transaction)
{
    ASSERT(transaction && transaction->database());

    String actualVersion;

    if (!transaction->database()->getVersionFromDatabase(actualVersion)) {
        LOG_ERROR("Unable to retrieve actual current version from database");
        m_sqlError = SQLError::create(SQLError::UNKNOWN_ERR, "unable to verify current version of database");
        return false;
    }

    if (actualVersion != m_oldVersion) {
        LOG_ERROR("Old version doesn't match actual version");
        m_sqlError = SQLError::create(SQLError::VERSION_ERR, "current version of the database and `oldVersion` argument do not match");
        return false;
    }

    return true;
}

bool ChangeVersionWrapper::performPostflight(SQLTransaction* transaction)
{
    ASSERT(transaction && transaction->database());

    if (!transaction->database()->setVersionInDatabase(m_newVersion)) {
        LOG_ERROR("Unable to set new version in database");
        m_sqlError = SQLError::create(SQLError::UNKNOWN_ERR, "unable to set new version in database");
        return false;
    }

    transaction->database()->setExpectedVersion(m_newVersion);

    return true;
}

} // namespace WebCore

#endif // ENABLE(DATABASE)
