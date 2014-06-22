/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ewk_private_h
#define ewk_private_h

#include <wtf/Assertions.h>

#define COMPILE_ASSERT_MATCHING_ENUM(ewkName, webcoreName) \
        COMPILE_ASSERT(int(ewkName) == int(webcoreName), mismatchingEnums)

extern int _ewk_log_dom;

#define CRITICAL(...) EINA_LOG_DOM_CRIT(_ewk_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_ewk_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_ewk_log_dom, __VA_ARGS__)
#define INFO(...) EINA_LOG_DOM_INFO(_ewk_log_dom, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(_ewk_log_dom, __VA_ARGS__)

#endif // ewk_private_h
