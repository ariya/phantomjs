/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef qquickwebpage_p_p_h
#define qquickwebpage_p_p_h

#include "qquickwebpage_p.h"
#include <QTransform>

class QQuickWebViewPrivate;

class QQuickWebPagePrivate {
public:
    QQuickWebPagePrivate(QQuickWebPage* q, QQuickWebView* viewportItem);
    ~QQuickWebPagePrivate();

    void updateSize();

    void paint(QPainter*);
    void resetPaintNode();
    QQuickWebPage* const q;
    QQuickWebView* const viewportItem;
    bool paintingIsInitialized;

    QSizeF contentsSize;
    qreal contentsScale;
};

#endif // qquickwebpage_p_p_h
