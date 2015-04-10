/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef FindIndicator_h
#define FindIndicator_h

#include "ShareableBitmap.h"
#include <WebCore/FloatRect.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {
    class GraphicsContext;
}

namespace WebKit {

class FindIndicator : public RefCounted<FindIndicator> {
public:
    static PassRefPtr<FindIndicator> create(const WebCore::FloatRect& selectionRectInWindowCoordinates, const Vector<WebCore::FloatRect>& textRectsInSelectionRectCoordinates, float contentImageScaleFactor, const ShareableBitmap::Handle& contentImageHandle);
    ~FindIndicator();

    WebCore::FloatRect selectionRectInWindowCoordinates() const { return m_selectionRectInWindowCoordinates; }
    WebCore::FloatRect frameRect() const;

    ShareableBitmap* contentImage() const { return m_contentImage.get(); }

    void draw(WebCore::GraphicsContext&, const WebCore::IntRect& dirtyRect);

private:
    FindIndicator(const WebCore::FloatRect& selectionRect, const Vector<WebCore::FloatRect>& textRects, float contentImageScaleFactor, PassRefPtr<ShareableBitmap> contentImage);

    WebCore::FloatRect m_selectionRectInWindowCoordinates;
    Vector<WebCore::FloatRect> m_textRectsInSelectionRectCoordinates;
    float m_contentImageScaleFactor;
    RefPtr<ShareableBitmap> m_contentImage;
};

} // namespace WebKit

#endif // FindIndicator_h
