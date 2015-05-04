/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPAINTENGINE_PREVIEW_P_H
#define QPAINTENGINE_PREVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QPreviewPrinter and friends.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtGui/qpaintengine.h>
#include <QtPrintSupport/qprintengine.h>

#ifndef QT_NO_PRINTPREVIEWWIDGET

QT_BEGIN_NAMESPACE

class QPreviewPaintEnginePrivate;

class QPreviewPaintEngine : public QPaintEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(QPreviewPaintEngine)
public:
    QPreviewPaintEngine();
    ~QPreviewPaintEngine();

    bool begin(QPaintDevice *dev);
    bool end();

    void updateState(const QPaintEngineState &state);

    void drawPath(const QPainterPath &path);
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    void drawTextItem(const QPointF &p, const QTextItem &textItem);

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    void drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &p);

    QList<const QPicture *> pages();

    QPaintEngine::Type type() const { return Picture; }

    void setProxyEngines(QPrintEngine *printEngine, QPaintEngine *paintEngine);

    void setProperty(PrintEnginePropertyKey key, const QVariant &value);
    QVariant property(PrintEnginePropertyKey key) const;

    bool newPage();
    bool abort();

    int metric(QPaintDevice::PaintDeviceMetric) const;

    QPrinter::PrinterState printerState() const;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTPREVIEWWIDGET

#endif
