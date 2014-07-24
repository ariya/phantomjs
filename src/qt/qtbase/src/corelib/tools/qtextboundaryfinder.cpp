/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include <QtCore/qtextboundaryfinder.h>
#include <QtCore/qvarlengtharray.h>

#include <private/qunicodetools_p.h>

QT_BEGIN_NAMESPACE

class QTextBoundaryFinderPrivate
{
public:
    QCharAttributes attributes[1];
};

static void init(QTextBoundaryFinder::BoundaryType type, const QChar *chars, int length, QCharAttributes *attributes)
{
    const ushort *string = reinterpret_cast<const ushort *>(chars);

    QVarLengthArray<QUnicodeTools::ScriptItem> scriptItems;
    {
        QVarLengthArray<uchar> scripts(length);

        QUnicodeTools::initScripts(string, length, scripts.data());

        int start = 0;
        for (int i = start + 1; i <= length; ++i) {
            if (i == length || scripts[i] != scripts[start]) {
                QUnicodeTools::ScriptItem item;
                item.position = start;
                item.script = scripts[start];
                scriptItems.append(item);
                start = i;
            }
        }
    }

    QUnicodeTools::CharAttributeOptions options = 0;
    switch (type) {
    case QTextBoundaryFinder::Grapheme: options |= QUnicodeTools::GraphemeBreaks; break;
    case QTextBoundaryFinder::Word: options |= QUnicodeTools::WordBreaks; break;
    case QTextBoundaryFinder::Sentence: options |= QUnicodeTools::SentenceBreaks; break;
    case QTextBoundaryFinder::Line: options |= QUnicodeTools::LineBreaks; break;
    default: break;
    }
    QUnicodeTools::initCharAttributes(string, length, scriptItems.data(), scriptItems.count(), attributes, options);
}

/*!
    \class QTextBoundaryFinder
    \inmodule QtCore

    \brief The QTextBoundaryFinder class provides a way of finding Unicode text boundaries in a string.

    \since 4.4
    \ingroup tools
    \ingroup shared
    \ingroup string-processing
    \reentrant

    QTextBoundaryFinder allows to find Unicode text boundaries in a
    string, accordingly to the Unicode text boundary specification (see
    \l{http://www.unicode.org/reports/tr14/}{Unicode Standard Annex #14} and
    \l{http://www.unicode.org/reports/tr29/}{Unicode Standard Annex #29}).

    QTextBoundaryFinder can operate on a QString in four possible
    modes depending on the value of \a BoundaryType.

    Units of Unicode characters that make up what the user thinks of
    as a character or basic unit of the language are here called
    Grapheme clusters. The two unicode characters 'A' + diaeresis do
    for example form one grapheme cluster as the user thinks of them
    as one character, yet it is in this case represented by two
    unicode code points
    (see \l{http://www.unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries}).

    Word boundaries are there to locate the start and end of what a
    language considers to be a word
    (see \l{http://www.unicode.org/reports/tr29/#Word_Boundaries}).

    Line break boundaries give possible places where a line break
    might happen and sentence boundaries will show the beginning and
    end of whole sentences
    (see \l{http://www.unicode.org/reports/tr29/#Sentence_Boundaries} and
    \l{http://www.unicode.org/reports/tr14/}).

    The first position in a string is always a valid boundary and
    refers to the position before the first character. The last
    position at the length of the string is also valid and refers
    to the position after the last character.
*/

/*!
    \enum QTextBoundaryFinder::BoundaryType

    \value Grapheme Finds a grapheme which is the smallest boundary. It
                    including letters, punctuation marks, numerals and more.
    \value Word Finds a word.
    \value Line Finds possible positions for breaking the text into multiple
    lines.
    \value Sentence Finds sentence boundaries. These include periods, question
    marks etc.
*/

/*!
  \enum QTextBoundaryFinder::BoundaryReason

  \value NotAtBoundary  The boundary finder is not at a boundary position.
  \value BreakOpportunity  The boundary finder is at a break opportunity position.
                           Such a break opportunity might also be an item boundary
                           (either StartOfItem, EndOfItem, or combination of both),
                           a mandatory line break, or a soft hyphen.
  \value StartOfItem  Since 5.0. The boundary finder is at the start of
                      a grapheme, a word, a sentence, or a line.
  \value EndOfItem  Since 5.0. The boundary finder is at the end of
                    a grapheme, a word, a sentence, or a line.
  \value MandatoryBreak  Since 5.0. The boundary finder is at the end of line
                         (can occur for a Line boundary type only).
  \value SoftHyphen  The boundary finder is at the soft hyphen
                     (can occur for a Line boundary type only).
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
    , d(0)
{
    if (other.d) {
        Q_ASSERT(length > 0);
        d = (QTextBoundaryFinderPrivate *) malloc((length + 1) * sizeof(QCharAttributes));
        Q_CHECK_PTR(d);
        memcpy(d, other.d, (length + 1) * sizeof(QCharAttributes));
    }
}

/*!
  Assigns the object, \a other, to another QTextBoundaryFinder object.
*/
QTextBoundaryFinder &QTextBoundaryFinder::operator=(const QTextBoundaryFinder &other)
{
    if (&other == this)
        return *this;

    if (other.d) {
        Q_ASSERT(other.length > 0);
        uint newCapacity = (other.length + 1) * sizeof(QCharAttributes);
        QTextBoundaryFinderPrivate *newD = (QTextBoundaryFinderPrivate *) realloc(freePrivate ? d : 0, newCapacity);
        Q_CHECK_PTR(newD);
        freePrivate = true;
        d = newD;
    }

    t = other.t;
    s = other.s;
    chars = other.chars;
    length = other.length;
    pos = other.pos;

    if (other.d) {
        memcpy(d, other.d, (length + 1) * sizeof(QCharAttributes));
    } else {
        if (freePrivate)
            free(d);
        d = 0;
    }

    return *this;
}

/*!
  Destructs the QTextBoundaryFinder object.
*/
QTextBoundaryFinder::~QTextBoundaryFinder()
{
    Q_UNUSED(unused);
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
    , d(0)
{
    if (length > 0) {
        d = (QTextBoundaryFinderPrivate *) malloc((length + 1) * sizeof(QCharAttributes));
        Q_CHECK_PTR(d);
        init(t, chars, length, d->attributes);
    }
}

/*!
  Creates a QTextBoundaryFinder object of \a type operating on \a chars
  with \a length.

  \a buffer is an optional working buffer of size \a bufferSize you can pass to
  the QTextBoundaryFinder. If the buffer is large enough to hold the working
  data required (bufferSize >= length + 1), it will use this
  instead of allocating its own buffer.

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
    , freePrivate(true)
    , d(0)
{
    if (!chars) {
        length = 0;
    } else if (length > 0) {
        if (buffer && (uint)bufferSize >= (length + 1) * sizeof(QCharAttributes)) {
            d = (QTextBoundaryFinderPrivate *)buffer;
            freePrivate = false;
        } else {
            d = (QTextBoundaryFinderPrivate *) malloc((length + 1) * sizeof(QCharAttributes));
            Q_CHECK_PTR(d);
        }
        init(t, chars, length, d->attributes);
    }
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

   Returns \c true if the text boundary finder is valid; otherwise returns \c false.
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
    if (!d || pos < 0 || pos >= length) {
        pos = -1;
        return pos;
    }

    ++pos;
    switch(t) {
    case Grapheme:
        while (pos < length && !d->attributes[pos].graphemeBoundary)
            ++pos;
        break;
    case Word:
        while (pos < length && !d->attributes[pos].wordBreak)
            ++pos;
        break;
    case Sentence:
        while (pos < length && !d->attributes[pos].sentenceBoundary)
            ++pos;
        break;
    case Line:
        while (pos < length && !d->attributes[pos].lineBreak)
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
    if (!d || pos <= 0 || pos > length) {
        pos = -1;
        return pos;
    }

    --pos;
    switch(t) {
    case Grapheme:
        while (pos > 0 && !d->attributes[pos].graphemeBoundary)
            --pos;
        break;
    case Word:
        while (pos > 0 && !d->attributes[pos].wordBreak)
            --pos;
        break;
    case Sentence:
        while (pos > 0 && !d->attributes[pos].sentenceBoundary)
            --pos;
        break;
    case Line:
        while (pos > 0 && !d->attributes[pos].lineBreak)
            --pos;
        break;
    }

    return pos;
}

/*!
  Returns \c true if the object's position() is currently at a valid text boundary.
*/
bool QTextBoundaryFinder::isAtBoundary() const
{
    if (!d || pos < 0 || pos > length)
        return false;

    switch(t) {
    case Grapheme:
        return d->attributes[pos].graphemeBoundary;
    case Word:
        return d->attributes[pos].wordBreak;
    case Sentence:
        return d->attributes[pos].sentenceBoundary;
    case Line:
        // ### TR#14 LB2 prohibits break at sot
        return d->attributes[pos].lineBreak || pos == 0;
    }
    return false;
}

/*!
  Returns the reasons for the boundary finder to have chosen the current position as a boundary.
*/
QTextBoundaryFinder::BoundaryReasons QTextBoundaryFinder::boundaryReasons() const
{
    BoundaryReasons reasons = NotAtBoundary;
    if (!d || pos < 0 || pos > length)
        return reasons;

    const QCharAttributes attr = d->attributes[pos];
    switch (t) {
    case Grapheme:
        if (attr.graphemeBoundary) {
            reasons |= BreakOpportunity | StartOfItem | EndOfItem;
            if (pos == 0)
                reasons &= (~EndOfItem);
            else if (pos == length)
                reasons &= (~StartOfItem);
        }
        break;
    case Word:
        if (attr.wordBreak) {
            reasons |= BreakOpportunity;
            if (attr.wordStart)
                reasons |= StartOfItem;
            if (attr.wordEnd)
                reasons |= EndOfItem;
        }
        break;
    case Sentence:
        if (attr.sentenceBoundary) {
            reasons |= BreakOpportunity | StartOfItem | EndOfItem;
            if (pos == 0)
                reasons &= (~EndOfItem);
            else if (pos == length)
                reasons &= (~StartOfItem);
        }
        break;
    case Line:
        // ### TR#14 LB2 prohibits break at sot
        if (attr.lineBreak || pos == 0) {
            reasons |= BreakOpportunity;
            if (attr.mandatoryBreak || pos == 0) {
                reasons |= MandatoryBreak | StartOfItem | EndOfItem;
                if (pos == 0)
                    reasons &= (~EndOfItem);
                else if (pos == length)
                    reasons &= (~StartOfItem);
            } else if (pos > 0 && chars[pos - 1].unicode() == QChar::SoftHyphen) {
                reasons |= SoftHyphen;
            }
        }
        break;
    default:
        break;
    }

    return reasons;
}

QT_END_NAMESPACE
