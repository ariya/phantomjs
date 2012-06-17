/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIMAGE_P_H
#define QIMAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#include <QVector>

#ifndef QT_NO_IMAGE_TEXT
#include <QMap>
#endif

QT_BEGIN_NAMESPACE

class QImageWriter;

struct Q_GUI_EXPORT QImageData {        // internal image data
    QImageData();
    ~QImageData();
    static QImageData *create(const QSize &size, QImage::Format format, int numColors = 0);
    static QImageData *create(uchar *data, int w, int h,  int bpl, QImage::Format format, bool readOnly);

    QAtomicInt ref;

    int width;
    int height;
    int depth;
    int nbytes;               // number of bytes data
    QVector<QRgb> colortable;
    uchar *data;
#ifdef QT3_SUPPORT
    uchar **jumptable;
#endif
    QImage::Format format;
    int bytes_per_line;
    int ser_no;               // serial number
    int detach_no;

    qreal  dpmx;                // dots per meter X (or 0)
    qreal  dpmy;                // dots per meter Y (or 0)
    QPoint  offset;           // offset in pixels

    uint own_data : 1;
    uint ro_data : 1;
    uint has_alpha_clut : 1;
    uint is_cached : 1;

    bool checkForAlphaPixels() const;

    // Convert the image in-place, minimizing memory reallocation
    // Return false if the conversion cannot be done in-place.
    bool convertInPlace(QImage::Format newFormat, Qt::ImageConversionFlags);

#ifndef QT_NO_IMAGE_TEXT
    QMap<QString, QString> text;
#endif
    bool doImageIO(const QImage *image, QImageWriter* io, int quality) const;

    QPaintEngine *paintEngine;
};

void qInitImageConversions();
Q_GUI_EXPORT void qGamma_correct_back_to_linear_cs(QImage *image);

inline int qt_depthForFormat(QImage::Format format)
{
    int depth = 0;
    switch(format) {
    case QImage::Format_Invalid:
    case QImage::NImageFormats:
        Q_ASSERT(false);
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        depth = 1;
        break;
    case QImage::Format_Indexed8:
        depth = 8;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        depth = 32;
        break;
    case QImage::Format_RGB555:
    case QImage::Format_RGB16:
    case QImage::Format_RGB444:
    case QImage::Format_ARGB4444_Premultiplied:
        depth = 16;
        break;
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB888:
        depth = 24;
        break;
    }
    return depth;
}

QT_END_NAMESPACE

#endif
