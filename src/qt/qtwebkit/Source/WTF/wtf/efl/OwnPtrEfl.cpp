/*
 * Copyright (C) 2011 ProFUSION embedded systems
 * Copyright (C) 2011 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "OwnPtr.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#include <Eina.h>
#include <Evas.h>

#if USE(ACCELERATED_COMPOSITING)
#include <Evas_GL.h>
#endif

namespace WTF {

void deleteOwnedPtr(Ecore_Evas* ptr)
{
    if (ptr)
        ecore_evas_free(ptr);
}

void deleteOwnedPtr(Ecore_Pipe* ptr)
{
    if (ptr)
        ecore_pipe_del(ptr);
}

void deleteOwnedPtr(Eina_Hash* ptr)
{
    if (ptr)
        eina_hash_free(ptr);
}

void deleteOwnedPtr(Eina_Module* ptr)
{
    if (ptr)
        eina_module_free(ptr); // If module wasn't unloaded, eina_module_free() calls eina_module_unload().
}

void deleteOwnedPtr(Ecore_IMF_Context* ptr)
{
    if (ptr)
        ecore_imf_context_del(ptr);
}

#if USE(ACCELERATED_COMPOSITING)
void deleteOwnedPtr(Evas_GL* ptr)
{
    if (ptr)
        evas_gl_free(ptr);
}
#endif

}
