/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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

#import "config.h"
#import "FrameSnapshottingMac.h"

#import "BlockExceptions.h"
#import "Document.h"
#import "Frame.h"
#import "FrameSelection.h"
#import "FrameView.h"
#import "GraphicsContext.h"
#import "Range.h"
#import "RenderView.h"

namespace WebCore {

NSImage* imageFromRect(Frame* frame, NSRect rect)
{
    PaintBehavior oldBehavior = frame->view()->paintBehavior();
    frame->view()->setPaintBehavior(oldBehavior | PaintBehaviorFlattenCompositingLayers);
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    NSImage* resultImage = [[[NSImage alloc] initWithSize:rect.size] autorelease];
    
    if (rect.size.width != 0 && rect.size.height != 0) {
        [resultImage setFlipped:YES];
        [resultImage lockFocus];

        GraphicsContext graphicsContext((CGContextRef)[[NSGraphicsContext currentContext] graphicsPort]);        
        graphicsContext.save();
        graphicsContext.translate(-rect.origin.x, -rect.origin.y);
        frame->view()->paintContents(&graphicsContext, IntRect(rect));
        graphicsContext.restore();

        [resultImage unlockFocus];
        [resultImage setFlipped:NO];
    }
    
    frame->view()->setPaintBehavior(oldBehavior);
    return resultImage;
    
    END_BLOCK_OBJC_EXCEPTIONS;
    
    frame->view()->setPaintBehavior(oldBehavior);
    return nil;
}

NSImage* selectionImage(Frame* frame, bool forceBlackText)
{
    frame->view()->setPaintBehavior(PaintBehaviorSelectionOnly | (forceBlackText ? PaintBehaviorForceBlackText : 0));
    frame->document()->updateLayout();
    NSImage* result = imageFromRect(frame, frame->selection()->bounds());
    frame->view()->setPaintBehavior(PaintBehaviorNormal);
    return result;
}

NSImage *rangeImage(Frame* frame, Range* range, bool forceBlackText)
{
    frame->view()->setPaintBehavior(PaintBehaviorSelectionOnly | (forceBlackText ? PaintBehaviorForceBlackText : 0));
    frame->document()->updateLayout();
    RenderView* view = frame->contentRenderer();
    if (!view)
        return nil;

    Position start = range->startPosition();
    Position candidate = start.downstream();
    if (candidate.deprecatedNode() && candidate.deprecatedNode()->renderer())
        start = candidate;

    Position end = range->endPosition();
    candidate = end.upstream();
    if (candidate.deprecatedNode() && candidate.deprecatedNode()->renderer())
        end = candidate;

    if (start.isNull() || end.isNull() || start == end)
        return nil;

    RenderObject* savedStartRenderer;
    int savedStartOffset;
    RenderObject* savedEndRenderer;
    int savedEndOffset;
    view->getSelection(savedStartRenderer, savedStartOffset, savedEndRenderer, savedEndOffset);

    RenderObject* startRenderer = start.deprecatedNode()->renderer();
    if (!startRenderer)
        return nil;

    RenderObject* endRenderer = end.deprecatedNode()->renderer();
    if (!endRenderer)
        return nil;

    view->setSelection(startRenderer, start.deprecatedEditingOffset(), endRenderer, end.deprecatedEditingOffset(), RenderView::RepaintNothing);
    NSImage* result = imageFromRect(frame, view->selectionBounds());
    view->setSelection(savedStartRenderer, savedStartOffset, savedEndRenderer, savedEndOffset, RenderView::RepaintNothing);

    frame->view()->setPaintBehavior(PaintBehaviorNormal);
    return result;
}


NSImage* snapshotDragImage(Frame* frame, Node* node, NSRect* imageRect, NSRect* elementRect)
{
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return nil;
    
    renderer->updateDragState(true);    // mark dragged nodes (so they pick up the right CSS)
    frame->document()->updateLayout();  // forces style recalc - needed since changing the drag state might
                                        // imply new styles, plus JS could have changed other things


    // Document::updateLayout may have blown away the original RenderObject.
    renderer = node->renderer();
    if (!renderer)
        return nil;

    LayoutRect topLevelRect;
    NSRect paintingRect = pixelSnappedIntRect(renderer->paintingRootRect(topLevelRect));

    frame->view()->setNodeToDraw(node); // invoke special sub-tree drawing mode
    NSImage* result = imageFromRect(frame, paintingRect);
    renderer->updateDragState(false);
    frame->document()->updateLayout();
    frame->view()->setNodeToDraw(0);

    if (elementRect)
        *elementRect = pixelSnappedIntRect(topLevelRect);
    if (imageRect)
        *imageRect = paintingRect;
    return result;
}

} // namespace WebCore
