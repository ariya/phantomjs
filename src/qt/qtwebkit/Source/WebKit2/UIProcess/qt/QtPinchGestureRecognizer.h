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

#ifndef QtPinchGestureRecognizer_h
#define QtPinchGestureRecognizer_h

#include "QtGestureRecognizer.h"
#include <QTouchEvent>
#include <QtCore/QPointF>

namespace WebKit {

class QtPinchGestureRecognizer : public QtGestureRecognizer {
public:
    struct TouchPointInformation {
        inline TouchPointInformation();
        inline TouchPointInformation(const QTouchEvent::TouchPoint&);
        inline bool isValid() const;

        int id;
        QPointF initialScreenPosition;
        QPointF initialPosition;
    };

    QtPinchGestureRecognizer(QtWebPageEventHandler*);
    bool update(const QTouchEvent::TouchPoint& point1, const QTouchEvent::TouchPoint& point2);
    void finish();
    void cancel();

private:
    qreal m_initialFingerDistance;
};

} // namespace WebKit

#endif /* QtPinchGestureRecognizer_h */
