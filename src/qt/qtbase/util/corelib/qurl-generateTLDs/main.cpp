/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
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

#include <QtCore>

static QString utf8encode(const QByteArray &array) // turns e.g. tranøy.no to tran\xc3\xb8y.no
{
    QString result;
    result.reserve(array.length() + array.length() / 3);
    for (int i = 0; i < array.length(); ++i) {
        char c = array.at(i);
        // if char is non-ascii, escape it
        if (c < 0x20 || uchar(c) >= 0x7f) {
            result += "\\x" + QString::number(uchar(c), 16);
        } else {
            // if previous char was escaped, we need to make sure the next char is not
            // interpreted as part of the hex value, e.g. "äc.com" -> "\xabc.com"; this
            // should be "\xab""c.com"
            QRegExp hexEscape("\\\\x[a-fA-F0-9][a-fA-F0-9]$");
            bool isHexChar = ((c >= '0' && c <= '9') ||
                             (c >= 'a' && c <= 'f') ||
                             (c >= 'A' && c <= 'F'));
            if (result.contains(hexEscape) && isHexChar)
                result += "\"\"";
            result += c;
        }
    }
    return result;
}

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);
    if (argc < 3) {
        printf("\nusage: %s inputFile outputFile\n\n", argv[0]);
        printf("'inputFile' should be a list of effective TLDs, one per line,\n");
        printf("as obtained from http://publicsuffix.org . To create indices and data file\n");
        printf("file, do the following:\n\n");
        printf("       wget http://mxr.mozilla.org/mozilla-central/source/netwerk/dns/effective_tld_names.dat?raw=1 -O effective_tld_names.dat\n");
        printf("       grep '^[^\\/\\/]' effective_tld_names.dat > effective_tld_names.dat.trimmed\n");
        printf("       %s effective_tld_names.dat.trimmed effective_tld_names.dat.qt\n\n", argv[0]);
        printf("Now copy the data from effective_tld_names.dat.qt to the file src/corelib/io/qurltlds_p.h in your Qt repo\n\n");
        exit(1);
    }
    QFile file(argv[1]);
    QFile outFile(argv[2]);
    file.open(QIODevice::ReadOnly);
    outFile.open(QIODevice::WriteOnly);

    QByteArray outIndicesBufferBA;
    QBuffer outIndicesBuffer(&outIndicesBufferBA);
    outIndicesBuffer.open(QIODevice::WriteOnly);

    QByteArray outDataBufferBA;
    QBuffer outDataBuffer(&outDataBufferBA);
    outDataBuffer.open(QIODevice::WriteOnly);

    int lineCount = 0;
    while (!file.atEnd()) {
        file.readLine();
        lineCount++;
    }
    file.reset();
    QVector<QString> strings(lineCount);
    while (!file.atEnd()) {
        QString s = QString::fromUtf8(file.readLine());
        QString st = s.trimmed();
        int num = qt_hash(st) % lineCount;

        QString utf8String = utf8encode(st.toUtf8());

        // for domain 1.com, we could get something like
        // a.com\01.com, which would be interpreted as octal 01,
        // so we need to separate those strings with quotes
        QRegExp regexpOctalEscape(QLatin1String("^[0-9]"));
        if (!strings.at(num).isEmpty() && st.contains(regexpOctalEscape))
            strings[num].append("\"\"");

        strings[num].append(utf8String);
        strings[num].append("\\0");
    }

    outIndicesBuffer.write("static const quint16 tldCount = ");
    outIndicesBuffer.write(QByteArray::number(lineCount));
    outIndicesBuffer.write(";\n");
    outIndicesBuffer.write("static const quint32 tldIndices[");
//    outIndicesBuffer.write(QByteArray::number(lineCount+1)); // not needed
    outIndicesBuffer.write("] = {\n");

    int totalUtf8Size = 0;
    int chunkSize = 0;
    int stringUtf8Size = 0;
    QStringList chunks;
    for (int a = 0; a < lineCount; a++) {
        bool lineIsEmpty = strings.at(a).isEmpty();
        if (!lineIsEmpty) {
            strings[a].prepend("\"");
            strings[a].append("\"");
        }
        int zeroCount = strings.at(a).count(QLatin1String("\\0"));
        int utf8CharsCount = strings.at(a).count(QLatin1String("\\x"));
        int quoteCount = strings.at(a).count('"');
        stringUtf8Size = strings.at(a).count() - (zeroCount + quoteCount + utf8CharsCount * 3);
        chunkSize += stringUtf8Size;
        if (chunkSize > 65535) {
            static int chunkCount = 0;
            qWarning() << "chunk" << ++chunkCount << "has length" << chunkSize - stringUtf8Size;
            outDataBuffer.write(",\n\n");
            chunks.append(QByteArray::number(totalUtf8Size));
            chunkSize = 0;
        }
        outDataBuffer.write(strings.at(a).toUtf8());
        if (!lineIsEmpty)
            outDataBuffer.write("\n");
        outIndicesBuffer.write(QByteArray::number(totalUtf8Size));
        outIndicesBuffer.write(",\n");
        totalUtf8Size += stringUtf8Size;
    }
    chunks.append(QByteArray::number(totalUtf8Size));
    outIndicesBuffer.write(QByteArray::number(totalUtf8Size));
    outIndicesBuffer.write("};\n");
    outIndicesBuffer.close();
    outFile.write(outIndicesBufferBA);

    outDataBuffer.close();
    outFile.write("\nstatic const char *tldData[");
//    outFile.write(QByteArray::number(charSize)); // not needed
    outFile.write("] = {\n");
    outFile.write(outDataBufferBA);
    outFile.write("};\n");

    // write chunk information
    outFile.write("\nstatic const quint16 tldChunkCount = ");
    outFile.write(QByteArray::number(chunks.count()));
    outFile.write(";\nstatic const quint32 tldChunks[] = {");
    outFile.write(chunks.join(", ").toLatin1());
    outFile.write("};\n");
    outFile.close();
    printf("data generated to %s . Now copy the data from this file to src/corelib/io/qurltlds_p.h in your Qt repo\n", argv[2]);
    exit(0);
}
