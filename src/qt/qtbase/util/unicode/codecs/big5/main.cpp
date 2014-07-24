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
#include <qtextcodec.h>
#include <qbytearray.h>
#include <qstring.h>
#include <qdebug.h>
#include <qfile.h>
#include <qcoreapplication.h>
#include <qset.h>

struct Map { Map(uint u,  uint b) : uc(u),  b5(b) {} uint uc; uint b5; };

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextCodec *big5 = QTextCodec::codecForName("Big5-hkscs");

#if 0
    QFile f("/home/lars/dev/qt-4.0/util/unicode/data/big5-eten.txt");
    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.startsWith("#"))
            continue;
        line.replace("0x", "");
        line.replace("U+", "");
        line.replace("\t", " ");
        line = line.simplified();
        QList<QByteArray> split = line.split(' ');
        bool ok;
        int b5 = split.at(0).toInt(&ok, 16);
        Q_ASSERT(ok);
        int uc = split.at(1).toInt(&ok, 16);
        Q_ASSERT(ok);
        if (b5 < 0x100)
            continue;
#else
    QFile f(":/BIG5");
    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.startsWith("CHARMAP"))
            break;
    }
    QSet<uint> b5_ok;
    QSet<uint> uc_ok;
    QList<Map> b5_to_uc_map;
    QList<Map> uc_to_b5_map;
    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.startsWith("%"))
            continue;
        if (line.startsWith("END CHARMAP"))
            break;
        line.replace("/x", "");
        line.replace("<U", "");
        line.replace(">", "");
        line.replace("\t", " ");
        line = line.simplified();
        QList<QByteArray> split = line.split(' ');
        bool ok;
        int b5 = split.at(1).toInt(&ok, 16);
        Q_ASSERT(ok);
        int uc = split.at(0).toInt(&ok, 16);
        Q_ASSERT(ok);
        if (b5 < 0x100 || uc > 0xffff)
            continue;
#endif

//         qDebug() << hex << "testing: '" << b5 << "' - '" << uc << "'";
        QByteArray ba;

        ba += (char)(uchar)(b5 >> 8);
        ba += (char)(uchar)(b5 & 0xff);

        QString s = big5->toUnicode(ba);
        Q_ASSERT(s.length() == 1);
        QString s2;
        s2 = QChar(uc);
        ba = big5->fromUnicode(s2);
        Q_ASSERT(ba.length() <= 2);
        int round;
        if (ba.length() == 1)
            round = (int)(uchar)ba[0];
        else
            round = ((int)(uchar)ba[0] << 8) + (int)(uchar)ba[1];
        if (b5 != round)
            uc_to_b5_map += Map(uc, b5);
        else
            b5_ok.insert(b5);

        if (s[0].unicode() != uc)
            b5_to_uc_map += Map(uc, b5);
        else
            uc_ok.insert(uc);
    };

    QList<QByteArray> list;
    foreach(Map m, b5_to_uc_map) {
        if (!uc_ok.contains(m.b5))
            list += QByteArray("    { 0x" + QByteArray::number(m.b5, 16) + ", 0x" + QByteArray::number(m.uc, 16) + " }\n");;
    }
    QByteArray ba;
    qSort(list);
    foreach(QByteArray a, list)
        ba += a;
    qDebug() << "struct B5Map b5_to_uc_map = {\n" << ba + "\n};";

    list = QList<QByteArray>();
    foreach(Map m, uc_to_b5_map)
        if (!b5_ok.contains(m.uc))
            list += QByteArray("    { 0x" + QByteArray::number(m.uc, 16) + ", 0x" + QByteArray::number(m.b5, 16) + " }\n");;
    ba = QByteArray();
    qSort(list);
    foreach(QByteArray a, list)
        ba += a;
    qDebug() << "struct B5Map uc_to_b5_map = {\n" << ba + "\n};";
}
