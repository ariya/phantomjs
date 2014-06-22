/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *           (C) 2009 Brent Fulgham <bfulgham@webkit.org>
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

#ifndef PixelDumpSupportCairo_h
#define PixelDumpSupportCairo_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if PLATFORM(WIN)
#include <windows.h>
#include <cairo-win32.h>
#elif PLATFORM(GTK) || PLATFORM(EFL)
#include <cairo.h>
#endif

#if PLATFORM(WIN)
typedef HBITMAP PlatformBitmapBuffer;
#else
typedef void* PlatformBitmapBuffer;
#endif

class BitmapContext : public RefCounted<BitmapContext> {
public:
    static PassRefPtr<BitmapContext> createByAdoptingBitmapAndContext(PlatformBitmapBuffer buffer, cairo_t* context)
    {
        return adoptRef(new BitmapContext(buffer, context));
    }

    ~BitmapContext()
    {
        if (m_buffer)
#if PLATFORM(WIN)
            DeleteObject(m_buffer);
#else
            free(m_buffer);
#endif
        cairo_destroy(m_context);
    }

    cairo_t* cairoContext() const { return m_context; }

private:

    BitmapContext(PlatformBitmapBuffer buffer, cairo_t* context)
        : m_buffer(buffer)
        , m_context(context)
    {
    }

    PlatformBitmapBuffer m_buffer;
    cairo_t* m_context;
};

#endif // PixelDumpSupportCairo_h
