/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtCore/qtextboundaryfinder.h>
#include <QtCore/qvarlengtharray.h>
#include <private/qunicodetables_p.h>
#include <qdebug.h>
#include "private/qharfbuzz_p.h"

QT_BEGIN_NAMESPACE

class QTextBoundaryFinderPrivate
{
public:
    HB_CharAttributes attributes[1];
};

static void init(QTextBoundaryFinder::BoundaryType type, const QChar *chars, int length, HB_CharAttributes *attributes)
{
    QVarLengthArray<HB_ScriptItem> scriptItems;

    const ushort *string = reinterpret_cast<const ushort *>(chars);
    const ushort *unicode = string;
    // correctly assign script, isTab and isObject to the script analysis
    const ushort *uc = unicode;
    const ushort *e = uc + length;
    int script = QUnicodeTables::Common;
    int lastScript = QUnicodeTables::Common;
    const ushort *start = uc;
    while (uc < e) {
        int s = QUnicodeTables::script(*uc);
        if (s != QUnicodeTables::Inherited)
            script = s;
        if (*uc == QChar::ObjectReplacementCharacter || *uc == QChar::LineSeparator || *uc == 9) 
            script = QUnicodeTables::Common;
        if (script != lastScript) {
            if (uc != start) {
                HB_ScriptItem item;
                item.pos = start - string;
                item.length = uc - start;
                item.script = (HB_Script)lastScript;
                item.bidiLevel = 0; // ### what's the proper value?
                scriptItems.append(item);
                start = uc;
            }
            lastScript = script;
        }
        ++uc;
    }
    if (uc != start) {
        HB_ScriptItem item;
        item.pos = start - string;
        item.length = uc - start;
        item.script = (HB_Script)lastScript;
        item.bidiLevel = 0; // ### what's the proper value?
        scriptItems.append(item);
    }

    qGetCharAttributes(string, length, scriptItems.data(), scriptItems.count(), attributes);
    if (type == QTextBoundaryFinder::Word)
        HB_GetWordBoundaries(string, length, scriptItems.data(), scriptItems.count(), attributes);
    else if (type == QTextBoundaryFinder::Sentence)
        HB_GetSentenceBoundaries(string, length, scriptItems.data(), scriptItems.count(), attributes);
}

/*! 
    \class QTextBoundaryFinder

    \brief The QTextBoundaryFinder class provides a way of finding Unicode text boundaries in a string.

    \since 4.4
    \ingroup tools
    \ingroup shared
    \ingroup string-processing
    \reentrant

    QTextBoundaryFinder allows to find Unicode text boundaries in a
    string, similar to the Unicode text boundary specification (see
    http://www.unicode.org/reports/tr29/tr29-11.html).

    QTextBoundaryFinder can operate on a QString in four possible
    modes depending on the value of \a BoundaryType.

    Units of Unicode characters that make up what the user thinks of
    as a character or basic unit of the language are here called
    Grapheme clusters. The two unicode characters 'A' + diaeresis do
    for example form one grapheme cluster as the user thinks of them
    as one character, yet it is in this case represented by two
    unicode code points.

    Word boundaries are there to locate the start and end of what a
    language considers to be a word.

    Line break boundaries give possible places where a line break
    might happen and sentence boundaries will show the beginning and
    end of whole sentences.

    The first position in a string is always a valid boundary and
    refers to the position before the first character. The last
    position at the length of the string is also valid and refers
    to the position after the last character.
*/

/*!
    \enum QTextBoundaryFinder::BoundaryType

    \value Grapheme Finds a grapheme which is the smallest boundary. It
    including letters, punctation marks, numerals and more.
    \value Word Finds a word.
    \value Line Finds possible positions for breaking the text into multiple
    lines.
    \value Sentence Finds sentence boundaries. These include periods, question
    marks etc.
*/

/*!
  \enum QTextBoundaryFinder::BoundaryReason

  \value NotAtBoundary  The boundary finder is not at a boundary position.
  \value StartWord  The boundary finder is at the start of a word.
  \value EndWord  The boundary finder is at the end of a word.
*/

/*!
  Constructs an invalid QTextBoundaryFinder object.
*/
QTextBoundaryFinder::QTextBoundaryFinder()
    : t(Grapheme)
    , chars(0)
    , length(0)
    , freePrivate(true)
    , d(0)
{
}

/*!
  Copies the QTextBoundaryFinder object, \a other.
*/
QTextBoundaryFinder::QTextBoundaryFinder(const QTextBoundaryFinder &other)
    : t(other.t)
    , s(other.s)
    , chars(other.chars)
    , length(other.length)
    , pos(other.pos)
    , freePrivate(true)
{
    d = (QTextBoundaryFinderPrivate *) malloc(length*sizeof(HB_CharAttributes));
    Q_CHECK_PTR(d);
    memcpy(d, other.d, length*sizeof(HB_CharAttributes));
}

/*!
  Assigns the object, \a other, to another QTextBoundaryFinder object.
*/
QTextBoundaryFinder &QTextBoundaryFinder::operator=(const QTextBoundaryFinder &other)
{
    if (&other == this)
        return *this;

    t = other.t;
    s = other.s;
    chars = other.chars;
    length = other.length;
    pos = other.pos;

    QTextBoundaryFinderPrivate *newD = (QTextBoundaryFinderPrivate *)
        realloc(freePrivate ? d : 0, length*sizeof(HB_CharAttributes));
    Q_CHECK_PTR(newD);
    freePrivate = true;
    d = newD;
    memcpy(d, other.d, length*sizeof(HB_CharAttributes));

    return *this;
}

/*!
  Destructs the QTextBoundaryFinder object.
*/
QTextBoundaryFinder::~QTextBoundaryFinder()
{
    if (freePrivate)
        free(d);
}

/*!
  Creates a QTextBoundaryFinder object of \a type operating on \a string.
*/
QTextBoundaryFinder::QTextBoundaryFinder(BoundaryType type, const QString &string)
    : t(type)
    , s(string)
    , chars(string.unicode())
    , length(string.length())
    , pos(0)
    , freePrivate(true)
{
    d = (QTextBoundaryFinderPrivate *) malloc(length*sizeof(HB_CharAttributes));
    Q_CHECK_PTR(d);
    init(t, chars, length, d->attributes);
}

/*!
  Creates a QTextBoundaryFinder object of \a type operating on \a chars
  with \a length.

  \a buffer is an optional working buffer of size \a bufferSize you can pass to
  the QTextBoundaryFinder. If the buffer is large enough to hold the working
  data required, it will use this instead of allocating its own buffer.

  \warning QTextBoundaryFinder does not create a copy of \a chars. It is the
  application programmer's responsibility to ensure the array is allocated for
  as long as the QTextBoundaryFinder object stays alive. The same applies to
  \a buffer.
*/
QTextBoundaryFinder::QTextBoundaryFinder(BoundaryType type, const QChar *chars, int length, unsigned char *buffer, int bufferSize)
    : t(type)
    , chars(chars)
    , length(length)
    , pos(0)
{
    if (buffer && (uint)bufferSize >= length*sizeof(HB_CharAttributes)) {
        d = (QTextBoundaryFinderPrivate *)buffer;
        freePrivate = false;
    } else {
        d = (QTextBoundaryFinderPrivate *) malloc(length*sizeof(HB_CharAttributes));
        Q_CHECK_PTR(d);
        freePrivate = true;
    }
    init(t, chars, length, d->attributes);
}

/*!
  Moves the finder to the start of the string. This is equivalent to setPosition(0).

  \sa setPosition(), position()
*/
void QTextBoundaryFinder::toStart()
{
    pos = 0;
}

/*!
  Moves the finder to the end of the string. This is equivalent to setPosition(string.length()).

  \sa setPosition(), position()
*/
void QTextBoundaryFinder::toEnd()
{
    pos = length;
}

/*!
  Returns the current position of the QTextBoundaryFinder.

  The range is from 0 (the beginning of the string) to the length of
  the string inclusive.

  \sa setPosition()
*/
int QTextBoundaryFinder::position() const
{
    return pos;
}

/*!
  Sets the current position of the QTextBoundaryFinder to \a position.

  If \a position is out of bounds, it will be bound to only valid
  positions. In this case, valid positions are from 0 to the length of
  the string inclusive.

  \sa position()
*/
void QTextBoundaryFinder::setPosition(int position)
{
    pos = qBound(0, position, length);
}

/*! \fn QTextBoundaryFinder::BoundaryType QTextBoundaryFinder::type() const

  Returns the type of the QTextBoundaryFinder.
*/

/*! \fn bool QTextBoundaryFinder::isValid() const

   Returns true if the text boundary finder is valid; otherwise returns false.
   A default QTextBoundaryFinder is invalid.
*/

/*!
  Returns the string  the QTextBoundaryFinder object operates on.
*/
QString QTextBoundaryFinder::string() const
{
    if (chars == s.unicode() && length == s.length())
        return s;
    return QString(chars, length);
}


/*!
  Moves the QTextBoundaryFinder to the next boundary position and returns that position.

  Returns -1 if there is no next boundary.
*/
int QTextBoundaryFinder::toNextBoundary()
{
    if (!d) {
        pos = -1;
        return pos;
    }

    if (pos < 0 || pos >= length) {
        pos = -1;
        return pos;
    }
    ++pos;
    if (pos == length)
        return pos;
    
    switch(t) {
    case Grapheme:
        while (pos < length && !d->attributes[pos].charStop)
            ++pos;
        break;
    case Word:
        while (pos < length && !d->attributes[pos].wordBoundary)
            ++pos;
        break;
    case Sentence:
        while (pos < length && !d->attributes[pos].sentenceBoundary)
            ++pos;
        break;
    case Line:
        Q_ASSERT(pos);
        while (pos < length && d->attributes[pos-1].lineBreakType < HB_Break)
            ++pos;
        break;
    }

    return pos;
}

/*!
  Moves the QTextBoundaryFinder to the previous boundary position and returns that position.

  Returns -1 if there is no previous boundary.
*/
int QTextBoundaryFinder::toPreviousBoundary()
{
    if (!d) {
        pos = -1;
        return pos;
    }

    if (pos <= 0 || pos > length) {
        pos = -1;
        return pos;
    }
    --pos;
    if (pos == 0)
        return pos;

    switch(t) {
    case Grapheme:
        while (pos > 0 && !d->attributes[pos].charStop)
            --pos;
        break;
    case Word:
        while (pos > 0 && !d->attributes[pos].wordBoundary)
            --pos;
        break;
    case Sentence:
        while (pos > 0 && !d->attributes[pos].sentenceBoundary)
            --pos;
        break;
    case Line:
        while (pos > 0 && d->attributes[pos-1].lineBreakType < HB_Break)
            --pos;
        break;
    }

    return pos;
}

/*!
  Returns true if the object's position() is currently at a valid text boundary.
*/
bool QTextBoundaryFinder::isAtBoundary() const
{
    if (!d || pos < 0)
        return false;

    if (pos == length)
        return true;

    switch(t) {
    case Grapheme:
        return d->attributes[pos].charStop;
    case Word:
        return d->attributes[pos].wordBoundary;
    case Line:
        return (pos > 0) ? d->attributes[pos-1].lineBreakType >= HB_Break : true;
    case Sentence:
        return d->attributes[pos].sentenceBoundary;
    }
    return false;
}

/*!
  Returns the reasons for the boundary finder to have chosen the current position as a boundary.
*/
QTextBoundaryFinder::BoundaryReasons QTextBoundaryFinder::boundaryReasons() const
{
    if (!d)
        return NotAtBoundary;
    if (! isAtBoundary())
        return NotAtBoundary;
    if (pos == 0) {
        if (d->attributes[pos].whiteSpace)
            return NotAtBoundary;
        return StartWord;
    }
    if (pos == length) {
        if (d->attributes[length-1].whiteSpace)
            return NotAtBoundary;
        return EndWord;
    }

    const bool nextIsSpace = d->attributes[pos].whiteSpace;
    const bool prevIsSpace = d->attributes[pos - 1].whiteSpace;

    if (prevIsSpace && !nextIsSpace)
        return StartWord;
    else if (!prevIsSpace && nextIsSpace)
        return EndWord;
    else if (!prevIsSpace && !nextIsSpace)
        return BoundaryReasons(StartWord | EndWord);
    else
        return NotAtBoundary;
}

QT_END_NAMESPACE
