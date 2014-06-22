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

#include "config.h"
#include "RenderDialog.h"

#if ENABLE(DIALOG_ELEMENT)
#include "FrameView.h"
#include "LayoutRepainter.h"
#include "RenderLayer.h"
#include "RenderView.h"

namespace WebCore {

void RenderDialog::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    LayoutRepainter repainter(*this, true);
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    RenderBlock::layout();

    RenderStyle* styleToUse = style();
    if (styleToUse->position() != AbsolutePosition || !styleToUse->top().isAuto() || !styleToUse->bottom().isAuto()) {
        statePusher.pop();
        return;
    }

    // Adjust the dialog's position to be centered in or at the top of the viewport.
    // FIXME: Figure out what to do in vertical writing mode.
    FrameView* frameView = document()->view();
    int scrollTop = frameView->scrollOffset().height();
    FloatPoint absolutePoint(0, scrollTop);
    int visibleHeight = frameView->visibleContentRect(ScrollableArea::IncludeScrollbars).height();
    if (height() < visibleHeight)
        absolutePoint.move(0, (visibleHeight - height()) / 2);
    FloatPoint localPoint = containingBlock()->absoluteToLocal(absolutePoint);
    LayoutUnit localTop = LayoutSize(localPoint.x(), localPoint.y()).height();
    setY(localTop);

    statePusher.pop();
    // FIXME: Since there is always a layer here, repainter shouldn't be necessary. But without it, the dialog is sometimes not painted (see bug 90670).
    repainter.repaintAfterLayout();
}

}

#endif
