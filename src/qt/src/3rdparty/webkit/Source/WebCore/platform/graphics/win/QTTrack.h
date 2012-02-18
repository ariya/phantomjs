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

#ifndef QTTrack_h
#define QTTrack_h

#include <Unicode.h>
#include <windows.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

// We must include this after <Unicode.h>, or the definition of OSErr will change:
#include <CoreGraphics/CGAffineTransform.h>

#ifdef QTMOVIEWIN_EXPORTS
#define QTMOVIEWIN_API __declspec(dllexport)
#else
#define QTMOVIEWIN_API __declspec(dllimport)
#endif

class QTTrack;
class QTTrackPrivate;
typedef struct TrackType** Track;

class QTMOVIEWIN_API QTTrack : public RefCounted<QTTrack> {
public:
    static PassRefPtr<QTTrack> create(Track);
    ~QTTrack();

    CGAffineTransform getTransform() const;
    void setTransform(CGAffineTransform);
    void resetTransform();

    bool isEnabled() const;
    void setEnabled(bool);

    Track getTrackHandle() const;
private:
    QTTrack(Track);
    QTTrackPrivate* m_private;
    friend class QTTrackPrivate;
};

#endif
