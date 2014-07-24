/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#import <WebCore/IntRectHash.h>
#import <wtf/RetainPtr.h>

@class WKPrintingViewData;
@class PDFDocument;

namespace WebKit {
    class ShareableBitmap;
    class WebFrameProxy;
}

@interface WKPrintingView : NSView {
@public
    NSPrintOperation *_printOperation; // WKPrintingView is owned by the operation.
    RetainPtr<NSView> _wkView;

    RefPtr<WebKit::WebFrameProxy> _webFrame;
    Vector<WebCore::IntRect> _printingPageRects;
    double _totalScaleFactorForPrinting;
    HashMap<WebCore::IntRect, RefPtr<WebKit::ShareableBitmap>> _pagePreviews;

    Vector<uint8_t> _printedPagesData;
    RetainPtr<PDFDocument> _printedPagesPDFDocument;

    uint64_t _expectedComputedPagesCallback;
    HashMap<uint64_t, WebCore::IntRect> _expectedPreviewCallbacks;
    uint64_t _latestExpectedPreviewCallback;
    uint64_t _expectedPrintCallback;

    BOOL _isPrintingFromSecondaryThread;
    Mutex _printingCallbackMutex;
    ThreadCondition _printingCallbackCondition;

    NSTimer *_autodisplayResumeTimer;
}

- (id)initWithFrameProxy:(WebKit::WebFrameProxy*)frame view:(NSView *)wkView;

@end
