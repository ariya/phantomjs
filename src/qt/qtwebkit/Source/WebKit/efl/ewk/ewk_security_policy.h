/*
    Copyright (C) 2012 ProFUSION embedded systems

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

#ifndef ewk_security_policy_h
#define ewk_security_policy_h

#ifdef __cplusplus
extern "C" {
#endif

#include <Eina.h>

/**
 * Allows a page from @p source_url to request a resource from @p destination_url.
 *
 * @param source_url Source URL; protocol and domain only (e.g. app://clock.zip)
 * @param destination_url Destination URL; protocol and domain only (e.g. http://api.example.com)
 * @param allow_subdomains EINA_TRUE if subdomains of destination URLs should be whitelisted
 */
EAPI void ewk_security_policy_whitelist_origin_add(const char *source_url,
                                                   const char *destination_url,
                                                   Eina_Bool allow_subdomains);

/**
 * Remove a whitelist item added with ewk_security_policy_whitelist_origin_del().
 *
 * @param source_url Source URL; protocol and domain only (e.g. app://clock.zip)
 * @param destination_url Destination URL; protocol and domain only (e.g. http://api.example.com)
 * @param allow_subdomains EINA_TRUE if subdomains of destination URLs should be whitelisted
 */
EAPI void ewk_security_policy_whitelist_origin_del(const char *source_url,
                                                   const char *destination_url,
                                                   Eina_Bool allow_subdomains);

/**
 * Resets the whitelist to EWebKit's default, empty list.
 */
EAPI void ewk_security_policy_whitelist_origin_reset();

#ifdef __cplusplus
}
#endif

#endif // ewk_security_policy_h
