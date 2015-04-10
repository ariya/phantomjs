/*
 * Copyright (C) 2010, 2013 Apple Inc. All rights reserved.
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
#include "WebDatabaseManager.h"

#if ENABLE(SQL_DATABASE)

#include "OriginAndDatabases.h"
#include "WebCoreArgumentCoders.h"
#include "WebDatabaseManagerMessages.h"
#include "WebDatabaseManagerProxyMessages.h"
#include "WebProcess.h"
#include "WebProcessCreationParameters.h"
#include <WebCore/DatabaseDetails.h>
#include <WebCore/DatabaseManager.h>
#include <WebCore/SecurityOrigin.h>

using namespace WebCore;

namespace WebKit {

const char* WebDatabaseManager::supplementName()
{
    return "WebDatabaseManager";
}

WebDatabaseManager::WebDatabaseManager(WebProcess* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::WebDatabaseManager::messageReceiverName(), this);
}

void WebDatabaseManager::initialize(const WebProcessCreationParameters& parameters)
{
    DatabaseManager::manager().initialize(parameters.databaseDirectory);
    DatabaseManager::manager().setClient(this);
}

void WebDatabaseManager::getDatabasesByOrigin(uint64_t callbackID) const
{
    // FIXME: This could be made more efficient by adding a function to DatabaseManager
    // to get both the origins and the Vector of DatabaseDetails for each origin in one
    // shot.  That would avoid taking the numerous locks this requires.

    Vector<RefPtr<SecurityOrigin> > origins;
    DatabaseManager::manager().origins(origins);

    Vector<OriginAndDatabases> originAndDatabasesVector;
    originAndDatabasesVector.reserveInitialCapacity(origins.size());

    for (size_t i = 0; i < origins.size(); ++i) {
        OriginAndDatabases originAndDatabases;

        Vector<String> nameVector;
        if (!DatabaseManager::manager().databaseNamesForOrigin(origins[i].get(), nameVector))
            continue;

        Vector<DatabaseDetails> detailsVector;
        detailsVector.reserveInitialCapacity(nameVector.size());
        for (size_t j = 0; j < nameVector.size(); j++) {
            DatabaseDetails details = DatabaseManager::manager().detailsForNameAndOrigin(nameVector[j], origins[i].get());
            if (details.name().isNull())
                continue;

            detailsVector.append(details);
        }

        if (detailsVector.isEmpty())
            continue;

        originAndDatabases.originIdentifier = origins[i]->databaseIdentifier();
        originAndDatabases.originQuota = DatabaseManager::manager().quotaForOrigin(origins[i].get());
        originAndDatabases.originUsage = DatabaseManager::manager().usageForOrigin(origins[i].get());
        originAndDatabases.databases.swap(detailsVector); 
        originAndDatabasesVector.append(originAndDatabases);
    }

    m_process->send(Messages::WebDatabaseManagerProxy::DidGetDatabasesByOrigin(originAndDatabasesVector, callbackID), 0);
}

void WebDatabaseManager::getDatabaseOrigins(uint64_t callbackID) const
{
    Vector<RefPtr<SecurityOrigin> > origins;
    DatabaseManager::manager().origins(origins);

    size_t numOrigins = origins.size();

    Vector<String> identifiers(numOrigins);
    for (size_t i = 0; i < numOrigins; ++i)
        identifiers[i] = origins[i]->databaseIdentifier();
    m_process->send(Messages::WebDatabaseManagerProxy::DidGetDatabaseOrigins(identifiers, callbackID), 0);
}

void WebDatabaseManager::deleteDatabaseWithNameForOrigin(const String& databaseIdentifier, const String& originIdentifier) const
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromDatabaseIdentifier(originIdentifier);
    if (!origin)
        return;

    DatabaseManager::manager().deleteDatabase(origin.get(), databaseIdentifier);
}

void WebDatabaseManager::deleteDatabasesForOrigin(const String& originIdentifier) const
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromDatabaseIdentifier(originIdentifier);
    if (!origin)
        return;

    DatabaseManager::manager().deleteOrigin(origin.get());
}

void WebDatabaseManager::deleteAllDatabases() const
{
    DatabaseManager::manager().deleteAllDatabases();
}

void WebDatabaseManager::setQuotaForOrigin(const String& originIdentifier, unsigned long long quota) const
{
    // If the quota is set to a value lower than the current usage, that quota will
    // "stick" but no data will be purged to meet the new quota. This will simply
    // prevent new data from being added to databases in that origin.

    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromDatabaseIdentifier(originIdentifier);
    if (!origin)
        return;

    DatabaseManager::manager().setQuota(origin.get(), quota);
}

void WebDatabaseManager::dispatchDidModifyOrigin(SecurityOrigin* origin)
{
    // NOTE: This may be called on a non-main thread.
    m_process->send(Messages::WebDatabaseManagerProxy::DidModifyOrigin(origin->databaseIdentifier()), 0);
}

void WebDatabaseManager::dispatchDidModifyDatabase(WebCore::SecurityOrigin* origin, const String& databaseIdentifier)
{
    // NOTE: This may be called on a non-main thread.
    m_process->send(Messages::WebDatabaseManagerProxy::DidModifyDatabase(origin->databaseIdentifier(), databaseIdentifier), 0);
}

} // namespace WebKit

#endif // ENABLE(SQL_DATABASE)
