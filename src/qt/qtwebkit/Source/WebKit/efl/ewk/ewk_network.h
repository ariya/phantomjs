/*
    Copyright (C) 2011 Samsung Electronics

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
 * @file    ewk_network.h
 * @brief   Describes the network API.
 */

#ifndef ewk_network_h
#define ewk_network_h

#include <Eina.h>
#include <libsoup/soup.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets the given proxy URI to network backend.
 *
 * @param proxy URI to set
 *
 * @note If the libsoup backend is being used, this function has effect on
 * the @b default SoupSession, returned by ewk_network_default_soup_session_get().
 * If a different SoupSession is used and passed to ewk_view_soup_session_set(),
 * this function will not have any effect on it.
 */
EAPI void             ewk_network_proxy_uri_set(const char *proxy);

/**
 * Gets the proxy URI from the network backend.
 *
 * The returned string should be freed by eina_stringshare_del() after use.
 *
 * @return current proxy URI or @c NULL if it's not set
 *
 * @note If the libsoup backend is being used, this function has effect on
 * the @b default SoupSession, returned by ewk_network_default_soup_session_get().
 * If a different SoupSession is used and passed to ewk_view_soup_session_set(),
 * this function will not have any effect on it.
 */
EAPI const char      *ewk_network_proxy_uri_get(void);

/**
 * Returns whether HTTPS connections should check the received certificate and error out if it is invalid.
 *
 * By default, HTTPS connections are performed regardless of the validity of the certificate provided.
 *
 * @sa ewk_network_tls_ca_certificates_path_set
 *
 * @note If the libsoup backend is being used, this function has effect on
 * the @b default SoupSession, returned by ewk_network_default_soup_session_get().
 * If a different SoupSession is used and passed to ewk_view_soup_session_set(),
 * this function will not have any effect on it.
 */
EAPI Eina_Bool        ewk_network_tls_certificate_check_get(void);

/**
 * Sets whether HTTPS connections should check the received certificate and error out if it is invalid.
 *
 * By default, HTTPS connections are performed regardless of the validity of the certificate provided.
 *
 * @param enable Whether to check the provided certificates or not.
 *
 * @sa ewk_network_tls_ca_certificates_path_set
 *
 * @note If the libsoup backend is being used, this function has effect on
 * the @b default SoupSession, returned by ewk_network_default_soup_session_get().
 * If a different SoupSession is used and passed to ewk_view_soup_session_set(),
 * this function will not have any effect on it.
 */
EAPI void             ewk_network_tls_certificate_check_set(Eina_Bool enable);

/**
 * Returns the path to a file containing the platform's root X.509 CA certificates.
 *
 * The file is a list of concatenated PEM-format X.509 certificates used as root CA certificates.
 * They are used to validate all the certificates received when a TLS connection (such as an HTTPS one) is made.
 *
 * If @c ewk_network_tls_certificate_check_get() returns @c EINA_TRUE, the certificates set by this function
 * will be used to decide whether a certificate provided by a web site is invalid and the request should then
 * be cancelled.
 *
 * By default, the path is not set, so all certificates are considered as not signed by a trusted root CA.
 *
 * @sa ewk_network_tls_certificate_check_set
 *
 * @note If the libsoup backend is being used, this function has effect on
 * the @b default SoupSession, returned by ewk_network_default_soup_session_get().
 * If a different SoupSession is used and passed to ewk_view_soup_session_set(),
 * this function will not have any effect on it.
 */
EAPI const char      *ewk_network_tls_ca_certificates_path_get(void);

/**
 * Sets the path to a file containing the platform's root X.509 CA certificates.
 *
 * The file is a list of concatenated PEM-format X.509 certificates used as root CA certificates.
 * They are used to validate all the certificates received when a TLS connection (such as an HTTPS one) is made.
 *
 * If @c ewk_network_tls_certificate_check_get() returns @c EINA_TRUE, the certificates set by this function
 * will be used to decide whether a certificate provided by a web site is invalid and the request should then
 * be cancelled.
 *
 * By default, the path is not set, so all certificates are considered as not signed by a trusted root CA.
 *
 * @param path The path to the certificate bundle.
 *
 * @sa ewk_network_tls_certificate_check_set
 *
 * @note If the libsoup backend is being used, this function has effect on
 * the @b default SoupSession, returned by ewk_network_default_soup_session_get().
 * If a different SoupSession is used and passed to ewk_view_soup_session_set(),
 * this function will not have any effect on it.
 */
EAPI void             ewk_network_tls_ca_certificates_path_set(const char *path);

/**
 * Returns the default @c SoupSession used by all views.
 *
 * @return The default @c SoupSession in use.
 */
EAPI SoupSession     *ewk_network_default_soup_session_get(void);

#ifdef __cplusplus
}
#endif
#endif // ewk_network_h
