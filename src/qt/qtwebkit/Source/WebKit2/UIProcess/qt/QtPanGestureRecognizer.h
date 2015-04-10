/*
 * Copyright (C) 2011 Benjamin Poulain <benjamin@webkit.org>
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

#ifndef QtPanGestureRecognizer_h
#define QtPanGestureRecognizer_h

#include "QtGestureRecognizer.h"
#include <QPointF>
#include <QTouchEvent>

namespace WebKit {

const qreal panningInitialTriggerDistanceThreshold = 5.;

class QtPanGestureRecognizer : public QtGestureRecognizer {
public:
    QtPanGestureRecognizer(QtWebPageEventHandler*);
    bool update(const QTouchEvent::TouchPoint&, qint64 eventTimestampMillis);
    void finish(const QTouchEvent::TouchPoint&, qint64 eventTimestampMillis);
    void cancel();

private:
    QPointF m_firstScreenPosition;
    QPointF m_lastPosition;
    qint64 m_lastEventTimestampMillis;
};

} // namespace WebKit

#endif /* QtPanGestureRecognizer_h */
