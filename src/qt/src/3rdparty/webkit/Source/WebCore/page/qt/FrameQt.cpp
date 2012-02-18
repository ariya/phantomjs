/*
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
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "ImageBuffer.h"

#include "NotImplemented.h"

namespace WebCore {

DragImageRef Frame::nodeImage(Node*)
{
    notImplemented();
    return 0;
}

DragImageRef Frame::dragImageForSelection()
{
    if (!selection()->isRange())
        return 0;

    m_doc->updateLayout();

    IntRect paintingRect = enclosingIntRect(selection()->bounds());
    OwnPtr<ImageBuffer> buffer(ImageBuffer::create(paintingRect.size()));
    if (!buffer)
        return 0;

    GraphicsContext* context = buffer->context();
    context->translate(-paintingRect.x(), -paintingRect.y());
    context->clip(FloatRect(0, 0, paintingRect.maxX(), paintingRect.maxY()));

    PaintBehavior previousPaintBehavior = m_view->paintBehavior();
    m_view->setPaintBehavior(PaintBehaviorSelectionOnly);
    m_view->paintContents(context, paintingRect);
    m_view->setPaintBehavior(previousPaintBehavior);

    RefPtr<Image> image = buffer->copyImage();
    return createDragImageFromImage(image.get());
}

}
// vim: ts=4 sw=4 et
