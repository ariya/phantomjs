/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qglobal.h"

#if !defined(QT_NO_RAWFONT)

#include "qglyphrun.h"
#include "qglyphrun_p.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QGlyphRun
    \brief The QGlyphRun class provides direct access to the internal glyphs in a font.
    \since 4.8
    \inmodule QtGui

    \ingroup text
    \ingroup shared
    \mainclass

    When Qt displays a string of text encoded in Unicode, it will first convert the Unicode points
    into a list of glyph indexes and a list of positions based on one or more fonts. The Unicode
    representation of the text and the QFont object will in this case serve as a convenient
    abstraction that hides the details of what actually takes place when displaying the text
    on-screen. For instance, by the time the text actually reaches the screen, it may be represented
    by a set of fonts in addition to the one specified by the user, e.g. in case the originally
    selected font did not support all the writing systems contained in the text.

    Under certain circumstances, it can be useful as an application developer to have more low-level
    control over which glyphs in a specific font are drawn to the screen. This could for instance
    be the case in applications that use an external font engine and text shaper together with Qt.
    QGlyphRun provides an interface to the raw data needed to get text on the screen. It
    contains a list of glyph indexes, a position for each glyph and a font.

    It is the user's responsibility to ensure that the selected font actually contains the
    provided glyph indexes.

    QTextLayout::glyphRuns() or QTextFragment::glyphRuns() can be used to convert unicode encoded
    text into a list of QGlyphRun objects, and QPainter::drawGlyphRun() can be used to draw the
    glyphs.

    \note Please note that QRawFont is considered local to the thread in which it is constructed.
    This in turn means that a new QRawFont will have to be created and set on the QGlyphRun if it is
    moved to a different thread. If the QGlyphRun contains a reference to a QRawFont from a different
    thread than the current, it will not be possible to draw the glyphs using a QPainter, as the
    QRawFont is considered invalid and inaccessible in this case.
*/

/*!
  \enum QGlyphRun::GlyphRunFlag
  \since 5.0

  This enum describes flags that alter the way the run of glyphs might be presented or behave in
  a visual layout. The layout which generates the glyph runs can set these flags based on relevant
  internal data, to retain information needed to present the text as intended by the user of the
  layout.

  \value Overline Indicates that the glyphs should be visualized together with an overline.
  \value Underline Indicates that the glyphs should be visualized together with an underline.
  \value StrikeOut Indicates that the glyphs should be struck out visually.
  \value RightToLeft Indicates that the glyphs are ordered right to left. This can affect the
  positioning of other screen elements that are relative to the glyph run, such as an inline
  text object.
  \value SplitLigature Indicates that the glyph run splits a ligature glyph. This means
  that a ligature glyph is included in the run, but the characters represented by it corresponds
  only to part of that ligature. The glyph run's boundingRect() function can in this case be used
  to retrieve the area covered by glyphs that correspond to the characters represented by the
  glyph run. When visualizing the glyphs, care needs to be taken to clip to this bounding rect to
  ensure that only the corresponding part of the ligature is painted. In particular, this can be
  the case when retrieving a glyph run from a QTextLayout for a specific character range, e.g.
  when retrieving the selected area of a QTextLayout.
*/

/*!
    Constructs an empty QGlyphRun object.
*/
QGlyphRun::QGlyphRun() : d(new QGlyphRunPrivate)
{
}

/*!
    Constructs a QGlyphRun object which is a copy of \a other.
*/
QGlyphRun::QGlyphRun(const QGlyphRun &other)
{
    d = other.d;
}

/*!
    Destroys the QGlyphRun.
*/
QGlyphRun::~QGlyphRun()
{
    // Required for QExplicitlySharedDataPointer
}

/*!
    \internal
*/
void QGlyphRun::detach()
{
    if (d->ref.load() != 1)
        d.detach();
}

/*!
    Assigns \a other to this QGlyphRun object.
*/
QGlyphRun &QGlyphRun::operator=(const QGlyphRun &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QGlyphRun::swap(QGlyphRun &other)
    \since 5.0

    Swaps this glyph run instance with \a other. This function is very
    fast and never fails.
*/

/*!
    Compares \a other to this QGlyphRun object. Returns \c true if the list of glyph indexes,
    the list of positions and the font are all equal, otherwise returns \c false.
*/
bool QGlyphRun::operator==(const QGlyphRun &other) const
{
    if (d == other.d)
        return true;

    if ((d->glyphIndexDataSize != other.d->glyphIndexDataSize)
     || (d->glyphPositionDataSize != other.d->glyphPositionDataSize)) {
        return false;
    }

    if (d->glyphIndexData != other.d->glyphIndexData) {
        for (int i = 0; i < d->glyphIndexDataSize; ++i) {
            if (d->glyphIndexData[i] != other.d->glyphIndexData[i])
               return false;
        }
    }
    if (d->glyphPositionData != other.d->glyphPositionData) {
        for (int i = 0; i < d->glyphPositionDataSize; ++i) {
            if (d->glyphPositionData[i] != other.d->glyphPositionData[i])
               return false;
        }
    }

    return (d->flags == other.d->flags && d->rawFont == other.d->rawFont);
}

/*!
    \fn bool QGlyphRun::operator!=(const QGlyphRun &other) const

    Compares \a other to this QGlyphRun object. Returns \c true if any of the list of glyph
    indexes, the list of positions or the font are different, otherwise returns \c false.
*/

/*!
    Returns the font selected for this QGlyphRun object.

    \sa setRawFont()
*/
QRawFont QGlyphRun::rawFont() const
{
    return d->rawFont;
}

/*!
    Sets the font in which to look up the glyph indexes to the \a rawFont
    specified.

    \sa rawFont(), setGlyphIndexes()
*/
void QGlyphRun::setRawFont(const QRawFont &rawFont)
{
    detach();
    d->rawFont = rawFont;
}

/*!
    Returns the glyph indexes for this QGlyphRun object.

    \sa setGlyphIndexes(), setPositions()
*/
QVector<quint32> QGlyphRun::glyphIndexes() const
{
    if (d->glyphIndexes.constData() == d->glyphIndexData) {
        return d->glyphIndexes;
    } else {
        QVector<quint32> indexes(d->glyphIndexDataSize);
        memcpy(indexes.data(), d->glyphIndexData, d->glyphIndexDataSize * sizeof(quint32));
        return indexes;
    }
}

/*!
    Set the glyph indexes for this QGlyphRun object to \a glyphIndexes. The glyph indexes must
    be valid for the selected font.
*/
void QGlyphRun::setGlyphIndexes(const QVector<quint32> &glyphIndexes)
{
    detach();
    d->glyphIndexes = glyphIndexes; // Keep a reference to the QVector to avoid copying
    d->glyphIndexData = glyphIndexes.constData();
    d->glyphIndexDataSize = glyphIndexes.size();
}

/*!
    Returns the position of the edge of the baseline for each glyph in this set of glyph indexes.
*/
QVector<QPointF> QGlyphRun::positions() const
{
    if (d->glyphPositions.constData() == d->glyphPositionData) {
        return d->glyphPositions;
    } else {
        QVector<QPointF> glyphPositions(d->glyphPositionDataSize);
        memcpy(glyphPositions.data(), d->glyphPositionData,
                 d->glyphPositionDataSize * sizeof(QPointF));
        return glyphPositions;
    }
}

/*!
    Sets the positions of the edge of the baseline for each glyph in this set of glyph indexes to
    \a positions.
*/
void QGlyphRun::setPositions(const QVector<QPointF> &positions)
{
    detach();
    d->glyphPositions = positions; // Keep a reference to the vector to avoid copying
    d->glyphPositionData = positions.constData();
    d->glyphPositionDataSize = positions.size();
}

/*!
    Clears all data in the QGlyphRun object.
*/
void QGlyphRun::clear()
{
    detach();
    d->rawFont = QRawFont();
    d->flags = 0;

    setPositions(QVector<QPointF>());
    setGlyphIndexes(QVector<quint32>());
}

/*!
    Sets the glyph indexes and positions of this QGlyphRun to use the first \a size
    elements in the arrays \a glyphIndexArray and \a glyphPositionArray. The data is
    \e not copied. The caller must guarantee that the arrays are not deleted as long
    as this QGlyphRun and any copies of it exists.

    \sa setGlyphIndexes(), setPositions()
*/
void QGlyphRun::setRawData(const quint32 *glyphIndexArray, const QPointF *glyphPositionArray,
                           int size)
{
    detach();
    d->glyphIndexes.clear();
    d->glyphPositions.clear();

    d->glyphIndexData = glyphIndexArray;
    d->glyphPositionData = glyphPositionArray;
    d->glyphIndexDataSize = d->glyphPositionDataSize = size;
}

/*!
   Returns \c true if this QGlyphRun should be painted with an overline decoration.

   \sa setOverline(), flags()
*/
bool QGlyphRun::overline() const
{
    return d->flags & Overline;
}

/*!
  Indicates that this QGlyphRun should be painted with an overline decoration if \a overline is true.
  Otherwise the QGlyphRun should be painted with no overline decoration.

  \sa overline(), setFlag(), setFlags()
*/
void QGlyphRun::setOverline(bool overline)
{
    setFlag(Overline, overline);
}

/*!
   Returns \c true if this QGlyphRun should be painted with an underline decoration.

   \sa setUnderline(), flags()
*/
bool QGlyphRun::underline() const
{
    return d->flags & Underline;
}

/*!
  Indicates that this QGlyphRun should be painted with an underline decoration if \a underline is
  true. Otherwise the QGlyphRun should be painted with no underline decoration.

  \sa underline(), setFlag(), setFlags()
*/
void QGlyphRun::setUnderline(bool underline)
{
    setFlag(Underline, underline);
}

/*!
   Returns \c true if this QGlyphRun should be painted with a strike out decoration.

   \sa setStrikeOut(), flags()
*/
bool QGlyphRun::strikeOut() const
{
    return d->flags & StrikeOut;
}

/*!
  Indicates that this QGlyphRun should be painted with an strike out decoration if \a strikeOut is
  true. Otherwise the QGlyphRun should be painted with no strike out decoration.

  \sa strikeOut(), setFlag(), setFlags()
*/
void QGlyphRun::setStrikeOut(bool strikeOut)
{
    setFlag(StrikeOut, strikeOut);
}

/*!
  Returns \c true if this QGlyphRun contains glyphs that are painted from the right to the left.

  \since 5.0
  \sa setRightToLeft(), flags()
*/
bool QGlyphRun::isRightToLeft() const
{
    return d->flags & RightToLeft;
}

/*!
  Indicates that this QGlyphRun contains glyphs that should be ordered from the right to left
  if \a rightToLeft is true. Otherwise the order of the glyphs is assumed to be left to right.

  \since 5.0
  \sa isRightToLeft(), setFlag(), setFlags()
*/
void QGlyphRun::setRightToLeft(bool rightToLeft)
{
    setFlag(RightToLeft, rightToLeft);
}

/*!
  Returns the flags set for this QGlyphRun.

  \since 5.0
  \sa setFlag(), setFlag()
*/
QGlyphRun::GlyphRunFlags QGlyphRun::flags() const
{
    return d->flags;
}

/*!
  If \a enabled is true, then \a flag is enabled; otherwise, it is disabled.

  \since 5.0
  \sa flags(), setFlags()
*/
void QGlyphRun::setFlag(GlyphRunFlag flag, bool enabled)
{
    if (d->flags.testFlag(flag) == enabled)
        return;

    detach();
    if (enabled)
        d->flags |= flag;
    else
        d->flags &= ~flag;
}

/*!
  Sets the flags of this QGlyphRun to \a flags.

  \since 5.0
  \sa setFlag(), flags()
*/
void QGlyphRun::setFlags(GlyphRunFlags flags)
{
    if (d->flags == flags)
        return;

    detach();
    d->flags = flags;
}

/*!
  Sets the bounding rect of the glyphs in this QGlyphRun to be \a boundingRect. This rectangle
  will be returned by boundingRect() unless it is empty, in which case the bounding rectangle of the
  glyphs in the glyph run will be returned instead.

  \note Unless you are implementing text shaping, you should not have to use this function.
  It is used specifically when the QGlyphRun should represent an area which is smaller than the
  area of the glyphs it contains. This could happen e.g. if the glyph run is retrieved by calling
  QTextLayout::glyphRuns() and the specified range only includes part of a ligature (where two or
  more characters are combined to a single glyph.) When this is the case, the bounding rect should
  only include the appropriate part of the ligature glyph, based on a calculation of the average
  width of the characters in the ligature.

  In order to support such a case (an example is selections which should be drawn with a different
  color than the main text color), it is necessary to clip the painting mechanism to the rectangle
  returned from boundingRect() to avoid drawing the entire ligature glyph.

  \sa boundingRect()

  \since 5.0
*/
void QGlyphRun::setBoundingRect(const QRectF &boundingRect)
{
    detach();
    d->boundingRect = boundingRect;
}

/*!
  Returns the smallest rectangle that contains all glyphs in this QGlyphRun. If a bounding rect
  has been set using setBoundingRect(), then this will be returned. Otherwise the bounding rect
  will be calculated based on the font metrics of the glyphs in the glyph run.

  \since 5.0
*/
QRectF QGlyphRun::boundingRect() const
{
    if (!d->boundingRect.isEmpty() || !d->rawFont.isValid())
        return d->boundingRect;

    qreal minX, minY, maxX, maxY;
    minX = minY = maxX = maxY = 0;

    for (int i = 0, n = qMin(d->glyphIndexDataSize, d->glyphPositionDataSize); i < n; ++i) {
        QRectF glyphRect = d->rawFont.boundingRect(d->glyphIndexData[i]);
        glyphRect.translate(d->glyphPositionData[i]);

        if (i == 0) {
            minX = glyphRect.left();
            minY = glyphRect.top();
            maxX = glyphRect.right();
            maxY = glyphRect.bottom();
        } else {
            minX = qMin(glyphRect.left(), minX);
            minY = qMin(glyphRect.top(), minY);
            maxX = qMax(glyphRect.right(),maxX);
            maxY = qMax(glyphRect.bottom(), maxY);
        }
    }

    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

/*!
  Returns \c true if the QGlyphRun does not contain any glyphs.

  \since 5.0
*/
bool QGlyphRun::isEmpty() const
{
    return d->glyphIndexDataSize == 0;
}

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT
