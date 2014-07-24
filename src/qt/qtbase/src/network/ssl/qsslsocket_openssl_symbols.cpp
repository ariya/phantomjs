/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#include "qsslsocket_openssl_symbols_p.h"

#ifdef Q_OS_WIN
# include <private/qsystemlibrary_p.h>
#else
# include <QtCore/qlibrary.h>
#endif
#include <QtCore/qmutex.h>
#include <private/qmutexpool_p.h>
#include <QtCore/qdatetime.h>
#if defined(Q_OS_UNIX)
#include <QtCore/qdir.h>
#endif
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <link.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

/*
    Note to maintainer:
    -------------------

    We load OpenSSL symbols dynamically. Because symbols are known to
    disappear, and signatures sometimes change, between releases, we need to
    be careful about how this is done. To ensure we don't end up dereferencing
    null function pointers, and continue running even if certain functions are
    missing, we define helper functions for each of the symbols we load from
    OpenSSL, all prefixed with "q_" (declared in
    qsslsocket_openssl_symbols_p.h). So instead of calling SSL_connect
    directly, we call q_SSL_connect, which is a function that checks if the
    actual SSL_connect fptr is null, and returns a failure if it is, or calls
    SSL_connect if it isn't.

    This requires a somewhat tedious process of declaring each function we
    want to call in OpenSSL thrice: once with the q_, in _p.h, once using the
    DEFINEFUNC macros below, and once in the function that actually resolves
    the symbols, below the DEFINEFUNC declarations below.

    There's one DEFINEFUNC macro declared for every number of arguments
    exposed by OpenSSL (feel free to extend when needed). The easiest thing to
    do is to find an existing entry that matches the arg count of the function
    you want to import, and do the same.

    The first macro arg is the function return type. The second is the
    verbatim name of the function/symbol. Then follows a list of N pairs of
    argument types with a variable name, and just the variable name (char *a,
    a, char *b, b, etc). Finally there's two arguments - a suitable return
    statement for the error case (for an int function, return 0 or return -1
    is usually right). Then either just "return" or DUMMYARG, the latter being
    for void functions.

    Note: Take into account that these macros and declarations are processed
    at compile-time, and the result depends on the OpenSSL headers the
    compiling host has installed, but the symbols are resolved at run-time,
    possibly with a different version of OpenSSL.
*/

namespace {
void qsslSocketUnresolvedSymbolWarning(const char *functionName)
{
    qWarning("QSslSocket: cannot call unresolved function %s", functionName);
}

void qsslSocketCannotResolveSymbolWarning(const char *functionName)
{
    qWarning("QSslSocket: cannot resolve %s", functionName);
}
}

#ifdef SSLEAY_MACROS
DEFINEFUNC3(void *, ASN1_dup, i2d_of_void *a, a, d2i_of_void *b, b, char *c, c, return 0, return)
#endif
DEFINEFUNC(long, ASN1_INTEGER_get, ASN1_INTEGER *a, a, return 0, return)
DEFINEFUNC(unsigned char *, ASN1_STRING_data, ASN1_STRING *a, a, return 0, return)
DEFINEFUNC(int, ASN1_STRING_length, ASN1_STRING *a, a, return 0, return)
DEFINEFUNC2(int, ASN1_STRING_to_UTF8, unsigned char **a, a, ASN1_STRING *b, b, return 0, return);
DEFINEFUNC4(long, BIO_ctrl, BIO *a, a, int b, b, long c, c, void *d, d, return -1, return)
DEFINEFUNC(int, BIO_free, BIO *a, a, return 0, return)
DEFINEFUNC(BIO *, BIO_new, BIO_METHOD *a, a, return 0, return)
DEFINEFUNC2(BIO *, BIO_new_mem_buf, void *a, a, int b, b, return 0, return)
DEFINEFUNC3(int, BIO_read, BIO *a, a, void *b, b, int c, c, return -1, return)
DEFINEFUNC(BIO_METHOD *, BIO_s_mem, void, DUMMYARG, return 0, return)
DEFINEFUNC3(int, BIO_write, BIO *a, a, const void *b, b, int c, c, return -1, return)
DEFINEFUNC(int, BN_num_bits, const BIGNUM *a, a, return 0, return)
DEFINEFUNC(int, CRYPTO_num_locks, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(void, CRYPTO_set_locking_callback, void (*a)(int, int, const char *, int), a, return, DUMMYARG)
DEFINEFUNC(void, CRYPTO_set_id_callback, unsigned long (*a)(), a, return, DUMMYARG)
DEFINEFUNC(void, CRYPTO_free, void *a, a, return, DUMMYARG)
DEFINEFUNC(void, DSA_free, DSA *a, a, return, DUMMYARG)
#if  OPENSSL_VERSION_NUMBER < 0x00908000L
DEFINEFUNC3(X509 *, d2i_X509, X509 **a, a, unsigned char **b, b, long c, c, return 0, return)
#else // 0.9.8 broke SC and BC by changing this signature.
DEFINEFUNC3(X509 *, d2i_X509, X509 **a, a, const unsigned char **b, b, long c, c, return 0, return)
#endif
DEFINEFUNC2(char *, ERR_error_string, unsigned long a, a, char *b, b, return 0, return)
DEFINEFUNC(unsigned long, ERR_get_error, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(void, ERR_free_strings, void, DUMMYARG, return, DUMMYARG)
DEFINEFUNC(const EVP_CIPHER *, EVP_des_ede3_cbc, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC3(int, EVP_PKEY_assign, EVP_PKEY *a, a, int b, b, char *c, c, return -1, return)
DEFINEFUNC2(int, EVP_PKEY_set1_RSA, EVP_PKEY *a, a, RSA *b, b, return -1, return)
DEFINEFUNC2(int, EVP_PKEY_set1_DSA, EVP_PKEY *a, a, DSA *b, b, return -1, return)
DEFINEFUNC(void, EVP_PKEY_free, EVP_PKEY *a, a, return, DUMMYARG)
DEFINEFUNC(DSA *, EVP_PKEY_get1_DSA, EVP_PKEY *a, a, return 0, return)
DEFINEFUNC(RSA *, EVP_PKEY_get1_RSA, EVP_PKEY *a, a, return 0, return)
DEFINEFUNC(EVP_PKEY *, EVP_PKEY_new, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(int, EVP_PKEY_type, int a, a, return NID_undef, return)
DEFINEFUNC2(int, i2d_X509, X509 *a, a, unsigned char **b, b, return -1, return)
DEFINEFUNC(const char *, OBJ_nid2sn, int a, a, return 0, return)
DEFINEFUNC(const char *, OBJ_nid2ln, int a, a, return 0, return)
DEFINEFUNC3(int, i2t_ASN1_OBJECT, char *a, a, int b, b, ASN1_OBJECT *c, c, return -1, return)
DEFINEFUNC4(int, OBJ_obj2txt, char *a, a, int b, b, ASN1_OBJECT *c, c, int d, d, return -1, return)

DEFINEFUNC(int, OBJ_obj2nid, const ASN1_OBJECT *a, a, return NID_undef, return)
#ifdef SSLEAY_MACROS
DEFINEFUNC6(void *, PEM_ASN1_read_bio, d2i_of_void *a, a, const char *b, b, BIO *c, c, void **d, d, pem_password_cb *e, e, void *f, f, return 0, return)
DEFINEFUNC6(void *, PEM_ASN1_write_bio, d2i_of_void *a, a, const char *b, b, BIO *c, c, void **d, d, pem_password_cb *e, e, void *f, f, return 0, return)
#else
DEFINEFUNC4(DSA *, PEM_read_bio_DSAPrivateKey, BIO *a, a, DSA **b, b, pem_password_cb *c, c, void *d, d, return 0, return)
DEFINEFUNC4(RSA *, PEM_read_bio_RSAPrivateKey, BIO *a, a, RSA **b, b, pem_password_cb *c, c, void *d, d, return 0, return)
DEFINEFUNC7(int, PEM_write_bio_DSAPrivateKey, BIO *a, a, DSA *b, b, const EVP_CIPHER *c, c, unsigned char *d, d, int e, e, pem_password_cb *f, f, void *g, g, return 0, return)
DEFINEFUNC7(int, PEM_write_bio_RSAPrivateKey, BIO *a, a, RSA *b, b, const EVP_CIPHER *c, c, unsigned char *d, d, int e, e, pem_password_cb *f, f, void *g, g, return 0, return)
#endif
DEFINEFUNC4(DSA *, PEM_read_bio_DSA_PUBKEY, BIO *a, a, DSA **b, b, pem_password_cb *c, c, void *d, d, return 0, return)
DEFINEFUNC4(RSA *, PEM_read_bio_RSA_PUBKEY, BIO *a, a, RSA **b, b, pem_password_cb *c, c, void *d, d, return 0, return)
DEFINEFUNC2(int, PEM_write_bio_DSA_PUBKEY, BIO *a, a, DSA *b, b, return 0, return)
DEFINEFUNC2(int, PEM_write_bio_RSA_PUBKEY, BIO *a, a, RSA *b, b, return 0, return)
DEFINEFUNC2(void, RAND_seed, const void *a, a, int b, b, return, DUMMYARG)
DEFINEFUNC(int, RAND_status, void, DUMMYARG, return -1, return)
DEFINEFUNC(void, RSA_free, RSA *a, a, return, DUMMYARG)
DEFINEFUNC(int, sk_num, STACK *a, a, return -1, return)
DEFINEFUNC2(void, sk_pop_free, STACK *a, a, void (*b)(void*), b, return, DUMMYARG)
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
DEFINEFUNC(_STACK *, sk_new_null, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC2(void, sk_push, _STACK *a, a, void *b, b, return, DUMMYARG)
DEFINEFUNC(void, sk_free, _STACK *a, a, return, DUMMYARG)
DEFINEFUNC2(void *, sk_value, STACK *a, a, int b, b, return 0, return)
#else
DEFINEFUNC(STACK *, sk_new_null, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC2(void, sk_push, STACK *a, a, char *b, b, return, DUMMYARG)
DEFINEFUNC(void, sk_free, STACK *a, a, return, DUMMYARG)
DEFINEFUNC2(char *, sk_value, STACK *a, a, int b, b, return 0, return)
#endif
DEFINEFUNC(int, SSL_accept, SSL *a, a, return -1, return)
DEFINEFUNC(int, SSL_clear, SSL *a, a, return -1, return)
DEFINEFUNC3(char *, SSL_CIPHER_description, SSL_CIPHER *a, a, char *b, b, int c, c, return 0, return)
DEFINEFUNC(int, SSL_connect, SSL *a, a, return -1, return)
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
DEFINEFUNC(int, SSL_CTX_check_private_key, const SSL_CTX *a, a, return -1, return)
#else
DEFINEFUNC(int, SSL_CTX_check_private_key, SSL_CTX *a, a, return -1, return)
#endif
DEFINEFUNC4(long, SSL_CTX_ctrl, SSL_CTX *a, a, int b, b, long c, c, void *d, d, return -1, return)
DEFINEFUNC(void, SSL_CTX_free, SSL_CTX *a, a, return, DUMMYARG)
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
DEFINEFUNC(SSL_CTX *, SSL_CTX_new, const SSL_METHOD *a, a, return 0, return)
#else
DEFINEFUNC(SSL_CTX *, SSL_CTX_new, SSL_METHOD *a, a, return 0, return)
#endif
DEFINEFUNC2(int, SSL_CTX_set_cipher_list, SSL_CTX *a, a, const char *b, b, return -1, return)
DEFINEFUNC(int, SSL_CTX_set_default_verify_paths, SSL_CTX *a, a, return -1, return)
DEFINEFUNC3(void, SSL_CTX_set_verify, SSL_CTX *a, a, int b, b, int (*c)(int, X509_STORE_CTX *), c, return, DUMMYARG)
DEFINEFUNC2(void, SSL_CTX_set_verify_depth, SSL_CTX *a, a, int b, b, return, DUMMYARG)
DEFINEFUNC2(int, SSL_CTX_use_certificate, SSL_CTX *a, a, X509 *b, b, return -1, return)
DEFINEFUNC3(int, SSL_CTX_use_certificate_file, SSL_CTX *a, a, const char *b, b, int c, c, return -1, return)
DEFINEFUNC2(int, SSL_CTX_use_PrivateKey, SSL_CTX *a, a, EVP_PKEY *b, b, return -1, return)
DEFINEFUNC2(int, SSL_CTX_use_RSAPrivateKey, SSL_CTX *a, a, RSA *b, b, return -1, return)
DEFINEFUNC3(int, SSL_CTX_use_PrivateKey_file, SSL_CTX *a, a, const char *b, b, int c, c, return -1, return)
DEFINEFUNC(void, SSL_free, SSL *a, a, return, DUMMYARG)
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
DEFINEFUNC(STACK_OF(SSL_CIPHER) *, SSL_get_ciphers, const SSL *a, a, return 0, return)
#else
DEFINEFUNC(STACK_OF(SSL_CIPHER) *, SSL_get_ciphers, SSL *a, a, return 0, return)
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
DEFINEFUNC(const SSL_CIPHER *, SSL_get_current_cipher, SSL *a, a, return 0, return)
#else
DEFINEFUNC(SSL_CIPHER *, SSL_get_current_cipher, SSL *a, a, return 0, return)
#endif
DEFINEFUNC2(int, SSL_get_error, SSL *a, a, int b, b, return -1, return)
DEFINEFUNC(STACK_OF(X509) *, SSL_get_peer_cert_chain, SSL *a, a, return 0, return)
DEFINEFUNC(X509 *, SSL_get_peer_certificate, SSL *a, a, return 0, return)
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
DEFINEFUNC(long, SSL_get_verify_result, const SSL *a, a, return -1, return)
#else
DEFINEFUNC(long, SSL_get_verify_result, SSL *a, a, return -1, return)
#endif
DEFINEFUNC(int, SSL_library_init, void, DUMMYARG, return -1, return)
DEFINEFUNC(void, SSL_load_error_strings, void, DUMMYARG, return, DUMMYARG)
DEFINEFUNC(SSL *, SSL_new, SSL_CTX *a, a, return 0, return)
#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
DEFINEFUNC4(long, SSL_ctrl, SSL *a, a, int cmd, cmd, long larg, larg, void *parg, parg, return -1, return)
#endif
DEFINEFUNC3(int, SSL_read, SSL *a, a, void *b, b, int c, c, return -1, return)
DEFINEFUNC3(void, SSL_set_bio, SSL *a, a, BIO *b, b, BIO *c, c, return, DUMMYARG)
DEFINEFUNC(void, SSL_set_accept_state, SSL *a, a, return, DUMMYARG)
DEFINEFUNC(void, SSL_set_connect_state, SSL *a, a, return, DUMMYARG)
DEFINEFUNC(int, SSL_shutdown, SSL *a, a, return -1, return)
DEFINEFUNC2(int, SSL_set_session, SSL* to, to, SSL_SESSION *session, session, return -1, return)
DEFINEFUNC(void, SSL_SESSION_free, SSL_SESSION *ses, ses, return, DUMMYARG)
DEFINEFUNC(SSL_SESSION*, SSL_get1_session, SSL *ssl, ssl, return 0, return)
DEFINEFUNC(SSL_SESSION*, SSL_get_session, const SSL *ssl, ssl, return 0, return)
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#ifndef OPENSSL_NO_SSL2
DEFINEFUNC(const SSL_METHOD *, SSLv2_client_method, DUMMYARG, DUMMYARG, return 0, return)
#endif
DEFINEFUNC(const SSL_METHOD *, SSLv3_client_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(const SSL_METHOD *, SSLv23_client_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(const SSL_METHOD *, TLSv1_client_method, DUMMYARG, DUMMYARG, return 0, return)
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
DEFINEFUNC(const SSL_METHOD *, TLSv1_1_client_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(const SSL_METHOD *, TLSv1_2_client_method, DUMMYARG, DUMMYARG, return 0, return)
#endif
#ifndef OPENSSL_NO_SSL2
DEFINEFUNC(const SSL_METHOD *, SSLv2_server_method, DUMMYARG, DUMMYARG, return 0, return)
#endif
DEFINEFUNC(const SSL_METHOD *, SSLv3_server_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(const SSL_METHOD *, SSLv23_server_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(const SSL_METHOD *, TLSv1_server_method, DUMMYARG, DUMMYARG, return 0, return)
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
DEFINEFUNC(const SSL_METHOD *, TLSv1_1_server_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(const SSL_METHOD *, TLSv1_2_server_method, DUMMYARG, DUMMYARG, return 0, return)
#endif
#else
DEFINEFUNC(SSL_METHOD *, SSLv2_client_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(SSL_METHOD *, SSLv3_client_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(SSL_METHOD *, SSLv23_client_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(SSL_METHOD *, TLSv1_client_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(SSL_METHOD *, SSLv2_server_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(SSL_METHOD *, SSLv3_server_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(SSL_METHOD *, SSLv23_server_method, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(SSL_METHOD *, TLSv1_server_method, DUMMYARG, DUMMYARG, return 0, return)
#endif
DEFINEFUNC3(int, SSL_write, SSL *a, a, const void *b, b, int c, c, return -1, return)
DEFINEFUNC2(int, X509_cmp, X509 *a, a, X509 *b, b, return -1, return)
#ifndef SSLEAY_MACROS
DEFINEFUNC(X509 *, X509_dup, X509 *a, a, return 0, return)
#endif
DEFINEFUNC2(void, X509_print, BIO *a, a, X509 *b, b, return, DUMMYARG);
DEFINEFUNC(ASN1_OBJECT *, X509_EXTENSION_get_object, X509_EXTENSION *a, a, return 0, return)
DEFINEFUNC(void, X509_free, X509 *a, a, return, DUMMYARG)
DEFINEFUNC2(X509_EXTENSION *, X509_get_ext, X509 *a, a, int b, b, return 0, return)
DEFINEFUNC(int, X509_get_ext_count, X509 *a, a, return 0, return)
DEFINEFUNC4(void *, X509_get_ext_d2i, X509 *a, a, int b, b, int *c, c, int *d, d, return 0, return)
DEFINEFUNC(const X509V3_EXT_METHOD *, X509V3_EXT_get, X509_EXTENSION *a, a, return 0, return)
DEFINEFUNC(void *, X509V3_EXT_d2i, X509_EXTENSION *a, a, return 0, return)
DEFINEFUNC(int, X509_EXTENSION_get_critical, X509_EXTENSION *a, a, return 0, return)
DEFINEFUNC(ASN1_OCTET_STRING *, X509_EXTENSION_get_data, X509_EXTENSION *a, a, return 0, return)
DEFINEFUNC(void, BASIC_CONSTRAINTS_free, BASIC_CONSTRAINTS *a, a, return, DUMMYARG)
DEFINEFUNC(void, AUTHORITY_KEYID_free, AUTHORITY_KEYID *a, a, return, DUMMYARG)
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
DEFINEFUNC2(int, ASN1_STRING_print, BIO *a, a, const ASN1_STRING *b, b, return 0, return)
#else
DEFINEFUNC2(int, ASN1_STRING_print, BIO *a, a, ASN1_STRING *b, b, return 0, return)
#endif
DEFINEFUNC(X509_NAME *, X509_get_issuer_name, X509 *a, a, return 0, return)
DEFINEFUNC(X509_NAME *, X509_get_subject_name, X509 *a, a, return 0, return)
DEFINEFUNC(int, X509_verify_cert, X509_STORE_CTX *a, a, return -1, return)
DEFINEFUNC(int, X509_NAME_entry_count, X509_NAME *a, a, return 0, return)
DEFINEFUNC2(X509_NAME_ENTRY *, X509_NAME_get_entry, X509_NAME *a, a, int b, b, return 0, return)
DEFINEFUNC(ASN1_STRING *, X509_NAME_ENTRY_get_data, X509_NAME_ENTRY *a, a, return 0, return)
DEFINEFUNC(ASN1_OBJECT *, X509_NAME_ENTRY_get_object, X509_NAME_ENTRY *a, a, return 0, return)
DEFINEFUNC(EVP_PKEY *, X509_PUBKEY_get, X509_PUBKEY *a, a, return 0, return)
DEFINEFUNC(void, X509_STORE_free, X509_STORE *a, a, return, DUMMYARG)
DEFINEFUNC(X509_STORE *, X509_STORE_new, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC2(int, X509_STORE_add_cert, X509_STORE *a, a, X509 *b, b, return 0, return)
DEFINEFUNC(void, X509_STORE_CTX_free, X509_STORE_CTX *a, a, return, DUMMYARG)
DEFINEFUNC4(int, X509_STORE_CTX_init, X509_STORE_CTX *a, a, X509_STORE *b, b, X509 *c, c, STACK_OF(X509) *d, d, return -1, return)
DEFINEFUNC2(int, X509_STORE_CTX_set_purpose, X509_STORE_CTX *a, a, int b, b, return -1, return)
DEFINEFUNC(int, X509_STORE_CTX_get_error, X509_STORE_CTX *a, a, return -1, return)
DEFINEFUNC(int, X509_STORE_CTX_get_error_depth, X509_STORE_CTX *a, a, return -1, return)
DEFINEFUNC(X509 *, X509_STORE_CTX_get_current_cert, X509_STORE_CTX *a, a, return 0, return)
DEFINEFUNC(STACK_OF(X509) *, X509_STORE_CTX_get_chain, X509_STORE_CTX *a, a, return 0, return)
DEFINEFUNC(X509_STORE_CTX *, X509_STORE_CTX_new, DUMMYARG, DUMMYARG, return 0, return)
#ifdef SSLEAY_MACROS
DEFINEFUNC2(int, i2d_DSAPrivateKey, const DSA *a, a, unsigned char **b, b, return -1, return)
DEFINEFUNC2(int, i2d_RSAPrivateKey, const RSA *a, a, unsigned char **b, b, return -1, return)
DEFINEFUNC3(RSA *, d2i_RSAPrivateKey, RSA **a, a, unsigned char **b, b, long c, c, return 0, return)
DEFINEFUNC3(DSA *, d2i_DSAPrivateKey, DSA **a, a, unsigned char **b, b, long c, c, return 0, return)
#endif
DEFINEFUNC(void, OPENSSL_add_all_algorithms_noconf, void, DUMMYARG, return, DUMMYARG)
DEFINEFUNC(void, OPENSSL_add_all_algorithms_conf, void, DUMMYARG, return, DUMMYARG)
DEFINEFUNC3(int, SSL_CTX_load_verify_locations, SSL_CTX *ctx, ctx, const char *CAfile, CAfile, const char *CApath, CApath, return 0, return)
DEFINEFUNC(long, SSLeay, void, DUMMYARG, return 0, return)
DEFINEFUNC(const char *, SSLeay_version, int a, a, return 0, return)
DEFINEFUNC2(int, i2d_SSL_SESSION, SSL_SESSION *in, in, unsigned char **pp, pp, return 0, return)
DEFINEFUNC3(SSL_SESSION *, d2i_SSL_SESSION, SSL_SESSION **a, a, const unsigned char **pp, pp, long length, length, return 0, return)
#if OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
DEFINEFUNC6(int, SSL_select_next_proto, unsigned char **out, out, unsigned char *outlen, outlen,
            const unsigned char *in, in, unsigned int inlen, inlen,
            const unsigned char *client, client, unsigned int client_len, client_len,
            return -1, return)
DEFINEFUNC3(void, SSL_CTX_set_next_proto_select_cb, SSL_CTX *s, s,
            int (*cb) (SSL *ssl, unsigned char **out,
                       unsigned char *outlen,
                       const unsigned char *in,
                       unsigned int inlen, void *arg), cb,
            void *arg, arg, return, DUMMYARG)
DEFINEFUNC3(void, SSL_get0_next_proto_negotiated, const SSL *s, s,
            const unsigned char **data, data, unsigned *len, len, return, DUMMYARG)
#endif // OPENSSL_VERSION_NUMBER >= 0x1000100fL ...
DEFINEFUNC(DH *, DH_new, DUMMYARG, DUMMYARG, return 0, return)
DEFINEFUNC(void, DH_free, DH *dh, dh, return, DUMMYARG)
DEFINEFUNC3(BIGNUM *, BN_bin2bn, const unsigned char *s, s, int len, len, BIGNUM *ret, ret, return 0, return)
DEFINEFUNC(EC_KEY *, EC_KEY_new_by_curve_name, int nid, nid, return 0, return)
DEFINEFUNC(void, EC_KEY_free, EC_KEY *ecdh, ecdh, return, DUMMYARG)

#define RESOLVEFUNC(func) \
    if (!(_q_##func = _q_PTR_##func(libs.first->resolve(#func)))     \
        && !(_q_##func = _q_PTR_##func(libs.second->resolve(#func)))) \
        qsslSocketCannotResolveSymbolWarning(#func);

#if !defined QT_LINKED_OPENSSL

#ifdef QT_NO_LIBRARY
bool q_resolveOpenSslSymbols()
{
    qWarning("QSslSocket: unable to resolve symbols. "
             "QT_NO_LIBRARY is defined which means runtime resolving of "
             "libraries won't work.");
    qWarning("Either compile Qt statically or with support for runtime resolving "
             "of libraries.");
    return false;
}
#else

# ifdef Q_OS_UNIX
static bool libGreaterThan(const QString &lhs, const QString &rhs)
{
    QStringList lhsparts = lhs.split(QLatin1Char('.'));
    QStringList rhsparts = rhs.split(QLatin1Char('.'));
    Q_ASSERT(lhsparts.count() > 1 && rhsparts.count() > 1);

    for (int i = 1; i < rhsparts.count(); ++i) {
        if (lhsparts.count() <= i)
            // left hand side is shorter, so it's less than rhs
            return false;

        bool ok = false;
        int b = 0;
        int a = lhsparts.at(i).toInt(&ok);
        if (ok)
            b = rhsparts.at(i).toInt(&ok);
        if (ok) {
            // both toInt succeeded
            if (a == b)
                continue;
            return a > b;
        } else {
            // compare as strings;
            if (lhsparts.at(i) == rhsparts.at(i))
                continue;
            return lhsparts.at(i) > rhsparts.at(i);
        }
    }

    // they compared strictly equally so far
    // lhs cannot be less than rhs
    return true;
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
static int dlIterateCallback(struct dl_phdr_info *info, size_t size, void *data)
{
    if (size < sizeof (info->dlpi_addr) + sizeof (info->dlpi_name))
        return 1;
    QSet<QString> *paths = (QSet<QString> *)data;
    QString path = QString::fromLocal8Bit(info->dlpi_name);
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        path = fi.absolutePath();
        if (!path.isEmpty())
            paths->insert(path);
    }
    return 0;
}
#endif

static QStringList libraryPathList()
{
    QStringList paths;
#  ifdef Q_OS_DARWIN
    paths = QString::fromLatin1(qgetenv("DYLD_LIBRARY_PATH"))
            .split(QLatin1Char(':'), QString::SkipEmptyParts);
#  else
    paths = QString::fromLatin1(qgetenv("LD_LIBRARY_PATH"))
            .split(QLatin1Char(':'), QString::SkipEmptyParts);
#  endif
    paths << QLatin1String("/lib") << QLatin1String("/usr/lib") << QLatin1String("/usr/local/lib");
    paths << QLatin1String("/lib64") << QLatin1String("/usr/lib64") << QLatin1String("/usr/local/lib64");
    paths << QLatin1String("/lib32") << QLatin1String("/usr/lib32") << QLatin1String("/usr/local/lib32");

#if defined(Q_OS_ANDROID)
    paths << QLatin1String("/system/lib");
#elif defined(Q_OS_LINUX)
    // discover paths of already loaded libraries
    QSet<QString> loadedPaths;
    dl_iterate_phdr(dlIterateCallback, &loadedPaths);
    paths.append(loadedPaths.toList());
#endif

    return paths;
}


static QStringList findAllLibSsl()
{
    QStringList paths = libraryPathList();
    QStringList foundSsls;

    foreach (const QString &path, paths) {
        QDir dir(path);
        QStringList entryList = dir.entryList(QStringList() << QLatin1String("libssl.*"), QDir::Files);

        std::sort(entryList.begin(), entryList.end(), libGreaterThan);
        foreach (const QString &entry, entryList)
            foundSsls << path + QLatin1Char('/') + entry;
    }

    return foundSsls;
}

static QStringList findAllLibCrypto()
{
    QStringList paths = libraryPathList();

    QStringList foundCryptos;
    foreach (const QString &path, paths) {
        QDir dir(path);
        QStringList entryList = dir.entryList(QStringList() << QLatin1String("libcrypto.*"), QDir::Files);

        std::sort(entryList.begin(), entryList.end(), libGreaterThan);
        foreach (const QString &entry, entryList)
            foundCryptos << path + QLatin1Char('/') + entry;
    }

    return foundCryptos;
}
# endif

#ifdef Q_OS_WIN
static bool tryToLoadOpenSslWin32Library(QLatin1String ssleay32LibName, QLatin1String libeay32LibName, QPair<QSystemLibrary*, QSystemLibrary*> &pair)
{
    pair.first = 0;
    pair.second = 0;

    QSystemLibrary *ssleay32 = new QSystemLibrary(ssleay32LibName);
    if (!ssleay32->load(false)) {
        delete ssleay32;
        return FALSE;
    }

    QSystemLibrary *libeay32 = new QSystemLibrary(libeay32LibName);
    if (!libeay32->load(false)) {
        delete ssleay32;
        delete libeay32;
        return FALSE;
    }

    pair.first = ssleay32;
    pair.second = libeay32;
    return TRUE;
}

static QPair<QSystemLibrary*, QSystemLibrary*> loadOpenSslWin32()
{
    QPair<QSystemLibrary*,QSystemLibrary*> pair;
    pair.first = 0;
    pair.second = 0;

    // When OpenSSL is built using MSVC then the libraries are named 'ssleay32.dll' and 'libeay32'dll'.
    // When OpenSSL is built using GCC then different library names are used (depending on the OpenSSL version)
    // The oldest version of a GCC-based OpenSSL which can be detected by the code below is 0.9.8g (released in 2007)
    if (!tryToLoadOpenSslWin32Library(QLatin1String("ssleay32"), QLatin1String("libeay32"), pair)) {
        if (!tryToLoadOpenSslWin32Library(QLatin1String("libssl-10"), QLatin1String("libcrypto-10"), pair)) {
            if (!tryToLoadOpenSslWin32Library(QLatin1String("libssl-8"), QLatin1String("libcrypto-8"), pair)) {
                tryToLoadOpenSslWin32Library(QLatin1String("libssl-7"), QLatin1String("libcrypto-7"), pair);
            }
        }
    }

    return pair;
}
#else

static QPair<QLibrary*, QLibrary*> loadOpenSsl()
{
    QPair<QLibrary*,QLibrary*> pair;

# if defined(Q_OS_UNIX)
    QLibrary *&libssl = pair.first;
    QLibrary *&libcrypto = pair.second;
    libssl = new QLibrary;
    libcrypto = new QLibrary;

    // Try to find the libssl library on the system.
    //
    // Up until Qt 4.3, this only searched for the "ssl" library at version -1, that
    // is, libssl.so on most Unix systems.  However, the .so file isn't present in
    // user installations because it's considered a development file.
    //
    // The right thing to do is to load the library at the major version we know how
    // to work with: the SHLIB_VERSION_NUMBER version (macro defined in opensslv.h)
    //
    // However, OpenSSL is a well-known case of binary-compatibility breakage. To
    // avoid such problems, many system integrators and Linux distributions change
    // the soname of the binary, letting the full version number be the soname. So
    // we'll find libssl.so.0.9.7, libssl.so.0.9.8, etc. in the system. For that
    // reason, we will search a few common paths (see findAllLibSsl() above) in hopes
    // we find one that works.
    //
    // It is important, however, to try the canonical name and the unversioned name
    // without going through the loop. By not specifying a path, we let the system
    // dlopen(3) function determine it for us. This will include any DT_RUNPATH or
    // DT_RPATH tags on our library header as well as other system-specific search
    // paths. See the man page for dlopen(3) on your system for more information.

#ifdef Q_OS_OPENBSD
    libcrypto->setLoadHints(QLibrary::ExportExternalSymbolsHint);
#endif
#if defined(SHLIB_VERSION_NUMBER) && !defined(Q_OS_QNX) // on QNX, the libs are always libssl.so and libcrypto.so
    // first attempt: the canonical name is libssl.so.<SHLIB_VERSION_NUMBER>
    libssl->setFileNameAndVersion(QLatin1String("ssl"), QLatin1String(SHLIB_VERSION_NUMBER));
    libcrypto->setFileNameAndVersion(QLatin1String("crypto"), QLatin1String(SHLIB_VERSION_NUMBER));
    if (libcrypto->load() && libssl->load()) {
        // libssl.so.<SHLIB_VERSION_NUMBER> and libcrypto.so.<SHLIB_VERSION_NUMBER> found
        return pair;
    } else {
        libssl->unload();
        libcrypto->unload();
    }
#endif

    // second attempt: find the development files libssl.so and libcrypto.so
    libssl->setFileNameAndVersion(QLatin1String("ssl"), -1);
    libcrypto->setFileNameAndVersion(QLatin1String("crypto"), -1);
    if (libcrypto->load() && libssl->load()) {
        // libssl.so.0 and libcrypto.so.0 found
        return pair;
    } else {
        libssl->unload();
        libcrypto->unload();
    }

    // third attempt: loop on the most common library paths and find libssl
    QStringList sslList = findAllLibSsl();
    QStringList cryptoList = findAllLibCrypto();

    foreach (const QString &crypto, cryptoList) {
        libcrypto->setFileNameAndVersion(crypto, -1);
        if (libcrypto->load()) {
            QFileInfo fi(crypto);
            QString version = fi.completeSuffix();

            foreach (const QString &ssl, sslList) {
                if (!ssl.endsWith(version))
                    continue;

                libssl->setFileNameAndVersion(ssl, -1);

                if (libssl->load()) {
                    // libssl.so.x and libcrypto.so.x found
                    return pair;
                } else {
                    libssl->unload();
                }
            }
        }
        libcrypto->unload();
    }

    // failed to load anything
    delete libssl;
    delete libcrypto;
    libssl = libcrypto = 0;
    return pair;

# else
    // not implemented for this platform yet
    return pair;
# endif
}
#endif

bool q_resolveOpenSslSymbols()
{
    static bool symbolsResolved = false;
    static bool triedToResolveSymbols = false;
#ifndef QT_NO_THREAD
    QMutexLocker locker(QMutexPool::globalInstanceGet((void *)&q_SSL_library_init));
#endif
    if (symbolsResolved)
        return true;
    if (triedToResolveSymbols)
        return false;
    triedToResolveSymbols = true;

#ifdef Q_OS_WIN
    QPair<QSystemLibrary *, QSystemLibrary *> libs = loadOpenSslWin32();
#else
    QPair<QLibrary *, QLibrary *> libs = loadOpenSsl();
#endif
    if (!libs.first || !libs.second)
        // failed to load them
        return false;

#ifdef SSLEAY_MACROS
    RESOLVEFUNC(ASN1_dup)
#endif
    RESOLVEFUNC(ASN1_INTEGER_get)
    RESOLVEFUNC(ASN1_STRING_data)
    RESOLVEFUNC(ASN1_STRING_length)
    RESOLVEFUNC(ASN1_STRING_to_UTF8)
    RESOLVEFUNC(BIO_ctrl)
    RESOLVEFUNC(BIO_free)
    RESOLVEFUNC(BIO_new)
    RESOLVEFUNC(BIO_new_mem_buf)
    RESOLVEFUNC(BIO_read)
    RESOLVEFUNC(BIO_s_mem)
    RESOLVEFUNC(BIO_write)
    RESOLVEFUNC(BN_num_bits)
    RESOLVEFUNC(CRYPTO_free)
    RESOLVEFUNC(CRYPTO_num_locks)
    RESOLVEFUNC(CRYPTO_set_id_callback)
    RESOLVEFUNC(CRYPTO_set_locking_callback)
    RESOLVEFUNC(DSA_free)
    RESOLVEFUNC(ERR_error_string)
    RESOLVEFUNC(ERR_get_error)
    RESOLVEFUNC(ERR_free_strings)
    RESOLVEFUNC(EVP_des_ede3_cbc)
    RESOLVEFUNC(EVP_PKEY_assign)
    RESOLVEFUNC(EVP_PKEY_set1_RSA)
    RESOLVEFUNC(EVP_PKEY_set1_DSA)
    RESOLVEFUNC(EVP_PKEY_free)
    RESOLVEFUNC(EVP_PKEY_get1_DSA)
    RESOLVEFUNC(EVP_PKEY_get1_RSA)
    RESOLVEFUNC(EVP_PKEY_new)
    RESOLVEFUNC(EVP_PKEY_type)
    RESOLVEFUNC(OBJ_nid2sn)
    RESOLVEFUNC(OBJ_nid2ln)
    RESOLVEFUNC(i2t_ASN1_OBJECT)
    RESOLVEFUNC(OBJ_obj2txt)
    RESOLVEFUNC(OBJ_obj2nid)
#ifdef SSLEAY_MACROS // ### verify
    RESOLVEFUNC(PEM_ASN1_read_bio)
#else
    RESOLVEFUNC(PEM_read_bio_DSAPrivateKey)
    RESOLVEFUNC(PEM_read_bio_RSAPrivateKey)
    RESOLVEFUNC(PEM_write_bio_DSAPrivateKey)
    RESOLVEFUNC(PEM_write_bio_RSAPrivateKey)
#endif
    RESOLVEFUNC(PEM_read_bio_DSA_PUBKEY)
    RESOLVEFUNC(PEM_read_bio_RSA_PUBKEY)
    RESOLVEFUNC(PEM_write_bio_DSA_PUBKEY)
    RESOLVEFUNC(PEM_write_bio_RSA_PUBKEY)
    RESOLVEFUNC(RAND_seed)
    RESOLVEFUNC(RAND_status)
    RESOLVEFUNC(RSA_free)
    RESOLVEFUNC(sk_new_null)
    RESOLVEFUNC(sk_push)
    RESOLVEFUNC(sk_free)
    RESOLVEFUNC(sk_num)
    RESOLVEFUNC(sk_pop_free)
    RESOLVEFUNC(sk_value)
    RESOLVEFUNC(SSL_CIPHER_description)
    RESOLVEFUNC(SSL_CTX_check_private_key)
    RESOLVEFUNC(SSL_CTX_ctrl)
    RESOLVEFUNC(SSL_CTX_free)
    RESOLVEFUNC(SSL_CTX_new)
    RESOLVEFUNC(SSL_CTX_set_cipher_list)
    RESOLVEFUNC(SSL_CTX_set_default_verify_paths)
    RESOLVEFUNC(SSL_CTX_set_verify)
    RESOLVEFUNC(SSL_CTX_set_verify_depth)
    RESOLVEFUNC(SSL_CTX_use_certificate)
    RESOLVEFUNC(SSL_CTX_use_certificate_file)
    RESOLVEFUNC(SSL_CTX_use_PrivateKey)
    RESOLVEFUNC(SSL_CTX_use_RSAPrivateKey)
    RESOLVEFUNC(SSL_CTX_use_PrivateKey_file)
    RESOLVEFUNC(SSL_accept)
    RESOLVEFUNC(SSL_clear)
    RESOLVEFUNC(SSL_connect)
    RESOLVEFUNC(SSL_free)
    RESOLVEFUNC(SSL_get_ciphers)
    RESOLVEFUNC(SSL_get_current_cipher)
    RESOLVEFUNC(SSL_get_error)
    RESOLVEFUNC(SSL_get_peer_cert_chain)
    RESOLVEFUNC(SSL_get_peer_certificate)
    RESOLVEFUNC(SSL_get_verify_result)
    RESOLVEFUNC(SSL_library_init)
    RESOLVEFUNC(SSL_load_error_strings)
    RESOLVEFUNC(SSL_new)
#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
    RESOLVEFUNC(SSL_ctrl)
#endif
    RESOLVEFUNC(SSL_read)
    RESOLVEFUNC(SSL_set_accept_state)
    RESOLVEFUNC(SSL_set_bio)
    RESOLVEFUNC(SSL_set_connect_state)
    RESOLVEFUNC(SSL_shutdown)
    RESOLVEFUNC(SSL_set_session)
    RESOLVEFUNC(SSL_SESSION_free)
    RESOLVEFUNC(SSL_get1_session)
    RESOLVEFUNC(SSL_get_session)
    RESOLVEFUNC(SSL_write)
#ifndef OPENSSL_NO_SSL2
    RESOLVEFUNC(SSLv2_client_method)
#endif
    RESOLVEFUNC(SSLv3_client_method)
    RESOLVEFUNC(SSLv23_client_method)
    RESOLVEFUNC(TLSv1_client_method)
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
    RESOLVEFUNC(TLSv1_1_client_method)
    RESOLVEFUNC(TLSv1_2_client_method)
#endif
#ifndef OPENSSL_NO_SSL2
    RESOLVEFUNC(SSLv2_server_method)
#endif
    RESOLVEFUNC(SSLv3_server_method)
    RESOLVEFUNC(SSLv23_server_method)
    RESOLVEFUNC(TLSv1_server_method)
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
    RESOLVEFUNC(TLSv1_1_server_method)
    RESOLVEFUNC(TLSv1_2_server_method)
#endif
    RESOLVEFUNC(X509_NAME_entry_count)
    RESOLVEFUNC(X509_NAME_get_entry)
    RESOLVEFUNC(X509_NAME_ENTRY_get_data)
    RESOLVEFUNC(X509_NAME_ENTRY_get_object)
    RESOLVEFUNC(X509_PUBKEY_get)
    RESOLVEFUNC(X509_STORE_free)
    RESOLVEFUNC(X509_STORE_new)
    RESOLVEFUNC(X509_STORE_add_cert)
    RESOLVEFUNC(X509_STORE_CTX_free)
    RESOLVEFUNC(X509_STORE_CTX_init)
    RESOLVEFUNC(X509_STORE_CTX_new)
    RESOLVEFUNC(X509_STORE_CTX_set_purpose)
    RESOLVEFUNC(X509_STORE_CTX_get_error)
    RESOLVEFUNC(X509_STORE_CTX_get_error_depth)
    RESOLVEFUNC(X509_STORE_CTX_get_current_cert)
    RESOLVEFUNC(X509_STORE_CTX_get_chain)
    RESOLVEFUNC(X509_cmp)
#ifndef SSLEAY_MACROS
    RESOLVEFUNC(X509_dup)
#endif
    RESOLVEFUNC(X509_print)
    RESOLVEFUNC(X509_EXTENSION_get_object)
    RESOLVEFUNC(X509_free)
    RESOLVEFUNC(X509_get_ext)
    RESOLVEFUNC(X509_get_ext_count)
    RESOLVEFUNC(X509_get_ext_d2i)
    RESOLVEFUNC(X509V3_EXT_get)
    RESOLVEFUNC(X509V3_EXT_d2i)
    RESOLVEFUNC(X509_EXTENSION_get_critical)
    RESOLVEFUNC(X509_EXTENSION_get_data)
    RESOLVEFUNC(BASIC_CONSTRAINTS_free)
    RESOLVEFUNC(AUTHORITY_KEYID_free)
    RESOLVEFUNC(ASN1_STRING_print)
    RESOLVEFUNC(X509_get_issuer_name)
    RESOLVEFUNC(X509_get_subject_name)
    RESOLVEFUNC(X509_verify_cert)
    RESOLVEFUNC(d2i_X509)
    RESOLVEFUNC(i2d_X509)
#ifdef SSLEAY_MACROS
    RESOLVEFUNC(i2d_DSAPrivateKey)
    RESOLVEFUNC(i2d_RSAPrivateKey)
    RESOLVEFUNC(d2i_DSAPrivateKey)
    RESOLVEFUNC(d2i_RSAPrivateKey)
#endif
    RESOLVEFUNC(OPENSSL_add_all_algorithms_noconf)
    RESOLVEFUNC(OPENSSL_add_all_algorithms_conf)
    RESOLVEFUNC(SSL_CTX_load_verify_locations)
    RESOLVEFUNC(SSLeay)
    RESOLVEFUNC(SSLeay_version)
    RESOLVEFUNC(i2d_SSL_SESSION)
    RESOLVEFUNC(d2i_SSL_SESSION)
#if OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
    RESOLVEFUNC(SSL_select_next_proto)
    RESOLVEFUNC(SSL_CTX_set_next_proto_select_cb)
    RESOLVEFUNC(SSL_get0_next_proto_negotiated)
#endif // OPENSSL_VERSION_NUMBER >= 0x1000100fL ...
    RESOLVEFUNC(DH_new)
    RESOLVEFUNC(DH_free)
    RESOLVEFUNC(BN_bin2bn)
    RESOLVEFUNC(EC_KEY_new_by_curve_name)
    RESOLVEFUNC(EC_KEY_free)

    symbolsResolved = true;
    delete libs.first;
    delete libs.second;
    return true;
}
#endif // QT_NO_LIBRARY

#else // !defined QT_LINKED_OPENSSL

bool q_resolveOpenSslSymbols()
{
#ifdef QT_NO_OPENSSL
    return false;
#endif
    return true;
}
#endif // !defined QT_LINKED_OPENSSL

//==============================================================================
// contributed by Jay Case of Sarvega, Inc.; http://sarvega.com/
// Based on X509_cmp_time() for intitial buffer hacking.
//==============================================================================
QDateTime q_getTimeFromASN1(const ASN1_TIME *aTime)
{
    size_t lTimeLength = aTime->length;
    char *pString = (char *) aTime->data;

    if (aTime->type == V_ASN1_UTCTIME) {

        char lBuffer[24];
        char *pBuffer = lBuffer;

        if ((lTimeLength < 11) || (lTimeLength > 17))
            return QDateTime();

        memcpy(pBuffer, pString, 10);
        pBuffer += 10;
        pString += 10;

        if ((*pString == 'Z') || (*pString == '-') || (*pString == '+')) {
            *pBuffer++ = '0';
            *pBuffer++ = '0';
        } else {
            *pBuffer++ = *pString++;
            *pBuffer++ = *pString++;
            // Skip any fractional seconds...
            if (*pString == '.') {
                pString++;
                while ((*pString >= '0') && (*pString <= '9'))
                    pString++;
            }
        }

        *pBuffer++ = 'Z';
        *pBuffer++ = '\0';

        time_t lSecondsFromUCT;
        if (*pString == 'Z') {
            lSecondsFromUCT = 0;
        } else {
            if ((*pString != '+') && (*pString != '-'))
                return QDateTime();

            lSecondsFromUCT = ((pString[1] - '0') * 10 + (pString[2] - '0')) * 60;
            lSecondsFromUCT += (pString[3] - '0') * 10 + (pString[4] - '0');
            lSecondsFromUCT *= 60;
            if (*pString == '-')
                lSecondsFromUCT = -lSecondsFromUCT;
        }

        tm lTime;
        lTime.tm_sec = ((lBuffer[10] - '0') * 10) + (lBuffer[11] - '0');
        lTime.tm_min = ((lBuffer[8] - '0') * 10) + (lBuffer[9] - '0');
        lTime.tm_hour = ((lBuffer[6] - '0') * 10) + (lBuffer[7] - '0');
        lTime.tm_mday = ((lBuffer[4] - '0') * 10) + (lBuffer[5] - '0');
        lTime.tm_mon = (((lBuffer[2] - '0') * 10) + (lBuffer[3] - '0')) - 1;
        lTime.tm_year = ((lBuffer[0] - '0') * 10) + (lBuffer[1] - '0');
        if (lTime.tm_year < 50)
            lTime.tm_year += 100; // RFC 2459

        QDate resDate(lTime.tm_year + 1900, lTime.tm_mon + 1, lTime.tm_mday);
        QTime resTime(lTime.tm_hour, lTime.tm_min, lTime.tm_sec);

        QDateTime result(resDate, resTime, Qt::UTC);
        result = result.addSecs(lSecondsFromUCT);
        return result;

    } else if (aTime->type == V_ASN1_GENERALIZEDTIME) {

        if (lTimeLength < 15)
            return QDateTime(); // hopefully never triggered

        // generalized time is always YYYYMMDDHHMMSSZ (RFC 2459, section 4.1.2.5.2)
        tm lTime;
        lTime.tm_sec = ((pString[12] - '0') * 10) + (pString[13] - '0');
        lTime.tm_min = ((pString[10] - '0') * 10) + (pString[11] - '0');
        lTime.tm_hour = ((pString[8] - '0') * 10) + (pString[9] - '0');
        lTime.tm_mday = ((pString[6] - '0') * 10) + (pString[7] - '0');
        lTime.tm_mon = (((pString[4] - '0') * 10) + (pString[5] - '0'));
        lTime.tm_year = ((pString[0] - '0') * 1000) + ((pString[1] - '0') * 100) +
                        ((pString[2] - '0') * 10) + (pString[3] - '0');

        QDate resDate(lTime.tm_year, lTime.tm_mon, lTime.tm_mday);
        QTime resTime(lTime.tm_hour, lTime.tm_min, lTime.tm_sec);

        QDateTime result(resDate, resTime, Qt::UTC);
        return result;

    } else {
        qWarning("unsupported date format detected");
        return QDateTime();
    }

}

QT_END_NAMESPACE
