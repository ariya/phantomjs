/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
    !!!!!! Warning !!!!!
    Please don't save this file in emacs. It contains utf8 text sequences emacs will
    silently convert to a series of question marks.
 */
#include <QtTest/QtTest>
#include <QtCore/qdebug.h>

#include <harfbuzz-shaper.h>

static QVector<HB_CharAttributes> getCharAttributes(const QString &str, HB_Script script = HB_Script_Common)
{
    QVector<HB_CharAttributes> attrs(str.length());
    HB_ScriptItem item;
    item.pos = 0;
    item.length = str.length();
    item.script = script;
    HB_GetCharAttributes(str.utf16(), str.length(),
                         &item, 1,
                         attrs.data());
    return attrs;
}

class tst_CharAttributes : public QObject
{
    Q_OBJECT

public:
    tst_CharAttributes();
    virtual ~tst_CharAttributes();

public slots:
    void init();
    void cleanup();
private slots:
    void lineBreaking();
    void charWordStopOnLineSeparator();
    void charStopForSurrogatePairs();
    void thaiWordBreak();
};


tst_CharAttributes::tst_CharAttributes()
{
}

tst_CharAttributes::~tst_CharAttributes()
{
}

void tst_CharAttributes::init()
{
}

void tst_CharAttributes::cleanup()
{
}


void tst_CharAttributes::lineBreaking()
{
    struct Breaks {
	const char *utf8;
	uchar breaks[32];
    };
    Breaks brks[] = {
	{ "11", { false, 0xff } },
	{ "aa", { false, 0xff } },
	{ "++", { false, 0xff } },
	{ "--", { false, 0xff } },
	{ "((", { false, 0xff } },
	{ "))", { false, 0xff } },
	{ "..", { false, 0xff } },
	{ "\"\"", { false, 0xff } },
	{ "$$", { false, 0xff } },
	{ "!!", { false, 0xff } },
	{ "??", { false, 0xff } },
	{ ",,", { false, 0xff } },

	{ ")()", { true, false, 0xff } },
	{ "?!?", { false, false, 0xff } },
	{ ".,.", { false, false, 0xff } },
	{ "+-+", { false, false, 0xff } },
	{ "+=+", { false, false, 0xff } },
	{ "+(+", { false, false, 0xff } },
	{ "+)+", { false, false, 0xff } },

	{ "a b", { false, true, 0xff } },
	{ "a(b", { false, false, 0xff } },
	{ "a)b", { false, false, 0xff } },
	{ "a-b", { false, true, 0xff } },
	{ "a.b", { false, false, 0xff } },
	{ "a+b", { false, false, 0xff } },
	{ "a?b", { false, false, 0xff } },
	{ "a!b", { false, false, 0xff } },
	{ "a$b", { false, false, 0xff } },
	{ "a,b", { false, false, 0xff } },
	{ "a/b", { false, false, 0xff } },
	{ "1/2", { false, false, 0xff } },
	{ "./.", { false, false, 0xff } },
	{ ",/,", { false, false, 0xff } },
	{ "!/!", { false, false, 0xff } },
	{ "\\/\\", { false, false, 0xff } },
	{ "1 2", { false, true, 0xff } },
	{ "1(2", { false, false, 0xff } },
	{ "1)2", { false, false, 0xff } },
	{ "1-2", { false, false, 0xff } },
	{ "1.2", { false, false, 0xff } },
	{ "1+2", { false, false, 0xff } },
	{ "1?2", { false, true, 0xff } },
	{ "1!2", { false, true, 0xff } },
	{ "1$2", { false, false, 0xff } },
	{ "1,2", { false, false, 0xff } },
	{ "1/2", { false, false, 0xff } },
	{ "\330\260\331\216\331\204\331\220\331\203\331\216", { false, false, false, false, false, 0xff } },
	{ "\330\247\331\204\331\205 \330\247\331\204\331\205", { false, false, false, true, false, false, 0xff } },
	{ "1#2", { false, false, 0xff } },
	{ "!#!", { false, false, 0xff } },
	{ 0, {} }
    };
    Breaks *b = brks;
    while (b->utf8) {
        QString str = QString::fromUtf8(b->utf8);

        QVector<HB_CharAttributes> attrs = getCharAttributes(str);

        int i;
        for (i = 0; i < (int)str.length() - 1; ++i) {
            QVERIFY(b->breaks[i] != 0xff);
            if ( (attrs[i].lineBreakType != HB_NoBreak) != (bool)b->breaks[i] ) {
                qDebug("test case \"%s\" failed at char %d; break type: %d", b->utf8, i, attrs[i].lineBreakType);
                QCOMPARE( (attrs[i].lineBreakType != HB_NoBreak), (bool)b->breaks[i] );
            }
        }
        QVERIFY(attrs[i].lineBreakType == HB_ForcedBreak);
        QCOMPARE(b->breaks[i], (uchar)0xff);
        ++b;
    }
}

void tst_CharAttributes::charWordStopOnLineSeparator()
{
    const QChar lineSeparator(QChar::LineSeparator);
    QString txt;
    txt.append(lineSeparator);
    txt.append(lineSeparator);
    QVector<HB_CharAttributes> attrs = getCharAttributes(txt);
    QVERIFY(attrs[1].charStop);
}

void tst_CharAttributes::charStopForSurrogatePairs()
{
    QString txt;
    txt.append("a");
    txt.append(0xd87e);
    txt.append(0xdc25);
    txt.append("b");
    QVector<HB_CharAttributes> attrs = getCharAttributes(txt);
    QVERIFY(attrs[0].charStop);
    QVERIFY(attrs[1].charStop);
    QVERIFY(!attrs[2].charStop);
    QVERIFY(attrs[3].charStop);
}

void tst_CharAttributes::thaiWordBreak()
{
    // สวัสดีครับ นี่เป็นการงทดสอบตัวเอ
    QTextCodec *codec = QTextCodec::codecForMib(2259);
    QString txt = codec->toUnicode(QByteArray("\xca\xc7\xd1\xca\xb4\xd5\xa4\xc3\xd1\xba\x20\xb9\xd5\xe8\xe0\xbb\xe7\xb9\xa1\xd2\xc3\xb7\xb4\xca\xcd\xba\xb5\xd1\xc7\xe0\xcd\xa7"));


    QCOMPARE(txt.length(), 32);
    QVector<HB_CharAttributes> attrs = getCharAttributes(txt, HB_Script_Thai);
    QVERIFY(attrs[0].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[1].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[2].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[3].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[4].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[5].lineBreakType == HB_Break);
    QVERIFY(attrs[6].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[7].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[8].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[9].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[10].lineBreakType == HB_Break);
    QVERIFY(attrs[11].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[12].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[13].lineBreakType == HB_Break);
    QVERIFY(attrs[14].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[15].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[16].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[17].lineBreakType == HB_Break);
    QVERIFY(attrs[18].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[19].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[20].lineBreakType == HB_Break);
    QVERIFY(attrs[21].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[22].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[23].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[24].lineBreakType == HB_NoBreak);
    QVERIFY(attrs[25].lineBreakType == HB_Break);
    QVERIFY(attrs[26].lineBreakType == HB_NoBreak);
    for (int i = 27; i < 32; ++i)
        QVERIFY(attrs[i].lineBreakType == HB_NoBreak);
}

QTEST_MAIN(tst_CharAttributes)
#include "main.moc"
