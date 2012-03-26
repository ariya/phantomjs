/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2006 Allan Sandfeld Jensen <sandfeld@kde.org>
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2008 Dirk Schulze <vbs85@gmx.de>
 *
 * All rights reserved.
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

#ifndef TransparencyLayer_h
#define TransparencyLayer_h

#include <QPaintEngine>
#include <QPainter>
#include <QPixmap>

namespace WebCore {

struct TransparencyLayer {
    WTF_MAKE_FAST_ALLOCATED;
public:
    TransparencyLayer(const QPainter* p, const QRect &rect, qreal opacity, QPixmap& alphaMask)
        : pixmap(rect.width(), rect.height())
        , opacity(opacity)
        , alphaMask(alphaMask)
        , saveCounter(1) // see the comment for saveCounter
    {
        offset = rect.topLeft();
        pixmap.fill(Qt::transparent);
        painter.begin(&pixmap);
        painter.setRenderHints(p->renderHints());
        painter.translate(-offset);
        painter.setPen(p->pen());
        painter.setBrush(p->brush());
        painter.setTransform(p->transform(), true);
        painter.setFont(p->font());
        painter.setOpacity(1);
    }

    TransparencyLayer()
    {
    }

    QPixmap pixmap;
    QPoint offset;
    QPainter painter;
    qreal opacity;
    // for clipToImageBuffer
    QPixmap alphaMask;
    // saveCounter is only used in combination with alphaMask
    // otherwise, its value is unspecified
    int saveCounter;
private:
    TransparencyLayer(const TransparencyLayer &) {}
    TransparencyLayer & operator=(const TransparencyLayer &) { return *this; }
};

} // namespace WebCore

#endif // TransparencyLayer_h
