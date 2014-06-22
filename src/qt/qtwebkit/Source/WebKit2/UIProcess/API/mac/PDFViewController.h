/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef PDFViewController_h
#define PDFViewController_h

#include "WebFindOptions.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

@class PDFView;
@class WKView;
@class WKPDFView;

namespace CoreIPC {
    class DataReference;
}

namespace WebKit {

class WebPageProxy;

class PDFViewController {
    WTF_MAKE_NONCOPYABLE(PDFViewController);

public:
    static PassOwnPtr<PDFViewController> create(WKView *);
    ~PDFViewController();

    WKView* wkView() const { return m_wkView; }
    WebPageProxy* page() const;
    NSView* pdfView() const;
    
    void setPDFDocumentData(const String& mimeType, const String& suggestedFilename, const CoreIPC::DataReference&);

    double zoomFactor() const;
    void setZoomFactor(double);

    static Class pdfPreviewViewClass();

    bool forwardScrollWheelEvent(NSEvent *);

    NSPrintOperation *makePrintOperation(NSPrintInfo *);
    void openPDFInFinder();
    void savePDFToDownloadsFolder();
    void linkClicked(const String& url);
    void print();

    void findString(const String&, FindOptions, unsigned maxMatchCount);
    void countStringMatches(const String&, FindOptions, unsigned maxMatchCount);

private:
    explicit PDFViewController(WKView *wkView);

    static Class pdfDocumentClass();
    static NSBundle* pdfKitBundle();

    WKView* m_wkView;

    RetainPtr<WKPDFView> m_wkPDFView;
    PDFView* m_pdfView;

    RetainPtr<NSString> m_suggestedFilename;
    RetainPtr<CFDataRef> m_pdfData;

    String m_temporaryPDFUUID;
};

} // namespace WebKit

#endif // PDFViewController_h
