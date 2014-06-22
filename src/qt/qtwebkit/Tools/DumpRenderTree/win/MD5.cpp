/*
 * Copyright (C) 2007 Apple, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MD5.h"

#include <windows.h>

typedef void (WINAPI*initPtr)(MD5_CTX*);
typedef void (WINAPI*updatePtr)(MD5_CTX*, unsigned char*, unsigned);
typedef void (WINAPI*finalPtr)(MD5_CTX*);

static HMODULE cryptDLL()
{
    static HMODULE module = LoadLibraryW(L"Cryptdll.dll");
    return module;
}

static initPtr init()
{
    static initPtr ptr = reinterpret_cast<initPtr>(GetProcAddress(cryptDLL(), "MD5Init"));
    return ptr;
}

static updatePtr update()
{
    static updatePtr ptr = reinterpret_cast<updatePtr>(GetProcAddress(cryptDLL(), "MD5Update"));
    return ptr;
}

static finalPtr final()
{
    static finalPtr ptr = reinterpret_cast<finalPtr>(GetProcAddress(cryptDLL(), "MD5Final"));
    return ptr;
}

void MD5_Init(MD5_CTX* context)
{
    init()(context);
}

void MD5_Update(MD5_CTX* context, unsigned char* input, unsigned length)
{
    update()(context, input, length);
}

void MD5_Final(unsigned char hash[16], MD5_CTX* context)
{
    final()(context);

    for (int i = 0; i < 16; ++i)
        hash[i] = context->digest[i];
}
