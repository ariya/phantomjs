/*
 * Copyright (C) 2012 Igalia S.L.
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

#ifndef WebPrintOperationGtk_h
#define WebPrintOperationGtk_h

#include "PrintInfo.h"
#include <WebCore/RefPtrCairo.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/gobject/GRefPtr.h>

typedef struct _GtkPrintSettings GtkPrintSettings;
typedef struct _GtkPageSetup GtkPageSetup;
typedef struct _GtkPageRange GtkPageRange;

namespace WebCore {
class PrintContext;
class ResourceError;
};

namespace WebKit {

class WebPage;

class WebPrintOperationGtk : public RefCounted<WebPrintOperationGtk> {
public:
    static PassRefPtr<WebPrintOperationGtk> create(WebPage*, const PrintInfo&);
    virtual ~WebPrintOperationGtk();

    WebCore::PrintContext* printContext() const { return m_printContext; }
    GtkPrintSettings* printSettings() const { return m_printSettings.get(); }
    GtkPageSetup* pageSetup() const { return m_pageSetup.get(); }
    void setNumberOfPagesToPrint(size_t numberOfPages) { m_numberOfPagesToPrint = numberOfPages; }
    unsigned int pagesToPrint() const { return m_pagesToPrint; }
    int pageCount() const;
    bool currentPageIsFirstPageOfSheet() const;
    bool currentPageIsLastPageOfSheet() const;
    size_t pagePosition() const { return m_pagePosition; }
    void setPagePosition(size_t position) { m_pagePosition = position; }
    GtkPageRange* pageRanges() const { return m_pageRanges; }
    size_t pageRangesCount() const { return m_pageRangesCount; }

    unsigned int numberUp() const { return m_numberUp; }
    unsigned int numberUpLayout() const { return m_numberUpLayout; }
    unsigned int pageSet() const { return m_pageSet; }
    bool reverse() const { return m_reverse; }
    unsigned int copies() const { return m_copies; }
    bool collateCopies() const { return m_collateCopies; }
    double scale() const { return m_scale; }

    virtual void startPrint(WebCore::PrintContext*, uint64_t callbackID) = 0;

protected:
    WebPrintOperationGtk(WebPage*, const PrintInfo&);

    virtual void startPage(cairo_t*) = 0;
    virtual void endPage(cairo_t*) = 0;
    virtual void endPrint() = 0;

    static gboolean printPagesIdle(gpointer);
    static void printPagesIdleDone(gpointer);

    void print(cairo_surface_t*, double xDPI, double yDPI);
    void renderPage(int pageNumber);
    void rotatePageIfNeeded();
    void getRowsAndColumnsOfPagesPerSheet(size_t& rows, size_t& columns);
    void getPositionOfPageInSheet(size_t rows, size_t columns, int& x, int&y);
    void prepareContextToDraw();
    void printPagesDone();
    void printDone(const WebCore::ResourceError&);

    WebPage* m_webPage;
    GRefPtr<GtkPrintSettings> m_printSettings;
    GRefPtr<GtkPageSetup> m_pageSetup;
    WebCore::PrintContext* m_printContext;
    uint64_t m_callbackID;
    RefPtr<cairo_t> m_cairoContext;
    double m_xDPI;
    double m_yDPI;

    unsigned int m_printPagesIdleId;
    size_t m_numberOfPagesToPrint;
    unsigned int m_pagesToPrint;
    size_t m_pagePosition;
    GtkPageRange* m_pageRanges;
    size_t m_pageRangesCount;
    bool m_needsRotation;

    // Manual capabilities.
    unsigned int m_numberUp;
    unsigned int m_numberUpLayout;
    unsigned int m_pageSet;
    bool m_reverse;
    unsigned int m_copies;
    bool m_collateCopies;
    double m_scale;
};

}

#endif // WebPrintOperationGtk_h
