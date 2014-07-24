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

/**
 * @file   ewk_database_manager.h
 * @brief  Describes the Ewk Database Manager API.
 *
 * Ewk Database Manager manages web database.
 */

#ifndef ewk_database_manager_h
#define ewk_database_manager_h

#include "ewk_error.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for Ewk_Database_Manager. */
typedef struct EwkDatabaseManager Ewk_Database_Manager;

/**
 * @typedef Ewk_Database_Origins_Get_Cb Ewk_Database_Origins_Get_Cb
 * @brief Callback type for use with ewk_database_manager_origins_get()
 *
 * @param origins @c Eina_List containing @c Ewk_Security_Origin elements or @c NULL in case of error,
 *                        the Eina_List and its items should be freed after use. Use ewk_object_unref()
 *                        to free the items
 */
typedef void (*Ewk_Database_Origins_Get_Cb)(Eina_List *origins, Ewk_Error *error, void *user_data);

/**
 * Gets list of origins using web database asynchronously.
 *
 * This function allocates memory for context structure made from callback and user_data.
 *
 * @param manager Ewk_Database_Manager object
 * @param callback callback to get database origins
 * @param user_data user_data will be passed when result_callback is called,
 *    -i.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_database_manager_origins_get(const Ewk_Database_Manager *manager, Ewk_Database_Origins_Get_Cb callback, void *user_data);

#ifdef __cplusplus
}
#endif
#endif // ewk_database_manager_h
