/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef ScrollbarThemeClient_h
#define ScrollbarThemeClient_h

#include "IntPoint.h"
#include "IntRect.h"
#include "IntSize.h"
#include "ScrollTypes.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class ScrollView;

class ScrollbarThemeClient {
public:
    virtual int x() const = 0;
    virtual int y() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual IntSize size() const = 0;
    virtual IntPoint location() const = 0;

    virtual ScrollView* parent() const = 0;
    virtual ScrollView* root() const = 0;

    virtual void setFrameRect(const IntRect&) = 0;
    virtual IntRect frameRect() const = 0;

    virtual void invalidate() = 0;
    virtual void invalidateRect(const IntRect&) = 0;

    virtual ScrollbarOverlayStyle scrollbarOverlayStyle() const = 0;
    virtual void getTickmarks(Vector<IntRect>&) const = 0;
    virtual bool isScrollableAreaActive() const = 0;
    virtual bool isScrollViewScrollbar() const = 0;

    virtual IntPoint convertFromContainingWindow(const IntPoint& windowPoint) = 0;

    virtual bool isCustomScrollbar() const = 0;
    virtual ScrollbarOrientation orientation() const = 0;

    virtual int value() const = 0;
    virtual float currentPos() const = 0;
    virtual int visibleSize() const = 0;
    virtual int totalSize() const = 0;
    virtual int maximum() const = 0;
    virtual ScrollbarControlSize controlSize() const = 0;

    virtual int lineStep() const = 0;
    virtual int pageStep() const = 0;

    virtual ScrollbarPart pressedPart() const = 0;
    virtual ScrollbarPart hoveredPart() const = 0;

    virtual void styleChanged() = 0;

    virtual bool enabled() const = 0;
    virtual void setEnabled(bool) = 0;

    virtual bool isOverlayScrollbar() const = 0;

    virtual bool isAlphaLocked() const = 0;
    virtual void setIsAlphaLocked(bool) = 0;

protected:
    virtual ~ScrollbarThemeClient() { }
};

} // namespace WebCore

#endif // ScollbarThemeClient_h
