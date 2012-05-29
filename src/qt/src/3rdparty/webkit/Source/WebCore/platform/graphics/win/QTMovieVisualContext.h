/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple, Inc.  All rights reserved.
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

#ifndef QTMovieVisualContext_h
#define QTMovieVisualContext_h

#ifdef QTMOVIEWIN_EXPORTS
#define QTMOVIEWIN_API __declspec(dllexport)
#else
#define QTMOVIEWIN_API __declspec(dllimport)
#endif

#include "QTMovie.h"
#include "QTMovieTask.h"
#include "QTPixelBuffer.h"
#include <WTF/OwnPtr.h>
#include <WTF/RefCounted.h>

typedef const struct __CFDictionary* CFDictionaryRef;
typedef struct OpaqueQTVisualContext*   QTVisualContextRef;

// QTCVTimeStamp is a struct containing only a CVTimeStamp.  This is to 
// work around the inability of CVTimeStamp to be forward declared, in 
// addition to it being declared in different header files when building
// the QTMovieWin and WebCore projects.
struct QTCVTimeStamp;

class QTMovieVisualContextClient {
public:
    virtual void imageAvailableForTime(const QTCVTimeStamp*) = 0;
};

class QTMOVIEWIN_API QTMovieVisualContext : public RefCounted<QTMovieVisualContext> {
public:
    static PassRefPtr<QTMovieVisualContext> create(QTMovieVisualContextClient*, QTPixelBuffer::Type);
    ~QTMovieVisualContext();

    bool isImageAvailableForTime(const QTCVTimeStamp*) const;
    QTPixelBuffer imageForTime(const QTCVTimeStamp*);
    void task();

    QTVisualContextRef visualContextRef();

    void setMovie(PassRefPtr<QTMovie>);
    QTMovie* movie() const;

    static double currentHostTime();

protected:
    QTMovieVisualContext(QTMovieVisualContextClient*, QTPixelBuffer::Type);
    void setupVisualContext();

    friend class QTMovieVisualContextPriv;
    OwnPtr<QTMovieVisualContextPriv> m_private;
};

#endif
