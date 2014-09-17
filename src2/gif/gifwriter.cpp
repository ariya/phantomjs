/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "gifwriter.h"

#include "gif_lib.h"

#include <QImage>
#include <QFile>

static int saveGifBlock(GifFileType *gif, const GifByteType *data, int i)
{
    QFile *file = (QFile*)(gif->UserData);
    return file->write((const char*)data, i);
}

bool exportGif(const QImage &img, const QString &fileName)
{
    QFile file;
    file.setFileName(fileName);
    if (!file.open(QFile::WriteOnly)) {
        return false;
    }

    QImage image = img;

    int dim = image.width() * image.height();
    GifByteType *rBuffer = new GifByteType[dim];
    GifByteType *gBuffer = new GifByteType[dim];
    GifByteType *bBuffer = new GifByteType[dim];
    for (int i = 0; i < image.width(); ++i) {
        for (int j = 0; j < image.height(); ++j) {
            QRgb color = image.pixel(i, j);
            rBuffer[i + j * image.width()] = qRed(color);
            gBuffer[i + j * image.width()] = qGreen(color);
            bBuffer[i + j * image.width()] = qBlue(color);
        }
    }

    int ColorMapSize = 255;
    ColorMapObject cmap;
    cmap.ColorCount = ColorMapSize;
    cmap.BitsPerPixel = 8;
    cmap.Colors = new GifColorType[ColorMapSize];
    GifByteType *outputBuffer = new GifByteType[dim];
    QuantizeBuffer(image.width(), image.height(), &ColorMapSize,
                   rBuffer, gBuffer, bBuffer, outputBuffer, cmap.Colors);

    QVector<QRgb> colorTable;
    colorTable.reserve(256);
    for (int i = 0; i < ColorMapSize; ++i) {
        GifColorType color = cmap.Colors[i];
        colorTable += qRgb(color.Red, color.Green, color.Blue);
    }
    colorTable += qRgba(0, 0, 0, 0);
    while (colorTable.count() < 256)
        colorTable += qRgb(0, 0, 0);

    delete [] outputBuffer;
    delete [] rBuffer;
    delete [] gBuffer;
    delete [] bBuffer;

    image = image.convertToFormat(QImage::Format_Indexed8, colorTable);

    colorTable = image.colorTable();
    cmap.ColorCount = 256;
    cmap.BitsPerPixel = 8;
    int bgcolor = -1;
    for (int c = 0; c < 256; ++c) {
        cmap.Colors[c].Red = qRed(colorTable[c]);
        cmap.Colors[c].Green = qGreen(colorTable[c]);
        cmap.Colors[c].Blue = qBlue(colorTable[c]);
        if (qAlpha(colorTable[c]) == 0)
            bgcolor = c;
    }
    EGifSetGifVersion("87a");

    GifFileType *gif = EGifOpen(&file, saveGifBlock);
    gif->ImageCount = 1;
    EGifPutScreenDesc(gif, image.width(), image.height(), 256, 0, &cmap);
    if (bgcolor >= 0) {
        char extension[] = { 1, 0, 0, bgcolor };
        EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, 4, extension);
    }
    EGifPutImageDesc(gif, 0, 0, image.width(), image.height(), 0, &cmap);

    for (int y = 0; y < image.height(); ++y) {
        if (EGifPutLine(gif, (GifPixelType*)(image.scanLine(y)), img.width()) == GIF_ERROR) {
            break;
        }
    }

    EGifCloseFile(gif);
    file.close();

    delete [] cmap.Colors;

    return true;
}
