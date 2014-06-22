/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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
#include "Frame.h"

#include "Document.h"
#include "FloatRect.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "HTMLTableCellElement.h"
#include "KeyboardEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "RenderFrame.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "ResourceHandle.h"

#include <windows.h>

using std::min;

namespace WebCore {

using namespace HTMLNames;

extern HDC g_screenDC;

void computePageRectsForFrame(Frame* frame, const IntRect& printRect, float headerHeight, float footerHeight, float userScaleFactor, Vector<IntRect>& pages, int& outPageHeight)
{
    ASSERT(frame);

    pages.clear();
    outPageHeight = 0;

    if (!frame->document() || !frame->view() || !frame->document()->renderView())
        return;

    RenderView* root = frame->document()->renderView();

    if (userScaleFactor <= 0) {
        LOG_ERROR("userScaleFactor has bad value %.2f", userScaleFactor);
        return;
    }

    float ratio = (float)printRect.height() / (float)printRect.width();

    float pageWidth  = (float) root->layoutOverflowRect().maxX();
    float pageHeight = pageWidth * ratio;
    outPageHeight = (int) pageHeight;   // this is the height of the page adjusted by margins
    pageHeight -= (headerHeight + footerHeight);

    if (pageHeight <= 0) {
        LOG_ERROR("pageHeight has bad value %.2f", pageHeight);
        return;
    }

    float currPageHeight = pageHeight / userScaleFactor;
    float docHeight      = root->layer()->size().height();
    float currPageWidth  = pageWidth / userScaleFactor;


    // always return at least one page, since empty files should print a blank page
    float printedPagesHeight = 0.0;
    do {
        float proposedBottom = min(docHeight, printedPagesHeight + pageHeight);
        frame->view()->adjustPageHeightDeprecated(&proposedBottom, printedPagesHeight, proposedBottom, printedPagesHeight);
        currPageHeight = max(1.0f, proposedBottom - printedPagesHeight);

        pages.append(IntRect(0, printedPagesHeight, currPageWidth, currPageHeight));
        printedPagesHeight += currPageHeight;
    } while (printedPagesHeight < docHeight);
}

HBITMAP imageFromSelection(Frame* frame, bool forceBlackText)
{
    if (!frame->view())
        return 0;

    frame->view()->setPaintBehavior(PaintBehaviorSelectionOnly | (forceBlackText ? PaintBehaviorForceBlackText : 0));
    FloatRect fr = frame->selection()->bounds();
    IntRect ir((int)fr.x(), (int)fr.y(), (int)fr.width(), (int)fr.height());
    if (ir.isEmpty())
        return 0;

    int w;
    int h;
    FrameView* view = frame->view();
    if (view->parent()) {
        ir.setLocation(view->parent()->convertChildToSelf(view, ir.location()));
        w = ir.width() * frame->pageZoomFactor() + 0.5;
        h = ir.height() * frame->pageZoomFactor() + 0.5;
    } else {
        ir = view->contentsToWindow(ir);
        w = ir.width();
        h = ir.height();
    }

    OwnPtr<HDC> bmpDC = adoptPtr(CreateCompatibleDC(g_screenDC));
    HBITMAP hBmp = CreateCompatibleBitmap(g_screenDC, w, h);
    if (!hBmp)
        return 0;

    HGDIOBJ hbmpOld = SelectObject(bmpDC.get(), hBmp);

    {
        GraphicsContext gc(bmpDC.get());
        frame->document()->updateLayout();
        view->paint(&gc, ir);
    }

    SelectObject(bmpDC.get(), hbmpOld);

    frame->view()->setPaintBehavior(PaintBehaviorNormal);

    return hBmp;
}

DragImageRef Frame::nodeImage(Node*)
{
    notImplemented();
    return 0;
}

DragImageRef Frame::dragImageForSelection()
{
    if (selection()->isRange())
        return imageFromSelection(this, false);

    return 0;
}

} // namespace WebCore
