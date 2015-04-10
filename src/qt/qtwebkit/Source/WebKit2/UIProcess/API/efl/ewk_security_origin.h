/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * @file    ewk_security_origin.h
 * @brief   Security Origin API.
 *
 * Security Origin is the mechanism that defines the access limits of a website.
 * Based on information such as domain, protocol and port, you can or cannot grant
 * authorization for accessing data and performing certain tasks.
 */

#ifndef ewk_security_origin_h
#define ewk_security_origin_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Security_Origin as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Security_Origin;

/**
 * Returns the host of the security origin.
 *
 * @param o security origin object
 *
 * @return the host domain pointer or @c NULL if there is not a host scheme.
 *         This pointer is guaranteed to be eina_stringshare, so whenever possible
 *         save yourself from cpu cycles and use eina_stringshare_ref()
 *         instead of eina_stringshare_add() or strdup().
 */
EAPI const char *ewk_security_origin_host_get(const Ewk_Security_Origin *o);

/**
 * Returns the port of the security origin.
 *
 * @param o security origin object
 *
 * @return the port number or @c 0 if there is not a proper security origin scheme
 */
EAPI uint32_t ewk_security_origin_port_get(const Ewk_Security_Origin *o);

/**
 * Returns the protocol of the security origin.
 *
 * @param o security origin object
 *
 * @return the protocol scheme pointer or @c NULL if there is not a protocol scheme.
 *         This pointer is guaranteed to be eina_stringshare, so whenever possible
 *         save yourself from cpu cycles and use eina_stringshare_ref()
 *         instead of eina_stringshare_add() or strdup().
 */
EAPI const char *ewk_security_origin_protocol_get(const Ewk_Security_Origin *o);

#ifdef __cplusplus
}
#endif
#endif // ewk_security_origin_h
