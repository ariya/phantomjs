/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#include <QtCore/qmath.h>

#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QStringList>

#include <stdio.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    qreal tolerance = 0; // Tolerated percentage of error pixels.

    QStringList args = app.arguments();
    for (int i = 0; i < argc; ++i)
        if (args[i] == "-t" || args[i] == "--tolerance")
            tolerance = args[i + 1].toDouble();

    char buffer[2048];
    QImage actualImage;
    QImage baselineImage;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        // remove the CR
        char* newLineCharacter = strchr(buffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strncmp("Content-Length: ", buffer, 16)) {
            strtok(buffer, " ");
            int imageSize = strtol(strtok(0, " "), 0, 10);

            if (imageSize <= 0) {
                fputs("error, image size must be specified.\n", stdout);
            } else {
                unsigned char buffer[2048];
                QBuffer data;

                // Read all the incoming chunks
                data.open(QBuffer::WriteOnly);
                while (imageSize > 0) {
                    size_t bytesToRead = qMin(imageSize, 2048);
                    size_t bytesRead = fread(buffer, 1, bytesToRead, stdin);
                    data.write(reinterpret_cast<const char*>(buffer), bytesRead);
                    imageSize -= static_cast<int>(bytesRead);
                }

                // Convert into QImage
                QImage decodedImage;
                decodedImage.loadFromData(data.data(), "PNG");
                decodedImage = decodedImage.convertToFormat(QImage::Format_ARGB32);

                // Place it in the right place
                if (actualImage.isNull())
                    actualImage = decodedImage;
                else
                    baselineImage = decodedImage;
            }
        }

        if (!actualImage.isNull() && !baselineImage.isNull()) {

            if (actualImage.size() != baselineImage.size()) {
                fprintf(stdout, "diff: 100%% failed\n");
                fprintf(stderr, "error, test and reference image have different properties.\n");
                fflush(stderr);
                fflush(stdout);
            } else {

                int w = actualImage.width();
                int h = actualImage.height();
                QImage diffImage(w, h, QImage::Format_ARGB32);

                int errorCount = 0;

                for (int x = 0; x < w; ++x) {
                    for (int y = 0; y < h; ++y) {
                        QRgb pixel = actualImage.pixel(x, y);
                        QRgb basePixel = baselineImage.pixel(x, y);
                        qreal red = (qRed(pixel) - qRed(basePixel)) / static_cast<float>(qMax(255 - qRed(basePixel), qRed(basePixel)));
                        qreal green = (qGreen(pixel) - qGreen(basePixel)) / static_cast<float>(qMax(255 - qGreen(basePixel), qGreen(basePixel)));
                        qreal blue = (qBlue(pixel) - qBlue(basePixel)) / static_cast<float>(qMax(255 - qBlue(basePixel), qBlue(basePixel)));
                        qreal alpha = (qAlpha(pixel) - qAlpha(basePixel)) / static_cast<float>(qMax(255 - qAlpha(basePixel), qAlpha(basePixel)));
                        qreal distance = qSqrt(red * red + green * green + blue * blue + alpha * alpha) / 2.0f;
                        if (distance >= 1 / qreal(255)) {
                            errorCount++;
                            diffImage.setPixel(x, y, qRgb(255, 0, 0));
                        } else
                            diffImage.setPixel(x, y, qRgba(qRed(basePixel), qGreen(basePixel), qBlue(basePixel), qAlpha(basePixel) * 0.5));
                    }
                }

                qreal difference = 0;
                if (errorCount)
                    difference = 100 * errorCount / static_cast<qreal>(w * h);

                if (difference <= tolerance)
                    fprintf(stdout, "diff: %01.2f%% passed\n", difference);
                else {
                    QBuffer buffer;
                    buffer.open(QBuffer::WriteOnly);
                    diffImage.save(&buffer, "PNG");
                    buffer.close();
                    const QByteArray &data = buffer.data();
                    printf("Content-Length: %lu\n", static_cast<unsigned long>(data.length()));

                    // We have to use the return value of fwrite to avoid "ignoring return value" gcc warning
                    // See https://bugs.webkit.org/show_bug.cgi?id=45384 for details.
                    if (fwrite(data.constData(), 1, data.length(), stdout)) {}

                    fprintf(stdout, "diff: %01.2f%% failed\n", difference);
                }

                fflush(stdout);
            }
            actualImage = QImage();
            baselineImage = QImage();
        }
    }

    return 0;
}
