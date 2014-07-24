/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
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


#include "qpagelayout.h"

#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

// Multiplier for converting units to points.
Q_GUI_EXPORT qreal qt_pointMultiplier(QPageLayout::Unit unit)
{
    switch (unit) {
    case QPageLayout::Millimeter:
        return 2.83464566929;
    case QPageLayout::Point:
        return 1.0;
    case QPageLayout::Inch:
        return 72.0;
    case QPageLayout::Pica:
        return 12;
    case QPageLayout::Didot:
        return 1.065826771;
    case QPageLayout::Cicero:
        return 12.789921252;
    }
    return 1.0;
}

// Multiplier for converting pixels to points.
extern qreal qt_pixelMultiplier(int resolution);

QPointF qt_convertPoint(const QPointF &xy, QPageLayout::Unit fromUnits, QPageLayout::Unit toUnits)
{
    // If the size have the same units, or are all 0, then don't need to convert
    if (fromUnits == toUnits || xy.isNull())
        return xy;

    // If converting to points then convert and round to 0 decimal places
    if (toUnits == QPageLayout::Point) {
        const qreal multiplier = qt_pointMultiplier(fromUnits);
        return QPointF(qRound(xy.x() * multiplier),
                       qRound(xy.y() * multiplier));
    }

    // If converting to other units, need to convert to unrounded points first
    QPointF pointXy = (fromUnits == QPageLayout::Point) ? xy : xy * qt_pointMultiplier(fromUnits);

    // Then convert from points to required units rounded to 2 decimal places
    const qreal multiplier = qt_pointMultiplier(toUnits);
    return QPointF(qRound(pointXy.x() * 100 / multiplier) / 100.0,
                   qRound(pointXy.y() * 100 / multiplier) / 100.0);
}

Q_GUI_EXPORT QMarginsF qt_convertMargins(const QMarginsF &margins, QPageLayout::Unit fromUnits, QPageLayout::Unit toUnits)
{
    // If the margins have the same units, or are all 0, then don't need to convert
    if (fromUnits == toUnits || margins.isNull())
        return margins;

    // If converting to points then convert and round to 0 decimal places
    if (toUnits == QPageLayout::Point) {
        const qreal multiplier = qt_pointMultiplier(fromUnits);
        return QMarginsF(qRound(margins.left() * multiplier),
                         qRound(margins.top() * multiplier),
                         qRound(margins.right() * multiplier),
                         qRound(margins.bottom() * multiplier));
    }

    // If converting to other units, need to convert to unrounded points first
    QMarginsF pointMargins = fromUnits == QPageLayout::Point ? margins : margins * qt_pointMultiplier(fromUnits);

    // Then convert from points to required units rounded to 2 decimal places
    const qreal multiplier = qt_pointMultiplier(toUnits);
    return QMarginsF(qRound(pointMargins.left() * 100 / multiplier) / 100.0,
                     qRound(pointMargins.top() * 100 / multiplier) / 100.0,
                     qRound(pointMargins.right() * 100 / multiplier) / 100.0,
                     qRound(pointMargins.bottom() * 100 / multiplier) / 100.0);
}

class QPageLayoutPrivate : public QSharedData
{
public:

    QPageLayoutPrivate();
    QPageLayoutPrivate(const QPageSize &pageSize, QPageLayout::Orientation orientation,
                       const QMarginsF &margins, QPageLayout::Unit units,
                       const QMarginsF &minMargins);
    ~QPageLayoutPrivate();

    bool operator==(const QPageLayoutPrivate &other) const;
    bool isEquivalentTo(const QPageLayoutPrivate &other) const;

    bool isValid() const;

    void clampMargins(const QMarginsF &margins);

    QMarginsF margins(QPageLayout::Unit units) const;
    QMargins marginsPoints() const;
    QMargins marginsPixels(int resolution) const;

    void setDefaultMargins(const QMarginsF &minMargins);

    QSizeF paintSize() const;

    QRectF fullRect() const;
    QRectF fullRect(QPageLayout::Unit units) const;
    QRect fullRectPoints() const;
    QRect fullRectPixels(int resolution) const;

    QRectF paintRect() const;

private:
    friend class QPageLayout;

    QSizeF fullSizeUnits(QPageLayout::Unit units) const;

    QPageSize m_pageSize;
    QPageLayout::Orientation m_orientation;
    QPageLayout::Mode m_mode;
    QPageLayout::Unit m_units;
    QSizeF m_fullSize;
    QMarginsF m_margins;
    QMarginsF m_minMargins;
    QMarginsF m_maxMargins;
};

QPageLayoutPrivate::QPageLayoutPrivate()
    : m_orientation(QPageLayout::Landscape),
      m_mode(QPageLayout::StandardMode)
{
}

QPageLayoutPrivate::QPageLayoutPrivate(const QPageSize &pageSize, QPageLayout::Orientation orientation,
                                       const QMarginsF &margins, QPageLayout::Unit units,
                                       const QMarginsF &minMargins)
    : m_pageSize(pageSize),
      m_orientation(orientation),
      m_mode(QPageLayout::StandardMode),
      m_units(units),
      m_margins(margins)
{
    m_fullSize = fullSizeUnits(m_units);
    setDefaultMargins(minMargins);
}

QPageLayoutPrivate::~QPageLayoutPrivate()
{
}

bool QPageLayoutPrivate::operator==(const QPageLayoutPrivate &other) const
{
    return m_pageSize == other.m_pageSize
           && m_orientation == other.m_orientation
           && m_units == other.m_units
           && m_margins == other.m_margins
           && m_minMargins == other.m_minMargins
           && m_maxMargins == other.m_maxMargins;
}

bool QPageLayoutPrivate::isEquivalentTo(const QPageLayoutPrivate &other) const
{
    return m_pageSize.isEquivalentTo(other.m_pageSize)
           && m_orientation == other.m_orientation
           && qt_convertMargins(m_margins, m_units, QPageLayout::Point)
              == qt_convertMargins(other.m_margins, other.m_units, QPageLayout::Point);
}

bool QPageLayoutPrivate::isValid() const
{
    return m_pageSize.isValid();
}

void QPageLayoutPrivate::clampMargins(const QMarginsF &margins)
{
    m_margins = QMarginsF(qBound(m_minMargins.left(),   margins.left(),   m_maxMargins.left()),
                          qBound(m_minMargins.top(),    margins.top(),    m_maxMargins.top()),
                          qBound(m_minMargins.right(),  margins.right(),  m_maxMargins.right()),
                          qBound(m_minMargins.bottom(), margins.bottom(), m_maxMargins.bottom()));
}

QMarginsF QPageLayoutPrivate::margins(QPageLayout::Unit units) const
{
    return qt_convertMargins(m_margins, m_units, units);
}

QMargins QPageLayoutPrivate::marginsPoints() const
{
    return qt_convertMargins(m_margins, m_units, QPageLayout::Point).toMargins();
}

QMargins QPageLayoutPrivate::marginsPixels(int resolution) const
{
    return marginsPoints() / qt_pixelMultiplier(resolution);
}

void QPageLayoutPrivate::setDefaultMargins(const QMarginsF &minMargins)
{
    m_minMargins = minMargins;
    m_maxMargins = QMarginsF(m_fullSize.width() - m_minMargins.right(),
                             m_fullSize.height() - m_minMargins.bottom(),
                             m_fullSize.width() - m_minMargins.left(),
                             m_fullSize.height() - m_minMargins.top());
    if (m_mode == QPageLayout::StandardMode)
        clampMargins(m_margins);
}

QSizeF QPageLayoutPrivate::fullSizeUnits(QPageLayout::Unit units) const
{
    QSizeF fullPageSize = m_pageSize.size(QPageSize::Unit(units));
    return m_orientation == QPageLayout::Landscape ? fullPageSize.transposed() : fullPageSize;
}

QRectF QPageLayoutPrivate::fullRect() const
{
    return QRectF(QPointF(0, 0), m_fullSize);
}

QRectF QPageLayoutPrivate::fullRect(QPageLayout::Unit units) const
{
    return units == m_units ? fullRect() : QRectF(QPointF(0, 0), fullSizeUnits(units));
}

QRect QPageLayoutPrivate::fullRectPoints() const
{
    if (m_orientation == QPageLayout::Landscape)
        return QRect(QPoint(0, 0), m_pageSize.sizePoints().transposed());
    else
        return QRect(QPoint(0, 0), m_pageSize.sizePoints());
}

QRect QPageLayoutPrivate::fullRectPixels(int resolution) const
{
    if (m_orientation == QPageLayout::Landscape)
        return QRect(QPoint(0, 0), m_pageSize.sizePixels(resolution).transposed());
    else
        return QRect(QPoint(0, 0), m_pageSize.sizePixels(resolution));
}

QRectF QPageLayoutPrivate::paintRect() const
{
    return m_mode == QPageLayout::FullPageMode ? fullRect() : fullRect() - m_margins;
}


/*!
    \class QPageLayout
    \inmodule QtGui
    \since 5.3
    \brief Describes the size, orientation and margins of a page.

    The QPageLayout class defines the layout of a page in a paged document, with the
    page size, orientation and margins able to be set and the full page and paintable
    page rectangles defined by those attributes able to be queried in a variety of units.

    The page size is defined by the QPageSize class which can be queried for page size
    attributes.  Note that the QPageSize itself is always defined in a Portrait
    orientation.

    The minimum margins can be defined for the layout but normally default to 0.
    When used in conjunction with Qt's printing support the minimum margins
    will reflect the minimum printable area defined by the printer.

    In the default StandardMode the current margins and minimum margins are
    always taken into account.  The paintable rectangle is the full page
    rectangle less the current margins, and the current margins can only be set
    to values between the minimum margins and the maximum margins allowed by
    the full page size.

    In FullPageMode the current margins and minimum margins are not taken
    into account. The paintable rectangle is the full page rectangle, and the
    current margins can be set to any values regardless of the minimum margins
    and page size.

    \sa QPageSize
*/

/*!
    \enum QPageLayout::Unit

    This enum type is used to specify the measurement unit for page layout and margins.

    \value Millimeter
    \value Point  1/72th of an inch
    \value Inch
    \value Pica  1/72th of a foot, 1/6th of an inch, 12 Points
    \value Didot  1/72th of a French inch, 0.375 mm
    \value Cicero  1/6th of a French inch, 12 Didot, 4.5mm
*/

/*!
    \enum QPageLayout::Orientation

    This enum type defines the page orientation

    \value Portrait The page size is used in its default orientation
    \value Landscape The page size is rotated through 90 degrees

    Note that some standard page sizes are defined with a width larger than
    their height, hence the orientation is defined relative to the standard
    page size and not using the relative page dimensions.
*/

/*!
    \enum QPageLayout::Mode

    Defines the page layout mode

    \value StandardMode Paint Rect includes margins, margins must fall between the minimum and maximum.
    \value FullPageMode Paint Rect excludes margins, margins can be any value and must be managed manually.
*/

/*!
    Creates an invalid QPageLayout.
*/

QPageLayout::QPageLayout()
    : d(new QPageLayoutPrivate())
{
}

/*!
    Creates a QPageLayout with the given \a pageSize, \a orientation and
    \a margins in the given \a units.

    Optionally define the minimum allowed margins \a minMargins, e.g. the minimum
    margins able to be printed by a physical print device.

    The constructed QPageLayout will be in StandardMode.

    The \a margins given will be clamped to the minimum margins and the maximum
    margins allowed by the page size.
*/

QPageLayout::QPageLayout(const QPageSize &pageSize, Orientation orientation,
                         const QMarginsF &margins, Unit units,
                         const QMarginsF &minMargins)
    : d(new QPageLayoutPrivate(pageSize, orientation, margins, units, minMargins))
{
}

/*!
    Copy constructor, copies \a other to this.
*/

QPageLayout::QPageLayout(const QPageLayout &other)
    : d(other.d)
{
}

/*!
    Destroys the page layout.
*/

QPageLayout::~QPageLayout()
{
}

/*!
    Assignment operator, assigns \a other to this.
*/

QPageLayout &QPageLayout::operator=(const QPageLayout &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QPageLayout::swap(QPageLayout &other)

    Swaps this page layout with \a other. This function is very fast and
    never fails.
*/

/*!
    \fn QPageLayout &QPageLayout::operator=(QPageLayout &&other)

    Move-assigns \a other to this QPageLayout instance, transferring the
    ownership of the managed pointer to this instance.
*/

/*!
    \relates QPageLayout

    Returns \c true if page layout \a lhs is equal to page layout \a rhs,
    i.e. if all the attributes are exactly equal.

    Note that this is a strict equality, especially for page size where the
    QPageSize ID, name and size must exactly match, and the margins where the
    units must match.

    \sa QPageLayout::isEquivalentTo()
*/

bool operator==(const QPageLayout &lhs, const QPageLayout &rhs)
{
    return lhs.d == rhs.d || *lhs.d == *rhs.d;
}

/*!
    \fn bool operator!=(const QPageLayout &lhs, const QPageLayout &rhs)
    \relates QPageLayout

    Returns \c true if page layout \a lhs is not equal to page layout \a rhs,
    i.e. if any of the attributes differ.

    Note that this is a strict equality, especially for page size where the
    QPageSize ID, name and size must exactly match, and the margins where the
    units must match.

    \sa QPageLayout::isEquivalentTo()
*/

/*!
    Returns \c true if this page layout is equivalent to the \a other page layout,
    i.e. if the page has the same size, margins and orientation.
*/

bool QPageLayout::isEquivalentTo(const QPageLayout &other) const
{
    return d && other.d && d->isEquivalentTo(*other.d);
}

/*!
    Returns \c true if this page layout is valid.
*/

bool QPageLayout::isValid() const
{
    return d->isValid();
}

/*!
    Sets a page layout mode to \a mode.
*/

void QPageLayout::setMode(Mode mode)
{
    d.detach();
    d->m_mode = mode;
}

/*!
    Returns the page layout mode.
*/

QPageLayout::Mode QPageLayout::mode() const
{
    return d->m_mode;
}

/*!
    Sets the page size of the page layout to \a pageSize.

    Optionally define the minimum allowed margins \a minMargins, e.g. the minimum
    margins able to be printed by a physical print device, otherwise the
    minimum margins will default to 0.

    If StandardMode is set then the existing margins will be clamped
    to the new minimum margins and the maximum margins allowed by the page size.
    If FullPageMode is set then the existing margins will be unchanged.
*/

void QPageLayout::setPageSize(const QPageSize &pageSize, const QMarginsF &minMargins)
{
    if (!pageSize.isValid())
        return;
    d.detach();
    d->m_pageSize = pageSize;
    d->m_fullSize = d->fullSizeUnits(d->m_units);
    d->setDefaultMargins(minMargins);
}

/*!
    Returns the page size of the page layout.

    Note that the QPageSize is always defined in a Portrait orientation.  To
    obtain a size that takes the set orientation into account you must use
    fullRect().
*/

QPageSize QPageLayout::pageSize() const
{
    return d->m_pageSize;
}

/*!
    Sets the page orientation of the page layout to \a orientation.

    Changing the orientation does not affect the current margins or
    the minimum margins.
*/

void QPageLayout::setOrientation(Orientation orientation)
{
    if (orientation != d->m_orientation) {
        d.detach();
        d->m_orientation = orientation;
        d->m_fullSize = d->fullSizeUnits(d->m_units);
        // Adust the max margins to reflect change in max page size
        const qreal change = d->m_fullSize.width() - d->m_fullSize.height();
        d->m_maxMargins.setLeft(d->m_maxMargins.left() + change);
        d->m_maxMargins.setRight(d->m_maxMargins.right() + change);
        d->m_maxMargins.setTop(d->m_maxMargins.top() - change);
        d->m_maxMargins.setBottom(d->m_maxMargins.bottom() - change);
    }
}

/*!
    Returns the page orientation of the page layout.
*/

QPageLayout::Orientation QPageLayout::orientation() const
{
    return d->m_orientation;
}

/*!
    Sets the \a units used to define the page layout.
*/

void QPageLayout::setUnits(Unit units)
{
    if (units != d->m_units) {
        d.detach();
        d->m_margins = qt_convertMargins(d->m_margins, d->m_units, units);
        d->m_minMargins = qt_convertMargins(d->m_minMargins, d->m_units, units);
        d->m_maxMargins = qt_convertMargins(d->m_maxMargins, d->m_units, units);
        d->m_units = units;
        d->m_fullSize = d->fullSizeUnits(d->m_units);
    }
}

/*!
    Returns the units the page layout is currently defined in.
*/

QPageLayout::Unit QPageLayout::units() const
{
    return d->m_units;
}

/*!
    Sets the page margins of the page layout to \a margins
    Returns true if the margins were successfully set.

    The units used are those currently defined for the layout.  To use different
    units then call setUnits() first.

    If in the default StandardMode then all the new margins must fall between the
    minimum margins set and the maximum margins allowed by the page size,
    otherwise the margins will not be set.

    If in FullPageMode then any margin values will be accepted.

    \sa margins(), units()
*/

bool QPageLayout::setMargins(const QMarginsF &margins)
{
    if (d->m_mode == FullPageMode) {
        d.detach();
        d->m_margins = margins;
        return true;
    } else if (margins.left() >= d->m_minMargins.left()
               && margins.right() >= d->m_minMargins.right()
               && margins.top() >= d->m_minMargins.top()
               && margins.bottom() >= d->m_minMargins.bottom()
               && margins.left() <= d->m_maxMargins.left()
               && margins.right() <= d->m_maxMargins.right()
               && margins.top() <= d->m_maxMargins.top()
               && margins.bottom() <= d->m_maxMargins.bottom()) {
        d.detach();
        d->m_margins = margins;
        return true;
    }
    return false;
}

/*!
    Sets the left page margin of the page layout to \a leftMargin.
    Returns true if the margin was successfully set.

    The units used are those currently defined for the layout.  To use different
    units call setUnits() first.

    If in the default StandardMode then the new margin must fall between the
    minimum margin set and the maximum margin allowed by the page size,
    otherwise the margin will not be set.

    If in FullPageMode then any margin values will be accepted.

    \sa setMargins(), margins()
*/

bool QPageLayout::setLeftMargin(qreal leftMargin)
{
    if (d->m_mode == FullPageMode
        || (leftMargin >= d->m_minMargins.left() && leftMargin <= d->m_maxMargins.left())) {
        d.detach();
        d->m_margins.setLeft(leftMargin);
        return true;
    }
    return false;
}

/*!
    Sets the right page margin of the page layout to \a rightMargin.
    Returns true if the margin was successfully set.

    The units used are those currently defined for the layout.  To use different
    units call setUnits() first.

    If in the default StandardMode then the new margin must fall between the
    minimum margin set and the maximum margin allowed by the page size,
    otherwise the margin will not be set.

    If in FullPageMode then any margin values will be accepted.

    \sa setMargins(), margins()
*/

bool QPageLayout::setRightMargin(qreal rightMargin)
{
    if (d->m_mode == FullPageMode
        || (rightMargin >= d->m_minMargins.right() && rightMargin <= d->m_maxMargins.right())) {
        d.detach();
        d->m_margins.setRight(rightMargin);
        return true;
    }
    return false;
}

/*!
    Sets the top page margin of the page layout to \a topMargin.
    Returns true if the margin was successfully set.

    The units used are those currently defined for the layout.  To use different
    units call setUnits() first.

    If in the default StandardMode then the new margin must fall between the
    minimum margin set and the maximum margin allowed by the page size,
    otherwise the margin will not be set.

    If in FullPageMode then any margin values will be accepted.

    \sa setMargins(), margins()
*/

bool QPageLayout::setTopMargin(qreal topMargin)
{
    if (d->m_mode == FullPageMode
        || (topMargin >= d->m_minMargins.top() && topMargin <= d->m_maxMargins.top())) {
        d.detach();
        d->m_margins.setTop(topMargin);
        return true;
    }
    return false;
}

/*!
    Sets the bottom page margin of the page layout to \a bottomMargin.
    Returns true if the margin was successfully set.

    The units used are those currently defined for the layout.  To use different
    units call setUnits() first.

    If in the default StandardMode then the new margin must fall between the
    minimum margin set and the maximum margin allowed by the page size,
    otherwise the margin will not be set.

    If in FullPageMode then any margin values will be accepted.

    \sa setMargins(), margins()
*/

bool QPageLayout::setBottomMargin(qreal bottomMargin)
{
    if (d->m_mode == FullPageMode
        || (bottomMargin >= d->m_minMargins.bottom() && bottomMargin <= d->m_maxMargins.bottom())) {
        d.detach();
        d->m_margins.setBottom(bottomMargin);
        return true;
    }
    return false;
}

/*!
    Returns the margins of the page layout using the currently set units.

    \sa setMargins(), units()
*/

QMarginsF QPageLayout::margins() const
{
    return d->m_margins;
}

/*!
    Returns the margins of the page layout using the requested \a units.

    \sa setMargins(), margins()
*/

QMarginsF QPageLayout::margins(Unit units) const
{
    return d->margins(units);
}

/*!
    Returns the margins of the page layout in Postscript Points (1/72 of an inch).

    \sa setMargins(), margins()
*/

QMargins QPageLayout::marginsPoints() const
{
    return d->marginsPoints();
}

/*!
    Returns the margins of the page layout in device pixels for the given \a resolution.

    \sa setMargins()
*/

QMargins QPageLayout::marginsPixels(int resolution) const
{
    return d->marginsPixels(resolution);
}

/*!
    Sets the minimum page margins of the page layout to \a minMargins.

    It is not recommended to override the default values set for a page size
    as this may be the minimum printable area for a physical print device.

    If the StandardMode mode is set then the existing margins will be clamped
    to the new \a minMargins and the maximum allowed by the page size.  If the
    FullPageMode is set then the existing margins will be unchanged.

    \sa minimumMargins(), setMargins()
*/

void QPageLayout::setMinimumMargins(const QMarginsF &minMargins)
{
    d.detach();
    d->setDefaultMargins(minMargins);
}

/*!
    Returns the minimum margins of the page layout.

    \sa setMinimumMargins(), maximumMargins()
*/

QMarginsF QPageLayout::minimumMargins() const
{
    return d->m_minMargins;
}

/*!
    Returns the maximum margins that would be applied if the page layout was
    in StandardMode.

    The maximum margins allowed are calculated as the full size of the page
    minus the minimum margins set. For example, if the page width is 100 points
    and the minimum right margin is 10 points, then the maximum left margin
    will be 90 points.

    \sa setMinimumMargins(), minimumMargins()
*/

QMarginsF QPageLayout::maximumMargins() const
{
    return d->m_maxMargins;
}

/*!
    Returns the full page rectangle in the current layout units.

    The page rectangle takes into account the page size and page orientation,
    but not the page margins.

    \sa paintRect(), units()
*/

QRectF QPageLayout::fullRect() const
{
    return isValid() ? d->fullRect() : QRect();
}

/*!
    Returns the full page rectangle in the required \a units.

    The page rectangle takes into account the page size and page orientation,
    but not the page margins.

    \sa paintRect()
*/

QRectF QPageLayout::fullRect(Unit units) const
{
    return isValid() ? d->fullRect(units) : QRect();
}

/*!
    Returns the full page rectangle in Postscript Points (1/72 of an inch).

    The page rectangle takes into account the page size and page orientation,
    but not the page margins.

    \sa paintRect()
*/

QRect QPageLayout::fullRectPoints() const
{
    return isValid() ? d->fullRectPoints() : QRect();
}

/*!
    Returns the full page rectangle in device pixels for the given \a resolution.

    The page rectangle takes into account the page size and page orientation,
    but not the page margins.

    \sa paintRect()
*/

QRect QPageLayout::fullRectPixels(int resolution) const
{
    return isValid() ? d->fullRectPixels(resolution) : QRect();
}

/*!
    Returns the page rectangle in the current layout units.

    The paintable rectangle takes into account the page size, orientation
    and margins.

    If the FullPageMode mode is set then the fullRect() is returned and
    the margins must be manually managed.
*/

QRectF QPageLayout::paintRect() const
{
    return isValid() ? d->paintRect() : QRectF();
}

/*!
    Returns the page rectangle in the required \a units.

    The paintable rectangle takes into account the page size, orientation
    and margins.

    If the FullPageMode mode is set then the fullRect() is returned and
    the margins must be manually managed.
*/

QRectF QPageLayout::paintRect(Unit units) const
{
    if (!isValid())
        return QRectF();
    if (units == d->m_units)
        return d->paintRect();
    return d->m_mode == FullPageMode ? d->fullRect(units)
                                                  : d->fullRect(units) - d->margins(units);
}

/*!
    Returns the paintable rectangle in rounded Postscript Points (1/72 of an inch).

    The paintable rectangle takes into account the page size, orientation
    and margins.

    If the FullPageMode mode is set then the fullRect() is returned and
    the margins must be manually managed.
*/

QRect QPageLayout::paintRectPoints() const
{
    if (!isValid())
        return QRect();
    return d->m_mode == FullPageMode ? d->fullRectPoints()
                                                  : d->fullRectPoints() - d->marginsPoints();
}

/*!
    Returns the paintable rectangle in rounded device pixels for the given \a resolution.

    The paintable rectangle takes into account the page size, orientation
    and margins.

    If the FullPageMode mode is set then the fullRect() is returned and
    the margins must be manually managed.
*/

QRect QPageLayout::paintRectPixels(int resolution) const
{
    if (!isValid())
        return QRect();
    return d->m_mode == FullPageMode ? d->fullRectPixels(resolution)
                                                  : d->fullRectPixels(resolution) - d->marginsPixels(resolution);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPageLayout &layout)
{
    if (layout.isValid()) {
        QString output = QStringLiteral("QPageLayout(%1, %2, l:%3 r:%4 t:%5 b:%6 %7)");
        QString units;
        switch (layout.units()) {
        case QPageLayout::Millimeter:
            units = QStringLiteral("mm");
            break;
        case QPageLayout::Point:
            units = QStringLiteral("pt");
            break;
        case QPageLayout::Inch:
            units = QStringLiteral("in");
            break;
        case QPageLayout::Pica:
            units = QStringLiteral("pc");
            break;
        case QPageLayout::Didot:
            units = QStringLiteral("DD");
            break;
        case QPageLayout::Cicero:
            units = QStringLiteral("CC");
            break;
        }
        output = output.arg(layout.pageSize().name())
                       .arg(layout.orientation() == QPageLayout::Portrait ? QStringLiteral("Portrait") : QStringLiteral("Landscape"))
                       .arg(layout.margins().left())
                       .arg(layout.margins().right())
                       .arg(layout.margins().top())
                       .arg(layout.margins().bottom())
                       .arg(units);
        dbg.nospace() << output;
    } else {
        dbg.nospace() << QStringLiteral("QPageLayout()");
    }
    return dbg.space();
}
#endif

QT_END_NAMESPACE
