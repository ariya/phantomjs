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

#ifndef QTMovieGWorld_h
#define QTMovieGWorld_h


#include "QTMovie.h"
#include <Unicode.h>
#include <WTF/RefCounted.h>
#include <WTF/RefPtr.h>
#include <windows.h>

#ifdef QTMOVIEWIN_EXPORTS
#define QTMOVIEWIN_API __declspec(dllexport)
#else
#define QTMOVIEWIN_API __declspec(dllimport)
#endif

class QTMovieGWorld;
class QTMovieGWorldPrivate;

class QTMovieGWorldClient {
public:
    virtual void movieNewImageAvailable(QTMovieGWorld*) = 0;
};

class QTMovieGWorldFullscreenClient {
public:
    virtual LRESULT fullscreenClientWndProc(HWND, UINT message, WPARAM, LPARAM) = 0;
};

class QTMOVIEWIN_API QTMovieGWorld : public RefCounted<QTMovieGWorld> {
public:
    QTMovieGWorld(QTMovieGWorldClient*);
    ~QTMovieGWorld();

    void getNaturalSize(int& width, int& height);
    void setSize(int width, int height);

    void setVisible(bool);
    void paint(HDC, int x, int y);
    void getCurrentFrameInfo(void*& buffer, unsigned& bitsPerPixel, unsigned& rowBytes, unsigned& width, unsigned& height);

    void setDisabled(bool);
    bool isDisabled() const;

    // Returns the full-screen window created
    HWND enterFullscreen(QTMovieGWorldFullscreenClient*);
    void exitFullscreen();

    void setMovie(PassRefPtr<QTMovie>);
    QTMovie* movie() const;

private:
    static LRESULT fullscreenWndProc(HWND, UINT message, WPARAM, LPARAM);

    QTMovieGWorldPrivate* m_private;
    friend class QTMovieGWorldPrivate;
};

#endif
