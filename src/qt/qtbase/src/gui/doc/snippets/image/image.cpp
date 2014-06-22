/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

int main()
{
    int x, y;
    {
        // BIT ACCESS
        QImage image;
        // sets bit at (x, y) to 1
        if (image.format() == QImage::Format_MonoLSB)
            image.scanLine(y)[x >> 3] |= 1 << (x & 7);
        else
            image.scanLine(y)[x >> 3] |= 1 << (7 - (x & 7));
    }

    {
        // 8-BIT ACCESS
        QImage image;
        // set entry 19 in the color table to yellow
        image.setColor(19, qRgb(255, 255, 0));

        // set 8 bit pixel at (x,y) to value yellow (in color table)
        image.scanLine(y)[x] = 19;
    }

    {
        // 32-BIT
        QImage image;
        // sets 32 bit pixel at (x,y) to yellow.
        uint *ptr = reinterpret_cast<uint *>(image.scanLine(y)) + x;
        *ptr = qRgb(255, 255, 0);
    }

    {
        // SAVE
//! [0]
        QImage image;
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG"); // writes image into ba in PNG format
//! [0]
    }

    {
        // PIX SAVE
//! [1]
        QPixmap pixmap;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
//! [1]
    }

    {
        // MASK
//! [2]
        QPixmap alpha("image-with-alpha.png");
        QPixmap alphacopy = alpha;
        alphacopy.setMask(alphacopy.mask());
//! [2]
    }
}
