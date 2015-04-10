/*
    Copyright (C) 2012 Intel Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_security_origin.h"

#include "ApplicationCache.h"
#include "ApplicationCacheStorage.h"
#include "DatabaseManager.h"
#include "SecurityOrigin.h"
#include "ewk_security_origin_private.h"
#include "ewk_web_database.h"
#include "ewk_web_database_private.h"
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>

struct _Ewk_Security_Origin {
    RefPtr<WebCore::SecurityOrigin> securityOrigin;
    const char* protocol;
    const char* host;
    const char* originString;
};

const char* ewk_security_origin_protocol_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    return origin->protocol;
}

const char* ewk_security_origin_host_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    return origin->host;
}

const char* ewk_security_origin_string_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    return origin->originString;
}

uint32_t ewk_security_origin_port_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    return origin->securityOrigin->port();
}

uint64_t ewk_security_origin_web_database_usage_get(const Ewk_Security_Origin* origin)
{
#if ENABLE(SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);

    return WebCore::DatabaseManager::manager().usageForOrigin(origin->securityOrigin.get());
#else
    UNUSED_PARAM(origin);
    return 0;
#endif
}

uint64_t ewk_security_origin_web_database_quota_get(const Ewk_Security_Origin* origin)
{
#if ENABLE(SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);

    return WebCore::DatabaseManager::manager().quotaForOrigin(origin->securityOrigin.get());
#else
    UNUSED_PARAM(origin);
    return 0;
#endif
}

void ewk_security_origin_web_database_quota_set(const Ewk_Security_Origin* origin, uint64_t quota)
{
    EINA_SAFETY_ON_NULL_RETURN(origin);

#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().setQuota(origin->securityOrigin.get(), quota);
#endif
}

void ewk_security_origin_application_cache_quota_set(const Ewk_Security_Origin* origin, int64_t quota)
{
    EINA_SAFETY_ON_NULL_RETURN(origin);
    WebCore::cacheStorage().storeUpdatedQuotaForOrigin(origin->securityOrigin.get(), quota);
}

void ewk_security_origin_application_cache_clear(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN(origin);
    WebCore::ApplicationCache::deleteCacheForOrigin(origin->securityOrigin.get());
}

Eina_List* ewk_security_origin_web_database_get_all(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);

    Eina_List* databases = 0;
#if ENABLE(SQL_DATABASE)
    Vector<WTF::String> names;

    if (!WebCore::DatabaseManager::manager().databaseNamesForOrigin(origin->securityOrigin.get(), names))
        return 0;

    for (unsigned i = 0; i < names.size(); i++) {
        Ewk_Web_Database* database = ewk_web_database_new(origin->securityOrigin.get(), names[i].utf8().data());
        databases = eina_list_append(databases, database);
    }
#else
    UNUSED_PARAM(origin);
#endif
    return databases;
}

void ewk_security_origin_free(Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN(origin);

    origin->securityOrigin = 0;
    eina_stringshare_del(origin->host);
    eina_stringshare_del(origin->protocol);
    eina_stringshare_del(origin->originString);

    delete origin;
}

Ewk_Security_Origin* ewk_security_origin_new_from_string(const char* url)
{
    return ewk_security_origin_new(WebCore::SecurityOrigin::createFromString(String::fromUTF8(url)).get());
}

/**
 * @internal
 * Creates a EWK wrapper for WebCore Security Origin object.
 *
 * @param coreOrigin WebCore Security Origin object
 *
 * @return a EWK wrapper of WebCore Security Origin object which should be
 * freed by ewk_security_origin_free()
 */
Ewk_Security_Origin* ewk_security_origin_new(WebCore::SecurityOrigin* coreOrigin)
{
    Ewk_Security_Origin* origin = new Ewk_Security_Origin;

    origin->securityOrigin = coreOrigin;
    origin->protocol = eina_stringshare_add(coreOrigin->protocol().utf8().data());
    origin->host = eina_stringshare_add(coreOrigin->host().utf8().data());
    origin->originString = eina_stringshare_add(origin->securityOrigin->toString().utf8().data());

    return origin;
}
