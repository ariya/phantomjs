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
#include "config.h"

#include "QTTrack.h"

#include <Movies.h>
#include <QTML.h>

using namespace std;

class QTTrackPrivate {
    WTF_MAKE_NONCOPYABLE(QTTrackPrivate);
public:
    QTTrackPrivate();
    ~QTTrackPrivate();

    QTTrack* m_track;
    Track m_trackHandle;
};

QTTrackPrivate::QTTrackPrivate()
    : m_track(0)
    , m_trackHandle(0)
{
}

QTTrackPrivate::~QTTrackPrivate()
{
    m_trackHandle = 0;
}

PassRefPtr<QTTrack> QTTrack::create(Track trackHandle)
{
    return adoptRef(new QTTrack(trackHandle));
}

QTTrack::QTTrack(Track trackHandle)
    : m_private(new QTTrackPrivate())
{
    m_private->m_track = this;
    m_private->m_trackHandle = trackHandle;
}

QTTrack::~QTTrack()
{
    delete m_private;
}

bool QTTrack::isEnabled() const
{
    ASSERT(m_private->m_track);
    return GetTrackEnabled(m_private->m_trackHandle);
}

void QTTrack::setEnabled(bool enabled)
{
    ASSERT(m_private->m_trackHandle);
    SetTrackEnabled(m_private->m_trackHandle, enabled);
}

CGAffineTransform QTTrack::getTransform() const
{
    ASSERT(m_private->m_trackHandle);
    MatrixRecord m = {0};
    GetTrackMatrix(m_private->m_trackHandle, &m);

    ASSERT(!m.matrix[0][2]);
    ASSERT(!m.matrix[1][2]);
    CGAffineTransform transform = CGAffineTransformMake(
        Fix2X(m.matrix[0][0]),
        Fix2X(m.matrix[0][1]),
        Fix2X(m.matrix[1][0]),
        Fix2X(m.matrix[1][1]),
        Fix2X(m.matrix[2][0]),
        Fix2X(m.matrix[2][1]));

    return transform;
}

void QTTrack::setTransform(CGAffineTransform t)
{
    ASSERT(m_private->m_trackHandle);
    MatrixRecord m = {{
        {X2Fix(t.a), X2Fix(t.b), 0},
        {X2Fix(t.c), X2Fix(t.d), 0},
        {X2Fix(t.tx), X2Fix(t.ty), fract1},
    }};

    SetTrackMatrix(m_private->m_trackHandle, &m);
}

void QTTrack::resetTransform()
{
    ASSERT(m_private->m_trackHandle);
    SetTrackMatrix(m_private->m_trackHandle, 0);
}

