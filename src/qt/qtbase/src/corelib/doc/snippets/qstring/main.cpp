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
#include <QApplication>
#include <stdio.h>

class Widget : public QWidget
{
public:
    Widget(QWidget *parent = 0);

    void constCharPointer();
    void constCharArray();
    void characterReference();
    void atFunction();
    void stringLiteral();
    void modify();
    void index();
    QString boolToString(bool b);
    void nullVsEmpty();

    void appendFunction();
    void argFunction();
    void chopFunction();
    void compareFunction();
    void compareSensitiveFunction();
    void containsFunction();
    void countFunction();
    void dataFunction();
    void endsWithFunction();
    void fillFunction();
    void fromRawDataFunction();

    void indexOfFunction();
    void firstIndexOfFunction();
    void insertFunction();
    void isNullFunction();
    void isEmptyFunction();
    void lastIndexOfFunction();
    void leftFunction();
    void leftJustifiedFunction();
    void leftRefFunction();
    void midFunction();
    void midRefFunction();
    void numberFunction();

    void prependFunction();
    void removeFunction();
    void replaceFunction();
    void reserveFunction();
    void resizeFunction();
    void rightFunction();
    void rightJustifiedFunction();
    void rightRefFunction();
    void sectionFunction();
    void setNumFunction();
    void simplifiedFunction();

    void sizeFunction();
    void splitFunction();
    void splitCaseSensitiveFunction();
    void sprintfFunction();
    void startsWithFunction();
    void toDoubleFunction();
    void toFloatFunction();
    void toIntFunction();
    void toLongFunction();
    void toLongLongFunction();

    void toLowerFunction();
    void toShortFunction();
    void toUIntFunction();
    void toULongFunction();
    void toULongLongFunction();
    void toUShortFunction();
    void toUpperFunction();
    void trimmedFunction();
    void truncateFunction();

    void plusEqualOperator();
    void arrayOperator();
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
}

void Widget::constCharPointer()
{
    //! [0]
    QString str = "Hello";
    //! [0]
}

void Widget::constCharArray()
{
    //! [1]
    static const QChar data[4] = { 0x0055, 0x006e, 0x10e3, 0x03a3 };
    QString str(data, 4);
    //! [1]
}

void Widget::characterReference()
{
    //! [2]
    QString str;
    str.resize(4);

    str[0] = QChar('U');
    str[1] = QChar('n');
    str[2] = QChar(0x10e3);
    str[3] = QChar(0x03a3);
    //! [2]
}

void Widget::atFunction()
{
    //! [3]
    QString str;

    for (int i = 0; i < str.size(); ++i) {
        if (str.at(i) >= QChar('a') && str.at(i) <= QChar('f'))
            qDebug() << "Found character in range [a-f]";
    }
    //! [3]
}

void Widget::stringLiteral()
{
    //! [4]
    QString str;

    if (str == "auto" || str == "extern"
            || str == "static" || str == "register") {
        // ...
    }
    //! [4]
}

void Widget::modify()
{
    //! [5]
    QString str = "and";
    str.prepend("rock ");     // str == "rock and"
    str.append(" roll");        // str == "rock and roll"
    str.replace(5, 3, "&");   // str == "rock & roll"
    //! [5]
}

void Widget::index()
{
    //! [6]
    QString str = "We must be <b>bold</b>, very <b>bold</b>";
    int j = 0;

    while ((j = str.indexOf("<b>", j)) != -1) {
        qDebug() << "Found <b> tag at index position" << j;
        ++j;
    }
    //! [6]
}

//! [7]
QString Widget::boolToString(bool b)
{
    QString result;
    if (b)
        result = "True";
    else
        result = "False";
    return result;
}
//! [7]


void Widget::nullVsEmpty()
{
    //! [8]
    QString().isNull();               // returns true
    QString().isEmpty();              // returns true

    QString("").isNull();             // returns false
    QString("").isEmpty();            // returns true

    QString("abc").isNull();          // returns false
    QString("abc").isEmpty();         // returns false
    //! [8]
}

void Widget::appendFunction()
{
    //! [9]
    QString x = "free";
    QString y = "dom";

    x.append(y);
    // x == "freedom"
    //! [9]

    //! [10]
    x.insert(x.size(), y);
    //! [10]
}

void Widget::argFunction()
{
    //! [11]
    QString i;           // current file's number
    QString total;       // number of files to process
    QString fileName;    // current file's name

    QString status = QString("Processing file %1 of %2: %3")
                    .arg(i).arg(total).arg(fileName);
    //! [11]

    //! [12] //! [13]
    QString str;
    //! [12]
    str = "%1 %2";

    str.arg("%1f", "Hello");        // returns "%1f Hello"
    str.arg("%1f").arg("Hello");    // returns "Hellof %2"
    //! [13]

    //! [14]
    str = QString("Decimal 63 is %1 in hexadecimal")
            .arg(63, 0, 16);
    // str == "Decimal 63 is 3f in hexadecimal"

    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    str = QString("%1 %L2 %L3")
            .arg(12345)
            .arg(12345)
            .arg(12345, 0, 16);
    // str == "12345 12,345 3039"
    //! [14]
}

void Widget::chopFunction()
{
    //! [15]
    QString str("LOGOUT\r\n");
    str.chop(2);
    // str == "LOGOUT"
    //! [15]
}

void Widget::compareFunction()
{
    int x = QString::compare("auto", "auto");   // x == 0
    int y = QString::compare("auto", "car");    // y < 0
    int z = QString::compare("car", "auto");    // z > 0
}

void Widget::compareSensitiveFunction()
{
    //! [16]
    int x = QString::compare("aUtO", "AuTo", Qt::CaseInsensitive);  // x == 0
    int y = QString::compare("auto", "Car", Qt::CaseSensitive);     // y > 0
    int z = QString::compare("auto", "Car", Qt::CaseInsensitive);   // z < 0
    //! [16]
}

void Widget::containsFunction()
{
    //! [17]
    QString str = "Peter Pan";
    str.contains("peter", Qt::CaseInsensitive);    // returns true
    //! [17]
}

void Widget::countFunction()
{
    //! [18]
    QString str = "banana and panama";
    str.count(QRegExp("a[nm]a"));    // returns 4
    //! [18]

    //! [95]
    QString str = "banana and panama";
    str.count(QRegularExpression("a[nm]a"));    // returns 4
    //! [95]
}

void Widget::dataFunction()
{
    //! [19]
    QString str = "Hello world";
    QChar *data = str.data();
    while (!data->isNull()) {
        qDebug() << data->unicode();
        ++data;
    }
    //! [19]
}

void Widget::endsWithFunction()
{
    //! [20]
    QString str = "Bananas";
    str.endsWith("anas");         // returns true
    str.endsWith("pple");         // returns false
    //! [20]
}

void Widget::fillFunction()
{
    //! [21]
    QString str = "Berlin";
    str.fill('z');
    // str == "zzzzzz"

    str.fill('A', 2);
    // str == "AA"
    //! [21]
}

void Widget::fromRawDataFunction()
{
    //! [22]
    QRegExp pattern;
    static const QChar unicode[] = {
            0x005A, 0x007F, 0x00A4, 0x0060,
            0x1009, 0x0020, 0x0020};
    int size = sizeof(unicode) / sizeof(QChar);

    QString str = QString::fromRawData(unicode, size);
    if (str.contains(QRegExp(pattern))) {
        // ...
    //! [22] //! [23]
    }
    //! [23]
}

void Widget::indexOfFunction()
{
    //! [24]
    QString x = "sticky question";
    QString y = "sti";
    x.indexOf(y);               // returns 0
    x.indexOf(y, 1);            // returns 10
    x.indexOf(y, 10);           // returns 10
    x.indexOf(y, 11);           // returns -1
    //! [24]
}

void Widget::firstIndexOfFunction()
{
    //! [25]
    QString str = "the minimum";
    str.indexOf(QRegExp("m[aeiou]"), 0);       // returns 4
    //! [25]

    //! [93]
    QString str = "the minimum";
    str.indexOf(QRegularExpression("m[aeiou]"), 0);       // returns 4
    //! [93]
}

void Widget::insertFunction()
{
    //! [26]
    QString str = "Meal";
    str.insert(1, QString("ontr"));
    // str == "Montreal"
    //! [26]
}

void Widget::isEmptyFunction()
{
    //! [27]
    QString().isEmpty();            // returns true
    QString("").isEmpty();          // returns true
    QString("x").isEmpty();         // returns false
    QString("abc").isEmpty();       // returns false
    //! [27]
}

void Widget::isNullFunction()
{
    //! [28]
    QString().isNull();             // returns true
    QString("").isNull();           // returns false
    QString("abc").isNull();        // returns false
    //! [28]
}

void Widget::lastIndexOfFunction()
{
    //! [29]
    QString x = "crazy azimuths";
    QString y = "az";
    x.lastIndexOf(y);           // returns 6
    x.lastIndexOf(y, 6);        // returns 6
    x.lastIndexOf(y, 5);        // returns 2
    x.lastIndexOf(y, 1);        // returns -1
    //! [29]

    //! [30]
    QString str = "the minimum";
    str.lastIndexOf(QRegExp("m[aeiou]"));      // returns 8
    //! [30]

    //! [94]
    QString str = "the minimum";
    str.lastIndexOf(QRegularExpression("m[aeiou]"));      // returns 8
    //! [94]
}

void Widget::leftFunction()
{
    //! [31]
    QString x = "Pineapple";
    QString y = x.left(4);      // y == "Pine"
    //! [31]
}

void Widget::leftJustifiedFunction()
{
    //! [32]
    QString s = "apple";
    QString t = s.leftJustified(8, '.');    // t == "apple..."
    //! [32]

    //! [33]
    QString str = "Pineapple";
    str = str.leftJustified(5, '.', true);    // str == "Pinea"
    //! [33]
}

void Widget::midFunction()
{
    //! [34]
    QString x = "Nine pineapples";
    QString y = x.mid(5, 4);            // y == "pine"
    QString z = x.mid(5);               // z == "pineapples"
    //! [34]
}

void Widget::numberFunction()
{
    //! [35]
    long a = 63;
    QString s = QString::number(a, 16);             // s == "3f"
    QString t = QString::number(a, 16).toUpper();     // t == "3F"
    //! [35]
}

void Widget::prependFunction()
{
    //! [36]
    QString x = "ship";
    QString y = "air";
    x.prepend(y);
    // x == "airship"
    //! [36]
}

void Widget::removeFunction()
{
    //! [37]
    QString s = "Montreal";
    s.remove(1, 4);
    // s == "Meal"
    //! [37]

    //! [38]
    QString t = "Ali Baba";
    t.remove(QChar('a'), Qt::CaseInsensitive);
    // t == "li Bb"
    //! [38]

    //! [39]
    QString r = "Telephone";
    r.remove(QRegExp("[aeiou]."));
    // r == "The"
    //! [39]

    //! [96]
    QString r = "Telephone";
    r.remove(QRegularExpression("[aeiou]."));
    // r == "The"
    //! [96]
}

void Widget::replaceFunction()
{
    //! [40]
    QString x = "Say yes!";
    QString y = "no";
    x.replace(4, 3, y);
    // x == "Say no!"
    //! [40]

    //! [41]
    QString str = "colour behaviour flavour neighbour";
    str.replace(QString("ou"), QString("o"));
    // str == "color behavior flavor neighbor"
    //! [41]

    //! [42]
    QString s = "Banana";
    s.replace(QRegExp("a[mn]"), "ox");
    // s == "Boxoxa"
    //! [42]

    //! [43]
    QString t = "A <i>bon mot</i>.";
    t.replace(QRegExp("<i>([^<]*)</i>"), "\\emph{\\1}");
    // t == "A \\emph{bon mot}."
    //! [43]

    //! [86]
    QString equis = "xxxxxx";
    equis.replace("xx", "x");
    // equis == "xxx"
    //! [86]

    //! [87]
    QString s = "Banana";
    s.replace(QRegularExpression("a[mn]"), "ox");
    // s == "Boxoxa"
    //! [87]

    //! [88]
    QString t = "A <i>bon mot</i>.";
    t.replace(QRegularExpression("<i>([^<]*)</i>"), "\\emph{\\1}");
    // t == "A \\emph{bon mot}."
    //! [88]
}

void Widget::reserveFunction()
{
    //! [44]
    QString result;
    int maxSize;
    bool condition;
    QChar nextChar;

    result.reserve(maxSize);

    while (condition)
        result.append(nextChar);

    result.squeeze();
    //! [44]
}

void Widget::resizeFunction()
{
    //! [45]
    QString s = "Hello world";
    s.resize(5);
    // s == "Hello"

    s.resize(8);
    // s == "Hello???" (where ? stands for any character)
    //! [45]

    //! [46]
    QString t = "Hello";
    t += QString(10, 'X');
    // t == "HelloXXXXXXXXXX"
    //! [46]

    //! [47]
    QString r = "Hello";
    r = r.leftJustified(10, ' ');
    // r == "Hello     "
    //! [47]
}

void Widget::rightFunction()
{
    //! [48]
    QString x = "Pineapple";
    QString y = x.right(5);      // y == "apple"
    //! [48]
}

void Widget::rightJustifiedFunction()
{
    //! [49]
    QString s = "apple";
    QString t = s.rightJustified(8, '.');    // t == "...apple"
    //! [49]

    //! [50]
    QString str = "Pineapple";
    str = str.rightJustified(5, '.', true);    // str == "Pinea"
    //! [50]
}

void Widget::sectionFunction()
{
    //! [51] //! [52]
    QString str;
    //! [51]
    QString csv = "forename,middlename,surname,phone";
    QString path = "/usr/local/bin/myapp"; // First field is empty
    QString::SectionFlag flag = QString::SectionSkipEmpty;


    str = csv.section(',', 2, 2);   // str == "surname"
    str = path.section('/', 3, 4);  // str == "bin/myapp"
    str = path.section('/', 3, 3, flag); // str == "myapp"
    //! [52]

    //! [53]
    str = csv.section(',', -3, -2);  // str == "middlename,surname"
    str = path.section('/', -1); // str == "myapp"
    //! [53]

    //! [54]
    QString data = "forename**middlename**surname**phone";

    str = data.section("**", 2, 2); // str == "surname"
    str = data.section("**", -3, -2); // str == "middlename**surname"
    //! [54]

    //! [55]
    QString line = "forename\tmiddlename  surname \t \t phone";
    QRegExp sep("\\s+");
    str = line.section(sep, 2, 2); // str == "surname"
    str = line.section(sep, -3, -2); // str == "middlename  surname"
    //! [55]

    //! [89]
    QString line = "forename\tmiddlename  surname \t \t phone";
    QRegularExpression sep("\\s+");
    str = line.section(sep, 2, 2); // str == "surname"
    str = line.section(sep, -3, -2); // str == "middlename  surname"
    //! [89]
}

void Widget::setNumFunction()
{
    //! [56]
    QString str;
    str.setNum(1234);       // str == "1234"
    //! [56]
}

void Widget::simplifiedFunction()
{
    //! [57]
    QString str = "  lots\t of\nwhitespace\r\n ";
    str = str.simplified();
    // str == "lots of whitespace";
    //! [57]
}

void Widget::sizeFunction()
{
    //! [58]
    QString str = "World";
    int n = str.size();         // n == 5
    str.data()[0];              // returns 'W'
    str.data()[4];              // returns 'd'
    str.data()[5];              // returns '\0'
    //! [58]
}

void Widget::splitFunction()
{
    //! [59]
    QString str;
    QStringList list;

    str = "Some  text\n\twith  strange whitespace.";
    list = str.split(QRegExp("\\s+"));
    // list: [ "Some", "text", "with", "strange", "whitespace." ]
    //! [59]

    //! [60]
    str = "This time, a normal English sentence.";
    list = str.split(QRegExp("\\W+"), QString::SkipEmptyParts);
    // list: [ "This", "time", "a", "normal", "English", "sentence" ]
    //! [60]

    //! [61]
    str = "Now: this sentence fragment.";
    list = str.split(QRegExp("\\b"));
    // list: [ "", "Now", ": ", "this", " ", "sentence", " ", "fragment", "." ]
    //! [61]

    //! [90]
    QString str;
    QStringList list;

    str = "Some  text\n\twith  strange whitespace.";
    list = str.split(QRegularExpression("\\s+"));
    // list: [ "Some", "text", "with", "strange", "whitespace." ]
    //! [90]

    //! [91]
    str = "This time, a normal English sentence.";
    list = str.split(QRegularExpression("\\W+"), QString::SkipEmptyParts);
    // list: [ "This", "time", "a", "normal", "English", "sentence" ]
    //! [91]

    //! [92]
    str = "Now: this sentence fragment.";
    list = str.split(QRegularExpression("\\b"));
    // list: [ "", "Now", ": ", "this", " ", "sentence", " ", "fragment", "." ]
    //! [92]
}

void Widget::splitCaseSensitiveFunction()
{
    //! [62]
    QString str = "a,,b,c";

    QStringList list1 = str.split(",");
    // list1: [ "a", "", "b", "c" ]

    QStringList list2 = str.split(",", QString::SkipEmptyParts);
    // list2: [ "a", "b", "c" ]
    //! [62]
}

void Widget::sprintfFunction()
{
    //! [63]
    size_t BufSize;
    char buf[BufSize];

    ::snprintf(buf, BufSize, "%lld", 123456789LL);
    QString str = QString::fromUtf8(buf);
    //! [63]

    //! [64]
    QString result;
    QTextStream(&result) << "pi = " << 3.14;
    // result == "pi = 3.14"
    //! [64]
}

void Widget::startsWithFunction()
{
    //! [65]
    QString str = "Bananas";
    str.startsWith("Ban");     // returns true
    str.startsWith("Car");     // returns false
    //! [65]
}

void Widget::toDoubleFunction()
{
    //! [66]
    QString str = "1234.56";
    double val = str.toDouble();   // val == 1234.56
    //! [66]

    //! [67]
    bool ok;
    double d;

    d = QString( "1234.56e-02" ).toDouble(&ok); // ok == true, d == 12.3456
    //! [67]

    //! [68]
    d = QString( "1234,56" ).toDouble(&ok); // ok == false
    d = QString( "1234.56" ).toDouble(&ok); // ok == true, d == 1234.56
    //! [68]

    //! [69]
    d = QString( "1,234,567.89" ).toDouble(&ok); // ok == false
    d = QString( "1234567.89" ).toDouble(&ok); // ok == true
     //! [69]
}

void Widget::toFloatFunction()
{
    //! [71]
    QString str1 = "1234.56";
    str1.toFloat();             // returns 1234.56

    bool ok;
    QString str2 = "R2D2";
    str2.toFloat(&ok);          // returns 0.0, sets ok to false
    //! [71]
}

void Widget::toIntFunction()
{
    //! [72]
    QString str = "FF";
    bool ok;
    int hex = str.toInt(&ok, 16);       // hex == 255, ok == true
    int dec = str.toInt(&ok, 10);       // dec == 0, ok == false
    //! [72]
}

void Widget::toLongFunction()
{
    //! [73]
    QString str = "FF";
    bool ok;

    long hex = str.toLong(&ok, 16);     // hex == 255, ok == true
    long dec = str.toLong(&ok, 10);     // dec == 0, ok == false
    //! [73]
}

void Widget::toLongLongFunction()
{
    //! [74]
    QString str = "FF";
    bool ok;

    qint64 hex = str.toLongLong(&ok, 16);      // hex == 255, ok == true
    qint64 dec = str.toLongLong(&ok, 10);      // dec == 0, ok == false
    //! [74]
}

void Widget::toLowerFunction()
{
    //! [75]
    QString str = "The Qt PROJECT";
    str = str.toLower();        // str == "the qt project"
    //! [75]
}

void Widget::toShortFunction()
{
    //! [76]
    QString str = "FF";
    bool ok;

    short hex = str.toShort(&ok, 16);   // hex == 255, ok == true
    short dec = str.toShort(&ok, 10);   // dec == 0, ok == false
    //! [76]
}

void Widget::toUIntFunction()
{
    //! [77]
    QString str = "FF";
    bool ok;

    uint hex = str.toUInt(&ok, 16);     // hex == 255, ok == true
    uint dec = str.toUInt(&ok, 10);     // dec == 0, ok == false
    //! [77]
}

void Widget::toULongFunction()
{
    //! [78]
    QString str = "FF";
    bool ok;

    ulong hex = str.toULong(&ok, 16);   // hex == 255, ok == true
    ulong dec = str.toULong(&ok, 10);   // dec == 0, ok == false
    //! [78]
}

void Widget::toULongLongFunction()
{
    //! [79]
    QString str = "FF";
    bool ok;

    quint64 hex = str.toULongLong(&ok, 16);    // hex == 255, ok == true
    quint64 dec = str.toULongLong(&ok, 10);    // dec == 0, ok == false
    //! [79]
}

void Widget::toUShortFunction()
{
    //! [80]
    QString str = "FF";
    bool ok;

    ushort hex = str.toUShort(&ok, 16);     // hex == 255, ok == true
    ushort dec = str.toUShort(&ok, 10);     // dec == 0, ok == false
    //! [80]
}

void Widget::toUpperFunction()
{
    //! [81]
    QString str = "TeXt";
    str = str.toUpper();        // str == "TEXT"
    //! [81]
}

void Widget::trimmedFunction()
{
    //! [82]
    QString str = "  lots\t of\nwhitespace\r\n ";
    str = str.trimmed();
    // str == "lots\t of\nwhitespace"
    //! [82]
}

void Widget::truncateFunction()
{
    //! [83]
    QString str = "Vladivostok";
    str.truncate(4);
    // str == "Vlad"
    //! [83]
}

void Widget::plusEqualOperator()
{
    //! [84]
    QString x = "free";
    QString y = "dom";
    x += y;
    // x == "freedom"
    //! [84]
}

void Widget::arrayOperator()
{
    //! [85]
    QString str;

    if (str[0] == QChar('?'))
        str[0] = QChar('_');
    //! [85]
}

void Widget::midRefFunction()
{
    //! [midRef]
    QString x = "Nine pineapples";
    QStringRef y = x.midRef(5, 4);      // y == "pine"
    QStringRef z = x.midRef(5);         // z == "pineapples"
    //! [midRef]
}

void Widget::leftRefFunction()
{
    //! [leftRef]
    QString x = "Pineapple";
    QStringRef y = x.leftRef(4);        // y == "Pine"
    //! [leftRef]
}

void Widget::rightRefFunction()
{
    //! [rightRef]
    QString x = "Pineapple";
    QStringRef y = x.rightRef(5);       // y == "apple"
    //! [rightRef]
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Widget widget;
    widget.show();
    return app.exec();
}
