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
#include "ewk_web_database.h"

#include "DatabaseManager.h"
#include "SecurityOrigin.h"
#include "ewk_security_origin.h"
#include "ewk_security_origin_private.h"
#include "ewk_web_database_private.h"
#include <Eina.h>
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

struct _Ewk_Web_Database {
    WTF::RefPtr<WebCore::SecurityOrigin> securityOrigin;
    WTF::String coreName;
    const char* displayName;
    const char* filename;
    const char* name;
};

const char* ewk_web_database_display_name_get(Ewk_Web_Database* database)
{
#if ENABLE(SQL_DATABASE)
    if (database->displayName)
        return database->displayName;

    WebCore::SecurityOrigin* origin = database->securityOrigin.get();
    WebCore::DatabaseDetails details = WebCore::DatabaseManager::manager().detailsForNameAndOrigin(database->name, origin);
    database->displayName = eina_stringshare_add(details.displayName().utf8().data());

    return database->displayName;
#else
    UNUSED_PARAM(database);
    return 0;
#endif
}

uint64_t ewk_web_database_expected_size_get(const Ewk_Web_Database* database)
{
#if ENABLE(SQL_DATABASE)
    WebCore::SecurityOrigin* origin = database->securityOrigin.get();
    WebCore::DatabaseDetails details = WebCore::DatabaseManager::manager().detailsForNameAndOrigin(database->name, origin);
    return details.expectedUsage();
#else
    UNUSED_PARAM(database);
    return 0;
#endif
}

const char* ewk_web_database_filename_get(Ewk_Web_Database* database)
{
#if ENABLE(SQL_DATABASE)
    if (database->filename)
        return database->filename;

    WebCore::SecurityOrigin* origin = database->securityOrigin.get();
    WTF::String path = WebCore::DatabaseManager::manager().fullPathForDatabase(origin, database->coreName);
    database->filename = eina_stringshare_add(path.utf8().data());

    return database->filename;
#else
    UNUSED_PARAM(database);
    return 0;
#endif
}

const char* ewk_web_database_name_get(Ewk_Web_Database* database)
{
#if ENABLE(SQL_DATABASE)
    if (database->name)
        return database->name;

    database->name = eina_stringshare_add(database->coreName.utf8().data());

    return database->name;
#else
    UNUSED_PARAM(database);
    return 0;
#endif
}

Ewk_Security_Origin* ewk_web_database_security_origin_get(const Ewk_Web_Database* database)
{
    return ewk_security_origin_new(database->securityOrigin.get());
}

uint64_t ewk_web_database_size_get(const Ewk_Web_Database* database)
{
#if ENABLE(SQL_DATABASE)
    WebCore::SecurityOrigin* origin = database->securityOrigin.get();
    WebCore::DatabaseDetails details = WebCore::DatabaseManager::manager().detailsForNameAndOrigin(database->name, origin);
    return details.currentUsage();
#else
    UNUSED_PARAM(database);
    return 0;
#endif
}

void ewk_web_database_remove(Ewk_Web_Database* database)
{
#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().deleteDatabase(database->securityOrigin.get(), database->coreName);
#else
    UNUSED_PARAM(database);
#endif
}

void ewk_web_database_remove_all(void)
{
#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().deleteAllDatabases();
#endif
}

void ewk_web_database_free(Ewk_Web_Database* database)
{
#if ENABLE(SQL_DATABASE)
    eina_stringshare_del(database->displayName);
    eina_stringshare_del(database->filename);
    eina_stringshare_del(database->name);

    delete database;
#else
    UNUSED_PARAM(database);
#endif
}

void ewk_web_database_list_free(Eina_List* databaseList)
{
    void* database;
    EINA_LIST_FREE(databaseList, database)
        ewk_web_database_free(static_cast<Ewk_Web_Database*>(database));
}

/**
 * @internal
 * Creates a wrapper representing a Web Database.
 *
 * @param coreOrigin WebCore Security Origin object
 * @param coreName Web Database name
 *
 * @return a wrapper for manipulating a Web Database. It should be freed
 * by ewk_web_database_free().
 */
Ewk_Web_Database* ewk_web_database_new(WebCore::SecurityOrigin* coreOrigin, const WTF::String& coreName)
{
#if ENABLE(SQL_DATABASE)
    Ewk_Web_Database* database = new Ewk_Web_Database;

    database->securityOrigin = coreOrigin;
    database->coreName = coreName;
    database->displayName = 0;
    database->filename = 0;
    database->name = 0;

    return database;
#else
    UNUSED_PARAM(coreOrigin);
    UNUSED_PARAM(coreName);
    return 0;
#endif
}
