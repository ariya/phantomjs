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

#ifndef QTMovie_h
#define QTMovie_h

#include "QTTrack.h"
#include <wtf/Vector.h>
#include <wtf/unicode/Unicode.h>

#ifdef QTMOVIEWIN_EXPORTS
#define QTMOVIEWIN_API __declspec(dllexport)
#else
#define QTMOVIEWIN_API __declspec(dllimport)
#endif

class QTMovie;
class QTMoviePrivate;
typedef struct MovieType** Movie;
typedef Vector<RefPtr<QTTrack>> QTTrackArray;

class QTMovieClient {
public:
    virtual void movieEnded(QTMovie*) = 0;
    virtual void movieLoadStateChanged(QTMovie*) = 0;
    virtual void movieTimeChanged(QTMovie*) = 0;
};

enum {
    QTMovieLoadStateError = -1L,
    QTMovieLoadStateLoaded  = 2000L,
    QTMovieLoadStatePlayable = 10000L,
    QTMovieLoadStatePlaythroughOK = 20000L,
    QTMovieLoadStateComplete = 100000L
};

typedef const struct __CFURL * CFURLRef;

class QTMOVIEWIN_API QTMovie : public RefCounted<QTMovie> {
public:
    static bool initializeQuickTime();
    static void taskTimerFired();

    static void disableComponent(uint32_t[5]);

    QTMovie(QTMovieClient*);
    ~QTMovie();

    void addClient(QTMovieClient*);
    void removeClient(QTMovieClient*);

    void loadPath(const UChar* url, int len, bool preservesPitch);
    void load(const UChar* url, int len, bool preservesPitch);
    void load(CFURLRef, bool preservesPitch);

    long loadState() const;
    float maxTimeLoaded() const;

    void play();
    void pause();

    float rate() const;
    void setRate(float);

    float duration() const;
    float currentTime() const;
    void setCurrentTime(float) const;

    void setVolume(float);
    void setPreservesPitch(bool);

    unsigned dataSize() const;

    void getNaturalSize(int& width, int& height);

    void disableUnsupportedTracks(unsigned& enabledTrackCount, unsigned& totalTrackCount);

    bool isDisabled() const;
    void setDisabled(bool);

    bool hasVideo() const;
    bool hasAudio() const;

    QTTrackArray videoTracks() const;

    bool hasClosedCaptions() const;
    void setClosedCaptionsVisible(bool);

    static unsigned countSupportedTypes();
    static void getSupportedType(unsigned index, const UChar*& str, unsigned& len);

    CGAffineTransform getTransform() const;
    void setTransform(CGAffineTransform);
    void resetTransform();

    Movie getMovieHandle() const;

    long timeScale() const;

    void setPrivateBrowsingMode(bool);

private:
    QTMoviePrivate* m_private;
    friend class QTMoviePrivate;
};

#endif
