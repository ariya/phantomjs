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
bool function(const T &t);
//! [0]


//! [1]
bool allLowerCase(const QString &string)
{
    return string.lowered() == string;
}

QStringList strings = ...;
QFuture<QString> lowerCaseStrings = QtConcurrent::filtered(strings, allLowerCase);
//! [1]


//! [2]
QStringList strings = ...;
QFuture<void> future = QtConcurrent::filter(strings, allLowerCase);
//! [2]


//! [3]
V function(T &result, const U &intermediate)
//! [3]


//! [4]
void addToDictionary(QSet<QString> &dictionary, const QString &string)
{
    dictionary.insert(string);
}

QStringList strings = ...;
QFuture<QSet<QString> > dictionary = QtConcurrent::filteredReduced(strings, allLowerCase, addToDictionary);
//! [4]


//! [5]
QStringList strings = ...;
QFuture<QString> lowerCaseStrings = QtConcurrent::filtered(strings.constBegin(), strings.constEnd(), allLowerCase);

// filter in-place only works on non-const iterators
QFuture<void> future = QtConcurrent::filter(strings.begin(), strings.end(), allLowerCase);

QFuture<QSet<QString> > dictionary = QtConcurrent::filteredReduced(strings.constBegin(), strings.constEnd(), allLowerCase, addToDictionary);
//! [5]


//! [6]
QStringList strings = ...;

// each call blocks until the entire operation is finished
QStringList lowerCaseStrings = QtConcurrent::blockingFiltered(strings, allLowerCase);


QtConcurrent::blockingFilter(strings, allLowerCase);

QSet<QString> dictionary = QtConcurrent::blockingFilteredReduced(strings, allLowerCase, addToDictionary);
//! [6]


//! [7]
// keep only images with an alpha channel
QList<QImage> images = ...;
QFuture<void> alphaImages = QtConcurrent::filter(strings, &QImage::hasAlphaChannel);

// keep only gray scale images
QList<QImage> images = ...;
QFuture<QImage> grayscaleImages = QtConcurrent::filtered(images, &QImage::isGrayscale);

// create a set of all printable characters
QList<QChar> characters = ...;
QFuture<QSet<QChar> > set = QtConcurrent::filteredReduced(characters, &QChar::isPrint, &QSet<QChar>::insert);
//! [7]


//! [8]
// can mix normal functions and member functions with QtConcurrent::filteredReduced()

// create a dictionary of all lower cased strings
extern bool allLowerCase(const QString &string);
QStringList strings = ...;
QFuture<QSet<int> > averageWordLength = QtConcurrent::filteredReduced(strings, allLowerCase, QSet<QString>::insert);

// create a collage of all gray scale images
extern void addToCollage(QImage &collage, const QImage &grayscaleImage);
QList<QImage> images = ...;
QFuture<QImage> collage = QtConcurrent::filteredReduced(images, &QImage::isGrayscale, addToCollage);
//! [8]


//! [9]
bool QString::contains(const QRegExp &regexp) const;
//! [9]


//! [10]
std::bind(&QString::contains, QRegExp("^\\S+$")); // matches strings without whitespace
//! [10]


//! [11]
bool contains(const QString &string)
//! [11]


//! [12]
QStringList strings = ...;
std::bind(static_cast<bool(QString::*)(const QRegExp&)>( &QString::contains ), QRegExp("..." ));
//! [12]

//! [13]
struct StartsWith
{
    StartsWith(const QString &string)
    : m_string(string) { }

    typedef bool result_type;

    bool operator()(const QString &testString)
    {
        return testString.startsWith(m_string);
    }

    QString m_string;
};

QList<QString> strings = ...;
QFuture<QString> fooString = QtConcurrent::filtered(images, StartsWith(QLatin1String("Foo")));
//! [13]
