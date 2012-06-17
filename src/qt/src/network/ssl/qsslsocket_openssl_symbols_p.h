/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QSSLSOCKET_OPENSSL_SYMBOLS_P_H
#define QSSLSOCKET_OPENSSL_SYMBOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qsslsocket_openssl_p.h"

QT_BEGIN_NAMESPACE

#define DUMMYARG

#if !defined QT_LINKED_OPENSSL
// **************** Shared declarations ******************
// ret func(arg)

#  define DEFINEFUNC(ret, func, arg, a, err, funcret)				\
    typedef ret (*_q_PTR_##func)(arg);					\
    static _q_PTR_##func _q_##func = 0;					\
    ret q_##func(arg) {						\
        if (!_q_##func) {				\
            qWarning("QSslSocket: cannot call unresolved function "#func);	\
            err;								\
        } \
        funcret _q_##func(a); \
    }

// ret func(arg1, arg2)
#  define DEFINEFUNC2(ret, func, arg1, a, arg2, b, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2);         \
    static _q_PTR_##func _q_##func = 0;               \
    ret q_##func(arg1, arg2) { \
        if (!_q_##func) { \
            qWarning("QSslSocket: cannot call unresolved function "#func);\
            err; \
        } \
        funcret _q_##func(a, b); \
    }

// ret func(arg1, arg2, arg3)
#  define DEFINEFUNC3(ret, func, arg1, a, arg2, b, arg3, c, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3);            \
    static _q_PTR_##func _q_##func = 0;                        \
    ret q_##func(arg1, arg2, arg3) { \
        if (!_q_##func) { \
            qWarning("QSslSocket: cannot call unresolved function "#func); \
            err; \
        } \
        funcret _q_##func(a, b, c); \
    }

// ret func(arg1, arg2, arg3, arg4)
#  define DEFINEFUNC4(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4);               \
    static _q_PTR_##func _q_##func = 0;                                 \
    ret q_##func(arg1, arg2, arg3, arg4) { \
         if (!_q_##func) { \
             qWarning("QSslSocket: cannot call unresolved function "#func); \
             err; \
         } \
         funcret _q_##func(a, b, c, d); \
    }

// ret func(arg1, arg2, arg3, arg4, arg5)
#  define DEFINEFUNC5(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5);         \
    static _q_PTR_##func _q_##func = 0;                                 \
    ret q_##func(arg1, arg2, arg3, arg4, arg5) { \
        if (!_q_##func) { \
            qWarning("QSslSocket: cannot call unresolved function "#func); \
            err; \
        } \
        funcret _q_##func(a, b, c, d, e); \
    }

// ret func(arg1, arg2, arg3, arg4, arg6)
#  define DEFINEFUNC6(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6);   \
    static _q_PTR_##func _q_##func = 0;                                 \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6) { \
        if (!_q_##func) { \
            qWarning("QSslSocket: cannot call unresolved function "#func); \
            err; \
        } \
        funcret _q_##func(a, b, c, d, e, f); \
    }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7)
#  define DEFINEFUNC7(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);   \
    static _q_PTR_##func _q_##func = 0;                                       \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7) { \
        if (!_q_##func) { \
            qWarning("QSslSocket: cannot call unresolved function "#func); \
            err; \
        } \
        funcret _q_##func(a, b, c, d, e, f, g); \
    }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7, arg8, arg9)
#  define DEFINEFUNC9(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, arg8, h, arg9, i, err, funcret) \
    typedef ret (*_q_PTR_##func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);   \
    static _q_PTR_##func _q_##func = 0;                                                   \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { \
        if (_q_##func) { \
            qWarning("QSslSocket: cannot call unresolved function "#func); \
            err; \
        }   \
        funcret _q_##func(a, b, c, d, e, f, g, h, i); \
    }
// **************** Shared declarations ******************

#else // !defined QT_LINKED_OPENSSL

// **************** Static declarations ******************

// ret func(arg)
#  define DEFINEFUNC(ret, func, arg, a, err, funcret)				\
    ret q_##func(arg) {	funcret func(a); }

// ret func(arg1, arg2)
#  define DEFINEFUNC2(ret, func, arg1, a, arg2, b, err, funcret) \
    ret q_##func(arg1, arg2) { funcret func(a, b); }

// ret func(arg1, arg2, arg3)
#  define DEFINEFUNC3(ret, func, arg1, a, arg2, b, arg3, c, err, funcret) \
    ret q_##func(arg1, arg2, arg3) { funcret func(a, b, c); }

// ret func(arg1, arg2, arg3, arg4)
#  define DEFINEFUNC4(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, err, funcret) \
    ret q_##func(arg1, arg2, arg3, arg4) { funcret func(a, b, c, d); }

// ret func(arg1, arg2, arg3, arg4, arg5)
#  define DEFINEFUNC5(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, err, funcret) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5) { funcret func(a, b, c, d, e); }

// ret func(arg1, arg2, arg3, arg4, arg6)
#  define DEFINEFUNC6(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, err, funcret) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6) { funcret func(a, b, c, d, e, f); }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7)
#  define DEFINEFUNC7(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, err, funcret) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7) { funcret func(a, b, c, d, e, f, g); }

// ret func(arg1, arg2, arg3, arg4, arg6, arg7, arg8, arg9)
#  define DEFINEFUNC9(ret, func, arg1, a, arg2, b, arg3, c, arg4, d, arg5, e, arg6, f, arg7, g, arg8, h, arg9, i, err, funcret) \
    ret q_##func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) { funcret func(a, b, c, d, e, f, g, h, i); }

// **************** Static declarations ******************

#endif // !defined QT_LINKED_OPENSSL

bool q_resolveOpenSslSymbols();
long q_ASN1_INTEGER_get(ASN1_INTEGER *a);
unsigned char * q_ASN1_STRING_data(ASN1_STRING *a);
int q_ASN1_STRING_length(ASN1_STRING *a);
int q_ASN1_STRING_to_UTF8(unsigned char **a, ASN1_STRING *b);
long q_BIO_ctrl(BIO *a, int b, long c, void *d);
int q_BIO_free(BIO *a);
BIO *q_BIO_new(BIO_METHOD *a);
BIO *q_BIO_new_mem_buf(void *a, int b);
int q_BIO_read(BIO *a, void *b, int c);
BIO_METHOD *q_BIO_s_mem();
int q_BIO_write(BIO *a, const void *b, int c);
int q_BN_num_bits(const BIGNUM *a);
int q_CRYPTO_num_locks();
void q_CRYPTO_set_locking_callback(void (*a)(int, int, const char *, int));
void q_CRYPTO_set_id_callback(unsigned long (*a)());
void q_CRYPTO_free(void *a);
void q_DSA_free(DSA *a);
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
X509 *q_d2i_X509(X509 **a, const unsigned char **b, long c);
#else
X509 *q_d2i_X509(X509 **a, unsigned char **b, long c);
#endif
char *q_ERR_error_string(unsigned long a, char *b);
unsigned long q_ERR_get_error();
const EVP_CIPHER *q_EVP_des_ede3_cbc();
int q_EVP_PKEY_assign(EVP_PKEY *a, int b, char *c);
int q_EVP_PKEY_set1_RSA(EVP_PKEY *a, RSA *b);
int q_EVP_PKEY_set1_DSA(EVP_PKEY *a, DSA *b);
void q_EVP_PKEY_free(EVP_PKEY *a);
RSA *q_EVP_PKEY_get1_RSA(EVP_PKEY *a);
DSA *q_EVP_PKEY_get1_DSA(EVP_PKEY *a);
int q_EVP_PKEY_type(int a);
EVP_PKEY *q_EVP_PKEY_new();
int q_i2d_X509(X509 *a, unsigned char **b);
const char *q_OBJ_nid2sn(int a);
int q_OBJ_obj2nid(const ASN1_OBJECT *a);
#ifdef SSLEAY_MACROS
// ### verify
void *q_PEM_ASN1_read_bio(d2i_of_void *a, const char *b, BIO *c, void **d, pem_password_cb *e,
                          void *f);
// ### ditto for write
#else
DSA *q_PEM_read_bio_DSAPrivateKey(BIO *a, DSA **b, pem_password_cb *c, void *d);
RSA *q_PEM_read_bio_RSAPrivateKey(BIO *a, RSA **b, pem_password_cb *c, void *d);
int q_PEM_write_bio_DSAPrivateKey(BIO *a, DSA *b, const EVP_CIPHER *c, unsigned char *d,
                                  int e, pem_password_cb *f, void *g);
int q_PEM_write_bio_RSAPrivateKey(BIO *a, RSA *b, const EVP_CIPHER *c, unsigned char *d,
                                  int e, pem_password_cb *f, void *g);
#endif
DSA *q_PEM_read_bio_DSA_PUBKEY(BIO *a, DSA **b, pem_password_cb *c, void *d);
RSA *q_PEM_read_bio_RSA_PUBKEY(BIO *a, RSA **b, pem_password_cb *c, void *d);
int q_PEM_write_bio_DSA_PUBKEY(BIO *a, DSA *b);
int q_PEM_write_bio_RSA_PUBKEY(BIO *a, RSA *b);
void q_RAND_seed(const void *a, int b);
int q_RAND_status();
void q_RSA_free(RSA *a);
int q_sk_num(STACK *a);
void q_sk_pop_free(STACK *a, void (*b)(void *));
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
void q_sk_free(_STACK *a);
void * q_sk_value(STACK *a, int b);
#else
void q_sk_free(STACK *a);
char * q_sk_value(STACK *a, int b);
#endif
int q_SSL_accept(SSL *a);
int q_SSL_clear(SSL *a);
char *q_SSL_CIPHER_description(SSL_CIPHER *a, char *b, int c);
int q_SSL_connect(SSL *a);
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
int q_SSL_CTX_check_private_key(const SSL_CTX *a);
#else
int q_SSL_CTX_check_private_key(SSL_CTX *a);
#endif
long q_SSL_CTX_ctrl(SSL_CTX *a, int b, long c, void *d);
void q_SSL_CTX_free(SSL_CTX *a);
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
SSL_CTX *q_SSL_CTX_new(const SSL_METHOD *a);
#else
SSL_CTX *q_SSL_CTX_new(SSL_METHOD *a);
#endif
int q_SSL_CTX_set_cipher_list(SSL_CTX *a, const char *b);
int q_SSL_CTX_set_default_verify_paths(SSL_CTX *a);
void q_SSL_CTX_set_verify(SSL_CTX *a, int b, int (*c)(int, X509_STORE_CTX *));
void q_SSL_CTX_set_verify_depth(SSL_CTX *a, int b);
int q_SSL_CTX_use_certificate(SSL_CTX *a, X509 *b);
int q_SSL_CTX_use_certificate_file(SSL_CTX *a, const char *b, int c);
int q_SSL_CTX_use_PrivateKey(SSL_CTX *a, EVP_PKEY *b);
int q_SSL_CTX_use_RSAPrivateKey(SSL_CTX *a, RSA *b);
int q_SSL_CTX_use_PrivateKey_file(SSL_CTX *a, const char *b, int c);
void q_SSL_free(SSL *a);
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
STACK_OF(SSL_CIPHER) *q_SSL_get_ciphers(const SSL *a);
#else
STACK_OF(SSL_CIPHER) *q_SSL_get_ciphers(SSL *a);
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
const SSL_CIPHER *q_SSL_get_current_cipher(SSL *a);
#else
SSL_CIPHER *q_SSL_get_current_cipher(SSL *a);
#endif
int q_SSL_get_error(SSL *a, int b);
STACK_OF(X509) *q_SSL_get_peer_cert_chain(SSL *a);
X509 *q_SSL_get_peer_certificate(SSL *a);
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
// 0.9.8 broke SC and BC by changing this function's signature.
long q_SSL_get_verify_result(const SSL *a);
#else
long q_SSL_get_verify_result(SSL *a);
#endif
int q_SSL_library_init();
void q_SSL_load_error_strings();
SSL *q_SSL_new(SSL_CTX *a);
#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
long q_SSL_ctrl(SSL *ssl,int cmd, long larg, void *parg);
#endif
int q_SSL_read(SSL *a, void *b, int c);
void q_SSL_set_bio(SSL *a, BIO *b, BIO *c);
void q_SSL_set_accept_state(SSL *a);
void q_SSL_set_connect_state(SSL *a);
int q_SSL_shutdown(SSL *a);
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
const SSL_METHOD *q_SSLv2_client_method();
const SSL_METHOD *q_SSLv3_client_method();
const SSL_METHOD *q_SSLv23_client_method();
const SSL_METHOD *q_TLSv1_client_method();
const SSL_METHOD *q_SSLv2_server_method();
const SSL_METHOD *q_SSLv3_server_method();
const SSL_METHOD *q_SSLv23_server_method();
const SSL_METHOD *q_TLSv1_server_method();
#else
SSL_METHOD *q_SSLv2_client_method();
SSL_METHOD *q_SSLv3_client_method();
SSL_METHOD *q_SSLv23_client_method();
SSL_METHOD *q_TLSv1_client_method();
SSL_METHOD *q_SSLv2_server_method();
SSL_METHOD *q_SSLv3_server_method();
SSL_METHOD *q_SSLv23_server_method();
SSL_METHOD *q_TLSv1_server_method();
#endif
int q_SSL_write(SSL *a, const void *b, int c);
int q_X509_cmp(X509 *a, X509 *b);
#ifdef SSLEAY_MACROS
void *q_ASN1_dup(i2d_of_void *i2d, d2i_of_void *d2i, char *x);
#define q_X509_dup(x509) (X509 *)q_ASN1_dup((i2d_of_void *)q_i2d_X509, \
		(d2i_of_void *)q_d2i_X509,(char *)x509)
#else
X509 *q_X509_dup(X509 *a);
#endif
ASN1_OBJECT *q_X509_EXTENSION_get_object(X509_EXTENSION *a);
void q_X509_free(X509 *a);
X509_EXTENSION *q_X509_get_ext(X509 *a, int b);
int q_X509_get_ext_count(X509 *a);
void *q_X509_get_ext_d2i(X509 *a, int b, int *c, int *d);
X509_NAME *q_X509_get_issuer_name(X509 *a);
X509_NAME *q_X509_get_subject_name(X509 *a);
int q_X509_verify_cert(X509_STORE_CTX *ctx);
int q_X509_NAME_entry_count(X509_NAME *a);
X509_NAME_ENTRY *q_X509_NAME_get_entry(X509_NAME *a,int b);
ASN1_STRING *q_X509_NAME_ENTRY_get_data(X509_NAME_ENTRY *a);
ASN1_OBJECT *q_X509_NAME_ENTRY_get_object(X509_NAME_ENTRY *a);
EVP_PKEY *q_X509_PUBKEY_get(X509_PUBKEY *a);
void q_X509_STORE_free(X509_STORE *store);
X509_STORE *q_X509_STORE_new();
int q_X509_STORE_add_cert(X509_STORE *ctx, X509 *x);
void q_X509_STORE_CTX_free(X509_STORE_CTX *storeCtx);
int q_X509_STORE_CTX_init(X509_STORE_CTX *ctx, X509_STORE *store,
                          X509 *x509, STACK_OF(X509) *chain);
X509_STORE_CTX *q_X509_STORE_CTX_new();
int q_X509_STORE_CTX_set_purpose(X509_STORE_CTX *ctx, int purpose);

#define q_BIO_get_mem_data(b, pp) (int)q_BIO_ctrl(b,BIO_CTRL_INFO,0,(char *)pp)
#define q_BIO_pending(b) (int)q_BIO_ctrl(b,BIO_CTRL_PENDING,0,NULL)
#ifdef SSLEAY_MACROS
int 	q_i2d_DSAPrivateKey(const DSA *a, unsigned char **pp);
int 	q_i2d_RSAPrivateKey(const RSA *a, unsigned char **pp);
RSA *q_d2i_RSAPrivateKey(RSA **a, unsigned char **pp, long length);
DSA *q_d2i_DSAPrivateKey(DSA **a, unsigned char **pp, long length);
#define	q_PEM_read_bio_RSAPrivateKey(bp, x, cb, u) \
        (RSA *)q_PEM_ASN1_read_bio( \
        (void *(*)(void**, const unsigned char**, long int))q_d2i_RSAPrivateKey, PEM_STRING_RSA, bp, (void **)x, cb, u)
#define	q_PEM_read_bio_DSAPrivateKey(bp, x, cb, u) \
        (DSA *)q_PEM_ASN1_read_bio( \
        (void *(*)(void**, const unsigned char**, long int))q_d2i_DSAPrivateKey, PEM_STRING_DSA, bp, (void **)x, cb, u)
#define	q_PEM_write_bio_RSAPrivateKey(bp,x,enc,kstr,klen,cb,u) \
		PEM_ASN1_write_bio((int (*)(void*, unsigned char**))q_i2d_RSAPrivateKey,PEM_STRING_RSA,\
			bp,(char *)x,enc,kstr,klen,cb,u)
#define	q_PEM_write_bio_DSAPrivateKey(bp,x,enc,kstr,klen,cb,u) \
		PEM_ASN1_write_bio((int (*)(void*, unsigned char**))q_i2d_DSAPrivateKey,PEM_STRING_DSA,\
			bp,(char *)x,enc,kstr,klen,cb,u)
#endif
#define q_SSL_CTX_set_options(ctx,op) q_SSL_CTX_ctrl((ctx),SSL_CTRL_OPTIONS,(op),NULL)
#define q_SKM_sk_num(type, st) ((int (*)(const STACK_OF(type) *))q_sk_num)(st)
#define q_SKM_sk_value(type, st,i) ((type * (*)(const STACK_OF(type) *, int))q_sk_value)(st, i)
#define q_sk_GENERAL_NAME_num(st) q_SKM_sk_num(GENERAL_NAME, (st))
#define q_sk_GENERAL_NAME_value(st, i) q_SKM_sk_value(GENERAL_NAME, (st), (i))
#define q_sk_X509_num(st) q_SKM_sk_num(X509, (st))
#define q_sk_X509_value(st, i) q_SKM_sk_value(X509, (st), (i))
#define q_sk_SSL_CIPHER_num(st) q_SKM_sk_num(SSL_CIPHER, (st))
#define q_sk_SSL_CIPHER_value(st, i) q_SKM_sk_value(SSL_CIPHER, (st), (i))
#define q_SSL_CTX_add_extra_chain_cert(ctx,x509) \
        q_SSL_CTX_ctrl(ctx,SSL_CTRL_EXTRA_CHAIN_CERT,0,(char *)x509)
#define q_X509_get_notAfter(x) X509_get_notAfter(x)
#define q_X509_get_notBefore(x) X509_get_notBefore(x)
#define q_EVP_PKEY_assign_RSA(pkey,rsa) q_EVP_PKEY_assign((pkey),EVP_PKEY_RSA,\
					(char *)(rsa))
#define q_EVP_PKEY_assign_DSA(pkey,dsa) q_EVP_PKEY_assign((pkey),EVP_PKEY_DSA,\
					(char *)(dsa))
#ifdef OPENSSL_LOAD_CONF
#define q_OpenSSL_add_all_algorithms() q_OPENSSL_add_all_algorithms_conf()
#else
#define q_OpenSSL_add_all_algorithms() q_OPENSSL_add_all_algorithms_noconf()
#endif
void q_OPENSSL_add_all_algorithms_noconf();
void q_OPENSSL_add_all_algorithms_conf();
int q_SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile, const char *CApath);
long q_SSLeay();

// Helper function
class QDateTime;
QDateTime q_getTimeFromASN1(const ASN1_TIME *aTime);

QT_END_NAMESPACE

#endif
