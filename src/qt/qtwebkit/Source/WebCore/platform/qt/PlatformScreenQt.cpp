/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
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

#include "config.h"
#include "PlatformScreen.h"

#include "FloatRect.h"
#include "Frame.h"
#include "FrameView.h"
#include "HostWindow.h"
#include "NotImplemented.h"
#include "QWebPageClient.h"
#include "Widget.h"
#include <QGuiApplication>
#include <QScreen>

namespace WebCore {

static int screenNumber(Widget* w)
{
    if (!w)
        return 0;

    QWebPageClient* client = w->root()->hostWindow()->platformPageClient();
    return client ? client->screenNumber() : 0;
}

int screenDepth(Widget* w)
{
    return QGuiApplication::screens().value(screenNumber(w))->depth();
}

int screenDepthPerComponent(Widget* w)
{
    int depth = QGuiApplication::primaryScreen()->depth();
    // FIXME: Use widget's screen
    Q_UNUSED(w);
    // An interface to establish the actual number of bits per color
    // doesn't exist in Qt, or probably at all, so use common-sense
    // values for each screen depth and assume RGB/RGBA where appropriate.
    // Per http://www.w3.org/TR/css3-mediaqueries/#color, 'If different color
    // components are represented by different number of bits, the smallest
    // number is used.'
    switch (depth) {
    case 8:
        return 2;
    case 32:
        return 8;
    default:
        return depth / 3;
    }
}

bool screenIsMonochrome(Widget* w)
{
    Q_UNUSED(w);
    // FIXME: In Qt 5 colorCount() isn't even implemented beyond returning 256 :)
    return false;
}

FloatRect screenRect(Widget* widget)
{
    QRect r = QGuiApplication::screens().value(screenNumber(widget))->geometry();
    return FloatRect(r.x(), r.y(), r.width(), r.height());
}

FloatRect screenAvailableRect(Widget* widget)
{
    QRect r = QGuiApplication::screens().value(screenNumber(widget))->availableGeometry();
    return FloatRect(r.x(), r.y(), r.width(), r.height());
}

void screenColorProfile(ColorProfile&)
{
    notImplemented();
}

} // namespace WebCore
