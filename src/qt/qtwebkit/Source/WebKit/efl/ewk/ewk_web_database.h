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

/**
 * @file    ewk_web_database.h
 * @brief   Web Database API.
 *
 * This API provides functions for managing Web Database created for a security
 * origin. WebKit needs to be compiled with Web Database support in order to use
 * this API.
 */

#ifndef ewk_web_database_h
#define ewk_web_database_h

#include "ewk_security_origin.h"

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Ewk_Web_Database Ewk_Web_Database;

/**
 * Returns the user friendly name of the database.
 *
 * It returns a internal string which should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o web database object
 *
 * @return the database name
 */
EAPI const char  *ewk_web_database_display_name_get(Ewk_Web_Database *o);

/**
 * Returns the size of the database expected by the web author.
 *
 * @param o web database object
 *
 * @return expected size
 */
EAPI uint64_t     ewk_web_database_expected_size_get(const Ewk_Web_Database *o);

/**
 * Returns the absolute path to the database file.
 *
 * It returns a internal string which should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o web database object
 *
 * @return the database path
 */
EAPI const char  *ewk_web_database_filename_get(Ewk_Web_Database *o);

/**
 * Returns the name of the database.
 *
 * It returns a internal string which should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o web database object
 *
 * @return the database name
 */
EAPI const char  *ewk_web_database_name_get(Ewk_Web_Database *o);

/**
 * Retrieves the security origin of the database.
 *
 * The returned value should be freed by ewk_security_origin_free().
 *
 * @param o web database object
 *
 * @return the security origin
 */
EAPI Ewk_Security_Origin *ewk_web_database_security_origin_get(const Ewk_Web_Database *o);

/**
 * Returns the size of the database.
 *
 * @param o web database object
 *
 * @return the current size of the database
 */
EAPI uint64_t     ewk_web_database_size_get(const Ewk_Web_Database *o);

/**
 * Remove the database from the security origin and destroy all data.
 *
 * @param o web database object
 */
EAPI void         ewk_web_database_remove(Ewk_Web_Database *o);

/**
 * Remove all the databases in the current default path.
 */
EAPI void         ewk_web_database_remove_all(void);

/**
 * Release all resources allocated by a web database object.
 *
 * @param o web database object
 */
EAPI void         ewk_web_database_free(Ewk_Web_Database *o);

/**
 * Convenience function for releasing all web database objects from a list.
 *
 * @param database_list list of web database objects to be freed
 */
EAPI void         ewk_web_database_list_free(Eina_List *database_list);

#ifdef __cplusplus
}
#endif
#endif // ewk_web_database_h
