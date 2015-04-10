/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include "config.h"
#include "MIMESniffing.h"

#include "TestData.h"

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtTest/QtTest>

class tst_MIMESniffing : public QObject {
    Q_OBJECT

public:
    tst_MIMESniffing();

private Q_SLOTS:
    void testCase1();
};

tst_MIMESniffing::tst_MIMESniffing()
{
}

static inline const char* errorText(const TestData& data, const char* sniffedType)
{
    return QString("file: %1, advertised: %2, image: %3. sniffed mime type was expected to be \"%4\" but instead was \"%5\"").arg(data.file).arg(data.advertisedType).arg(data.isImage).arg(data.sniffedType).arg(sniffedType).toLatin1();
}

void tst_MIMESniffing::testCase1()
{

    for (int i = 0; i < testListSize; ++i) {
        QFile file(testList[i].file);
        QVERIFY2(file.open(QIODevice::ReadOnly), QString("unable to open file %1").arg(file.fileName()).toLatin1());

        MIMESniffer sniffer(testList[i].advertisedType, testList[i].isImage);
        QByteArray data = file.peek(sniffer.dataSize());

        const char* sniffedType = sniffer.sniff(data.constData(), data.size());

        QVERIFY2(!(sniffedType || testList[i].sniffedType) || (sniffedType && testList[i].sniffedType), errorText(testList[i], sniffedType));

        if (sniffedType)
            QVERIFY2(!strcmp(sniffedType, testList[i].sniffedType), errorText(testList[i], sniffedType));

    }

    QVERIFY2(true, "Failure");
}

QTEST_APPLESS_MAIN(tst_MIMESniffing);

#include "tst_MIMESniffing.moc"
