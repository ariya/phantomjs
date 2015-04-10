/*
    Copyright (C) 2010 ProFUSION embedded systems
    Copyright (C) 2010 Samsung Electronics

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
 * @file    ewk_cookies.h
 * @brief   The Ewk cookies API.
 *
 * @note If the libsoup network backend is being used the functions here will
 * only work with the @b default SoupSession, which can be retrieved with
 * ewk_network_defaul_soup_session_get(). If a different SoupSession is used
 * and associated with a view with ewk_view_soup_session_set(), all cookie
 * management will have to be done manually.
 */

#ifndef ewk_cookies_h
#define ewk_cookies_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \struct  _Ewk_Cookie
 *
 * @brief   Describes properties of an HTTP cookie.
 *
 * All the strings are guaranteed to be stringshared, so use eina_stringshare_ref()
 * instead of eina_stringshare_add() or strdup().
 */
struct _Ewk_Cookie {
    /// the cookie name
    const char *name;
    /// the cookie value
    const char *value;
    /// the "domain" attribute, or else the hostname that the cookie came from
    const char *domain;
    /// the "path" attribute, or @c NULL
    const char *path;
    /// the cookie expiration time, or @c 0 for a session cookie
    time_t expires;
    /// @c EINA_TRUE if the cookie should only be tranferred over SSL
    Eina_Bool secure;
    /// @c EINA_TRUE if the cookie should not be exposed to scripts
    Eina_Bool http_only;
};
/// Creates a type name for the _Ewk_Cookie.
typedef struct _Ewk_Cookie Ewk_Cookie;

/**
 * \enum    _Ewk_Cookie_Policy
 *
 * @brief   Contains a policy for the cookies.
 */
enum _Ewk_Cookie_Policy {
    /// Rejects all cookies.
    EWK_COOKIE_JAR_ACCEPT_NEVER,
    /// Accepts every cookie sent from any page.
    EWK_COOKIE_JAR_ACCEPT_ALWAYS,
    /// Accepts cookies only from the main page.
    EWK_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY
};
/// Creates a type name for the _Ewk_Cookie_Policy.
typedef enum _Ewk_Cookie_Policy Ewk_Cookie_Policy;

/************************** Exported functions ***********************/
 
/**
 * Sets the path where the cookies are going to be stored. 
 *
 * @param filename path to the cookies.txt file, use @c NULL for keep
 *        cookies just in memory.
 *
 * @return @c EINA_FALSE if it wasn't possible to create the cookie jar,
 *          @c EINA_TRUE otherwise.
 */
EAPI Eina_Bool          ewk_cookies_file_set(const char *filename);

/**
 * Clears all the cookies from the cookie jar.
 */
EAPI void               ewk_cookies_clear(void);

/**
 * Returns a list of cookies in the cookie jar.
 *
 * @return an @c Eina_List with all the cookies in the cookie jar
 */
EAPI Eina_List          *ewk_cookies_get_all(void);

/**
 * Deletes a cookie from the cookie jar.
 *
 * Note that the fields name, value, domain and path are used to match this
 * cookie in the cookie jar.
 *
 * @param cookie an @c Ewk_Cookie that has the info relative to that cookie
 */
EAPI void               ewk_cookies_cookie_del(Ewk_Cookie *cookie);

/**
 * Frees the memory used by a cookie.
 *
 * @param cookie the Ewk_Cookie struct that will be freed
 */
EAPI void               ewk_cookies_cookie_free(Ewk_Cookie *cookie);

/**
 * Sets accept policy for the cookies.
 *
 * @param p the acceptance policy
 *
 * @see Ewk_Cookie_Policy
 */
EAPI void               ewk_cookies_policy_set(Ewk_Cookie_Policy p);

/**
 * Gets the acceptance policy used in the current cookie jar.
 *
 * @return the current acceptance policy
 * @see Ewk_Cookie_Policy
 */
EAPI Ewk_Cookie_Policy  ewk_cookies_policy_get(void);

#ifdef __cplusplus
}
#endif
#endif // ewk_cookies_h
