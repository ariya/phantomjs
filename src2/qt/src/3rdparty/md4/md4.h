/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security,
 * Inc. MD4 Message-Digest Algorithm.
 *
 * Written by Solar Designer <solar@openwall.com> in 2001, and placed in
 * the public domain.  See md4.c for more information.
 */

#ifndef __MD4_H
#define __MD4_H

#include <qglobal.h>

QT_BEGIN_NAMESPACE

#define	MD4_RESULTLEN (128/8)

struct md4_context {
	quint32 lo, hi;
	quint32 a, b, c, d;
	unsigned char buffer[64];
	quint32 block[MD4_RESULTLEN];
};

static void md4_init(struct md4_context *ctx);
static void md4_update(struct md4_context *ctx, const unsigned char *data, size_t size);
static void md4_final(struct md4_context *ctx, unsigned char result[MD4_RESULTLEN]);

QT_END_NAMESPACE

#endif
