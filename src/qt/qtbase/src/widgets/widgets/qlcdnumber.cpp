/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qlcdnumber.h"
#ifndef QT_NO_LCDNUMBER
#include "qbitarray.h"
#include "qpainter.h"
#include "private/qframe_p.h"

QT_BEGIN_NAMESPACE

class QLCDNumberPrivate : public QFramePrivate
{
    Q_DECLARE_PUBLIC(QLCDNumber)
public:
    void init();
    void internalSetString(const QString& s);
    void drawString(const QString& s, QPainter &, QBitArray * = 0, bool = true);
    //void drawString(const QString &, QPainter &, QBitArray * = 0) const;
    void drawDigit(const QPoint &, QPainter &, int, char, char = ' ');
    void drawSegment(const QPoint &, char, QPainter &, int, bool = false);

    int ndigits;
    double val;
    uint base : 2;
    uint smallPoint : 1;
    uint fill : 1;
    uint shadow : 1;
    QString digitStr;
    QBitArray points;
};

/*!
    \class QLCDNumber

    \brief The QLCDNumber widget displays a number with LCD-like digits.

    \ingroup basicwidgets
    \inmodule QtWidgets

    It can display a number in just about any size. It can display
    decimal, hexadecimal, octal or binary numbers. It is easy to
    connect to data sources using the display() slot, which is
    overloaded to take any of five argument types.

    There are also slots to change the base with setMode() and the
    decimal point with setSmallDecimalPoint().

    QLCDNumber emits the overflow() signal when it is asked to display
    something beyond its range. The range is set by setDigitCount(),
    but setSmallDecimalPoint() also influences it. If the display is
    set to hexadecimal, octal or binary, the integer equivalent of the
    value is displayed.

    These digits and other symbols can be shown: 0/O, 1, 2, 3, 4, 5/S,
    6, 7, 8, 9/g, minus, decimal point, A, B, C, D, E, F, h, H, L, o,
    P, r, u, U, Y, colon, degree sign (which is specified as single
    quote in the string) and space. QLCDNumber substitutes spaces for
    illegal characters.

    It is not possible to retrieve the contents of a QLCDNumber
    object, although you can retrieve the numeric value with value().
    If you really need the text, we recommend that you connect the
    signals that feed the display() slot to another slot as well and
    store the value there.

    Incidentally, QLCDNumber is the very oldest part of Qt, tracing
    its roots back to a BASIC program on the \l{Sinclair Spectrum}{Sinclair Spectrum}.

    \table
    \row \li
    \inlineimage windows-lcdnumber.png Screenshot of a Windows style LCD number widget
    \inlineimage windowsvista-lcdnumber.png Screenshot of a Windows Vista style LCD number widget
    \inlineimage macintosh-lcdnumber.png Screenshot of a Macintosh style LCD number widget
    \inlineimage fusion-lcdnumber.png Screenshot of a Fusion style LCD number widget
    \row \li LCD number widgets shown in various widget styles (from left to right):
    \l{Windows Style Widget Gallery}{Windows}, \l{Windows Vista Style Widget Gallery}{Windows Vista},
    \l{Macintosh Style Widget Gallery}{Macintosh}, \l{Fusion Style Widget Gallery}{Fusion}.
    \endtable

    \sa QLabel, QFrame, {Digital Clock Example}, {Tetrix Example}
*/

/*!
    \enum QLCDNumber::Mode

    This type determines how numbers are shown.

    \value Hex  Hexadecimal
    \value Dec  Decimal
    \value Oct  Octal
    \value Bin  Binary

    If the display is set to hexadecimal, octal or binary, the integer
    equivalent of the value is displayed.
*/

/*!
    \enum QLCDNumber::SegmentStyle

    This type determines the visual appearance of the QLCDNumber
    widget.

    \value Outline gives raised segments filled with the background color.
    \value Filled gives raised segments filled with the windowText color.
    \value Flat gives flat segments filled with the windowText color.
*/



/*!
    \fn void QLCDNumber::overflow()

    This signal is emitted whenever the QLCDNumber is asked to display
    a too-large number or a too-long string.

    It is never emitted by setDigitCount().
*/


static QString int2string(int num, int base, int ndigits, bool *oflow)
{
    QString s;
    bool negative;
    if (num < 0) {
        negative = true;
        num      = -num;
    } else {
        negative = false;
    }
    switch(base) {
        case QLCDNumber::Hex:
            s.sprintf("%*x", ndigits, num);
            break;
        case QLCDNumber::Dec:
            s.sprintf("%*i", ndigits, num);
            break;
        case QLCDNumber::Oct:
            s.sprintf("%*o", ndigits, num);
            break;
        case QLCDNumber::Bin:
            {
                char buf[42];
                char *p = &buf[41];
                uint n = num;
                int len = 0;
                *p = '\0';
                do {
                    *--p = (char)((n&1)+'0');
                    n >>= 1;
                    len++;
                } while (n != 0);
                len = ndigits - len;
                if (len > 0)
                s.fill(QLatin1Char(' '), len);
                s += QString::fromLatin1(p);
            }
            break;
    }
    if (negative) {
        for (int i=0; i<(int)s.length(); i++) {
            if (s[i] != QLatin1Char(' ')) {
                if (i != 0) {
                    s[i-1] = QLatin1Char('-');
                } else {
                    s.insert(0, QLatin1Char('-'));
                }
                break;
            }
        }
    }
    if (oflow)
        *oflow = (int)s.length() > ndigits;
    return s;
}


static QString double2string(double num, int base, int ndigits, bool *oflow)
{
    QString s;
    if (base != QLCDNumber::Dec) {
        bool of = num >= 2147483648.0 || num < -2147483648.0;
        if (of) {                             // oops, integer overflow
            if (oflow)
                *oflow = true;
            return s;
        }
        s = int2string((int)num, base, ndigits, 0);
    } else {                                    // decimal base
        int nd = ndigits;
        do {
            s.sprintf("%*.*g", ndigits, nd, num);
            int i = s.indexOf(QLatin1Char('e'));
            if (i > 0 && s[i+1]==QLatin1Char('+')) {
                s[i] = QLatin1Char(' ');
                s[i+1] = QLatin1Char('e');
            }
        } while (nd-- && (int)s.length() > ndigits);
    }
    if (oflow)
        *oflow = (int)s.length() > ndigits;
    return s;
}


static const char *getSegments(char ch)               // gets list of segments for ch
{
    static const char segments[30][8] =
       { { 0, 1, 2, 4, 5, 6,99, 0},             // 0    0 / O
         { 2, 5,99, 0, 0, 0, 0, 0},             // 1    1
         { 0, 2, 3, 4, 6,99, 0, 0},             // 2    2
         { 0, 2, 3, 5, 6,99, 0, 0},             // 3    3
         { 1, 2, 3, 5,99, 0, 0, 0},             // 4    4
         { 0, 1, 3, 5, 6,99, 0, 0},             // 5    5 / S
         { 0, 1, 3, 4, 5, 6,99, 0},             // 6    6
         { 0, 2, 5,99, 0, 0, 0, 0},             // 7    7
         { 0, 1, 2, 3, 4, 5, 6,99},             // 8    8
         { 0, 1, 2, 3, 5, 6,99, 0},             // 9    9 / g
         { 3,99, 0, 0, 0, 0, 0, 0},             // 10   -
         { 7,99, 0, 0, 0, 0, 0, 0},             // 11   .
         { 0, 1, 2, 3, 4, 5,99, 0},             // 12   A
         { 1, 3, 4, 5, 6,99, 0, 0},             // 13   B
         { 0, 1, 4, 6,99, 0, 0, 0},             // 14   C
         { 2, 3, 4, 5, 6,99, 0, 0},             // 15   D
         { 0, 1, 3, 4, 6,99, 0, 0},             // 16   E
         { 0, 1, 3, 4,99, 0, 0, 0},             // 17   F
         { 1, 3, 4, 5,99, 0, 0, 0},             // 18   h
         { 1, 2, 3, 4, 5,99, 0, 0},             // 19   H
         { 1, 4, 6,99, 0, 0, 0, 0},             // 20   L
         { 3, 4, 5, 6,99, 0, 0, 0},             // 21   o
         { 0, 1, 2, 3, 4,99, 0, 0},             // 22   P
         { 3, 4,99, 0, 0, 0, 0, 0},             // 23   r
         { 4, 5, 6,99, 0, 0, 0, 0},             // 24   u
         { 1, 2, 4, 5, 6,99, 0, 0},             // 25   U
         { 1, 2, 3, 5, 6,99, 0, 0},             // 26   Y
         { 8, 9,99, 0, 0, 0, 0, 0},             // 27   :
         { 0, 1, 2, 3,99, 0, 0, 0},             // 28   '
         {99, 0, 0, 0, 0, 0, 0, 0} };           // 29   empty

    if (ch >= '0' && ch <= '9')
        return segments[ch - '0'];
    if (ch >= 'A' && ch <= 'F')
        return segments[ch - 'A' + 12];
    if (ch >= 'a' && ch <= 'f')
        return segments[ch - 'a' + 12];

    int n;
    switch (ch) {
        case '-':
            n = 10;  break;
        case 'O':
            n = 0;   break;
        case 'g':
            n = 9;   break;
        case '.':
            n = 11;  break;
        case 'h':
            n = 18;  break;
        case 'H':
            n = 19;  break;
        case 'l':
        case 'L':
            n = 20;  break;
        case 'o':
            n = 21;  break;
        case 'p':
        case 'P':
            n = 22;  break;
        case 'r':
        case 'R':
            n = 23;  break;
        case 's':
        case 'S':
            n = 5;   break;
        case 'u':
            n = 24;  break;
        case 'U':
            n = 25;  break;
        case 'y':
        case 'Y':
            n = 26;  break;
        case ':':
            n = 27;  break;
        case '\'':
            n = 28;  break;
        default:
            n = 29;  break;
    }
    return segments[n];
}



/*!
    Constructs an LCD number, sets the number of digits to 5, the base
    to decimal, the decimal point mode to 'small' and the frame style
    to a raised box. The segmentStyle() is set to \c Outline.

    The \a parent argument is passed to the QFrame constructor.

    \sa setDigitCount(), setSmallDecimalPoint()
*/

QLCDNumber::QLCDNumber(QWidget *parent)
        : QFrame(*new QLCDNumberPrivate, parent)
{
    Q_D(QLCDNumber);
    d->ndigits = 5;
    d->init();
}


/*!
    Constructs an LCD number, sets the number of digits to \a
    numDigits, the base to decimal, the decimal point mode to 'small'
    and the frame style to a raised box. The segmentStyle() is set to
    \c Filled.

    The \a parent argument is passed to the QFrame constructor.

    \sa setDigitCount(), setSmallDecimalPoint()
*/

QLCDNumber::QLCDNumber(uint numDigits, QWidget *parent)
        : QFrame(*new QLCDNumberPrivate, parent)
{
    Q_D(QLCDNumber);
    d->ndigits = numDigits;
    d->init();
}

void QLCDNumberPrivate::init()
{
    Q_Q(QLCDNumber);

    q->setFrameStyle(QFrame::Box | QFrame::Raised);
    val        = 0;
    base       = QLCDNumber::Dec;
    smallPoint = false;
    q->setDigitCount(ndigits);
    q->setSegmentStyle(QLCDNumber::Filled);
    q->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
}

/*!
    Destroys the LCD number.
*/

QLCDNumber::~QLCDNumber()
{
}


/*!
    \since 4.6
    \property QLCDNumber::digitCount
    \brief the current number of digits displayed

    Corresponds to the current number of digits. If \l
    QLCDNumber::smallDecimalPoint is false, the decimal point occupies
    one digit position.

    By default, this property contains a value of 5.

    \sa smallDecimalPoint
*/

/*!
  Sets the current number of digits to \a numDigits. Must
  be in the range 0..99.
 */
void QLCDNumber::setDigitCount(int numDigits)
{
    Q_D(QLCDNumber);
    if (numDigits > 99) {
        qWarning("QLCDNumber::setNumDigits: (%s) Max 99 digits allowed",
                 objectName().toLocal8Bit().constData());
        numDigits = 99;
    }
    if (numDigits < 0) {
        qWarning("QLCDNumber::setNumDigits: (%s) Min 0 digits allowed",
                 objectName().toLocal8Bit().constData());
        numDigits = 0;
    }
    if (d->digitStr.isNull()) {                  // from constructor
        d->ndigits = numDigits;
        d->digitStr.fill(QLatin1Char(' '), d->ndigits);
        d->points.fill(0, d->ndigits);
        d->digitStr[d->ndigits - 1] = QLatin1Char('0'); // "0" is the default number
    } else {
        bool doDisplay = d->ndigits == 0;
        if (numDigits == d->ndigits)             // no change
            return;
        int i;
        int dif;
        if (numDigits > d->ndigits) {            // expand
            dif = numDigits - d->ndigits;
            QString buf;
            buf.fill(QLatin1Char(' '), dif);
            d->digitStr.insert(0, buf);
            d->points.resize(numDigits);
            for (i=numDigits-1; i>=dif; i--)
                d->points.setBit(i, d->points.testBit(i-dif));
            for (i=0; i<dif; i++)
                d->points.clearBit(i);
        } else {                                        // shrink
            dif = d->ndigits - numDigits;
            d->digitStr = d->digitStr.right(numDigits);
            QBitArray tmpPoints = d->points;
            d->points.resize(numDigits);
            for (i=0; i<(int)numDigits; i++)
                d->points.setBit(i, tmpPoints.testBit(i+dif));
        }
        d->ndigits = numDigits;
        if (doDisplay)
            display(value());
        update();
    }
}

/*!
  Returns the current number of digits.
 */
int QLCDNumber::digitCount() const
{
    Q_D(const QLCDNumber);
    return d->ndigits;
}

/*!
    \overload

    Returns \c true if \a num is too big to be displayed in its entirety;
    otherwise returns \c false.

    \sa display(), digitCount(), smallDecimalPoint()
*/

bool QLCDNumber::checkOverflow(int num) const
{
    Q_D(const QLCDNumber);
    bool of;
    int2string(num, d->base, d->ndigits, &of);
    return of;
}


/*!
    Returns \c true if \a num is too big to be displayed in its entirety;
    otherwise returns \c false.

    \sa display(), digitCount(), smallDecimalPoint()
*/

bool QLCDNumber::checkOverflow(double num) const
{
    Q_D(const QLCDNumber);
    bool of;
    double2string(num, d->base, d->ndigits, &of);
    return of;
}


/*!
    \property QLCDNumber::mode
    \brief the current display mode (number base)

    Corresponds to the current display mode, which is one of \c Bin,
    \c Oct, \c Dec (the default) and \c Hex. \c Dec mode can display
    floating point values, the other modes display the integer
    equivalent.

    \sa smallDecimalPoint(), setHexMode(), setDecMode(), setOctMode(), setBinMode()
*/

QLCDNumber::Mode QLCDNumber::mode() const
{
    Q_D(const QLCDNumber);
    return (QLCDNumber::Mode) d->base;
}

void QLCDNumber::setMode(Mode m)
{
    Q_D(QLCDNumber);
    d->base = m;
    display(d->val);
}


/*!
    \property QLCDNumber::value
    \brief the displayed value

    This property corresponds to the current value displayed by the
    LCDNumber.

    If the displayed value is not a number, the property has a value
    of 0.

    By default, this property contains a value of 0.
*/

double QLCDNumber::value() const
{
    Q_D(const QLCDNumber);
    return d->val;
}

/*!
    \overload

    Displays the number \a num.
*/
void QLCDNumber::display(double num)
{
    Q_D(QLCDNumber);
    d->val = num;
    bool of;
    QString s = double2string(d->val, d->base, d->ndigits, &of);
    if (of)
        emit overflow();
    else
        d->internalSetString(s);
}

/*!
    \property QLCDNumber::intValue
    \brief the displayed value rounded to the nearest integer

    This property corresponds to the nearest integer to the current
    value displayed by the LCDNumber. This is the value used for
    hexadecimal, octal and binary modes.

    If the displayed value is not a number, the property has a value
    of 0.

    By default, this property contains a value of 0.
*/
int QLCDNumber::intValue() const
{
    Q_D(const QLCDNumber);
    return qRound(d->val);
}


/*!
    \overload

    Displays the number \a num.
*/
void QLCDNumber::display(int num)
{
    Q_D(QLCDNumber);
    d->val = (double)num;
    bool of;
    QString s = int2string(num, d->base, d->ndigits, &of);
    if (of)
        emit overflow();
    else
        d->internalSetString(s);
}


/*!
    Displays the number represented by the string \a s.

    This version of the function disregards mode() and
    smallDecimalPoint().

    These digits and other symbols can be shown: 0/O, 1, 2, 3, 4, 5/S,
    6, 7, 8, 9/g, minus, decimal point, A, B, C, D, E, F, h, H, L, o,
    P, r, u, U, Y, colon, degree sign (which is specified as single
    quote in the string) and space. QLCDNumber substitutes spaces for
    illegal characters.
*/

void QLCDNumber::display(const QString &s)
{
    Q_D(QLCDNumber);
    d->val = 0;
    bool ok = false;
    double v = s.toDouble(&ok);
    if (ok)
        d->val = v;
    d->internalSetString(s);
}

/*!
    Calls setMode(Hex). Provided for convenience (e.g. for
    connecting buttons to it).

    \sa setMode(), setDecMode(), setOctMode(), setBinMode(), mode()
*/

void QLCDNumber::setHexMode()
{
    setMode(Hex);
}


/*!
    Calls setMode(Dec). Provided for convenience (e.g. for
    connecting buttons to it).

    \sa setMode(), setHexMode(), setOctMode(), setBinMode(), mode()
*/

void QLCDNumber::setDecMode()
{
    setMode(Dec);
}


/*!
    Calls setMode(Oct). Provided for convenience (e.g. for
    connecting buttons to it).

    \sa setMode(), setHexMode(), setDecMode(), setBinMode(), mode()
*/

void QLCDNumber::setOctMode()
{
    setMode(Oct);
}


/*!
    Calls setMode(Bin). Provided for convenience (e.g. for
    connecting buttons to it).

    \sa setMode(), setHexMode(), setDecMode(), setOctMode(), mode()
*/

void QLCDNumber::setBinMode()
{
    setMode(Bin);
}


/*!
    \property QLCDNumber::smallDecimalPoint
    \brief the style of the decimal point

    If true the decimal point is drawn between two digit positions.
    Otherwise it occupies a digit position of its own, i.e. is drawn
    in a digit position. The default is false.

    The inter-digit space is made slightly wider when the decimal
    point is drawn between the digits.

    \sa mode
*/

void QLCDNumber::setSmallDecimalPoint(bool b)
{
    Q_D(QLCDNumber);
    d->smallPoint = b;
    update();
}

bool QLCDNumber::smallDecimalPoint() const
{
    Q_D(const QLCDNumber);
    return d->smallPoint;
}



/*!\reimp
*/


void QLCDNumber::paintEvent(QPaintEvent *)
{
    Q_D(QLCDNumber);
    QPainter p(this);
    drawFrame(&p);
    p.setRenderHint(QPainter::Antialiasing);
    if (d->shadow)
        p.translate(0.5, 0.5);

    if (d->smallPoint)
        d->drawString(d->digitStr, p, &d->points, false);
    else
        d->drawString(d->digitStr, p, 0, false);
}


void QLCDNumberPrivate::internalSetString(const QString& s)
{
    Q_Q(QLCDNumber);
    QString buffer;
    int i;
    int len = s.length();
    QBitArray newPoints(ndigits);

    if (!smallPoint) {
        if (len == ndigits)
            buffer = s;
        else
            buffer = s.right(ndigits).rightJustified(ndigits, QLatin1Char(' '));
    } else {
        int  index = -1;
        bool lastWasPoint = true;
        newPoints.clearBit(0);
        for (i=0; i<len; i++) {
            if (s[i] == QLatin1Char('.')) {
                if (lastWasPoint) {           // point already set for digit?
                    if (index == ndigits - 1) // no more digits
                        break;
                    index++;
                    buffer[index] = QLatin1Char(' ');        // 2 points in a row, add space
                }
                newPoints.setBit(index);        // set decimal point
                lastWasPoint = true;
            } else {
                if (index == ndigits - 1)
                    break;
                index++;
                buffer[index] = s[i];
                newPoints.clearBit(index);      // decimal point default off
                lastWasPoint = false;
            }
        }
        if (index < ((int) ndigits) - 1) {
            for(i=index; i>=0; i--) {
                buffer[ndigits - 1 - index + i] = buffer[i];
                newPoints.setBit(ndigits - 1 - index + i,
                                   newPoints.testBit(i));
            }
            for(i=0; i<ndigits-index-1; i++) {
                buffer[i] = QLatin1Char(' ');
                newPoints.clearBit(i);
            }
        }
    }

    if (buffer == digitStr)
        return;

    digitStr = buffer;
    if (smallPoint)
        points = newPoints;
    q->update();
}

/*!
  \internal
*/

void QLCDNumberPrivate::drawString(const QString &s, QPainter &p,
                                   QBitArray *newPoints, bool newString)
{
    Q_Q(QLCDNumber);
    QPoint  pos;

    int digitSpace = smallPoint ? 2 : 1;
    int xSegLen    = q->width()*5/(ndigits*(5 + digitSpace) + digitSpace);
    int ySegLen    = q->height()*5/12;
    int segLen     = ySegLen > xSegLen ? xSegLen : ySegLen;
    int xAdvance   = segLen*(5 + digitSpace)/5;
    int xOffset    = (q->width() - ndigits*xAdvance + segLen/5)/2;
    int yOffset    = (q->height() - segLen*2)/2;

    for (int i=0;  i<ndigits; i++) {
        pos = QPoint(xOffset + xAdvance*i, yOffset);
        if (newString)
            drawDigit(pos, p, segLen, s[i].toLatin1(), digitStr[i].toLatin1());
        else
            drawDigit(pos, p, segLen, s[i].toLatin1());
        if (newPoints) {
            char newPoint = newPoints->testBit(i) ? '.' : ' ';
            if (newString) {
                char oldPoint = points.testBit(i) ? '.' : ' ';
                drawDigit(pos, p, segLen, newPoint, oldPoint);
            } else {
                drawDigit(pos, p, segLen, newPoint);
            }
        }
    }
    if (newString) {
        digitStr = s;
        digitStr.truncate(ndigits);
        if (newPoints)
            points = *newPoints;
    }
}


/*!
  \internal
*/

void QLCDNumberPrivate::drawDigit(const QPoint &pos, QPainter &p, int segLen,
                                  char newCh, char oldCh)
{
// Draws and/or erases segments to change display of a single digit
// from oldCh to newCh

    char updates[18][2];        // can hold 2 times number of segments, only
                                // first 9 used if segment table is correct
    int  nErases;
    int  nUpdates;
    const char *segs;
    int  i,j;

    const char erase      = 0;
    const char draw       = 1;
    const char leaveAlone = 2;

    segs = getSegments(oldCh);
    for (nErases=0; segs[nErases] != 99; nErases++) {
        updates[nErases][0] = erase;            // get segments to erase to
        updates[nErases][1] = segs[nErases];    // remove old char
    }
    nUpdates = nErases;
    segs = getSegments(newCh);
    for(i = 0 ; segs[i] != 99 ; i++) {
        for (j=0;  j<nErases; j++)
            if (segs[i] == updates[j][1]) {   // same segment ?
                updates[j][0] = leaveAlone;     // yes, already on screen
                break;
            }
        if (j == nErases) {                   // if not already on screen
            updates[nUpdates][0] = draw;
            updates[nUpdates][1] = segs[i];
            nUpdates++;
        }
    }
    for (i=0; i<nUpdates; i++) {
        if (updates[i][0] == draw)
            drawSegment(pos, updates[i][1], p, segLen);
        if (updates[i][0] == erase)
            drawSegment(pos, updates[i][1], p, segLen, true);
    }
}


static void addPoint(QPolygon &a, const QPoint &p)
{
    uint n = a.size();
    a.resize(n + 1);
    a.setPoint(n, p);
}

/*!
  \internal
*/

void QLCDNumberPrivate::drawSegment(const QPoint &pos, char segmentNo, QPainter &p,
                                    int segLen, bool erase)
{
    Q_Q(QLCDNumber);
    QPoint ppt;
    QPoint pt = pos;
    int width = segLen/5;

    const QPalette &pal = q->palette();
    QColor lightColor,darkColor,fgColor;
    if (erase){
        lightColor = pal.color(q->backgroundRole());
        darkColor  = lightColor;
        fgColor    = lightColor;
    } else {
        lightColor = pal.light().color();
        darkColor  = pal.dark().color();
        fgColor    = pal.color(q->foregroundRole());
    }


#define LINETO(X,Y) addPoint(a, QPoint(pt.x() + (X),pt.y() + (Y)))
#define LIGHT
#define DARK

    if (fill) {
        QPolygon a(0);
        //The following is an exact copy of the switch below.
        //don't make any changes here
        switch (segmentNo) {
        case 0 :
            ppt = pt;
            LIGHT;
            LINETO(segLen - 1,0);
            DARK;
            LINETO(segLen - width - 1,width);
            LINETO(width,width);
            LINETO(0,0);
            break;
        case 1 :
            pt += QPoint(0 , 1);
            ppt = pt;
            LIGHT;
            LINETO(width,width);
            DARK;
            LINETO(width,segLen - width/2 - 2);
            LINETO(0,segLen - 2);
            LIGHT;
            LINETO(0,0);
            break;
        case 2 :
            pt += QPoint(segLen - 1 , 1);
            ppt = pt;
            DARK;
            LINETO(0,segLen - 2);
            LINETO(-width,segLen - width/2 - 2);
            LIGHT;
            LINETO(-width,width);
            LINETO(0,0);
            break;
        case 3 :
            pt += QPoint(0 , segLen);
            ppt = pt;
            LIGHT;
            LINETO(width,-width/2);
            LINETO(segLen - width - 1,-width/2);
            LINETO(segLen - 1,0);
            DARK;
            if (width & 1) {            // adjust for integer division error
                LINETO(segLen - width - 3,width/2 + 1);
                LINETO(width + 2,width/2 + 1);
            } else {
                LINETO(segLen - width - 1,width/2);
                LINETO(width,width/2);
            }
            LINETO(0,0);
            break;
        case 4 :
            pt += QPoint(0 , segLen + 1);
            ppt = pt;
            LIGHT;
            LINETO(width,width/2);
            DARK;
            LINETO(width,segLen - width - 2);
            LINETO(0,segLen - 2);
            LIGHT;
            LINETO(0,0);
            break;
        case 5 :
            pt += QPoint(segLen - 1 , segLen + 1);
            ppt = pt;
            DARK;
            LINETO(0,segLen - 2);
            LINETO(-width,segLen - width - 2);
            LIGHT;
            LINETO(-width,width/2);
            LINETO(0,0);
            break;
        case 6 :
            pt += QPoint(0 , segLen*2);
            ppt = pt;
            LIGHT;
            LINETO(width,-width);
            LINETO(segLen - width - 1,-width);
            LINETO(segLen - 1,0);
            DARK;
            LINETO(0,0);
            break;
        case 7 :
            if (smallPoint)   // if smallpoint place'.' between other digits
                pt += QPoint(segLen + width/2 , segLen*2);
            else
                pt += QPoint(segLen/2 , segLen*2);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        case 8 :
            pt += QPoint(segLen/2 - width/2 + 1 , segLen/2 + width);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        case 9 :
            pt += QPoint(segLen/2 - width/2 + 1 , 3*segLen/2 + width);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        default :
            qWarning("QLCDNumber::drawSegment: (%s) Illegal segment id: %d\n",
                     q->objectName().toLocal8Bit().constData(), segmentNo);
        }
        // End exact copy
        p.setPen(Qt::NoPen);
        p.setBrush(fgColor);
        p.drawPolygon(a);
        p.setBrush(Qt::NoBrush);

        pt = pos;
    }
#undef LINETO
#undef LIGHT
#undef DARK

#define LINETO(X,Y) p.drawLine(ppt.x(), ppt.y(), pt.x()+(X), pt.y()+(Y)); \
                    ppt = QPoint(pt.x()+(X), pt.y()+(Y))
#define LIGHT p.setPen(lightColor)
#define DARK  p.setPen(darkColor)
    if (shadow)
        switch (segmentNo) {
        case 0 :
            ppt = pt;
            LIGHT;
            LINETO(segLen - 1,0);
            DARK;
            LINETO(segLen - width - 1,width);
            LINETO(width,width);
            LINETO(0,0);
            break;
        case 1 :
            pt += QPoint(0,1);
            ppt = pt;
            LIGHT;
            LINETO(width,width);
            DARK;
            LINETO(width,segLen - width/2 - 2);
            LINETO(0,segLen - 2);
            LIGHT;
            LINETO(0,0);
            break;
        case 2 :
            pt += QPoint(segLen - 1 , 1);
            ppt = pt;
            DARK;
            LINETO(0,segLen - 2);
            LINETO(-width,segLen - width/2 - 2);
            LIGHT;
            LINETO(-width,width);
            LINETO(0,0);
            break;
        case 3 :
            pt += QPoint(0 , segLen);
            ppt = pt;
            LIGHT;
            LINETO(width,-width/2);
            LINETO(segLen - width - 1,-width/2);
            LINETO(segLen - 1,0);
            DARK;
            if (width & 1) {            // adjust for integer division error
                LINETO(segLen - width - 3,width/2 + 1);
                LINETO(width + 2,width/2 + 1);
            } else {
                LINETO(segLen - width - 1,width/2);
                LINETO(width,width/2);
            }
            LINETO(0,0);
            break;
        case 4 :
            pt += QPoint(0 , segLen + 1);
            ppt = pt;
            LIGHT;
            LINETO(width,width/2);
            DARK;
            LINETO(width,segLen - width - 2);
            LINETO(0,segLen - 2);
            LIGHT;
            LINETO(0,0);
            break;
        case 5 :
            pt += QPoint(segLen - 1 , segLen + 1);
            ppt = pt;
            DARK;
            LINETO(0,segLen - 2);
            LINETO(-width,segLen - width - 2);
            LIGHT;
            LINETO(-width,width/2);
            LINETO(0,0);
            break;
        case 6 :
            pt += QPoint(0 , segLen*2);
            ppt = pt;
            LIGHT;
            LINETO(width,-width);
            LINETO(segLen - width - 1,-width);
            LINETO(segLen - 1,0);
            DARK;
            LINETO(0,0);
            break;
        case 7 :
            if (smallPoint)   // if smallpoint place'.' between other digits
                pt += QPoint(segLen + width/2 , segLen*2);
            else
                pt += QPoint(segLen/2 , segLen*2);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        case 8 :
            pt += QPoint(segLen/2 - width/2 + 1 , segLen/2 + width);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        case 9 :
            pt += QPoint(segLen/2 - width/2 + 1 , 3*segLen/2 + width);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        default :
            qWarning("QLCDNumber::drawSegment: (%s) Illegal segment id: %d\n",
                     q->objectName().toLocal8Bit().constData(), segmentNo);
        }

#undef LINETO
#undef LIGHT
#undef DARK
}



/*!
    \property QLCDNumber::segmentStyle
    \brief the style of the LCDNumber

    \table
    \header \li Style \li Result
    \row \li \c Outline
         \li Produces raised segments filled with the background color
    \row \li \c Filled
            (this is the default).
         \li Produces raised segments filled with the foreground color.
    \row \li \c Flat
         \li Produces flat segments filled with the foreground color.
    \endtable

    \c Outline and \c Filled will additionally use
    QPalette::light() and QPalette::dark() for shadow effects.
*/
void QLCDNumber::setSegmentStyle(SegmentStyle s)
{
    Q_D(QLCDNumber);
    d->fill = (s == Flat || s == Filled);
    d->shadow = (s == Outline || s == Filled);
    update();
}

QLCDNumber::SegmentStyle QLCDNumber::segmentStyle() const
{
    Q_D(const QLCDNumber);
    Q_ASSERT(d->fill || d->shadow);
    if (!d->fill && d->shadow)
        return Outline;
    if (d->fill && d->shadow)
        return Filled;
    return Flat;
}


/*!\reimp
*/
QSize QLCDNumber::sizeHint() const
{
    return QSize(10 + 9 * (digitCount() + (smallDecimalPoint() ? 0 : 1)), 23);
}

/*! \reimp */
bool QLCDNumber::event(QEvent *e)
{
    return QFrame::event(e);
}

QT_END_NAMESPACE

#endif // QT_NO_LCDNUMBER
