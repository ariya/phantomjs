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

void myProcessing(const QString &)
{
}

int main()
{
    QWidget myWidget;
    {
        // RECORD
//! [0]
        QPicture picture;
        QPainter painter;
        painter.begin(&picture);           // paint in picture
        painter.drawEllipse(10,20, 80,70); // draw an ellipse
        painter.end();                     // painting done
        picture.save("drawing.pic");       // save picture
//! [0]
    }

    {
        // REPLAY
//! [1]
        QPicture picture;
        picture.load("drawing.pic");           // load picture
        QPainter painter;
        painter.begin(&myImage);               // paint in myImage
        painter.drawPicture(0, 0, picture);    // draw the picture at (0,0)
        painter.end();                         // painting done
//! [1]
    }

    QPicture myPicture;
    {
        // FORMATS
//! [2]
        QStringList list = QPicture::inputFormatList();
        foreach (const QString &string, list)
            myProcessing(string);
//! [2]
    }

    {
        // OUTPUT
//! [3]
        QStringList list = QPicture::outputFormatList();
        foreach (const QString &string, list)
            myProcessing(string);
//! [3]
    }

    {
        // PIC READ
//! [4]
        QPictureIO iio;
        QPixmap  pixmap;
        iio.setFileName("vegeburger.pic");
        if (iio.read()) {        // OK
            QPicture picture = iio.picture();
            QPainter painter(&pixmap);
            painter.drawPicture(0, 0, picture);
        }
//! [4]
    }

    {
        QPixmap pixmap;
        // PIC WRITE
//! [5]
        QPictureIO iio;
        QPicture   picture;
        QPainter painter(&picture);
        painter.drawPixmap(0, 0, pixmap);
        iio.setPicture(picture);
        iio.setFileName("vegeburger.pic");
        iio.setFormat("PIC");
        if (iio.write())
            return true; // returned true if written successfully
//! [5]
    }

}

// SVG READ
//! [6]
void readSVG(QPictureIO *picture)
{
    // read the picture using the picture->ioDevice()
}
//! [6]

// SVG WRITE
//! [7]
void writeSVG(QPictureIO *picture)
{
    // write the picture using the picture->ioDevice()
}
//! [7]

// USE SVG
void foo() {

//! [8]
    // add the SVG picture handler
    // ...
//! [8]
    QPictureIO::defineIOHandler("SVG", 0, 0, readSVG, writeSVG);
    // ...

}
