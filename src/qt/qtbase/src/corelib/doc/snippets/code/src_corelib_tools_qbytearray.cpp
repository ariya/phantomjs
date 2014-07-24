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


void wrapInFunction()
{

//! [0]
QByteArray ba("Hello");
//! [0]


//! [1]
QByteArray ba;
ba.resize(5);
ba[0] = 0x3c;
ba[1] = 0xb8;
ba[2] = 0x64;
ba[3] = 0x18;
ba[4] = 0xca;
//! [1]


//! [2]
for (int i = 0; i < ba.size(); ++i) {
    if (ba.at(i) >= 'a' && ba.at(i) <= 'f')
        cout << "Found character in range [a-f]" << endl;
}
//! [2]


//! [3]
QByteArray x("and");
x.prepend("rock ");         // x == "rock and"
x.append(" roll");          // x == "rock and roll"
x.replace(5, 3, "&");       // x == "rock & roll"
//! [3]


//! [4]
QByteArray ba("We must be <b>bold</b>, very <b>bold</b>");
int j = 0;
while ((j = ba.indexOf("<b>", j)) != -1) {
    cout << "Found <b> tag at index position " << j << endl;
    ++j;
}
//! [4]


//! [5]
QByteArray().isNull();          // returns true
QByteArray().isEmpty();         // returns true

QByteArray("").isNull();        // returns false
QByteArray("").isEmpty();       // returns true

QByteArray("abc").isNull();     // returns false
QByteArray("abc").isEmpty();    // returns false
//! [5]


//! [6]
QByteArray ba("Hello");
int n = ba.size();          // n == 5
ba.data()[0];               // returns 'H'
ba.data()[4];               // returns 'o'
ba.data()[5];               // returns '\0'
//! [6]


//! [7]
QByteArray().isEmpty();         // returns true
QByteArray("").isEmpty();       // returns true
QByteArray("abc").isEmpty();    // returns false
//! [7]


//! [8]
QByteArray ba("Hello world");
char *data = ba.data();
while (*data) {
    cout << "[" << *data << "]" << endl;
    ++data;
}
//! [8]


//! [9]
QByteArray ba;
for (int i = 0; i < 10; ++i)
    ba[i] = 'A' + i;
// ba == "ABCDEFGHIJ"
//! [9]


//! [10]
QByteArray ba("Stockholm");
ba.truncate(5);             // ba == "Stock"
//! [10]


//! [11]
QByteArray ba("STARTTLS\r\n");
ba.chop(2);                 // ba == "STARTTLS"
//! [11]


//! [12]
QByteArray x("free");
QByteArray y("dom");
x += y;
// x == "freedom"
//! [12]


//! [13]
QByteArray().isNull();          // returns true
QByteArray("").isNull();        // returns false
QByteArray("abc").isNull();     // returns false
//! [13]


//! [14]
QByteArray ba("Istambul");
ba.fill('o');
// ba == "oooooooo"

ba.fill('X', 2);
// ba == "XX"
//! [14]


//! [15]
QByteArray x("ship");
QByteArray y("air");
x.prepend(y);
// x == "airship"
//! [15]


//! [16]
QByteArray x("free");
QByteArray y("dom");
x.append(y);
// x == "freedom"
//! [16]


//! [17]
QByteArray ba("Meal");
ba.insert(1, QByteArray("ontr"));
// ba == "Montreal"
//! [17]


//! [18]
QByteArray ba("Montreal");
ba.remove(1, 4);
// ba == "Meal"
//! [18]


//! [19]
QByteArray x("Say yes!");
QByteArray y("no");
x.replace(4, 3, y);
// x == "Say no!"
//! [19]


//! [20]
QByteArray ba("colour behaviour flavour neighbour");
ba.replace(QByteArray("ou"), QByteArray("o"));
// ba == "color behavior flavor neighbor"
//! [20]


//! [21]
QByteArray x("sticky question");
QByteArray y("sti");
x.indexOf(y);               // returns 0
x.indexOf(y, 1);            // returns 10
x.indexOf(y, 10);           // returns 10
x.indexOf(y, 11);           // returns -1
//! [21]


//! [22]
QByteArray ba("ABCBA");
ba.indexOf("B");            // returns 1
ba.indexOf("B", 1);         // returns 1
ba.indexOf("B", 2);         // returns 3
ba.indexOf("X");            // returns -1
//! [22]


//! [23]
QByteArray x("crazy azimuths");
QByteArray y("az");
x.lastIndexOf(y);           // returns 6
x.lastIndexOf(y, 6);        // returns 6
x.lastIndexOf(y, 5);        // returns 2
x.lastIndexOf(y, 1);        // returns -1
//! [23]


//! [24]
QByteArray ba("ABCBA");
ba.lastIndexOf("B");        // returns 3
ba.lastIndexOf("B", 3);     // returns 3
ba.lastIndexOf("B", 2);     // returns 1
ba.lastIndexOf("X");        // returns -1
//! [24]


//! [25]
QByteArray url("ftp://ftp.qt-project.org/");
if (url.startsWith("ftp:"))
    ...
//! [25]


//! [26]
QByteArray url("http://qt-project.org/doc/qt-5.0/qtdoc/index.html");
if (url.endsWith(".html"))
    ...
//! [26]


//! [27]
QByteArray x("Pineapple");
QByteArray y = x.left(4);
// y == "Pine"
//! [27]


//! [28]
QByteArray x("Pineapple");
QByteArray y = x.right(5);
// y == "apple"
//! [28]


//! [29]
QByteArray x("Five pineapples");
QByteArray y = x.mid(5, 4);     // y == "pine"
QByteArray z = x.mid(5);        // z == "pineapples"
//! [29]


//! [30]
QByteArray x("Qt by DIGIA");
QByteArray y = x.toLower();
// y == "qt by digia"
//! [30]


//! [31]
QByteArray x("Qt by DIGIA");
QByteArray y = x.toUpper();
// y == "QT BY DIGIA"
//! [31]


//! [32]
QByteArray ba("  lots\t of\nwhitespace\r\n ");
ba = ba.simplified();
// ba == "lots of whitespace";
//! [32]


//! [33]
QByteArray ba("  lots\t of\nwhitespace\r\n ");
ba = ba.trimmed();
// ba == "lots\t of\nwhitespace";
//! [33]


//! [34]
QByteArray x("apple");
QByteArray y = x.leftJustified(8, '.');   // y == "apple..."
//! [34]


//! [35]
QByteArray x("apple");
QByteArray y = x.rightJustified(8, '.');    // y == "...apple"
//! [35]


//! [36]
QByteArray str("FF");
bool ok;
int hex = str.toInt(&ok, 16);     // hex == 255, ok == true
int dec = str.toInt(&ok, 10);     // dec == 0, ok == false
//! [36]


//! [37]
QByteArray str("FF");
bool ok;
long hex = str.toLong(&ok, 16);   // hex == 255, ok == true
long dec = str.toLong(&ok, 10);   // dec == 0, ok == false
//! [37]


//! [38]
QByteArray string("1234.56");
double a = string.toDouble();   // a == 1234.56
//! [38]


//! [39]
QByteArray text("Qt is great!");
text.toBase64();        // returns "UXQgaXMgZ3JlYXQh"
//! [39]

//! [39bis]
QByteArray text("<p>Hello?</p>");
text.toBase64(QByteArray::Base64 | QByteArray::OmitTrailingEquals);      // returns "PHA+SGVsbG8/PC9wPg"
text.toBase64(QByteArray::Base64);                                       // returns "PHA+SGVsbG8/PC9wPg=="
text.toBase64(QByteArray::Base64Url);                                    // returns "PHA-SGVsbG8_PC9wPg=="
text.toBase64(QByteArray::Base64Url | QByteArray::OmitTrailingEquals);   // returns "PHA-SGVsbG8_PC9wPg"
//! [39bis]


//! [40]
QByteArray ba;
int n = 63;
ba.setNum(n);           // ba == "63"
ba.setNum(n, 16);       // ba == "3f"
//! [40]


//! [41]
int n = 63;
QByteArray::number(n);              // returns "63"
QByteArray::number(n, 16);          // returns "3f"
QByteArray::number(n, 16).toUpper();  // returns "3F"
//! [41]


//! [42]
QByteArray ba = QByteArray::number(12.3456, 'E', 3);
// ba == 1.235E+01
//! [42]


//! [43]
 static const char mydata[] = {
    0x00, 0x00, 0x03, 0x84, 0x78, 0x9c, 0x3b, 0x76,
    0xec, 0x18, 0xc3, 0x31, 0x0a, 0xf1, 0xcc, 0x99,
    ...
    0x6d, 0x5b
};

QByteArray data = QByteArray::fromRawData(mydata, sizeof(mydata));
QDataStream in(&data, QIODevice::ReadOnly);
...
//! [43]


//! [44]
QByteArray text = QByteArray::fromBase64("UXQgaXMgZ3JlYXQh");
text.data();            // returns "Qt is great!"
//! [44]

//! [44bis]
QByteArray::fromBase64("PHA+SGVsbG8/PC9wPg==", QByteArray::Base64Encoding); // returns "<p>Hello?</p>"
QByteArray::fromBase64("PHA-SGVsbG8_PC9wPg==", QByteArray::Base64UrlEncoding); // returns "<p>Hello?</p>"
//! [44bis]


//! [45]
QByteArray text = QByteArray::fromHex("517420697320677265617421");
text.data();            // returns "Qt is great!"
//! [45]

//! [46]
QString tmp = "test";
QByteArray text = tmp.toLocal8Bit();
char *data = new char[text.size()];
strcpy(data, text.data());
delete [] data;
//! [46]

//! [47]
QString tmp = "test";
QByteArray text = tmp.toLocal8Bit();
char *data = new char[text.size() + 1];
strcpy(data, text.data());
delete [] data;
//! [47]

//! [48]
QByteArray ba1("ca\0r\0t");
ba1.size();                     // Returns 2.
ba1.constData();                // Returns "ca" with terminating \0.

QByteArray ba2("ca\0r\0t", 3);
ba2.size();                     // Returns 3.
ba2.constData();                // Returns "ca\0" with terminating \0.

QByteArray ba3("ca\0r\0t", 4);
ba3.size();                     // Returns 4.
ba3.constData();                // Returns "ca\0r" with terminating \0.

const char cart[] = {'c', 'a', '\0', 'r', '\0', 't'};
QByteArray ba4(QByteArray::fromRawData(cart, 6));
ba4.size();                     // Returns 6.
ba4.constData();                // Returns "ca\0r\0t" without terminating \0.
//! [48]

}


