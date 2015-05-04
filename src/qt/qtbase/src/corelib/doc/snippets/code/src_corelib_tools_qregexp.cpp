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

//! [0]
QRegExp rx("(\\d+)");
QString str = "Offsets: 12 14 99 231 7";
QStringList list;
int pos = 0;

while ((pos = rx.indexIn(str, pos)) != -1) {
    list << rx.cap(1);
    pos += rx.matchedLength();
}
// list: ["12", "14", "99", "231", "7"]
//! [0]


//! [1]
QRegExp rx("*.txt");
rx.setPatternSyntax(QRegExp::Wildcard);
rx.exactMatch("README.txt");        // returns true
rx.exactMatch("welcome.txt.bak");   // returns false
//! [1]


//! [2]
QRegExp rx("ro+m");
rx.setMinimal(true);
//! [2]


//! [3]
QRegExp mark("\\b"      // word boundary
              "[Mm]ark" // the word we want to match
            );
//! [3]


//! [4]
QRegExp rx("^\\d\\d?$");    // match integers 0 to 99
rx.indexIn("123");          // returns -1 (no match)
rx.indexIn("-6");           // returns -1 (no match)
rx.indexIn("6");            // returns 0 (matched at position 0)
//! [4]


//! [5]
QRegExp rx("^\\S+$");       // match strings without whitespace
rx.indexIn("Hello world");  // returns -1 (no match)
rx.indexIn("This_is-OK");   // returns 0 (matched at position 0)
//! [5]


//! [6]
QRegExp rx("\\b(mail|letter|correspondence)\\b");
rx.indexIn("I sent you an email");     // returns -1 (no match)
rx.indexIn("Please write the letter"); // returns 17
//! [6]


//! [7]
QString captured = rx.cap(1); // captured == "letter"
//! [7]


//! [8]
QRegExp rx("&(?!amp;)");      // match ampersands but not &amp;
QString line1 = "This & that";
line1.replace(rx, "&amp;");
// line1 == "This &amp; that"
QString line2 = "His &amp; hers & theirs";
line2.replace(rx, "&amp;");
// line2 == "His &amp; hers &amp; theirs"
//! [8]


//! [9]
QString str = "One Eric another Eirik, and an Ericsson. "
              "How many Eiriks, Eric?";
QRegExp rx("\\b(Eric|Eirik)\\b"); // match Eric or Eirik
int pos = 0;    // where we are in the string
int count = 0;  // how many Eric and Eirik's we've counted
while (pos >= 0) {
    pos = rx.indexIn(str, pos);
    if (pos >= 0) {
        ++pos;      // move along in str
        ++count;    // count our Eric or Eirik
    }
}
//! [9]


//! [10]
str = "Digia Plc\tqt.digia.com\tFinland";
QString company, web, country;
rx.setPattern("^([^\t]+)\t([^\t]+)\t([^\t]+)$");
if (rx.indexIn(str) != -1) {
    company = rx.cap(1);
    web = rx.cap(2);
    country = rx.cap(3);
}
//! [10]


//! [11]
QStringList field = str.split("\t");
//! [11]


//! [12]
QRegExp rx("*.html");
rx.setPatternSyntax(QRegExp::Wildcard);
rx.exactMatch("index.html");                // returns true
rx.exactMatch("default.htm");               // returns false
rx.exactMatch("readme.txt");                // returns false
//! [12]


//! [13]
QString str = "offsets: 1.23 .50 71.00 6.00";
QRegExp rx("\\d*\\.\\d+");    // primitive floating point matching
int count = 0;
int pos = 0;
while ((pos = rx.indexIn(str, pos)) != -1) {
    ++count;
    pos += rx.matchedLength();
}
// pos will be 9, 14, 18 and finally 24; count will end up as 4
//! [13]


//! [14]
QRegExp rx("(\\d+)(\\s*)(cm|inch(es)?)");
int pos = rx.indexIn("Length: 36 inches");
QStringList list = rx.capturedTexts();
// list is now ("36 inches", "36", " ", "inches", "es")
//! [14]


//! [15]
QRegExp rx("(\\d+)(?:\\s*)(cm|inch(?:es)?)");
int pos = rx.indexIn("Length: 36 inches");
QStringList list = rx.capturedTexts();
// list is now ("36 inches", "36", "inches")
//! [15]


//! [16]
QStringList list = rx.capturedTexts();
QStringList::iterator it = list.begin();
while (it != list.end()) {
    myProcessing(*it);
    ++it;
}
//! [16]


//! [17]
QRegExp rxlen("(\\d+)(?:\\s*)(cm|inch)");
int pos = rxlen.indexIn("Length: 189cm");
if (pos > -1) {
    QString value = rxlen.cap(1); // "189"
    QString unit = rxlen.cap(2);  // "cm"
    // ...
}
//! [17]


//! [18]
QRegExp rx("/([a-z]+)/([a-z]+)");
rx.indexIn("Output /dev/null");   // returns 7 (position of /dev/null)
rx.pos(0);                        // returns 7 (position of /dev/null)
rx.pos(1);                        // returns 8 (position of dev)
rx.pos(2);                        // returns 12 (position of null)
//! [18]


//! [19]
s1 = QRegExp::escape("bingo");   // s1 == "bingo"
s2 = QRegExp::escape("f(x)");    // s2 == "f\\(x\\)"
//! [19]


//! [20]
QRegExp rx("(" + QRegExp::escape(name) +
           "|" + QRegExp::escape(alias) + ")");
//! [20]
