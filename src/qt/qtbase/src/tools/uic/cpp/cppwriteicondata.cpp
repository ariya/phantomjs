/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "cppwriteicondata.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"

#include <qtextstream.h>

QT_BEGIN_NAMESPACE

namespace CPP {

static QByteArray transformImageData(QString data)
{
    int baSize = data.length() / 2;
    uchar *ba = new uchar[baSize];
    for (int i = 0; i < baSize; ++i) {
        char h = data[2 * (i)].toLatin1();
        char l = data[2 * (i) + 1].toLatin1();
        uchar r = 0;
        if (h <= '9')
            r += h - '0';
        else
            r += h - 'a' + 10;
        r = r << 4;
        if (l <= '9')
            r += l - '0';
        else
            r += l - 'a' + 10;
        ba[i] = r;
    }
    QByteArray ret(reinterpret_cast<const char *>(ba), baSize);
    delete [] ba;
    return ret;
}

static QByteArray unzipXPM(QString data, ulong& length)
{
#ifndef QT_NO_COMPRESS
    const int lengthOffset = 4;
    QByteArray ba(lengthOffset, ' ');

    // qUncompress() expects the first 4 bytes to be the expected length of the
    // uncompressed data
    ba[0] = (length & 0xff000000) >> 24;
    ba[1] = (length & 0x00ff0000) >> 16;
    ba[2] = (length & 0x0000ff00) >> 8;
    ba[3] = (length & 0x000000ff);
    ba.append(transformImageData(data));
    QByteArray baunzip = qUncompress(ba);
    return baunzip;
#else
    Q_UNUSED(data);
    Q_UNUSED(length);
    return QByteArray();
#endif
}


WriteIconData::WriteIconData(Uic *uic)
    : driver(uic->driver()), output(uic->output()), option(uic->option())
{
}

void WriteIconData::acceptUI(DomUI *node)
{
    TreeWalker::acceptUI(node);
}

void WriteIconData::acceptImages(DomImages *images)
{
    TreeWalker::acceptImages(images);
}

void WriteIconData::acceptImage(DomImage *image)
{
    // Limit line length when writing code.
    writeImage(output, option.indent, true, image);
}

void WriteIconData::writeImage(QTextStream &output, const QString &indent,
                               bool limitXPM_LineLength, const DomImage *image)
{
    QString img = image->attributeName() + QLatin1String("_data");
    QString data = image->elementData()->text();
    QString fmt = image->elementData()->attributeFormat();
    int size = image->elementData()->attributeLength();

    if (fmt == QLatin1String("XPM.GZ")) {
        ulong length = size;
        QByteArray baunzip = unzipXPM(data, length);
        length = baunzip.size();
        // shouldn't we test the initial 'length' against the
        // resulting 'length' to catch corrupt UIC files?
        int a = 0;
        int column = 0;
        bool inQuote = false;
        output << indent << "/* XPM */\n"
               << indent << "static const char* const " << img << "[] = { \n";
        while (baunzip[a] != '\"')
            a++;
        for (; a < (int) length; a++) {
            output << baunzip[a];
            if (baunzip[a] == '\n') {
                column = 0;
            } else if (baunzip[a] == '"') {
                inQuote = !inQuote;
            }

            column++;
            if (limitXPM_LineLength && column >= 512 && inQuote) {
                output << "\"\n\""; // be nice with MSVC & Co.
                column = 1;
            }
        }

        if (! baunzip.trimmed ().endsWith ("};"))
            output << "};";

        output << "\n\n";
    } else {
        output << indent << "static const unsigned char " << img << "[] = { \n";
        output << indent;
        int a ;
        for (a = 0; a < (int) (data.length()/2)-1; a++) {
            output << "0x" << QString(data[2*a]) << QString(data[2*a+1]) << ',';
            if (a % 12 == 11)
                output << '\n' << indent;
            else
                output << ' ';
        }
        output << "0x" << QString(data[2*a]) << QString(data[2*a+1]) << '\n';
        output << "};\n\n";
    }
}

void WriteIconData::writeImage(QIODevice &output, DomImage *image)
{
    const QByteArray array = transformImageData(image->elementData()->text());
    output.write(array.constData(), array.size());
}

} // namespace CPP

QT_END_NAMESPACE
