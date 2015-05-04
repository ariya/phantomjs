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

#ifndef QBMPHANDLER_P_H
#define QBMPHANDLER_P_H

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

#include "QtGui/qimageiohandler.h"

#ifndef QT_NO_IMAGEFORMAT_BMP

QT_BEGIN_NAMESPACE

struct BMP_FILEHDR {                     // BMP file header
    char   bfType[2];                    // "BM"
    qint32  bfSize;                      // size of file
    qint16  bfReserved1;
    qint16  bfReserved2;
    qint32  bfOffBits;                   // pointer to the pixmap bits
};

struct BMP_INFOHDR {                     // BMP information header
    qint32  biSize;                      // size of this struct
    qint32  biWidth;                     // pixmap width
    qint32  biHeight;                    // pixmap height
    qint16  biPlanes;                    // should be 1
    qint16  biBitCount;                  // number of bits per pixel
    qint32  biCompression;               // compression method
    qint32  biSizeImage;                 // size of image
    qint32  biXPelsPerMeter;             // horizontal resolution
    qint32  biYPelsPerMeter;             // vertical resolution
    qint32  biClrUsed;                   // number of colors used
    qint32  biClrImportant;              // number of important colors
};

// BMP-Handler, which is also able to read and write the DIB
// (Device-Independent-Bitmap) format used internally in the Windows operating
// system for OLE/clipboard operations. DIB is a subset of BMP (without file
// header). The Windows-Lighthouse plugin accesses the DIB-functionality.

class QBmpHandler : public QImageIOHandler
{
public:
    enum InternalFormat {
        DibFormat,
        BmpFormat
    };

    explicit QBmpHandler(InternalFormat fmt = BmpFormat);
    bool canRead() const;
    bool read(QImage *image);
    bool write(const QImage &image);

    QByteArray name() const;

    static bool canRead(QIODevice *device);

    QVariant option(ImageOption option) const;
    void setOption(ImageOption option, const QVariant &value);
    bool supportsOption(ImageOption option) const;

private:
    bool readHeader();
    inline QByteArray formatName() const;

    enum State {
        Ready,
        ReadHeader,
        Error
    };

    const InternalFormat m_format;

    State state;
    BMP_FILEHDR fileHeader;
    BMP_INFOHDR infoHeader;
    int startpos;
};

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_BMP

#endif // QBMPHANDLER_P_H
