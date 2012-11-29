/*
    Copyright (C) 2012 Milian Wolff, KDAB (milian.wolff@kdab.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QWEBFRAME_PRINTINGADDONS_P_H
#define QWEBFRAME_PRINTINGADDONS_P_H

#include "qwebframe.h"
#include "qwebframe_p.h"

#include <qprinter.h>
#include <qstring.h>

#include "GraphicsContext.h"
#include "PrintContext.h"

// for custom header or footers in printing
class HeaderFooter
{
public:
    HeaderFooter(const QWebFrame* frame, QPrinter* printer, QWebFrame::PrintCallback* callback);
    ~HeaderFooter();

    void setPageRect(const WebCore::IntRect& rect);

    void paintHeader(WebCore::GraphicsContext& ctx, const WebCore::IntRect& pageRect, int pageNum, int totalPages);
    void paintFooter(WebCore::GraphicsContext& ctx, const WebCore::IntRect& pageRect, int pageNum, int totalPages);

    bool isValid()
    {
        return callback && (headerHeightPixel > 0 || footerHeightPixel > 0);
    }

private:
    QWebPage page;
    QWebFrame::PrintCallback* callback;
    int headerHeightPixel;
    int footerHeightPixel;

    WebCore::PrintContext* printCtx;

    void paint(WebCore::GraphicsContext& ctx, const WebCore::IntRect& pageRect, const QString& contents, int height);
};

HeaderFooter::HeaderFooter(const QWebFrame* frame, QPrinter* printer, QWebFrame::PrintCallback* callback_)
: printCtx(0)
, callback(callback_)
, headerHeightPixel(0)
, footerHeightPixel(0)
{
    if (callback) {
        qreal headerHeight = qMax(qreal(0), callback->headerHeight());
        qreal footerHeight = qMax(qreal(0), callback->footerHeight());

        if (headerHeight || footerHeight) {
            // figure out the header/footer height in *DevicePixel*
            // based on the height given in *Points*
            qreal marginLeft, marginRight, marginTop, marginBottom;
            // find existing margins
            printer->getPageMargins(&marginLeft, &marginTop, &marginRight, &marginBottom, QPrinter::DevicePixel);
            const qreal oldMarginTop = marginTop;
            const qreal oldMarginBottom = marginBottom;

            printer->getPageMargins(&marginLeft, &marginTop, &marginRight, &marginBottom, QPrinter::Point);
            // increase margins to hold header+footer
            marginTop += headerHeight;
            marginBottom += footerHeight;
            printer->setPageMargins(marginLeft, marginTop, marginRight, marginBottom, QPrinter::Point);

            // calculate actual heights
            printer->getPageMargins(&marginLeft, &marginTop, &marginRight, &marginBottom, QPrinter::DevicePixel);
            headerHeightPixel = marginTop - oldMarginTop;
            footerHeightPixel = marginBottom - oldMarginBottom;

            printCtx = new WebCore::PrintContext(QWebFramePrivate::webcoreFrame(page.mainFrame()));
        }
    }
}

HeaderFooter::~HeaderFooter()
{
    delete printCtx;
    printCtx = 0;
}

void HeaderFooter::paintHeader(WebCore::GraphicsContext& ctx, const WebCore::IntRect& pageRect, int pageNum, int totalPages)
{
    if (!headerHeightPixel) {
        return;
    }
    const QString c = callback->header(pageNum, totalPages);
    if (c.isEmpty()) {
        return;
    }

    ctx.translate(0, -headerHeightPixel);
    paint(ctx, pageRect, c, headerHeightPixel);
    ctx.translate(0, +headerHeightPixel);
}

void HeaderFooter::paintFooter(WebCore::GraphicsContext& ctx, const WebCore::IntRect& pageRect, int pageNum, int totalPages)
{
    if (!footerHeightPixel) {
        return;
    }
    const QString c = callback->footer(pageNum, totalPages);
    if (c.isEmpty()) {
        return;
    }

    const int offset = pageRect.height();
    ctx.translate(0, +offset);
    paint(ctx, pageRect, c, footerHeightPixel);
    ctx.translate(0, -offset);
}

void HeaderFooter::paint(WebCore::GraphicsContext& ctx, const WebCore::IntRect& pageRect, const QString& contents, int height)
{
    page.mainFrame()->setHtml(contents);

    printCtx->begin(pageRect.width(), height);
    float tempHeight;
    printCtx->computePageRects(pageRect, /* headerHeight */ 0, /* footerHeight */ 0, /* userScaleFactor */ 1.0, tempHeight);

    printCtx->spoolPage(ctx, 0, pageRect.width());

    printCtx->end();
}


#endif // QWEBFRAME_PRINTINGADDONS_P_H
