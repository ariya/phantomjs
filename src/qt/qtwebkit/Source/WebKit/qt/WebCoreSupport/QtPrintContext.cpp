/*
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QtPrintContext.h"

#include "GraphicsContext.h"
#include "IntRect.h"
#include "PrintContext.h"
#include "QWebFrameAdapter.h"

using namespace WebCore;

QtPrintContext::QtPrintContext(QPainter* painter, const QRect& pageRect, QWebFrameAdapter* frameAdapter)
    : m_graphicsContext(new GraphicsContext(painter))
    , m_printContext(new PrintContext(frameAdapter->frame))
{
    m_printContext->begin(pageRect.width(), pageRect.height());

    float pageHeight = 0;
    m_printContext->computePageRects(IntRect(pageRect), /* headerHeight */ 0, /* footerHeight */ 0, /* userScaleFactor */ 1.0, pageHeight);
}

QtPrintContext::~QtPrintContext()
{
    m_printContext->end();
    delete m_graphicsContext;
    delete m_printContext;
}

int QtPrintContext::pageCount() const
{
    return m_printContext->pageCount();
}

void QtPrintContext::spoolPage(int pageNumber, float width)
{
    m_printContext->spoolPage(*m_graphicsContext, pageNumber, width);
}

WebCore::GraphicsContext& QtPrintContext::graphicsContext() const
{
    return *m_graphicsContext;
}
