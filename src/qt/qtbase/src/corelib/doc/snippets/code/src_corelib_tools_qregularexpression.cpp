/****************************************************************************
**
** Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
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

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

int main() {

{
//! [0]
QRegularExpression re("a pattern");
//! [0]
}

{
//! [1]
QRegularExpression re;
re.setPattern("another pattern");
//! [1]
}

{
//! [2]
// matches two digits followed by a space and a word
QRegularExpression re("\\d\\d \\w+");

// matches a backslash
QRegularExpression re2("\\\\");
//! [2]
}

{
//! [3]
QRegularExpression re("a third pattern");
QString pattern = re.pattern(); // pattern == "a third pattern"
//! [3]
}

{
//! [4]
// matches "Qt rocks", but also "QT rocks", "QT ROCKS", "qT rOcKs", etc.
QRegularExpression re("Qt rocks", QRegularExpression::CaseInsensitiveOption);
//! [4]
}

{
//! [5]
QRegularExpression re("^\\d+$");
re.setPatternOptions(QRegularExpression::MultilineOption);
// re matches any line in the subject string that contains only digits (but at least one)
//! [5]
}

{
//! [6]
QRegularExpression re = QRegularExpression("^two.*words$", QRegularExpression::MultilineOption
                                                           | QRegularExpression::DotMatchesEverythingOption);

QRegularExpression::PatternOptions options = re.patternOptions();
// options == QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption
//! [6]
}

{
//! [7]
// match two digits followed by a space and a word
QRegularExpression re("\\d\\d \\w+");
QRegularExpressionMatch match = re.match("abc123 def");
bool hasMatch = match.hasMatch(); // true
//! [7]
}

{
//! [8]
QRegularExpression re("\\d\\d \\w+");
QRegularExpressionMatch match = re.match("abc123 def");
if (match.hasMatch()) {
    QString matched = match.captured(0); // matched == "23 def"
    // ...
}
//! [8]
}

{
//! [9]
QRegularExpression re("\\d\\d \\w+");
QRegularExpressionMatch match = re.match("12 abc 45 def", 1);
if (match.hasMatch()) {
    QString matched = match.captured(0); // matched == "45 def"
    // ...
}
//! [9]
}

{
//! [10]
QRegularExpression re("^(\\d\\d)/(\\d\\d)/(\\d\\d\\d\\d)$");
QRegularExpressionMatch match = re.match("08/12/1985");
if (match.hasMatch()) {
    QString day = match.captured(1); // day == "08"
    QString month = match.captured(2); // month == "12"
    QString year = match.captured(3); // year == "1985"
    // ...
}
//! [10]
}

{
//! [11]
QRegularExpression re("abc(\\d+)def");
QRegularExpressionMatch match = re.match("XYZabc123defXYZ");
if (match.hasMatch()) {
    int startOffset = match.capturedStart(1); // startOffset == 6
    int endOffset = match.capturedEnd(1); // endOffset == 9
    // ...
}
//! [11]
}

{
//! [12]
QRegularExpression re("^(?<date>\\d\\d)/(?<month>\\d\\d)/(?<year>\\d\\d\\d\\d)$");
QRegularExpressionMatch match = re.match("08/12/1985");
if (match.hasMatch()) {
    QString date = match.captured("date"); // date == "08"
    QString month = match.captured("month"); // month == "12"
    QString year = match.captured("year"); // year == 1985
}
//! [12]
}

{
//! [13]
QRegularExpression re("(\\w+)");
QRegularExpressionMatchIterator i = re.globalMatch("the quick fox");
//! [13]

//! [14]
QStringList words;
while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString word = match.captured(1);
    words << word;
}
// words contains "the", "quick", "fox"
//! [14]
}

{
//! [15]
QString pattern("^(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) \\d\\d?, \\d\\d\\d\\d$");
QRegularExpression re(pattern);

QString input("Jan 21,");
QRegularExpressionMatch match = re.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
bool hasMatch = match.hasMatch(); // false
bool hasPartialMatch = match.hasPartialMatch(); // true
//! [15]
}

{
QString pattern("^(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) \\d\\d?, \\d\\d\\d\\d$");
QRegularExpression re(pattern);
//! [16]
QString input("Dec 8, 1985");
QRegularExpressionMatch match = re.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
bool hasMatch = match.hasMatch(); // true
bool hasPartialMatch = match.hasPartialMatch(); // false
//! [16]
}

{
//! [17]
QRegularExpression re("abc\\w+X|def");
QRegularExpressionMatch match = re.match("abcdef", 0, QRegularExpression::PartialPreferCompleteMatch);
bool hasMatch = match.hasMatch(); // true
bool hasPartialMatch = match.hasPartialMatch(); // false
QString captured = match.captured(0); // captured == "def"
//! [17]
}

{
//! [18]
QRegularExpression re("abc\\w+X|defY");
QRegularExpressionMatch match = re.match("abcdef", 0, QRegularExpression::PartialPreferCompleteMatch);
bool hasMatch = match.hasMatch(); // false
bool hasPartialMatch = match.hasPartialMatch(); // true
QString captured = match.captured(0); // captured == "abcdef"
//! [18]
}

{
//! [19]
QRegularExpression re("abc|ab");
QRegularExpressionMatch match = re.match("ab", 0, QRegularExpression::PartialPreferFirstMatch);
bool hasMatch = match.hasMatch(); // false
bool hasPartialMatch = match.hasPartialMatch(); // true
//! [19]
}

{
//! [20]
QRegularExpression re("abc(def)?");
QRegularExpressionMatch match = re.match("abc", 0, QRegularExpression::PartialPreferFirstMatch);
bool hasMatch = match.hasMatch(); // false
bool hasPartialMatch = match.hasPartialMatch(); // true
//! [20]
}

{
//! [21]
QRegularExpression re("(abc)*");
QRegularExpressionMatch match = re.match("abc", 0, QRegularExpression::PartialPreferFirstMatch);
bool hasMatch = match.hasMatch(); // false
bool hasPartialMatch = match.hasPartialMatch(); // true
//! [21]
}

{
//! [22]
QRegularExpression invalidRe("(unmatched|parenthesis");
bool isValid = invalidRe.isValid(); // false
//! [22]
}

{
//! [23]
QRegularExpression invalidRe("(unmatched|parenthesis");
if (!invalidRe.isValid()) {
    QString errorString = invalidRe.errorString(); // errorString == "missing )"
    int errorOffset = invalidRe.patternErrorOffset(); // errorOffset == 22
    // ...
}
//! [23]
}

{
//! [24]
QRegularExpression re("^this pattern must match exactly$");
//! [24]
}

{
//! [25]
QString p("a .*|pattern");
QRegularExpression re("\\A(?:" + p + ")\\z"); // re matches exactly the pattern string p
//! [25]
}

{
//! [26]
QString escaped = QRegularExpression::escape("a(x) = f(x) + g(x)");
// escaped == "a\\(x\\)\\ \\=\\ f\\(x\\)\\ \\+\\ g\\(x\\)"
//! [26]
}

{
QString name;
QString nickname;
//! [27]
QString pattern = "(" + QRegularExpression::escape(name) +
                  "|" + QRegularExpression::escape(nickname) + ")";
QRegularExpression re(pattern);
//! [27]
}

{
QString string;
QRegularExpression re;
//! [28]
QRegularExpressionMatch match = re.match(string);
for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
    QString captured = match.captured(i);
    // ...
}
//! [28]
}

{
//! [29]
QRegularExpression re("(\\d\\d) (?<name>\\w+)");
QRegularExpressionMatch match = re.match("23 Jordan");
if (match.hasMatch()) {
    QString number = match.captured(1); // first == "23"
    QString name = match.captured("name"); // name == "Jordan"
}
//! [29]
}

{
//! [30]
// extracts the words
QRegularExpression re("(\\w+)");
QString subject("the quick fox");
QRegularExpressionMatchIterator i = re.globalMatch(subject);
while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    // ...
}
//! [30]
}

}
