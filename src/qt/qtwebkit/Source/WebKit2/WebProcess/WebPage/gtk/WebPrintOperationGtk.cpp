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

#include "config.h"
#include "WebPrintOperationGtk.h"

#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/ErrorsGtk.h>
#include <WebCore/IntRect.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/PlatformContextCairo.h>
#include <WebCore/PrintContext.h>
#include <WebCore/ResourceError.h>
#include <gtk/gtk.h>
#include <wtf/Vector.h>
#include <wtf/gobject/GOwnPtr.h>

#ifdef HAVE_GTK_UNIX_PRINTING
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include <gtk/gtkunixprint.h>
#endif

namespace WebKit {

#ifdef HAVE_GTK_UNIX_PRINTING
class WebPrintOperationGtkUnix: public WebPrintOperationGtk {
public:
    WebPrintOperationGtkUnix(WebPage* page, const PrintInfo& printInfo)
        : WebPrintOperationGtk(page, printInfo)
        , m_printJob(0)
    {
    }

    static gboolean enumeratePrintersFunction(GtkPrinter* printer, WebPrintOperationGtkUnix* printOperation)
    {
        const char* printerName = gtk_print_settings_get_printer(printOperation->printSettings());
        if ((printerName && strcmp(printerName, gtk_printer_get_name(printer)))
            || (!printerName && !gtk_printer_is_default(printer)))
            return FALSE;

        static int jobNumber = 0;
        const char* applicationName = g_get_application_name();
        GOwnPtr<char>jobName(g_strdup_printf("%s job #%d", applicationName ? applicationName : "WebKit", ++jobNumber));
        printOperation->m_printJob = adoptGRef(gtk_print_job_new(jobName.get(), printer,
                                                                 printOperation->printSettings(),
                                                                 printOperation->pageSetup()));
        return TRUE;
    }

    static void enumeratePrintersFinished(WebPrintOperationGtkUnix* printOperation)
    {
        if (!printOperation->m_printJob) {
            printOperation->printDone(printerNotFoundError(printOperation->m_printContext));
            return;
        }

        GOwnPtr<GError> error;
        cairo_surface_t* surface = gtk_print_job_get_surface(printOperation->m_printJob.get(), &error.outPtr());
        if (!surface) {
            printOperation->printDone(printError(printOperation->m_printContext, error->message));
            return;
        }

        int rangesCount;
        printOperation->m_pageRanges = gtk_print_job_get_page_ranges(printOperation->m_printJob.get(), &rangesCount);
        printOperation->m_pageRangesCount = rangesCount;
        printOperation->m_pagesToPrint = gtk_print_job_get_pages(printOperation->m_printJob.get());
        printOperation->m_needsRotation = gtk_print_job_get_rotate(printOperation->m_printJob.get());

        // Manual capabilities.
        printOperation->m_numberUp = gtk_print_job_get_n_up(printOperation->m_printJob.get());
        printOperation->m_numberUpLayout = gtk_print_job_get_n_up_layout(printOperation->m_printJob.get());
        printOperation->m_pageSet = gtk_print_job_get_page_set(printOperation->m_printJob.get());
        printOperation->m_reverse = gtk_print_job_get_reverse(printOperation->m_printJob.get());
        printOperation->m_copies = gtk_print_job_get_num_copies(printOperation->m_printJob.get());
        printOperation->m_collateCopies = gtk_print_job_get_collate(printOperation->m_printJob.get());
        printOperation->m_scale = gtk_print_job_get_scale(printOperation->m_printJob.get());

        printOperation->print(surface, 72, 72);
    }

    void startPrint(WebCore::PrintContext* printContext, uint64_t callbackID)
    {
        m_printContext = printContext;
        m_callbackID = callbackID;
        gtk_enumerate_printers(reinterpret_cast<GtkPrinterFunc>(enumeratePrintersFunction), this,
                               reinterpret_cast<GDestroyNotify>(enumeratePrintersFinished), FALSE);
    }

    void startPage(cairo_t* cr)
    {
        if (!currentPageIsFirstPageOfSheet())
          return;

        GtkPaperSize* paperSize = gtk_page_setup_get_paper_size(m_pageSetup.get());
        double width = gtk_paper_size_get_width(paperSize, GTK_UNIT_POINTS);
        double height = gtk_paper_size_get_height(paperSize, GTK_UNIT_POINTS);

        cairo_surface_t* surface = gtk_print_job_get_surface(m_printJob.get(), 0);
        cairo_surface_type_t surfaceType = cairo_surface_get_type(surface);
        if (surfaceType == CAIRO_SURFACE_TYPE_PS) {
            cairo_ps_surface_set_size(surface, width, height);
            cairo_ps_surface_dsc_begin_page_setup(surface);

            switch (gtk_page_setup_get_orientation(m_pageSetup.get())) {
            case GTK_PAGE_ORIENTATION_PORTRAIT:
            case GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
                cairo_ps_surface_dsc_comment(surface, "%%PageOrientation: Portrait");
                break;
            case GTK_PAGE_ORIENTATION_LANDSCAPE:
            case GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
                cairo_ps_surface_dsc_comment(surface, "%%PageOrientation: Landscape");
                break;
            }
        } else if (surfaceType == CAIRO_SURFACE_TYPE_PDF)
            cairo_pdf_surface_set_size(surface, width, height);
    }

    void endPage(cairo_t* cr)
    {
        if (currentPageIsLastPageOfSheet())
            cairo_show_page(cr);
    }

    static void printJobComplete(GtkPrintJob* printJob, WebPrintOperationGtkUnix* printOperation, const GError* error)
    {
        printOperation->printDone(error ? printError(printOperation->m_printContext, error->message) : WebCore::ResourceError());
        printOperation->m_printJob = 0;
    }

    static void printJobFinished(WebPrintOperationGtkUnix* printOperation)
    {
        printOperation->deref();
    }

    void endPrint()
    {
        cairo_surface_finish(gtk_print_job_get_surface(m_printJob.get(), 0));
        // Make sure the operation is alive until the job is sent.
        ref();
        gtk_print_job_send(m_printJob.get(), reinterpret_cast<GtkPrintJobCompleteFunc>(printJobComplete), this,
                           reinterpret_cast<GDestroyNotify>(printJobFinished));
    }

    GRefPtr<GtkPrintJob> m_printJob;
};
#endif

#ifdef G_OS_WIN32
class WebPrintOperationGtkWin32: public WebPrintOperationGtk {
public:
    WebPrintOperationGtkWin32(WebPage* page, const PrintInfo& printInfo)
        : WebPrintOperationGtk(page, printInfo)
    {
    }

    void startPrint(WebCore::PrintContext* printContext, uint64_t callbackID)
    {
        m_printContext = printContext;
        m_callbackID = callbackID;
        notImplemented();
    }

    void startPage(cairo_t* cr)
    {
        notImplemented();
    }

    void endPage(cairo_t* cr)
    {
        notImplemented();
    }

    void endPrint()
    {
        notImplemented();
    }
};
#endif

struct PrintPagesData {
    PrintPagesData(WebPrintOperationGtk* printOperation)
        : printOperation(printOperation)
        , totalPrinted(-1)
        , pageNumber(0)
        , sheetNumber(0)
        , firstSheetNumber(0)
        , numberOfSheets(0)
        , firstPagePosition(0)
        , collated(0)
        , uncollated(0)
        , isDone(false)
        , isValid(true)
    {
        if (printOperation->collateCopies()) {
            collatedCopies = printOperation->copies();
            uncollatedCopies = 1;
        } else {
            collatedCopies = 1;
            uncollatedCopies = printOperation->copies();
        }

        if (printOperation->pagesToPrint() == GTK_PRINT_PAGES_RANGES) {
            Vector<GtkPageRange> pageRanges;
            GtkPageRange* ranges = printOperation->pageRanges();
            size_t rangesCount = printOperation->pageRangesCount();
            int pageCount = printOperation->pageCount();

            pageRanges.reserveCapacity(rangesCount);
            for (size_t i = 0; i < rangesCount; ++i) {
                if (ranges[i].start >= 0 && ranges[i].start < pageCount && ranges[i].end >= 0 && ranges[i].end < pageCount)
                    pageRanges.append(ranges[i]);
                else if (ranges[i].start >= 0 && ranges[i].start < pageCount && ranges[i].end >= pageCount) {
                    pageRanges.append(ranges[i]);
                    pageRanges.last().end = pageCount - 1;
                } else if (ranges[i].end >= 0 && ranges[i].end < pageCount && ranges[i].start < 0) {
                    pageRanges.append(ranges[i]);
                    pageRanges.last().start = 0;
                }
            }

            for (size_t i = 0; i < pageRanges.size(); ++i) {
                for (int j = pageRanges[i].start; j <= pageRanges[i].end; ++j)
                    pages.append(j);
            }

        } else {
            for (int i = 0; i < printOperation->pageCount(); ++i)
                pages.append(i);
        }

        if (!pages.size()) {
            isValid = false;
            return;
        }
        printOperation->setNumberOfPagesToPrint(pages.size());

        size_t numberUp = printOperation->numberUp();
        if (numberUp > 1)
            numberOfSheets = (pages.size() % numberUp) ? pages.size() / numberUp + 1 : pages.size() / numberUp;
        else
            numberOfSheets = pages.size();

        bool reverse = printOperation->reverse();
        switch (printOperation->pageSet()) {
        case GTK_PAGE_SET_ODD:
            if (reverse) {
                lastPagePosition = std::min(numberUp - 1, pages.size() - 1);
                sheetNumber = (numberOfSheets - 1) - (numberOfSheets - 1) % 2;
            } else
                lastPagePosition = std::min(((numberOfSheets - 1) - ((numberOfSheets - 1) % 2)) * numberUp - 1, pages.size() - 1);
            break;
        case GTK_PAGE_SET_EVEN:
            if (reverse) {
                lastPagePosition = std::min(2 * numberUp - 1, pages.size() - 1);
                sheetNumber = (numberOfSheets - 1) - (1 - (numberOfSheets - 1) % 2);
            } else {
                lastPagePosition = std::min(((numberOfSheets - 1) - (1 - (numberOfSheets - 1) % 2)) * numberUp - 1, pages.size() - 1);
                sheetNumber = numberOfSheets > 1 ? 1 : -1;
            }
            break;
        case GTK_PAGE_SET_ALL:
            if (reverse) {
                lastPagePosition = std::min(numberUp - 1, pages.size() - 1);
                sheetNumber = pages.size() - 1;
            } else
                lastPagePosition = pages.size() - 1;
            break;
        }

        if (sheetNumber * numberUp >= pages.size()) {
            isValid = false;
            return;
        }

        printOperation->setPagePosition(sheetNumber * numberUp);
        pageNumber = pages[printOperation->pagePosition()];
        firstPagePosition = printOperation->pagePosition();
        firstSheetNumber = sheetNumber;
    }

    size_t collatedCopiesLeft()
    {
        return collatedCopies > 1 ? collatedCopies - collated - 1 : 0;
    }

    size_t uncollatedCopiesLeft()
    {
        return uncollatedCopies > 1 ? uncollatedCopies - uncollated - 1 : 0;
    }

    size_t copiesLeft()
    {
        return collatedCopiesLeft() + uncollatedCopiesLeft();
    }

    void incrementPageSequence()
    {
        if (totalPrinted == -1) {
            totalPrinted = 0;
            return;
        }

        size_t pagePosition = printOperation->pagePosition();
        if (pagePosition == lastPagePosition && !copiesLeft()) {
            isDone = true;
            return;
        }

        if (pagePosition == lastPagePosition && uncollatedCopiesLeft()) {
            pagePosition = firstPagePosition;
            sheetNumber = firstSheetNumber;
            uncollated++;
        } else if (printOperation->currentPageIsLastPageOfSheet()) {
            if (!collatedCopiesLeft()) {
                int step = printOperation->pageSet() == GTK_PAGE_SET_ALL ? 1 : 2;
                step *= printOperation->reverse() ? -1 : 1;
                sheetNumber += step;
                collated = 0;
            } else
                collated++;
            pagePosition = sheetNumber * printOperation->numberUp();
        } else
            pagePosition++;
        printOperation->setPagePosition(pagePosition);

        if (pagePosition >= pages.size() || sheetNumber >= numberOfSheets) {
            isDone = true;
            return;
        }
        pageNumber = pages[pagePosition];
        totalPrinted++;
    }

    RefPtr<WebPrintOperationGtk> printOperation;

    int totalPrinted;
    size_t totalToPrint;
    int pageNumber;
    Vector<size_t> pages;
    size_t sheetNumber;
    size_t firstSheetNumber;
    size_t numberOfSheets;
    size_t firstPagePosition;
    size_t lastPagePosition;
    size_t collated;
    size_t uncollated;
    size_t collatedCopies;
    size_t uncollatedCopies;

    bool isDone : 1;
    bool isValid : 1;
};

PassRefPtr<WebPrintOperationGtk> WebPrintOperationGtk::create(WebPage* page, const PrintInfo& printInfo)
{
#ifdef HAVE_GTK_UNIX_PRINTING
    return adoptRef(new WebPrintOperationGtkUnix(page, printInfo));
#elif defined(G_OS_WIN32)
    return adoptRef(new WebPrintOperationGtkWin32(page, printInfo));
#else
    return 0;
#endif
}

WebPrintOperationGtk::WebPrintOperationGtk(WebPage* page, const PrintInfo& printInfo)
    : m_webPage(page)
    , m_printSettings(printInfo.printSettings.get())
    , m_pageSetup(printInfo.pageSetup.get())
    , m_printContext(0)
    , m_callbackID(0)
    , m_xDPI(1)
    , m_yDPI(1)
    , m_printPagesIdleId(0)
    , m_numberOfPagesToPrint(0)
    , m_pagesToPrint(GTK_PRINT_PAGES_ALL)
    , m_pagePosition(0)
    , m_pageRanges(0)
    , m_pageRangesCount(0)
    , m_needsRotation(false)
    , m_numberUp(1)
    , m_numberUpLayout(0)
    , m_pageSet(GTK_PAGE_SET_ALL)
    , m_reverse(false)
    , m_copies(1)
    , m_collateCopies(false)
    , m_scale(1)
{
}

WebPrintOperationGtk::~WebPrintOperationGtk()
{
    if (m_printPagesIdleId)
        g_source_remove(m_printPagesIdleId);
}

int WebPrintOperationGtk::pageCount() const
{
    return m_printContext ? m_printContext->pageCount() : 0;
}

bool WebPrintOperationGtk::currentPageIsFirstPageOfSheet() const
{
    return (m_numberUp < 2 || !((m_pagePosition) % m_numberUp));
}

bool WebPrintOperationGtk::currentPageIsLastPageOfSheet() const
{
    return (m_numberUp < 2 || !((m_pagePosition + 1) % m_numberUp) || m_pagePosition == m_numberOfPagesToPrint - 1);
}

void WebPrintOperationGtk::rotatePageIfNeeded()
{
    if (!m_needsRotation)
        return;

    GtkPaperSize* paperSize = gtk_page_setup_get_paper_size(m_pageSetup.get());
    double width = gtk_paper_size_get_width(paperSize, GTK_UNIT_INCH) * m_xDPI;
    double height = gtk_paper_size_get_height(paperSize, GTK_UNIT_INCH) * m_yDPI;

    cairo_matrix_t matrix;
    switch (gtk_page_setup_get_orientation(m_pageSetup.get())) {
    case GTK_PAGE_ORIENTATION_LANDSCAPE:
        cairo_translate(m_cairoContext.get(), 0, height);
        cairo_matrix_init(&matrix, 0, -1, 1, 0, 0, 0);
        cairo_transform(m_cairoContext.get(), &matrix);
        break;
    case GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
        cairo_translate(m_cairoContext.get(), width, height);
        cairo_matrix_init(&matrix, -1, 0, 0, -1, 0, 0);
        cairo_transform(m_cairoContext.get(), &matrix);
        break;
    case GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
        cairo_translate(m_cairoContext.get(), width, 0);
        cairo_matrix_init(&matrix, 0, 1, -1, 0, 0, 0);
        cairo_transform(m_cairoContext.get(), &matrix);
        break;
    case GTK_PAGE_ORIENTATION_PORTRAIT:
    default:
        break;
    }
}

void WebPrintOperationGtk::getRowsAndColumnsOfPagesPerSheet(size_t& rows, size_t& columns)
{
    switch (m_numberUp) {
    default:
        columns = 1;
        rows = 1;
        break;
    case 2:
        columns = 2;
        rows = 1;
        break;
    case 4:
        columns = 2;
        rows = 2;
        break;
    case 6:
        columns = 3;
        rows = 2;
        break;
    case 9:
        columns = 3;
        rows = 3;
        break;
    case 16:
        columns = 4;
        rows = 4;
        break;
    }
}

void WebPrintOperationGtk::getPositionOfPageInSheet(size_t rows, size_t columns, int& x, int&y)
{
    switch (m_numberUpLayout) {
    case GTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM:
        x = m_pagePosition % columns;
        y = (m_pagePosition / columns) % rows;
        break;
    case GTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP:
        x = m_pagePosition % columns;
        y = rows - 1 - (m_pagePosition / columns) % rows;
        break;
    case GTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM:
        x = columns - 1 - m_pagePosition % columns;
        y = (m_pagePosition / columns) % rows;
        break;
    case GTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP:
        x = columns - 1 - m_pagePosition % columns;
        y = rows - 1 - (m_pagePosition / columns) % rows;
        break;
    case GTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT:
        x = (m_pagePosition / rows) % columns;
        y = m_pagePosition % rows;
        break;
    case GTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT:
        x = columns - 1 - (m_pagePosition / rows) % columns;
        y = m_pagePosition % rows;
        break;
    case GTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT:
        x = (m_pagePosition / rows) % columns;
        y = rows - 1 - m_pagePosition % rows;
        break;
    case GTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT:
        x = columns - 1 - (m_pagePosition / rows) % columns;
        y = rows - 1 - m_pagePosition % rows;
        break;
    }
}

void WebPrintOperationGtk::prepareContextToDraw()
{
    if (m_numberUp < 2) {
        double left = gtk_page_setup_get_left_margin(m_pageSetup.get(), GTK_UNIT_INCH);
        double top = gtk_page_setup_get_top_margin(m_pageSetup.get(), GTK_UNIT_INCH);
        if (m_scale != 1.0)
            cairo_scale(m_cairoContext.get(), m_scale, m_scale);
        rotatePageIfNeeded();
        cairo_translate(m_cairoContext.get(), left * m_xDPI, top * m_yDPI);

        return;
    }

    rotatePageIfNeeded();

    // Multiple pages per sheet.
    double marginLeft = gtk_page_setup_get_left_margin(m_pageSetup.get(), GTK_UNIT_POINTS);
    double marginRight = gtk_page_setup_get_right_margin(m_pageSetup.get(), GTK_UNIT_POINTS);
    double marginTop = gtk_page_setup_get_top_margin(m_pageSetup.get(), GTK_UNIT_POINTS);
    double marginBottom = gtk_page_setup_get_bottom_margin(m_pageSetup.get(), GTK_UNIT_POINTS);

    double paperWidth = gtk_page_setup_get_paper_width(m_pageSetup.get(), GTK_UNIT_POINTS);
    double paperHeight = gtk_page_setup_get_paper_height(m_pageSetup.get(), GTK_UNIT_POINTS);

    size_t rows, columns;
    getRowsAndColumnsOfPagesPerSheet(rows, columns);

    GtkPageOrientation orientation = gtk_page_setup_get_orientation(m_pageSetup.get());
    double pageWidth = 0, pageHeight = 0;
    switch (orientation) {
    case GTK_PAGE_ORIENTATION_PORTRAIT:
    case GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
        pageWidth = paperWidth - (marginLeft + marginRight);
        pageHeight = paperHeight - (marginTop + marginBottom);
        cairo_translate(m_cairoContext.get(), marginLeft, marginTop);
        break;
    case GTK_PAGE_ORIENTATION_LANDSCAPE:
    case GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
        pageWidth = paperWidth - (marginTop + marginBottom);
        pageHeight = paperHeight - (marginLeft + marginRight);
        cairo_translate(m_cairoContext.get(), marginTop, marginLeft);

        size_t tmp = columns;
        columns = rows;
        rows = tmp;
        break;
    }

    int x, y;
    getPositionOfPageInSheet(rows, columns, x, y);

    switch (m_numberUp) {
    case 4:
    case 9:
    case 16: {
        double scaleX = pageWidth / (columns * paperWidth);
        double scaleY = pageHeight / (rows * paperHeight);
        double scale = std::min(scaleX, scaleY);

        double stepX = paperWidth * (scaleX / scale);
        double stepY = paperHeight * (scaleY / scale);

        double width = gtk_page_setup_get_page_width(m_pageSetup.get(), GTK_UNIT_INCH) * m_xDPI;
        double height = gtk_page_setup_get_page_height(m_pageSetup.get(), GTK_UNIT_INCH) * m_yDPI;

        double offsetX, offsetY;
        if (marginLeft + marginRight > 0) {
            offsetX = marginLeft * (stepX - width) / (marginLeft + marginRight);
            offsetY = marginTop * (stepY - height) / (marginTop + marginBottom);
        } else {
            offsetX = (stepX - width) / 2.0;
            offsetY = (stepY - height) / 2.0;
        }

        cairo_scale(m_cairoContext.get(), scale, scale);
        cairo_translate(m_cairoContext.get(), x * stepX + offsetX, y * stepY + offsetY);
        if (m_scale != 1.0)
            cairo_scale(m_cairoContext.get(), m_scale, m_scale);
        break;
    }
    case 2:
    case 6: {
        double scaleX = pageHeight / (columns * paperWidth);
        double scaleY = pageWidth / (rows * paperHeight);
        double scale = std::min(scaleX, scaleY);

        double stepX = paperWidth * (scaleX / scale);
        double stepY = paperHeight * (scaleY / scale);

        double offsetX = ((stepX - paperWidth) / 2.0 * columns) - marginRight;
        double offsetY = ((stepY - paperHeight) / 2.0 * rows) + marginTop;

        cairo_scale(m_cairoContext.get(), scale, scale);
        cairo_translate(m_cairoContext.get(), y * paperHeight + offsetY, (columns - x) * paperWidth + offsetX);
        if (m_scale != 1.0)
            cairo_scale(m_cairoContext.get(), m_scale, m_scale);
        cairo_rotate(m_cairoContext.get(), -G_PI / 2);
        break;
    }
    default:
        break;
    }
}

void WebPrintOperationGtk::renderPage(int pageNumber)
{
    startPage(m_cairoContext.get());
    cairo_save(m_cairoContext.get());

    prepareContextToDraw();

    double pageWidth = gtk_page_setup_get_page_width(m_pageSetup.get(), GTK_UNIT_INCH) * m_xDPI;
    WebCore::PlatformContextCairo platformContext(m_cairoContext.get());
    WebCore::GraphicsContext graphicsContext(&platformContext);
    m_printContext->spoolPage(graphicsContext, pageNumber, pageWidth / m_scale);

    cairo_restore(m_cairoContext.get());
    endPage(m_cairoContext.get());
}

gboolean WebPrintOperationGtk::printPagesIdle(gpointer userData)
{
    PrintPagesData* data = static_cast<PrintPagesData*>(userData);

    data->incrementPageSequence();
    if (data->isDone)
        return FALSE;

    data->printOperation->renderPage(data->pageNumber);
    return TRUE;
}

void WebPrintOperationGtk::printPagesIdleDone(gpointer userData)
{
    PrintPagesData* data = static_cast<PrintPagesData*>(userData);

    data->printOperation->printPagesDone();
    delete data;
}

void WebPrintOperationGtk::printPagesDone()
{
    m_printPagesIdleId = 0;
    endPrint();
    m_cairoContext = 0;
}

void WebPrintOperationGtk::printDone(const WebCore::ResourceError& error)
{
    if (m_printPagesIdleId)
        g_source_remove(m_printPagesIdleId);
    m_printPagesIdleId = 0;

    // Print finished or failed, notify the UI process that we are done.
    m_webPage->send(Messages::WebPageProxy::PrintFinishedCallback(error, m_callbackID));
}

void WebPrintOperationGtk::print(cairo_surface_t* surface, double xDPI, double yDPI)
{
    ASSERT(m_printContext);

    OwnPtr<PrintPagesData> data = adoptPtr(new PrintPagesData(this));
    if (!data->isValid) {
        cairo_surface_finish(surface);
        printDone(invalidPageRangeToPrint(m_printContext));
        return;
    }

    m_xDPI = xDPI;
    m_yDPI = yDPI;
    m_cairoContext = adoptRef(cairo_create(surface));
    m_printPagesIdleId = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE + 10, printPagesIdle,
                                                   data.leakPtr(), printPagesIdleDone);
}

}
