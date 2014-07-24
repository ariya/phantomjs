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
#ifndef QtPrintContext_h
#define QtPrintContext_h

#include <PlatformExportMacros.h>
#include <QPainter>
#include <QRect>
#include <qwebkitglobal.h>

namespace WebCore {
class PrintContext;
class GraphicsContext;
}

class QWebFrameAdapter;

class QtPrintContext {
public:
    QtPrintContext(QPainter*, const QRect& pageRect, QWebFrameAdapter*);
    ~QtPrintContext();

    int pageCount() const;
    void spoolPage(int pageNumber, float width);
    WebCore::GraphicsContext& graphicsContext() const;

private:
    WebCore::GraphicsContext* m_graphicsContext;
    WebCore::PrintContext* m_printContext;
};

#endif
