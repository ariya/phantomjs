/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qheaderview.h"

#ifndef QT_NO_ITEMVIEWS
#include <qbitarray.h>
#include <qbrush.h>
#include <qdebug.h>
#include <qevent.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qvector.h>
#include <qapplication.h>
#include <qvarlengtharray.h>
#include <qabstractitemdelegate.h>
#include <qvariant.h>
#include <private/qheaderview_p.h>
#include <private/qabstractitemmodel_p.h>

#ifndef QT_NO_DATASTREAM
#include <qdatastream.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &out, const QHeaderViewPrivate::SectionSpan &span)
{
    span.write(out);
    return out;
}

QDataStream &operator>>(QDataStream &in, QHeaderViewPrivate::SectionSpan &span)
{
    span.read(in);
    return in;
}
#endif // QT_NO_DATASTREAM


/*!
    \class QHeaderView

    \brief The QHeaderView class provides a header row or header column for
    item views.

    \ingroup model-view


    A QHeaderView displays the headers used in item views such as the
    QTableView and QTreeView classes. It takes the place of Qt3's \c QHeader
    class previously used for the same purpose, but uses the Qt's model/view
    architecture for consistency with the item view classes.

    The QHeaderView class is one of the \l{Model/View Classes} and is part of
    Qt's \l{Model/View Programming}{model/view framework}.

    The header gets the data for each section from the model using the
    QAbstractItemModel::headerData() function. You can set the data by using
    QAbstractItemModel::setHeaderData().

    Each header has an orientation() and a number of sections, given by the
    count() function. A section refers to a part of the header - either a row
    or a column, depending on the orientation.

    Sections can be moved and resized using moveSection() and resizeSection();
    they can also be hidden and shown with hideSection() and showSection().

    Each section of a header is described by a section ID, specified by its
    section(), and can be located at a particular visualIndex() in the header.
    A section can have a sort indicator set with setSortIndicator(); this
    indicates whether the items in the associated item view will be sorted in
    the order given by the section.

    For a horizontal header the section is equivalent to a column in the model,
    and for a vertical header the section is equivalent to a row in the model.

    \section1 Moving Header Sections

    A header can be fixed in place, or made movable with setMovable(). It can
    be made clickable with setClickable(), and has resizing behavior in
    accordance with setResizeMode().

    \note Double-clicking on a header to resize a section only applies for
    visible rows.

    A header will emit sectionMoved() if the user moves a section,
    sectionResized() if the user resizes a section, and sectionClicked() as
    well as sectionHandleDoubleClicked() in response to mouse clicks. A header
    will also emit sectionCountChanged() and sectionAutoResize().

    You can identify a section using the logicalIndex() and logicalIndexAt()
    functions, or by its index position, using the visualIndex() and
    visualIndexAt() functions. The visual index will change if a section is
    moved, but the logical index will not change.

    \section1 Appearance

    QTableWidget and QTableView create default headers. If you want
    the headers to be visible, you can use \l{QFrame::}{setVisible()}.

    Not all \l{Qt::}{ItemDataRole}s will have an effect on a
    QHeaderView. If you need to draw other roles, you can subclass
    QHeaderView and reimplement \l{QHeaderView::}{paintEvent()}.
    QHeaderView respects the following item data roles:
    \l{Qt::}{TextAlignmentRole}, \l{Qt::}{DisplayRole},
    \l{Qt::}{FontRole}, \l{Qt::}{DecorationRole},
    \l{Qt::}{ForegroundRole}, and \l{Qt::}{BackgroundRole}.

    \note Each header renders the data for each section itself, and does not
    rely on a delegate. As a result, calling a header's setItemDelegate()
    function will have no effect.

    \sa {Model/View Programming}, QListView, QTableView, QTreeView
*/

/*!
    \enum QHeaderView::ResizeMode

    The resize mode specifies the behavior of the header sections. It can be
    set on the entire header view or on individual sections using
    setResizeMode().

    \value Interactive The user can resize the section. The section can also be
           resized programmatically using resizeSection().  The section size
           defaults to \l defaultSectionSize. (See also
           \l cascadingSectionResizes.)

    \value Fixed The user cannot resize the section. The section can only be
           resized programmatically using resizeSection(). The section size
           defaults to \l defaultSectionSize.

    \value Stretch QHeaderView will automatically resize the section to fill
           the available space. The size cannot be changed by the user or
           programmatically.

    \value ResizeToContents QHeaderView will automatically resize the section
           to its optimal size based on the contents of the entire column or
           row. The size cannot be changed by the user or programmatically.
           (This value was introduced in 4.2)

    The following values are obsolete:
    \value Custom Use Fixed instead.

    \sa setResizeMode() stretchLastSection minimumSectionSize
*/

/*!
    \fn void QHeaderView::sectionMoved(int logicalIndex, int oldVisualIndex,
    int newVisualIndex)

    This signal is emitted when a section is moved. The section's logical index
    is specified by \a logicalIndex, the old index by \a oldVisualIndex, and
    the new index position by \a newVisualIndex.

    \sa moveSection()
*/

/*!
    \fn void QHeaderView::sectionResized(int logicalIndex, int oldSize,
    int newSize)

    This signal is emitted when a section is resized. The section's logical
    number is specified by \a logicalIndex, the old size by \a oldSize, and the
    new size by \a newSize.

    \sa resizeSection()
*/

/*!
    \fn void QHeaderView::sectionPressed(int logicalIndex)

    This signal is emitted when a section is pressed. The section's logical
    index is specified by \a logicalIndex.

    \sa setClickable()
*/

/*!
    \fn void QHeaderView::sectionClicked(int logicalIndex)

    This signal is emitted when a section is clicked. The section's logical
    index is specified by \a logicalIndex.

    Note that the sectionPressed signal will also be emitted.

    \sa setClickable(), sectionPressed()
*/

/*!
    \fn void QHeaderView::sectionEntered(int logicalIndex)
    \since 4.3

    This signal is emitted when the cursor moves over the section and the left
    mouse button is pressed. The section's logical index is specified by
    \a logicalIndex.

    \sa setClickable(), sectionPressed()
*/

/*!
    \fn void QHeaderView::sectionDoubleClicked(int logicalIndex)

    This signal is emitted when a section is double-clicked. The section's
    logical index is specified by \a logicalIndex.

    \sa setClickable()
*/

/*!
    \fn void QHeaderView::sectionCountChanged(int oldCount, int newCount)

    This signal is emitted when the number of sections changes, i.e., when
    sections are added or deleted. The original count is specified by
    \a oldCount, and the new count by \a newCount.

    \sa count(), length(), headerDataChanged()
*/

/*!
    \fn void QHeaderView::sectionHandleDoubleClicked(int logicalIndex)

    This signal is emitted when a section is double-clicked. The section's
    logical index is specified by \a logicalIndex.

    \sa setClickable()
*/

/*!
    \fn void QHeaderView::sortIndicatorChanged(int logicalIndex,
    Qt::SortOrder order)
    \since 4.3

    This signal is emitted when the section containing the sort indicator or
    the order indicated is changed. The section's logical index is specified
    by \a logicalIndex and the sort order is specified by \a order.

    \sa setSortIndicator()
*/

/*!
    \fn void QHeaderView::sectionAutoResize(int logicalIndex,
    QHeaderView::ResizeMode mode)

    This signal is emitted when a section is automatically resized. The
    section's logical index is specified by \a logicalIndex, and the resize
    mode by \a mode.

    \sa setResizeMode(), stretchLastSection()
*/
// ### Qt 5: change to sectionAutoResized()

/*!
    \fn void QHeaderView::geometriesChanged()
    \since 4.2

    This signal is emitted when the header's geometries have changed.
*/

/*!
    \property QHeaderView::highlightSections
    \brief whether the sections containing selected items are highlighted

    By default, this property is false.
*/

/*!
    Creates a new generic header with the given \a orientation and \a parent.
*/
QHeaderView::QHeaderView(Qt::Orientation orientation, QWidget *parent)
    : QAbstractItemView(*new QHeaderViewPrivate, parent)
{
    Q_D(QHeaderView);
    d->setDefaultValues(orientation);
    initialize();
}

/*!
  \internal
*/
QHeaderView::QHeaderView(QHeaderViewPrivate &dd,
                         Qt::Orientation orientation, QWidget *parent)
    : QAbstractItemView(dd, parent)
{
    Q_D(QHeaderView);
    d->setDefaultValues(orientation);
    initialize();
}

/*!
  Destroys the header.
*/

QHeaderView::~QHeaderView()
{
}

/*!
  \internal
*/
void QHeaderView::initialize()
{
    Q_D(QHeaderView);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(NoFrame);
    setFocusPolicy(Qt::NoFocus);
    d->viewport->setMouseTracking(true);
    d->viewport->setBackgroundRole(QPalette::Button);
    d->textElideMode = Qt::ElideNone;
    delete d->itemDelegate;
}

/*!
  \reimp
*/
void QHeaderView::setModel(QAbstractItemModel *model)
{
    if (model == this->model())
        return;
    Q_D(QHeaderView);
    if (d->model && d->model != QAbstractItemModelPrivate::staticEmptyModel()) {
    if (d->orientation == Qt::Horizontal) {
        QObject::disconnect(d->model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                            this, SLOT(sectionsInserted(QModelIndex,int,int)));
        QObject::disconnect(d->model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                            this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
        QObject::disconnect(d->model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                            this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
    } else {
        QObject::disconnect(d->model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                            this, SLOT(sectionsInserted(QModelIndex,int,int)));
        QObject::disconnect(d->model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                            this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
        QObject::disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                            this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
    }
    QObject::disconnect(d->model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                        this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
        QObject::disconnect(d->model, SIGNAL(layoutAboutToBeChanged()),
                            this, SLOT(_q_layoutAboutToBeChanged()));
    }

    if (model && model != QAbstractItemModelPrivate::staticEmptyModel()) {
        if (d->orientation == Qt::Horizontal) {
            QObject::connect(model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                             this, SLOT(sectionsInserted(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
        } else {
            QObject::connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                             this, SLOT(sectionsInserted(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                             this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                             this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
        }
        QObject::connect(model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                         this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
        QObject::connect(model, SIGNAL(layoutAboutToBeChanged()),
                         this, SLOT(_q_layoutAboutToBeChanged()));
    }

    d->state = QHeaderViewPrivate::NoClear;
    QAbstractItemView::setModel(model);
    d->state = QHeaderViewPrivate::NoState;

    // Users want to set sizes and modes before the widget is shown.
    // Thus, we have to initialize when the model is set,
    // and not lazily like we do in the other views.
    initializeSections();
}

/*!
    Returns the orientation of the header.

    \sa Qt::Orientation
*/

Qt::Orientation QHeaderView::orientation() const
{
    Q_D(const QHeaderView);
    return d->orientation;
}

/*!
    Returns the offset of the header: this is the header's left-most (or
    top-most for vertical headers) visible pixel.

    \sa setOffset()
*/

int QHeaderView::offset() const
{
    Q_D(const QHeaderView);
    return d->offset;
}

/*!
    \fn void QHeaderView::setOffset(int offset)

    Sets the header's offset to \a offset.

    \sa offset(), length()
*/

void QHeaderView::setOffset(int newOffset)
{
    Q_D(QHeaderView);
    if (d->offset == (int)newOffset)
        return;
    int ndelta = d->offset - newOffset;
    d->offset = newOffset;
    if (d->orientation == Qt::Horizontal)
        d->viewport->scroll(isRightToLeft() ? -ndelta : ndelta, 0);
    else
        d->viewport->scroll(0, ndelta);
    if (d->state == QHeaderViewPrivate::ResizeSection) {
        QPoint cursorPos = QCursor::pos();
        if (d->orientation == Qt::Horizontal)
            QCursor::setPos(cursorPos.x() + ndelta, cursorPos.y());
        else
            QCursor::setPos(cursorPos.x(), cursorPos.y() + ndelta);
        d->firstPos += ndelta;
        d->lastPos += ndelta;
    }
}

/*!
    \since 4.2
    Sets the offset to the start of the section at the given \a visualIndex.

    \sa setOffset(), sectionPosition()
*/
void QHeaderView::setOffsetToSectionPosition(int visualIndex)
{
    Q_D(QHeaderView);
    if (visualIndex > -1 && visualIndex < d->sectionCount) {
        int position = d->headerSectionPosition(d->adjustedVisualIndex(visualIndex));
        setOffset(position);
    }
}

/*!
    \since 4.2
    Sets the offset to make the last section visible.

    \sa setOffset(), sectionPosition(), setOffsetToSectionPosition()
*/
void QHeaderView::setOffsetToLastSection()
{
    Q_D(const QHeaderView);
    int size = (d->orientation == Qt::Horizontal ? viewport()->width() : viewport()->height());
    int position = length() - size;
    setOffset(position);
}

/*!
    Returns the length along the orientation of the header.

    \sa sizeHint(), setResizeMode(), offset()
*/

int QHeaderView::length() const
{
    Q_D(const QHeaderView);
    //Q_ASSERT(d->headerLength() == d->length);
    return d->length;
}

/*!
    Returns a suitable size hint for this header.

    \sa sectionSizeHint()
*/

QSize QHeaderView::sizeHint() const
{
    Q_D(const QHeaderView);
    if (d->cachedSizeHint.isValid())
        return d->cachedSizeHint;
    d->cachedSizeHint = QSize(0, 0); //reinitialize the cached size hint
    const int sectionCount = count();

    // get size hint for the first n sections
    int i = 0;
    for (int checked = 0; checked < 100 && i < sectionCount; ++i) {
        if (isSectionHidden(i))
            continue;
        checked++;
        QSize hint = sectionSizeFromContents(i);
        d->cachedSizeHint = d->cachedSizeHint.expandedTo(hint);
    }
    // get size hint for the last n sections
    i = qMax(i, sectionCount - 100 );
    for (int j = sectionCount - 1, checked = 0; j >= i && checked < 100; --j) {
        if (isSectionHidden(j))
            continue;
        checked++;
        QSize hint = sectionSizeFromContents(j);
        d->cachedSizeHint = d->cachedSizeHint.expandedTo(hint);
    }
    return d->cachedSizeHint;
}

/*!
    Returns a suitable size hint for the section specified by \a logicalIndex.

    \sa sizeHint(), defaultSectionSize(), minimumSectionSize(),
    Qt::SizeHintRole
*/

int QHeaderView::sectionSizeHint(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (isSectionHidden(logicalIndex))
        return 0;
    if (logicalIndex < 0 || logicalIndex >= count())
        return -1;
    QSize size;
    QVariant value = d->model->headerData(logicalIndex, d->orientation, Qt::SizeHintRole);
    if (value.isValid())
        size = qvariant_cast<QSize>(value);
    else
        size = sectionSizeFromContents(logicalIndex);
    int hint = d->orientation == Qt::Horizontal ? size.width() : size.height();
    return qMax(minimumSectionSize(), hint);
}

/*!
    Returns the visual index of the section that covers the given \a position
    in the viewport.

    \sa logicalIndexAt()
*/

int QHeaderView::visualIndexAt(int position) const
{
    Q_D(const QHeaderView);
    int vposition = position;
    d->executePostedLayout();
    d->executePostedResize();
    const int count = d->sectionCount;
    if (count < 1)
        return -1;

    if (d->reverse())
        vposition = d->viewport->width() - vposition;
    vposition += d->offset;

    if (vposition > d->length)
        return -1;
    int visual = d->headerVisualIndexAt(vposition);
    if (visual < 0)
        return -1;

    while (d->isVisualIndexHidden(visual)){
        ++visual;
        if (visual >= count)
            return -1;
    }
    return visual;
}

/*!
    Returns the section that covers the given \a position in the viewport.

    \sa visualIndexAt(), isSectionHidden()
*/

int QHeaderView::logicalIndexAt(int position) const
{
    const int visual = visualIndexAt(position);
    if (visual > -1)
        return logicalIndex(visual);
    return -1;
}

/*!
    Returns the width (or height for vertical headers) of the given
    \a logicalIndex.

    \sa length(), setResizeMode(), defaultSectionSize()
*/

int QHeaderView::sectionSize(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (isSectionHidden(logicalIndex))
        return 0;
    if (logicalIndex < 0 || logicalIndex >= count())
        return 0;
    int visual = visualIndex(logicalIndex);
    if (visual == -1)
        return 0;
    d->executePostedResize();
    return d->headerSectionSize(visual);
}

/*!

    Returns the section position of the given \a logicalIndex, or -1
    if the section is hidden. The position is measured in pixels from
    the first visible item's top-left corner to the top-left corner of
    the item with \a logicalIndex. The measurement is along the x-axis
    for horizontal headers and along the y-axis for vertical headers.

    \sa sectionViewportPosition()
*/

int QHeaderView::sectionPosition(int logicalIndex) const
{
    Q_D(const QHeaderView);
    int visual = visualIndex(logicalIndex);
    // in some cases users may change the selections
    // before we have a chance to do the layout
    if (visual == -1)
        return -1;
    d->executePostedResize();
    return d->headerSectionPosition(visual);
}

/*!
    Returns the section viewport position of the given \a logicalIndex.

    If the section is hidden, the return value is undefined.

    \sa sectionPosition(), isSectionHidden()
*/

int QHeaderView::sectionViewportPosition(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (logicalIndex >= count())
        return -1;
    int position = sectionPosition(logicalIndex);
    if (position < 0)
        return position; // the section was hidden
    int offsetPosition = position - d->offset;
    if (d->reverse())
        return d->viewport->width() - (offsetPosition + sectionSize(logicalIndex));
    return offsetPosition;
}

/*!
    \fn int QHeaderView::logicalIndexAt(int x, int y) const

    Returns the logical index of the section at the given coordinate. If the
    header is horizontal \a x will be used, otherwise \a y will be used to
    find the logical index.
*/

/*!
    \fn int QHeaderView::logicalIndexAt(const QPoint &pos) const

    Returns the logical index of the section at the position given in \a pos.
    If the header is horizontal the x-coordinate will be used, otherwise the
    y-coordinate will be used to find the logical index.

    \sa sectionPosition()
*/

/*!
    Moves the section at visual index \a from to occupy visual index \a to.

    \sa sectionsMoved()
*/

void QHeaderView::moveSection(int from, int to)
{
    Q_D(QHeaderView);

    d->executePostedLayout();
    if (from < 0 || from >= d->sectionCount || to < 0 || to >= d->sectionCount)
        return;

    if (from == to) {
        int logical = logicalIndex(from);
        Q_ASSERT(logical != -1);
        updateSection(logical);
        return;
    }

    if (stretchLastSection() &&  to == d->lastVisibleVisualIndex())
        d->lastSectionSize = sectionSize(from);

    //int oldHeaderLength = length(); // ### for debugging; remove later
    d->initializeIndexMapping();

    QBitArray sectionHidden = d->sectionHidden;
    int *visualIndices = d->visualIndices.data();
    int *logicalIndices = d->logicalIndices.data();
    int logical = logicalIndices[from];
    int visual = from;

    int affected_count = qAbs(to - from) + 1;
    QVarLengthArray<int> sizes(affected_count);
    QVarLengthArray<ResizeMode> modes(affected_count);

    // move sections and indices
    if (to > from) {
        sizes[to - from] = d->headerSectionSize(from);
        modes[to - from] = d->headerSectionResizeMode(from);
        while (visual < to) {
            sizes[visual - from] = d->headerSectionSize(visual + 1);
            modes[visual - from] = d->headerSectionResizeMode(visual + 1);
            if (!sectionHidden.isEmpty())
                sectionHidden.setBit(visual, sectionHidden.testBit(visual + 1));
            visualIndices[logicalIndices[visual + 1]] = visual;
            logicalIndices[visual] = logicalIndices[visual + 1];
            ++visual;
        }
    } else {
        sizes[0] = d->headerSectionSize(from);
        modes[0] = d->headerSectionResizeMode(from);
        while (visual > to) {
            sizes[visual - to] = d->headerSectionSize(visual - 1);
            modes[visual - to] = d->headerSectionResizeMode(visual - 1);
            if (!sectionHidden.isEmpty())
                sectionHidden.setBit(visual, sectionHidden.testBit(visual - 1));
            visualIndices[logicalIndices[visual - 1]] = visual;
            logicalIndices[visual] = logicalIndices[visual - 1];
            --visual;
        }
    }
    if (!sectionHidden.isEmpty()) {
        sectionHidden.setBit(to, d->sectionHidden.testBit(from));
        d->sectionHidden = sectionHidden;
    }
    visualIndices[logical] = to;
    logicalIndices[to] = logical;

    //Q_ASSERT(oldHeaderLength == length());
    // move sizes
    // ### check for spans of section sizes here
    if (to > from) {
        for (visual = from; visual <= to; ++visual) {
            int size = sizes[visual - from];
            ResizeMode mode = modes[visual - from];
            d->createSectionSpan(visual, visual, size, mode);
        }
    } else {
        for (visual = to; visual <= from; ++visual) {
            int size = sizes[visual - to];
            ResizeMode mode = modes[visual - to];
            d->createSectionSpan(visual, visual, size, mode);
        }
    }
    //Q_ASSERT(d->headerLength() == length());
    //Q_ASSERT(oldHeaderLength == length());
    //Q_ASSERT(d->logicalIndices.count() == d->sectionCount);

    if (d->hasAutoResizeSections())
        d->doDelayedResizeSections();
    d->viewport->update();

    emit sectionMoved(logical, from, to);
}

/*!
    \since 4.2
    Swaps the section at visual index \a first with the section at visual
    index \a second.

    \sa moveSection()
*/
void QHeaderView::swapSections(int first, int second)
{
    Q_D(QHeaderView);

    if (first == second)
        return;
    d->executePostedLayout();
    if (first < 0 || first >= d->sectionCount || second < 0 || second >= d->sectionCount)
        return;

    int firstSize = d->headerSectionSize(first);
    ResizeMode firstMode = d->headerSectionResizeMode(first);
    int firstLogical = d->logicalIndex(first);

    int secondSize = d->headerSectionSize(second);
    ResizeMode secondMode = d->headerSectionResizeMode(second);
    int secondLogical = d->logicalIndex(second);

    d->createSectionSpan(second, second, firstSize, firstMode);
    d->createSectionSpan(first, first, secondSize, secondMode);

    d->initializeIndexMapping();

    d->visualIndices[firstLogical] = second;
    d->logicalIndices[second] = firstLogical;

    d->visualIndices[secondLogical] = first;
    d->logicalIndices[first] = secondLogical;

    if (!d->sectionHidden.isEmpty()) {
        bool firstHidden = d->sectionHidden.testBit(first);
        bool secondHidden = d->sectionHidden.testBit(second);
        d->sectionHidden.setBit(first, secondHidden);
        d->sectionHidden.setBit(second, firstHidden);
    }

    d->viewport->update();
    emit sectionMoved(firstLogical, first, second);
    emit sectionMoved(secondLogical, second, first);
}

/*!
    \fn void QHeaderView::resizeSection(int logicalIndex, int size)

    Resizes the section specified by \a logicalIndex to \a size measured in
    pixels.

    \sa sectionResized(), resizeMode(), sectionSize()
*/

void QHeaderView::resizeSection(int logical, int size)
{
    Q_D(QHeaderView);
    if (logical < 0 || logical >= count())
        return;

    if (isSectionHidden(logical)) {
        d->hiddenSectionSize.insert(logical, size);
        return;
    }

    int visual = visualIndex(logical);
    if (visual == -1)
        return;

    int oldSize = d->headerSectionSize(visual);
    if (oldSize == size)
        return;

    d->executePostedLayout();
    d->invalidateCachedSizeHint();

    if (stretchLastSection() && visual == d->lastVisibleVisualIndex())
        d->lastSectionSize = size;

    if (size != oldSize)
        d->createSectionSpan(visual, visual, size, d->headerSectionResizeMode(visual));

    int w = d->viewport->width();
    int h = d->viewport->height();
    int pos = sectionViewportPosition(logical);
    QRect r;
    if (d->orientation == Qt::Horizontal)
        if (isRightToLeft())
            r.setRect(0, 0, pos + size, h);
        else
            r.setRect(pos, 0, w - pos, h);
    else
        r.setRect(0, pos, w, h - pos);

    if (d->hasAutoResizeSections()) {
        d->doDelayedResizeSections();
        r = d->viewport->rect();
    }
    d->viewport->update(r.normalized());
    emit sectionResized(logical, oldSize, size);
}

/*!
    Resizes the sections according to the given \a mode, ignoring the current
    resize mode.

    \sa resizeMode(), sectionResized()
*/

void QHeaderView::resizeSections(QHeaderView::ResizeMode mode)
{
    Q_D(QHeaderView);
    d->resizeSections(mode, true);
}

/*!
    \fn void QHeaderView::hideSection(int logicalIndex)
    Hides the section specified by \a logicalIndex.

    \sa showSection(), isSectionHidden(), hiddenSectionCount(),
    setSectionHidden()
*/

/*!
    \fn void QHeaderView::showSection(int logicalIndex)
    Shows the section specified by \a logicalIndex.

    \sa hideSection(), isSectionHidden(), hiddenSectionCount(),
    setSectionHidden()
*/

/*!
    Returns true if the section specified by \a logicalIndex is explicitly
    hidden from the user; otherwise returns false.

    \sa hideSection(), showSection(), setSectionHidden(), hiddenSectionCount()
*/

bool QHeaderView::isSectionHidden(int logicalIndex) const
{
    Q_D(const QHeaderView);
    d->executePostedLayout();
    if (logicalIndex >= d->sectionHidden.count() || logicalIndex < 0 || logicalIndex >= d->sectionCount)
        return false;
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);
    return d->sectionHidden.testBit(visual);
}

/*!
    \since 4.1

    Returns the number of sections in the header that has been hidden.

    \sa setSectionHidden(), isSectionHidden()
*/
int QHeaderView::hiddenSectionCount() const
{
    Q_D(const QHeaderView);
    return d->hiddenSectionSize.count();
}

/*!
  If \a hide is true the section specified by \a logicalIndex is hidden;
  otherwise the section is shown.

  \sa isSectionHidden(), hiddenSectionCount()
*/

void QHeaderView::setSectionHidden(int logicalIndex, bool hide)
{
    Q_D(QHeaderView);
    if (logicalIndex < 0 || logicalIndex >= count())
        return;

    d->executePostedLayout();
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);
    if (hide == d->isVisualIndexHidden(visual))
        return;
    if (hide) {
        int size = d->headerSectionSize(visual);
        if (!d->hasAutoResizeSections())
            resizeSection(logicalIndex, 0);
        d->hiddenSectionSize.insert(logicalIndex, size);
        if (d->sectionHidden.count() < count())
            d->sectionHidden.resize(count());
        d->sectionHidden.setBit(visual, true);
        if (d->hasAutoResizeSections())
            d->doDelayedResizeSections();
    } else {
        int size = d->hiddenSectionSize.value(logicalIndex, d->defaultSectionSize);
        d->hiddenSectionSize.remove(logicalIndex);
        if (d->hiddenSectionSize.isEmpty()) {
            d->sectionHidden.clear();
        } else {
            Q_ASSERT(visual <= d->sectionHidden.count());
            d->sectionHidden.setBit(visual, false);
        }
        resizeSection(logicalIndex, size);
    }
}

/*!
    Returns the number of sections in the header.

    \sa  sectionCountChanged(), length()
*/

int QHeaderView::count() const
{
    Q_D(const QHeaderView);
    //Q_ASSERT(d->sectionCount == d->headerSectionCount());
    // ### this may affect the lazy layout
    d->executePostedLayout();
    return d->sectionCount;
}

/*!
    Returns the visual index position of the section specified by the given
    \a logicalIndex, or -1 otherwise.

    Hidden sections still have valid visual indexes.

    \sa logicalIndex()
*/

int QHeaderView::visualIndex(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (logicalIndex < 0)
        return -1;
    d->executePostedLayout();
    if (d->visualIndices.isEmpty()) { // nothing has been moved, so we have no mapping
        if (logicalIndex < d->sectionCount)
            return logicalIndex;
    } else if (logicalIndex < d->visualIndices.count()) {
        int visual = d->visualIndices.at(logicalIndex);
        Q_ASSERT(visual < d->sectionCount);
        return visual;
    }
    return -1;
}

/*!
    Returns the logicalIndex for the section at the given \a visualIndex
    position, or -1 if visualIndex < 0 or visualIndex >= QHeaderView::count().

    Note that the visualIndex is not affected by hidden sections.

    \sa visualIndex(), sectionPosition()
*/

int QHeaderView::logicalIndex(int visualIndex) const
{
    Q_D(const QHeaderView);
    if (visualIndex < 0 || visualIndex >= d->sectionCount)
        return -1;
    return d->logicalIndex(visualIndex);
}

/*!
    If \a movable is true, the header may be moved by the user; otherwise it
    is fixed in place.

    \sa isMovable(), sectionMoved()
*/

// ### Qt 5: change to setSectionsMovable()
void QHeaderView::setMovable(bool movable)
{
    Q_D(QHeaderView);
    d->movableSections = movable;
}

/*!
    Returns true if the header can be moved by the user; otherwise returns
    false.

    \sa setMovable()
*/

// ### Qt 5: change to sectionsMovable()
bool QHeaderView::isMovable() const
{
    Q_D(const QHeaderView);
    return d->movableSections;
}

/*!
    If \a clickable is true, the header will respond to single clicks.

    \sa isClickable(), sectionClicked(), sectionPressed(),
    setSortIndicatorShown()
*/

// ### Qt 5: change to setSectionsClickable()
void QHeaderView::setClickable(bool clickable)
{
    Q_D(QHeaderView);
    d->clickableSections = clickable;
}

/*!
    Returns true if the header is clickable; otherwise returns false. A
    clickable header could be set up to allow the user to change the
    representation of the data in the view related to the header.

    \sa setClickable()
*/

// ### Qt 5: change to sectionsClickable()
bool QHeaderView::isClickable() const
{
    Q_D(const QHeaderView);
    return d->clickableSections;
}

void QHeaderView::setHighlightSections(bool highlight)
{
    Q_D(QHeaderView);
    d->highlightSelected = highlight;
}

bool QHeaderView::highlightSections() const
{
    Q_D(const QHeaderView);
    return d->highlightSelected;
}

/*!
    Sets the constraints on how the header can be resized to those described
    by the given \a mode.

    \sa resizeMode(), length(), sectionResized(), sectionAutoResize()
*/

void QHeaderView::setResizeMode(ResizeMode mode)
{
    Q_D(QHeaderView);
    initializeSections();
    d->stretchSections = (mode == Stretch ? count() : 0);
    d->contentsSections =  (mode == ResizeToContents ? count() : 0);
    d->setGlobalHeaderResizeMode(mode);
    if (d->hasAutoResizeSections())
        d->doDelayedResizeSections(); // section sizes may change as a result of the new mode
}

/*!
    \overload

    Sets the constraints on how the section specified by \a logicalIndex in
    the header can be resized to those described by the given \a mode. The logical
    index should exist at the time this function is called.

    \note This setting will be ignored for the last section if the stretchLastSection
    property is set to true. This is the default for the horizontal headers provided
    by QTreeView.

    \sa setStretchLastSection()
*/

// ### Qt 5: change to setSectionResizeMode()
void QHeaderView::setResizeMode(int logicalIndex, ResizeMode mode)
{
    Q_D(QHeaderView);
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);

    ResizeMode old = d->headerSectionResizeMode(visual);
    d->setHeaderSectionResizeMode(visual, mode);

    if (mode == Stretch && old != Stretch)
        ++d->stretchSections;
    else if (mode == ResizeToContents && old != ResizeToContents)
        ++d->contentsSections;
    else if (mode != Stretch && old == Stretch)
        --d->stretchSections;
    else if (mode != ResizeToContents && old == ResizeToContents)
        --d->contentsSections;

    if (d->hasAutoResizeSections() && d->state == QHeaderViewPrivate::NoState)
        d->doDelayedResizeSections(); // section sizes may change as a result of the new mode
}

/*!
    Returns the resize mode that applies to the section specified by the given
    \a logicalIndex.

    \sa setResizeMode()
*/

QHeaderView::ResizeMode QHeaderView::resizeMode(int logicalIndex) const
{
    Q_D(const QHeaderView);
    int visual = visualIndex(logicalIndex);
    if (visual == -1)
        return Fixed; //the default value
    return d->headerSectionResizeMode(visual);
}

/*!
    \since 4.1

    Returns the number of sections that are set to resize mode stretch. In
    views, this can be used to see if the headerview needs to resize the
    sections when the view's geometry changes.

    \sa stretchLastSection, resizeMode()
*/

int QHeaderView::stretchSectionCount() const
{
    Q_D(const QHeaderView);
    return d->stretchSections;
}

/*!
  \property QHeaderView::showSortIndicator
  \brief whether the sort indicator is shown

  By default, this property is false.

  \sa setClickable()
*/

void QHeaderView::setSortIndicatorShown(bool show)
{
    Q_D(QHeaderView);
    if (d->sortIndicatorShown == show)
        return;

    d->sortIndicatorShown = show;

    if (sortIndicatorSection() < 0 || sortIndicatorSection() > count())
        return;

    if (d->headerSectionResizeMode(sortIndicatorSection()) == ResizeToContents)
        resizeSections();

    d->viewport->update();
}

bool QHeaderView::isSortIndicatorShown() const
{
    Q_D(const QHeaderView);
    return d->sortIndicatorShown;
}

/*!
    Sets the sort indicator for the section specified by the given
    \a logicalIndex in the direction specified by \a order, and removes the
    sort indicator from any other section that was showing it.

    \a logicalIndex may be -1, in which case no sort indicator will be shown
    and the model will return to its natural, unsorted order. Note that not
    all models support this and may even crash in this case.

    \sa sortIndicatorSection() sortIndicatorOrder()
*/

void QHeaderView::setSortIndicator(int logicalIndex, Qt::SortOrder order)
{
    Q_D(QHeaderView);

    // This is so that people can set the position of the sort indicator before the fill the model
    int old = d->sortIndicatorSection;
    d->sortIndicatorSection = logicalIndex;
    d->sortIndicatorOrder = order;

    if (logicalIndex >= d->sectionCount) {
        emit sortIndicatorChanged(logicalIndex, order);
        return; // nothing to do
    }

    if (old != logicalIndex
        && ((logicalIndex >= 0 && resizeMode(logicalIndex) == ResizeToContents)
            || old >= d->sectionCount || (old >= 0 && resizeMode(old) == ResizeToContents))) {
        resizeSections();
        d->viewport->update();
    } else {
        if (old >= 0 && old != logicalIndex)
            updateSection(old);
        if (logicalIndex >= 0)
            updateSection(logicalIndex);
    }

    emit sortIndicatorChanged(logicalIndex, order);
}

/*!
    Returns the logical index of the section that has a sort indicator.
    By default this is section 0.

    \sa setSortIndicator() sortIndicatorOrder() setSortIndicatorShown()
*/

int QHeaderView::sortIndicatorSection() const
{
    Q_D(const QHeaderView);
    return d->sortIndicatorSection;
}

/*!
    Returns the order for the sort indicator. If no section has a sort
    indicator the return value of this function is undefined.

    \sa setSortIndicator() sortIndicatorSection()
*/

Qt::SortOrder QHeaderView::sortIndicatorOrder() const
{
    Q_D(const QHeaderView);
    return d->sortIndicatorOrder;
}

/*!
    \property QHeaderView::stretchLastSection
    \brief whether the last visible section in the header takes up all the
    available space

    The default value is false.

    \note The horizontal headers provided by QTreeView are configured with this
    property set to true, ensuring that the view does not waste any of the
    space assigned to it for its header. If this value is set to true, this
    property will override the resize mode set on the last section in the
    header.

    \sa setResizeMode()
*/
bool QHeaderView::stretchLastSection() const
{
    Q_D(const QHeaderView);
    return d->stretchLastSection;
}

void QHeaderView::setStretchLastSection(bool stretch)
{
    Q_D(QHeaderView);
    d->stretchLastSection = stretch;
    if (d->state != QHeaderViewPrivate::NoState)
        return;
    if (stretch)
        resizeSections();
    else if (count())
        resizeSection(count() - 1, d->defaultSectionSize);
}

/*!
    \since 4.2
    \property QHeaderView::cascadingSectionResizes
    \brief whether interactive resizing will be cascaded to the following
    sections once the section being resized by the user has reached its
    minimum size

    This property only affects sections that have \l Interactive as their
    resize mode.

    The default value is false.

    \sa setResizeMode()
*/
bool QHeaderView::cascadingSectionResizes() const
{
    Q_D(const QHeaderView);
    return d->cascadingResizing;
}

void QHeaderView::setCascadingSectionResizes(bool enable)
{
    Q_D(QHeaderView);
    d->cascadingResizing = enable;
}

/*!
    \property QHeaderView::defaultSectionSize
    \brief the default size of the header sections before resizing.

    This property only affects sections that have \l Interactive or \l Fixed
    as their resize mode.

    \sa setResizeMode() minimumSectionSize
*/
int QHeaderView::defaultSectionSize() const
{
    Q_D(const QHeaderView);
    return d->defaultSectionSize;
}

void QHeaderView::setDefaultSectionSize(int size)
{
    Q_D(QHeaderView);
    d->setDefaultSectionSize(size);
}

/*!
    \since 4.2
    \property QHeaderView::minimumSectionSize
    \brief the minimum size of the header sections.

    The minimum section size is the smallest section size allowed. If the
    minimum section size is set to -1, QHeaderView will use the maximum of
    the \l{QApplication::globalStrut()}{global strut} or the
    \l{fontMetrics()}{font metrics} size.

    This property is honored by all \l{ResizeMode}{resize modes}.

    \sa setResizeMode() defaultSectionSize
*/
int QHeaderView::minimumSectionSize() const
{
    Q_D(const QHeaderView);
    if (d->minimumSectionSize == -1) {
        QSize strut = QApplication::globalStrut();
        int margin = style()->pixelMetric(QStyle::PM_HeaderMargin, 0, this);
        if (d->orientation == Qt::Horizontal)
            return qMax(strut.width(), (fontMetrics().maxWidth() + margin));
        return qMax(strut.height(), (fontMetrics().height() + margin));
    }
    return d->minimumSectionSize;
}

void QHeaderView::setMinimumSectionSize(int size)
{
    Q_D(QHeaderView);
    d->minimumSectionSize = size;
}

/*!
    \since 4.1
    \property QHeaderView::defaultAlignment
    \brief the default alignment of the text in each header section
*/

Qt::Alignment QHeaderView::defaultAlignment() const
{
    Q_D(const QHeaderView);
    return d->defaultAlignment;
}

void QHeaderView::setDefaultAlignment(Qt::Alignment alignment)
{
    Q_D(QHeaderView);
    if (d->defaultAlignment == alignment)
        return;

    d->defaultAlignment = alignment;
    d->viewport->update();
}

/*!
    \internal
*/
void QHeaderView::doItemsLayout()
{
    initializeSections();
    QAbstractItemView::doItemsLayout();
}

/*!
    Returns true if sections in the header has been moved; otherwise returns
    false;

    \sa moveSection()
*/
bool QHeaderView::sectionsMoved() const
{
    Q_D(const QHeaderView);
    return !d->visualIndices.isEmpty();
}

/*!
    \since 4.1

    Returns true if sections in the header has been hidden; otherwise returns
    false;

    \sa setSectionHidden()
*/
bool QHeaderView::sectionsHidden() const
{
    Q_D(const QHeaderView);
    return !d->hiddenSectionSize.isEmpty();
}

#ifndef QT_NO_DATASTREAM
/*!
    \since 4.3

    Saves the current state of this header view.

    To restore the saved state, pass the return value to restoreState().

    \sa restoreState()
*/
QByteArray QHeaderView::saveState() const
{
    Q_D(const QHeaderView);
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << QHeaderViewPrivate::VersionMarker;
    stream << 0; // current version is 0
    d->write(stream);
    return data;
}

/*!
    \since 4.3
    Restores the \a state of this header view.
    This function returns \c true if the state was restored; otherwise returns
    false.

    \sa saveState()
*/
bool QHeaderView::restoreState(const QByteArray &state)
{
    Q_D(QHeaderView);
    if (state.isEmpty())
        return false;
    QByteArray data = state;
    QDataStream stream(&data, QIODevice::ReadOnly);
    int marker;
    int ver;
    stream >> marker;
    stream >> ver;
    if (stream.status() != QDataStream::Ok
        || marker != QHeaderViewPrivate::VersionMarker
        || ver != 0) // current version is 0
        return false;

    if (d->read(stream)) {
        emit sortIndicatorChanged(d->sortIndicatorSection, d->sortIndicatorOrder );
        d->viewport->update();
        return true;
    }
    return false;
}
#endif // QT_NO_DATASTREAM

/*!
  \reimp
*/
void QHeaderView::reset()
{
    QAbstractItemView::reset();
    // it would be correct to call clear, but some apps rely
    // on the header keeping the sections, even after calling reset
    //d->clear();
    initializeSections();
}

/*!
    Updates the changed header sections with the given \a orientation, from
    \a logicalFirst to \a logicalLast inclusive.
*/
void QHeaderView::headerDataChanged(Qt::Orientation orientation, int logicalFirst, int logicalLast)
{
    Q_D(QHeaderView);
    if (d->orientation != orientation)
        return;

    if (logicalFirst < 0 || logicalLast < 0 || logicalFirst >= count() || logicalLast >= count())
        return;

    d->invalidateCachedSizeHint();

    int firstVisualIndex = INT_MAX, lastVisualIndex = -1;

    for (int section = logicalFirst; section <= logicalLast; ++section) {
        const int visual = visualIndex(section);
        firstVisualIndex = qMin(firstVisualIndex, visual);
        lastVisualIndex =  qMax(lastVisualIndex,  visual);
    }

    d->executePostedResize();
    const int first = d->headerSectionPosition(firstVisualIndex),
              last = d->headerSectionPosition(lastVisualIndex)
                        + d->headerSectionSize(lastVisualIndex);

    if (orientation == Qt::Horizontal) {
        d->viewport->update(first, 0, last - first, d->viewport->height());
    } else {
        d->viewport->update(0, first, d->viewport->width(), last - first);
    }
}

/*!
    \internal
    \since 4.2

    Updates the section specified by the given \a logicalIndex.
*/

void QHeaderView::updateSection(int logicalIndex)
{
    Q_D(QHeaderView);
    if (d->orientation == Qt::Horizontal)
        d->viewport->update(QRect(sectionViewportPosition(logicalIndex),
                                  0, sectionSize(logicalIndex), d->viewport->height()));
    else
        d->viewport->update(QRect(0, sectionViewportPosition(logicalIndex),
                                  d->viewport->width(), sectionSize(logicalIndex)));
}

/*!
    Resizes the sections according to their size hints. Normally, you do not
    have to call this function.
*/

void QHeaderView::resizeSections()
{
    Q_D(QHeaderView);
    if (d->hasAutoResizeSections())
        d->resizeSections(Interactive, false); // no global resize mode
}

/*!
    This slot is called when sections are inserted into the \a parent.
    \a logicalFirst and \a logicalLast indices signify where the new sections
    were inserted.

    If only one section is inserted, \a logicalFirst and \a logicalLast will
    be the same.
*/

void QHeaderView::sectionsInserted(const QModelIndex &parent,
                                   int logicalFirst, int logicalLast)
{
    Q_D(QHeaderView);
    if (parent != d->root)
        return; // we only handle changes in the top level
    int oldCount = d->sectionCount;

    d->invalidateCachedSizeHint();

    // add the new sections
    int insertAt = 0;
    for (int spanStart = 0; insertAt < d->sectionSpans.count() && spanStart < logicalFirst; ++insertAt)
        spanStart += d->sectionSpans.at(insertAt).count;

    int insertCount = logicalLast - logicalFirst + 1;
    d->sectionCount += insertCount;

    if (d->sectionSpans.isEmpty() || insertAt >= d->sectionSpans.count()) {
        int insertLength = d->defaultSectionSize * insertCount;
        d->length += insertLength;
        QHeaderViewPrivate::SectionSpan span(insertLength, insertCount, d->globalResizeMode);
        d->sectionSpans.append(span);
    } else if ((d->sectionSpans.at(insertAt).sectionSize() == d->defaultSectionSize)
               && d->sectionSpans.at(insertAt).resizeMode == d->globalResizeMode) {
        // add the new sections to an existing span
        int insertLength = d->sectionSpans.at(insertAt).sectionSize() * insertCount;
        d->length += insertLength;
        d->sectionSpans[insertAt].size += insertLength;
        d->sectionSpans[insertAt].count += insertCount;
    } else {
        // separate them out into their own span
        int insertLength = d->defaultSectionSize * insertCount;
        d->length += insertLength;
        QHeaderViewPrivate::SectionSpan span(insertLength, insertCount, d->globalResizeMode);
        d->sectionSpans.insert(insertAt, span);
    }

    // update sorting column
    if (d->sortIndicatorSection >= logicalFirst)
        d->sortIndicatorSection += insertCount;

    // update resize mode section counts
    if (d->globalResizeMode == Stretch)
        d->stretchSections = d->sectionCount;
    else if (d->globalResizeMode == ResizeToContents)
        d->contentsSections = d->sectionCount;

    // clear selection cache
    d->sectionSelected.clear();

    // update mapping
    if (!d->visualIndices.isEmpty() && !d->logicalIndices.isEmpty()) {
        Q_ASSERT(d->visualIndices.count() == d->logicalIndices.count());
        int mappingCount = d->visualIndices.count();
        for (int i = 0; i < mappingCount; ++i) {
            if (d->visualIndices.at(i) >= logicalFirst)
               d->visualIndices[i] += insertCount;
            if (d->logicalIndices.at(i) >= logicalFirst)
                d->logicalIndices[i] += insertCount;
        }
        for (int j = logicalFirst; j <= logicalLast; ++j) {
            d->visualIndices.insert(j, j);
            d->logicalIndices.insert(j, j);
        }
    }

    // insert sections into sectionsHidden
    if (!d->sectionHidden.isEmpty()) {
        QBitArray sectionHidden(d->sectionHidden);
        sectionHidden.resize(sectionHidden.count() + insertCount);
        sectionHidden.fill(false, logicalFirst, logicalLast + 1);
        for (int j = logicalLast + 1; j < sectionHidden.count(); ++j)
            //here we simply copy the old sectionHidden
            sectionHidden.setBit(j, d->sectionHidden.testBit(j - insertCount));
        d->sectionHidden = sectionHidden;
    }

    // insert sections into hiddenSectionSize
    QHash<int, int> newHiddenSectionSize; // from logical index to section size
    for (int i = 0; i < logicalFirst; ++i)
        if (isSectionHidden(i))
            newHiddenSectionSize[i] = d->hiddenSectionSize[i];
    for (int j = logicalLast + 1; j < d->sectionCount; ++j)
        if (isSectionHidden(j))
            newHiddenSectionSize[j] = d->hiddenSectionSize[j - insertCount];
    d->hiddenSectionSize = newHiddenSectionSize;

    d->doDelayedResizeSections();
    emit sectionCountChanged(oldCount, count());

    // if the new sections were not updated by resizing, we need to update now
    if (!d->hasAutoResizeSections())
        d->viewport->update();
}

/*!
    This slot is called when sections are removed from the \a parent.
    \a logicalFirst and \a logicalLast signify where the sections were removed.

    If only one section is removed, \a logicalFirst and \a logicalLast will
    be the same.
*/

void QHeaderView::sectionsAboutToBeRemoved(const QModelIndex &parent,
                                           int logicalFirst, int logicalLast)
{
    Q_UNUSED(parent);
    Q_UNUSED(logicalFirst);
    Q_UNUSED(logicalLast);
}

void QHeaderViewPrivate::updateHiddenSections(int logicalFirst, int logicalLast)
{
    Q_Q(QHeaderView);
    const int changeCount = logicalLast - logicalFirst + 1;

    // remove sections from hiddenSectionSize
    QHash<int, int> newHiddenSectionSize; // from logical index to section size
    for (int i = 0; i < logicalFirst; ++i)
        if (q->isSectionHidden(i))
            newHiddenSectionSize[i] = hiddenSectionSize[i];
    for (int j = logicalLast + 1; j < sectionCount; ++j)
        if (q->isSectionHidden(j))
            newHiddenSectionSize[j - changeCount] = hiddenSectionSize[j];
    hiddenSectionSize = newHiddenSectionSize;

    // remove sections from sectionsHidden
    if (!sectionHidden.isEmpty()) {
        const int newsize = qMin(sectionCount - changeCount, sectionHidden.size());
        QBitArray newSectionHidden(newsize);
        for (int j = 0, k = 0; j < sectionHidden.size(); ++j) {
            const int logical = logicalIndex(j);
            if (logical < logicalFirst || logical > logicalLast) {
                newSectionHidden[k++] = sectionHidden[j];
            }
        }
        sectionHidden = newSectionHidden;
    }
}

void QHeaderViewPrivate::_q_sectionsRemoved(const QModelIndex &parent,
                                            int logicalFirst, int logicalLast)
{
    Q_Q(QHeaderView);
    if (parent != root)
        return; // we only handle changes in the top level
    if (qMin(logicalFirst, logicalLast) < 0
        || qMax(logicalLast, logicalFirst) >= sectionCount)
        return;
    int oldCount = q->count();
    int changeCount = logicalLast - logicalFirst + 1;

    updateHiddenSections(logicalFirst, logicalLast);

    if (visualIndices.isEmpty() && logicalIndices.isEmpty()) {
        //Q_ASSERT(headerSectionCount() == sectionCount);
        removeSectionsFromSpans(logicalFirst, logicalLast);
    } else {
        for (int l = logicalLast; l >= logicalFirst; --l) {
            int visual = visualIndices.at(l);
            for (int v = 0; v < sectionCount; ++v) {
                if (v >= logicalIndices.count())
                    continue; // the section doesn't exist
                if (v > visual) {
                    int logical = logicalIndices.at(v);
                    --(visualIndices[logical]);
                }
                if (logicalIndex(v) > l) // no need to move the positions before l
                    --(logicalIndices[v]);
            }
            logicalIndices.remove(visual);
            visualIndices.remove(l);
            //Q_ASSERT(headerSectionCount() == sectionCount);
            removeSectionsFromSpans(visual, visual);
        }
        // ### handle sectionSelection, sectionHidden
    }
    sectionCount -= changeCount;

    // update sorting column
    if (sortIndicatorSection >= logicalFirst) {
        if (sortIndicatorSection <= logicalLast)
            sortIndicatorSection = -1;
        else
            sortIndicatorSection -= changeCount;
    }

    // if we only have the last section (the "end" position) left, the header is empty
    if (sectionCount <= 0)
        clear();
    invalidateCachedSizeHint();
    emit q->sectionCountChanged(oldCount, q->count());
    viewport->update();
}

void QHeaderViewPrivate::_q_layoutAboutToBeChanged()
{
    //if there is no row/column we can't have mapping for columns
    //because no QModelIndex in the model would be valid
    // ### this is far from being bullet-proof and we would need a real system to 
    // ### map columns or rows persistently
    if ((orientation == Qt::Horizontal && model->rowCount(root) == 0)
        || model->columnCount(root) == 0)
        return;

    for (int i = 0; i < sectionHidden.count(); ++i)
        if (sectionHidden.testBit(i)) // ### note that we are using column or row 0
            persistentHiddenSections.append(orientation == Qt::Horizontal
                                            ? model->index(0, logicalIndex(i), root)
                                            : model->index(logicalIndex(i), 0, root));
}

void QHeaderViewPrivate::_q_layoutChanged()
{
    Q_Q(QHeaderView);
    viewport->update();
    if (persistentHiddenSections.isEmpty() || modelIsEmpty()) {
        if (modelSectionCount() != sectionCount)
            q->initializeSections();
        persistentHiddenSections.clear();
        return;
    }

    QBitArray oldSectionHidden = sectionHidden;
    bool sectionCountChanged = false;

    for (int i = 0; i < persistentHiddenSections.count(); ++i) {
        QModelIndex index = persistentHiddenSections.at(i);
        if (index.isValid()) {
            const int logical = (orientation == Qt::Horizontal
                                 ? index.column()
                                 : index.row());
            q->setSectionHidden(logical, true);
            oldSectionHidden.setBit(logical, false);
        } else if (!sectionCountChanged && (modelSectionCount() != sectionCount)) {
            sectionCountChanged = true;
            break;
        }
    }
    persistentHiddenSections.clear();

    for (int i = 0; i < oldSectionHidden.count(); ++i) {
        if (oldSectionHidden.testBit(i))
            q->setSectionHidden(i, false);
    }

    // the number of sections changed; we need to reread the state of the model
    if (sectionCountChanged)
        q->initializeSections();
}

/*!
  \internal
*/

void QHeaderView::initializeSections()
{
    Q_D(QHeaderView);
    const int oldCount = d->sectionCount;
    const int newCount = d->modelSectionCount();
    if (newCount <= 0) {
            d->clear();
            emit sectionCountChanged(oldCount, 0);
    } else if (newCount != oldCount) {
        const int min = qBound(0, oldCount, newCount - 1);
        initializeSections(min, newCount - 1);
        if (stretchLastSection()) // we've already gotten the size hint
            d->lastSectionSize = sectionSize(logicalIndex(d->sectionCount - 1));

        //make sure we update the hidden sections
        if (newCount < oldCount)
            d->updateHiddenSections(0, newCount-1);
    }
}

/*!
    \internal
*/

void QHeaderView::initializeSections(int start, int end)
{
    Q_D(QHeaderView);

    Q_ASSERT(start >= 0);
    Q_ASSERT(end >= 0);

    d->invalidateCachedSizeHint();

    if (end + 1 < d->sectionCount) {
        int newCount = end + 1;
        d->removeSectionsFromSpans(newCount, d->sectionCount);
        if (!d->hiddenSectionSize.isEmpty()) {
            if (d->sectionCount - newCount > d->hiddenSectionSize.count()) {
                for (int i = end + 1; i < d->sectionCount; ++i)
                    d->hiddenSectionSize.remove(i);
            } else {
                QHash<int, int>::iterator it = d->hiddenSectionSize.begin();
                while (it != d->hiddenSectionSize.end()) {
                    if (it.key() > end)
                        it = d->hiddenSectionSize.erase(it);
                    else
                        ++it;
                }
            }
        }
    }

    int oldCount = d->sectionCount;
    d->sectionCount = end + 1;

    if (!d->logicalIndices.isEmpty()) {
        if (oldCount <= d->sectionCount) {
            d->logicalIndices.resize(d->sectionCount);
            d->visualIndices.resize(d->sectionCount);
            for (int i = oldCount; i < d->sectionCount; ++i) {
                d->logicalIndices[i] = i;
                d->visualIndices[i] = i;
            }
        } else {
            int j = 0;
            for (int i = 0; i < oldCount; ++i) {
                int v = d->logicalIndices.at(i);
                if (v < d->sectionCount) {
                    d->logicalIndices[j] = v;
                    d->visualIndices[v] = j;
                    j++;
                }
            }
            d->logicalIndices.resize(d->sectionCount);
            d->visualIndices.resize(d->sectionCount);
        }
    }

    if (d->globalResizeMode == Stretch)
        d->stretchSections = d->sectionCount;
    else if (d->globalResizeMode == ResizeToContents)
         d->contentsSections = d->sectionCount;
    if (!d->sectionHidden.isEmpty())
        d->sectionHidden.resize(d->sectionCount);

    if (d->sectionCount > oldCount)
        d->createSectionSpan(start, end, (end - start + 1) * d->defaultSectionSize, d->globalResizeMode);
    //Q_ASSERT(d->headerLength() == d->length);

    if (d->sectionCount != oldCount)
        emit sectionCountChanged(oldCount,  d->sectionCount);
    d->viewport->update();
}

/*!
  \reimp
*/

void QHeaderView::currentChanged(const QModelIndex &current, const QModelIndex &old)
{
    Q_D(QHeaderView);

    if (d->orientation == Qt::Horizontal && current.column() != old.column()) {
        if (old.isValid() && old.parent() == d->root)
            d->viewport->update(QRect(sectionViewportPosition(old.column()), 0,
                                    sectionSize(old.column()), d->viewport->height()));
        if (current.isValid() && current.parent() == d->root)
            d->viewport->update(QRect(sectionViewportPosition(current.column()), 0,
                                    sectionSize(current.column()), d->viewport->height()));
    } else if (d->orientation == Qt::Vertical && current.row() != old.row()) {
        if (old.isValid() && old.parent() == d->root)
            d->viewport->update(QRect(0, sectionViewportPosition(old.row()),
                                    d->viewport->width(), sectionSize(old.row())));
        if (current.isValid() && current.parent() == d->root)
            d->viewport->update(QRect(0, sectionViewportPosition(current.row()),
                                    d->viewport->width(), sectionSize(current.row())));
    }
}


/*!
  \reimp
*/

bool QHeaderView::event(QEvent *e)
{
    Q_D(QHeaderView);
    switch (e->type()) {
    case QEvent::HoverEnter: {
        QHoverEvent *he = static_cast<QHoverEvent*>(e);
        d->hover = logicalIndexAt(he->pos());
        if (d->hover != -1)
            updateSection(d->hover);
        break; }
    case QEvent::Leave:
    case QEvent::HoverLeave: {
        if (d->hover != -1)
            updateSection(d->hover);
        d->hover = -1;
        break; }
    case QEvent::HoverMove: {
        QHoverEvent *he = static_cast<QHoverEvent*>(e);
        int oldHover = d->hover;
        d->hover = logicalIndexAt(he->pos());
        if (d->hover != oldHover) {
            if (oldHover != -1)
                updateSection(oldHover);
            if (d->hover != -1)
                updateSection(d->hover);
        }
        break; }
    case QEvent::Timer: {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        if (te->timerId() == d->delayedResize.timerId()) {
            d->delayedResize.stop();
            resizeSections();
        }
        break; }
    default:
        break;
    }
    return QAbstractItemView::event(e);
}

/*!
  \reimp
*/

void QHeaderView::paintEvent(QPaintEvent *e)
{
    Q_D(QHeaderView);

    if (count() == 0)
        return;

    QPainter painter(d->viewport);
    const QPoint offset = d->scrollDelayOffset;
    QRect translatedEventRect = e->rect();
    translatedEventRect.translate(offset);

    int start = -1;
    int end = -1;
    if (d->orientation == Qt::Horizontal) {
        start = visualIndexAt(translatedEventRect.left());
        end = visualIndexAt(translatedEventRect.right());
    } else {
        start = visualIndexAt(translatedEventRect.top());
        end = visualIndexAt(translatedEventRect.bottom());
    }

    if (d->reverse()) {
        start = (start == -1 ? count() - 1 : start);
        end = (end == -1 ? 0 : end);
    } else {
        start = (start == -1 ? 0 : start);
        end = (end == -1 ? count() - 1 : end);
    }

    int tmp = start;
    start = qMin(start, end);
    end = qMax(tmp, end);

    d->prepareSectionSelected(); // clear and resize the bit array

    QRect currentSectionRect;
    int logical;
    const int width = d->viewport->width();
    const int height = d->viewport->height();
    for (int i = start; i <= end; ++i) {
        if (d->isVisualIndexHidden(i))
            continue;
        painter.save();
        logical = logicalIndex(i);
        if (d->orientation == Qt::Horizontal) {
            currentSectionRect.setRect(sectionViewportPosition(logical), 0, sectionSize(logical), height);
        } else {
            currentSectionRect.setRect(0, sectionViewportPosition(logical), width, sectionSize(logical));
        }
        currentSectionRect.translate(offset);

        QVariant variant = d->model->headerData(logical, d->orientation,
                                                Qt::FontRole);
        if (variant.isValid() && variant.canConvert<QFont>()) {
            QFont sectionFont = qvariant_cast<QFont>(variant);
            painter.setFont(sectionFont);
        }
        paintSection(&painter, currentSectionRect, logical);
        painter.restore();
    }

    QStyleOption opt;
    opt.init(this);
    // Paint the area beyond where there are indexes
    if (d->reverse()) {
        opt.state |= QStyle::State_Horizontal;
        if (currentSectionRect.left() > translatedEventRect.left()) {
            opt.rect = QRect(translatedEventRect.left(), 0,
                             currentSectionRect.left() - translatedEventRect.left(), height);
            style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, &painter, this);
        }
    } else if (currentSectionRect.right() < translatedEventRect.right()) {
        // paint to the right
        opt.state |= QStyle::State_Horizontal;
        opt.rect = QRect(currentSectionRect.right() + 1, 0,
                         translatedEventRect.right() - currentSectionRect.right(), height);
        style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, &painter, this);
    } else if (currentSectionRect.bottom() < translatedEventRect.bottom()) {
        // paint the bottom section
        opt.state &= ~QStyle::State_Horizontal;
        opt.rect = QRect(0, currentSectionRect.bottom() + 1,
                         width, height - currentSectionRect.bottom() - 1);
        style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, &painter, this);
    }

#if 0
    // ### visualize section spans
    for (int a = 0, i = 0; i < d->sectionSpans.count(); ++i) {
        QColor color((i & 4 ? 255 : 0), (i & 2 ? 255 : 0), (i & 1 ? 255 : 0));
        if (d->orientation == Qt::Horizontal)
            painter.fillRect(a - d->offset, 0, d->sectionSpans.at(i).size, 4, color);
        else
            painter.fillRect(0, a - d->offset, 4, d->sectionSpans.at(i).size, color);
        a += d->sectionSpans.at(i).size;
    }

#endif
}

/*!
  \reimp
*/

void QHeaderView::mousePressEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    if (d->state != QHeaderViewPrivate::NoState || e->button() != Qt::LeftButton)
        return;
    int pos = d->orientation == Qt::Horizontal ? e->x() : e->y();
    int handle = d->sectionHandleAt(pos);
    d->originalSize = -1; // clear the stored original size
    if (handle == -1) {
        d->pressed = logicalIndexAt(pos);
        if (d->clickableSections)
            emit sectionPressed(d->pressed);
        if (d->movableSections) {
            d->section = d->target = d->pressed;
            if (d->section == -1)
                return;
            d->state = QHeaderViewPrivate::MoveSection;
            d->setupSectionIndicator(d->section, pos);
        } else if (d->clickableSections && d->pressed != -1) {
            updateSection(d->pressed);
            d->state = QHeaderViewPrivate::SelectSections;
        }
    } else if (resizeMode(handle) == Interactive) {
        d->originalSize = sectionSize(handle);
        d->state = QHeaderViewPrivate::ResizeSection;
        d->section = handle;
    }

    d->firstPos = pos;
    d->lastPos = pos;

    d->clearCascadingSections();
}

/*!
  \reimp
*/

void QHeaderView::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    int pos = d->orientation == Qt::Horizontal ? e->x() : e->y();
    if (pos < 0)
        return;
    if (e->buttons() == Qt::NoButton) {
#if !defined(Q_WS_MAC)
        // Under Cocoa, when the mouse button is released, may include an extra
        // simulated mouse moved event. The state of the buttons when this event
        // is generated is already "no button" and the code below gets executed
        // just before the mouseReleaseEvent and resets the state. This prevents
        // column dragging from working. So this code is disabled under Cocoa.
        d->state = QHeaderViewPrivate::NoState;
        d->pressed = -1;
#endif
    }
    switch (d->state) {
        case QHeaderViewPrivate::ResizeSection: {
            Q_ASSERT(d->originalSize != -1);
            if (d->cascadingResizing) {
                int delta = d->reverse() ? d->lastPos - pos : pos - d->lastPos;
                int visual = visualIndex(d->section);
                d->cascadingResize(visual, d->headerSectionSize(visual) + delta);
            } else {
                int delta = d->reverse() ? d->firstPos - pos : pos - d->firstPos;
                resizeSection(d->section, qMax(d->originalSize + delta, minimumSectionSize()));
            }
            d->lastPos = pos;
            return;
        }
        case QHeaderViewPrivate::MoveSection: {
            if (qAbs(pos - d->firstPos) >= QApplication::startDragDistance()
                || !d->sectionIndicator->isHidden()) {
                int visual = visualIndexAt(pos);
                if (visual == -1)
                    return;
                int posThreshold = d->headerSectionPosition(visual) + d->headerSectionSize(visual) / 2;
                int moving = visualIndex(d->section);
                if (visual < moving) {
                    if (pos < posThreshold)
                        d->target = d->logicalIndex(visual);
                    else
                        d->target = d->logicalIndex(visual + 1);
                } else if (visual > moving) {
                    if (pos > posThreshold)
                        d->target = d->logicalIndex(visual);
                    else
                        d->target = d->logicalIndex(visual - 1);
                } else {
                    d->target = d->section;
                }
                d->updateSectionIndicator(d->section, pos);
            }
            return;
        }
        case QHeaderViewPrivate::SelectSections: {
            int logical = logicalIndexAt(pos);
            if (logical == d->pressed)
                return; // nothing to do
            else if (d->pressed != -1)
                updateSection(d->pressed);
            d->pressed = logical;
            if (d->clickableSections && logical != -1) {
                emit sectionEntered(d->pressed);
                updateSection(d->pressed);
            }
            return;
        }
        case QHeaderViewPrivate::NoState: {
#ifndef QT_NO_CURSOR
            int handle = d->sectionHandleAt(pos);
            bool hasCursor = testAttribute(Qt::WA_SetCursor);
            if (handle != -1 && (resizeMode(handle) == Interactive)) {
                if (!hasCursor)
                    setCursor(d->orientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
            } else if (hasCursor) {
                unsetCursor();
            }
#endif
            return;
        }
        default:
            break;
    }
}

/*!
  \reimp
*/

void QHeaderView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    int pos = d->orientation == Qt::Horizontal ? e->x() : e->y();
    switch (d->state) {
    case QHeaderViewPrivate::MoveSection:
        if (!d->sectionIndicator->isHidden()) { // moving
            int from = visualIndex(d->section);
            Q_ASSERT(from != -1);
            int to = visualIndex(d->target);
            Q_ASSERT(to != -1);
            moveSection(from, to);
            d->section = d->target = -1;
            d->updateSectionIndicator(d->section, pos);
            break;
        } // not moving
    case QHeaderViewPrivate::SelectSections:
        if (!d->clickableSections) {
            int section = logicalIndexAt(pos);
            updateSection(section);
        }
        // fall through
    case QHeaderViewPrivate::NoState:
        if (d->clickableSections) {
            int section = logicalIndexAt(pos);
            if (section != -1 && section == d->pressed) {
                d->flipSortIndicator(section);
                emit sectionClicked(section);
            }
            if (d->pressed != -1)
                updateSection(d->pressed);
        }
        break;
    case QHeaderViewPrivate::ResizeSection:
        d->originalSize = -1;
        d->clearCascadingSections();
        break;
    default:
        break;
    }
    d->state = QHeaderViewPrivate::NoState;
    d->pressed = -1;
}

/*!
  \reimp
*/

void QHeaderView::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    int pos = d->orientation == Qt::Horizontal ? e->x() : e->y();
    int handle = d->sectionHandleAt(pos);
    if (handle > -1 && resizeMode(handle) == Interactive) {
        emit sectionHandleDoubleClicked(handle);
#ifndef QT_NO_CURSOR
        Qt::CursorShape splitCursor = (d->orientation == Qt::Horizontal)
                                      ? Qt::SplitHCursor : Qt::SplitVCursor;
        if (cursor().shape() == splitCursor) {
            // signal handlers may have changed the section size
            handle = d->sectionHandleAt(pos);
            if (!(handle > -1 && resizeMode(handle) == Interactive))
                setCursor(Qt::ArrowCursor);
        }
#endif
    } else {
        emit sectionDoubleClicked(logicalIndexAt(e->pos()));
    }
}

/*!
  \reimp
*/

bool QHeaderView::viewportEvent(QEvent *e)
{
    Q_D(QHeaderView);
    switch (e->type()) {
#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1) {
            QVariant variant = d->model->headerData(logical, d->orientation, Qt::ToolTipRole);
            if (variant.isValid()) {
                QToolTip::showText(he->globalPos(), variant.toString(), this);
                return true;
            }
        }
        break; }
#endif
#ifndef QT_NO_WHATSTHIS
    case QEvent::QueryWhatsThis: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1
            && d->model->headerData(logical, d->orientation, Qt::WhatsThisRole).isValid())
            return true;
        break; }
    case QEvent::WhatsThis: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1) {
             QVariant whatsthis = d->model->headerData(logical, d->orientation,
                                                      Qt::WhatsThisRole);
             if (whatsthis.isValid()) {
                 QWhatsThis::showText(he->globalPos(), whatsthis.toString(), this);
                 return true;
             }
        }
        break; }
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_STATUSTIP
    case QEvent::StatusTip: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1) {
            QString statustip = d->model->headerData(logical, d->orientation,
                                                    Qt::StatusTipRole).toString();
            if (!statustip.isEmpty())
                setStatusTip(statustip);
        }
        return true; }
#endif // QT_NO_STATUSTIP
    case QEvent::Hide:
    case QEvent::Show:
    case QEvent::FontChange:
    case QEvent::StyleChange:
        d->invalidateCachedSizeHint();
        resizeSections();
        emit geometriesChanged();
        break;
    case QEvent::ContextMenu: {
        d->state = QHeaderViewPrivate::NoState;
        d->pressed = d->section = d->target = -1;
        d->updateSectionIndicator(d->section, -1);
        break; }
    case QEvent::Wheel: {
        QAbstractScrollArea *asa = qobject_cast<QAbstractScrollArea *>(parentWidget());
        if (asa)
            return QApplication::sendEvent(asa->viewport(), e);
        break; }
    default:
        break;
    }
    return QAbstractItemView::viewportEvent(e);
}

/*!
    Paints the section specified by the given \a logicalIndex, using the given
    \a painter and \a rect.

    Normally, you do not have to call this function.
*/

void QHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (!rect.isValid())
        return;
    // get the state of the section
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    QStyle::State state = QStyle::State_None;
    if (isEnabled())
        state |= QStyle::State_Enabled;
    if (window()->isActiveWindow())
        state |= QStyle::State_Active;
    if (d->clickableSections) {
        if (logicalIndex == d->hover)
            state |= QStyle::State_MouseOver;
        if (logicalIndex == d->pressed)
            state |= QStyle::State_Sunken;
        else if (d->highlightSelected) {
            if (d->sectionIntersectsSelection(logicalIndex))
                state |= QStyle::State_On;
            if (d->isSectionSelected(logicalIndex))
                state |= QStyle::State_Sunken;
        }

    }
    if (isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
                            ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

    // setup the style options structure
    QVariant textAlignment = d->model->headerData(logicalIndex, d->orientation,
                                                  Qt::TextAlignmentRole);
    opt.rect = rect;
    opt.section = logicalIndex;
    opt.state |= state;
    opt.textAlignment = Qt::Alignment(textAlignment.isValid()
                                      ? Qt::Alignment(textAlignment.toInt())
                                      : d->defaultAlignment);

    opt.iconAlignment = Qt::AlignVCenter;
    opt.text = d->model->headerData(logicalIndex, d->orientation,
                                    Qt::DisplayRole).toString();
    if (d->textElideMode != Qt::ElideNone)
        opt.text = opt.fontMetrics.elidedText(opt.text, d->textElideMode , rect.width() - 4);

    QVariant variant = d->model->headerData(logicalIndex, d->orientation,
                                    Qt::DecorationRole);
    opt.icon = qvariant_cast<QIcon>(variant);
    if (opt.icon.isNull())
        opt.icon = qvariant_cast<QPixmap>(variant);
    QVariant foregroundBrush = d->model->headerData(logicalIndex, d->orientation,
                                                    Qt::ForegroundRole);
    if (foregroundBrush.canConvert<QBrush>())
        opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));

    QPointF oldBO = painter->brushOrigin();
    QVariant backgroundBrush = d->model->headerData(logicalIndex, d->orientation,
                                                    Qt::BackgroundRole);
    if (backgroundBrush.canConvert<QBrush>()) {
        opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
        opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
        painter->setBrushOrigin(opt.rect.topLeft());
    }

    // the section position
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);
    if (count() == 1)
        opt.position = QStyleOptionHeader::OnlyOneSection;
    else if (visual == 0)
        opt.position = QStyleOptionHeader::Beginning;
    else if (visual == count() - 1)
        opt.position = QStyleOptionHeader::End;
    else
        opt.position = QStyleOptionHeader::Middle;
    opt.orientation = d->orientation;
    // the selected position
    bool previousSelected = d->isSectionSelected(this->logicalIndex(visual - 1));
    bool nextSelected =  d->isSectionSelected(this->logicalIndex(visual + 1));
    if (previousSelected && nextSelected)
        opt.selectedPosition = QStyleOptionHeader::NextAndPreviousAreSelected;
    else if (previousSelected)
        opt.selectedPosition = QStyleOptionHeader::PreviousIsSelected;
    else if (nextSelected)
        opt.selectedPosition = QStyleOptionHeader::NextIsSelected;
    else
        opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
    // draw the section
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);

    painter->setBrushOrigin(oldBO);
}

/*!
    Returns the size of the contents of the section specified by the given
    \a logicalIndex.

    \sa defaultSectionSize()
*/

QSize QHeaderView::sectionSizeFromContents(int logicalIndex) const
{
    Q_D(const QHeaderView);
    Q_ASSERT(logicalIndex >= 0);

    ensurePolished();

    // use SizeHintRole
    QVariant variant = d->model->headerData(logicalIndex, d->orientation, Qt::SizeHintRole);
    if (variant.isValid())
        return qvariant_cast<QSize>(variant);

    // otherwise use the contents
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.section = logicalIndex;
    QVariant var = d->model->headerData(logicalIndex, d->orientation,
                                            Qt::FontRole);
    QFont fnt;
    if (var.isValid() && var.canConvert<QFont>())
        fnt = qvariant_cast<QFont>(var);
    else
        fnt = font();
    fnt.setBold(true);
    opt.fontMetrics = QFontMetrics(fnt);
    opt.text = d->model->headerData(logicalIndex, d->orientation,
                                    Qt::DisplayRole).toString();
    variant = d->model->headerData(logicalIndex, d->orientation, Qt::DecorationRole);
    opt.icon = qvariant_cast<QIcon>(variant);
    if (opt.icon.isNull())
        opt.icon = qvariant_cast<QPixmap>(variant);
    QSize size = style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize(), this);
    if (isSortIndicatorShown()) {
        int margin = style()->pixelMetric(QStyle::PM_HeaderMargin, &opt, this);
        if (d->orientation == Qt::Horizontal)
            size.rwidth() += size.height() + margin;
        else
            size.rheight() += size.width() + margin;
    }
    return size;
}

/*!
    Returns the horizontal offset of the header. This is 0 for vertical
    headers.

    \sa offset()
*/

int QHeaderView::horizontalOffset() const
{
    Q_D(const QHeaderView);
    if (d->orientation == Qt::Horizontal)
        return d->offset;
    return 0;
}

/*!
    Returns the vertical offset of the header. This is 0 for horizontal
    headers.

    \sa offset()
*/

int QHeaderView::verticalOffset() const
{
    Q_D(const QHeaderView);
    if (d->orientation == Qt::Vertical)
        return d->offset;
    return 0;
}

/*!
    \reimp
    \internal
*/

void QHeaderView::updateGeometries()
{
    Q_D(QHeaderView);
    d->layoutChildren();
    if (d->hasAutoResizeSections())
        d->doDelayedResizeSections();
}

/*!
    \reimp
    \internal
*/

void QHeaderView::scrollContentsBy(int dx, int dy)
{
    Q_D(QHeaderView);
    d->scrollDirtyRegion(dx, dy);
}

/*!
    \reimp
    \internal
*/
void QHeaderView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_D(QHeaderView);
    d->invalidateCachedSizeHint();
    if (d->hasAutoResizeSections()) {
        bool resizeRequired = d->globalResizeMode == ResizeToContents;
        int first = orientation() == Qt::Horizontal ? topLeft.column() : topLeft.row();
        int last = orientation() == Qt::Horizontal ? bottomRight.column() : bottomRight.row();
        for (int i = first; i <= last && !resizeRequired; ++i)
            resizeRequired = (resizeMode(i) == ResizeToContents);
        if (resizeRequired)
            d->doDelayedResizeSections();
    }
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/
void QHeaderView::rowsInserted(const QModelIndex &, int, int)
{
    // do nothing
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

QRect QHeaderView::visualRect(const QModelIndex &) const
{
    return QRect();
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

void QHeaderView::scrollTo(const QModelIndex &, ScrollHint)
{
    // do nothing - the header only displays sections
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

QModelIndex QHeaderView::indexAt(const QPoint &) const
{
    return QModelIndex();
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

bool QHeaderView::isIndexHidden(const QModelIndex &) const
{
    return true; // the header view has no items, just sections
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

QModelIndex QHeaderView::moveCursor(CursorAction, Qt::KeyboardModifiers)
{
    return QModelIndex();
}

/*!
    \reimp

    Selects the items in the given \a rect according to the specified
    \a flags.

    The base class implementation does nothing.
*/

void QHeaderView::setSelection(const QRect&, QItemSelectionModel::SelectionFlags)
{
    // do nothing
}

/*!
    \internal
*/

QRegion QHeaderView::visualRegionForSelection(const QItemSelection &selection) const
{
    Q_D(const QHeaderView);
    const int max = d->modelSectionCount();
    if (d->orientation == Qt::Horizontal) {
        int left = max;
        int right = 0;
        int rangeLeft, rangeRight;

        for (int i = 0; i < selection.count(); ++i) {
            QItemSelectionRange r = selection.at(i);
            if (r.parent().isValid() || !r.isValid())
                continue; // we only know about toplevel items and we don't want invalid ranges
            // FIXME an item inside the range may be the leftmost or rightmost
            rangeLeft = visualIndex(r.left());
            if (rangeLeft == -1) // in some cases users may change the selections
                continue;        // before we have a chance to do the layout
            rangeRight = visualIndex(r.right());
            if (rangeRight == -1) // in some cases users may change the selections
                continue;         // before we have a chance to do the layout
            if (rangeLeft < left)
                left = rangeLeft;
            if (rangeRight > right)
                right = rangeRight;
        }

        int logicalLeft = logicalIndex(left);
        int logicalRight = logicalIndex(right);

        if (logicalLeft < 0  || logicalLeft >= count() ||
            logicalRight < 0 || logicalRight >= count())
            return QRegion();

        int leftPos = sectionViewportPosition(logicalLeft);
        int rightPos = sectionViewportPosition(logicalRight);
        rightPos += sectionSize(logicalRight);
        return QRect(leftPos, 0, rightPos - leftPos, height());
    }
    // orientation() == Qt::Vertical
    int top = max;
    int bottom = 0;
    int rangeTop, rangeBottom;

    for (int i = 0; i < selection.count(); ++i) {
        QItemSelectionRange r = selection.at(i);
        if (r.parent().isValid() || !r.isValid())
            continue; // we only know about toplevel items
        // FIXME an item inside the range may be the leftmost or rightmost
        rangeTop = visualIndex(r.top());
        if (rangeTop == -1) // in some cases users may change the selections
            continue;       // before we have a chance to do the layout
        rangeBottom = visualIndex(r.bottom());
        if (rangeBottom == -1) // in some cases users may change the selections
            continue;          // before we have a chance to do the layout
        if (rangeTop < top)
            top = rangeTop;
        if (rangeBottom > bottom)
            bottom = rangeBottom;
    }

    int logicalTop = logicalIndex(top);
    int logicalBottom = logicalIndex(bottom);

    if (logicalTop == -1 || logicalBottom == -1)
        return QRect();

    int topPos = sectionViewportPosition(logicalTop);
    int bottomPos = sectionViewportPosition(logicalBottom) + sectionSize(logicalBottom);

    return QRect(0, topPos, width(), bottomPos - topPos);
}


// private implementation

int QHeaderViewPrivate::sectionHandleAt(int position)
{
    Q_Q(QHeaderView);
    int visual = q->visualIndexAt(position);
    if (visual == -1)
        return -1;
    int log = logicalIndex(visual);
    int pos = q->sectionViewportPosition(log);
    int grip = q->style()->pixelMetric(QStyle::PM_HeaderGripMargin, 0, q);

    bool atLeft = position < pos + grip;
    bool atRight = (position > pos + q->sectionSize(log) - grip);
    if (reverse())
        qSwap(atLeft, atRight);

    if (atLeft) {
        //grip at the beginning of the section
        while(visual > -1) {
            int logical = q->logicalIndex(--visual);
            if (!q->isSectionHidden(logical))
                return logical;
        }
    } else if (atRight) {
        //grip at the end of the section
        return log;
    }
    return -1;
}

void QHeaderViewPrivate::setupSectionIndicator(int section, int position)
{
    Q_Q(QHeaderView);
    if (!sectionIndicator) {
        sectionIndicator = new QLabel(viewport);
    }

    int w, h;
    int p = q->sectionViewportPosition(section);
    if (orientation == Qt::Horizontal) {
        w = q->sectionSize(section);
        h = viewport->height();
    } else {
        w = viewport->width();
        h = q->sectionSize(section);
    }
    sectionIndicator->resize(w, h);

    QPixmap pm(w, h);
    pm.fill(QColor(0, 0, 0, 45));
    QRect rect(0, 0, w, h);

    QPainter painter(&pm);
    painter.setOpacity(0.75);
    q->paintSection(&painter, rect, section);
    painter.end();

    sectionIndicator->setPixmap(pm);
    sectionIndicatorOffset = position - qMax(p, 0);
}

void QHeaderViewPrivate::updateSectionIndicator(int section, int position)
{
    if (!sectionIndicator)
        return;

    if (section == -1 || target == -1) {
        sectionIndicator->hide();
        return;
    }

    if (orientation == Qt::Horizontal)
        sectionIndicator->move(position - sectionIndicatorOffset, 0);
    else
        sectionIndicator->move(0, position - sectionIndicatorOffset);

    sectionIndicator->show();
}

/*!
    Initialize \a option with the values from this QHeaderView. This method is
    useful for subclasses when they need a QStyleOptionHeader, but do not want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QHeaderView::initStyleOption(QStyleOptionHeader *option) const
{
    Q_D(const QHeaderView);
    option->initFrom(this);
    option->state = QStyle::State_None | QStyle::State_Raised;
    option->orientation = d->orientation;
    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
    if (isEnabled())
        option->state |= QStyle::State_Enabled;
    option->section = 0;
}

bool QHeaderViewPrivate::isSectionSelected(int section) const
{
    int i = section * 2;
    if (i < 0 || i >= sectionSelected.count())
        return false;
    if (sectionSelected.testBit(i)) // if the value was cached
        return sectionSelected.testBit(i + 1);
    bool s = false;
    if (orientation == Qt::Horizontal)
        s = isColumnSelected(section);
    else
        s = isRowSelected(section);
    sectionSelected.setBit(i + 1, s); // selection state
    sectionSelected.setBit(i, true); // cache state
    return s;
}

/*!
    \internal
    Returns the last visible (ie. not hidden) visual index
*/
int QHeaderViewPrivate::lastVisibleVisualIndex() const
{
    Q_Q(const QHeaderView);
    for (int visual = q->count()-1; visual >= 0; --visual) {
        if (!q->isSectionHidden(q->logicalIndex(visual)))
            return visual;
    }

    //default value if no section is actually visible
    return -1;
}

/*!
    \internal
    Go through and resize all of the sections applying stretchLastSection,
    manualy stretches, sizes, and useGlobalMode.

    The different resize modes are:
    Interactive - the user decides the size
    Stretch - take up whatever space is left
    Fixed - the size is set programmatically outside the header
    ResizeToContentes - the size is set based on the contents of the row or column in the parent view

    The resize mode will not affect the last section if stretchLastSection is true.
*/
void QHeaderViewPrivate::resizeSections(QHeaderView::ResizeMode globalMode, bool useGlobalMode)
{
    Q_Q(QHeaderView);
    //stop the timer in case it is delayed
    delayedResize.stop();

    executePostedLayout();
    if (sectionCount == 0)
        return;

    if (resizeRecursionBlock)
        return;
    resizeRecursionBlock = true;

    invalidateCachedSizeHint();

    const int lastVisibleSection = lastVisibleVisualIndex();

    // find stretchLastSection if we have it
    int stretchSection = -1;
    if (stretchLastSection && !useGlobalMode)
        stretchSection = lastVisibleVisualIndex();

    // count up the number of strected sections and how much space left for them
    int lengthToStrech = (orientation == Qt::Horizontal ? viewport->width() : viewport->height());
    int numberOfStretchedSections = 0;
    QList<int> section_sizes;
    for (int i = 0; i < sectionCount; ++i) {
        if (isVisualIndexHidden(i))
            continue;

        QHeaderView::ResizeMode resizeMode;
        if (useGlobalMode && (i != stretchSection))
            resizeMode = globalMode;
        else
            resizeMode = (i == stretchSection ? QHeaderView::Stretch : headerSectionResizeMode(i));

        if (resizeMode == QHeaderView::Stretch) {
            ++numberOfStretchedSections;
            section_sizes.append(headerSectionSize(i));
            continue;
        }

        // because it isn't stretch, determine its width and remove that from lengthToStrech
        int sectionSize = 0;
        if (resizeMode == QHeaderView::Interactive || resizeMode == QHeaderView::Fixed) {
            sectionSize = headerSectionSize(i);
        } else { // resizeMode == QHeaderView::ResizeToContents
            int logicalIndex = q->logicalIndex(i);
            sectionSize = qMax(viewSectionSizeHint(logicalIndex),
                               q->sectionSizeHint(logicalIndex));
        }
        section_sizes.append(sectionSize);
        lengthToStrech -= sectionSize;
    }

    // calculate the new length for all of the stretched sections
    int stretchSectionLength = -1;
    int pixelReminder = 0;
    if (numberOfStretchedSections > 0 && lengthToStrech > 0) { // we have room to stretch in
        int hintLengthForEveryStretchedSection = lengthToStrech / numberOfStretchedSections;
        stretchSectionLength = qMax(hintLengthForEveryStretchedSection, q->minimumSectionSize());
        pixelReminder = lengthToStrech % numberOfStretchedSections;
    }

    int spanStartSection = 0;
    int previousSectionLength = 0;

    QHeaderView::ResizeMode previousSectionResizeMode = QHeaderView::Interactive;

    // resize each section along the total length
    for (int i = 0; i < sectionCount; ++i) {
        int oldSectionLength = headerSectionSize(i);
        int newSectionLength = -1;
        QHeaderView::ResizeMode newSectionResizeMode = headerSectionResizeMode(i);

        if (isVisualIndexHidden(i)) {
            newSectionLength = 0;
        } else {
            QHeaderView::ResizeMode resizeMode;
            if (useGlobalMode)
                resizeMode = globalMode;
            else
                resizeMode = (i == stretchSection
                              ? QHeaderView::Stretch
                              : newSectionResizeMode);
            if (resizeMode == QHeaderView::Stretch && stretchSectionLength != -1) {
                if (i == lastVisibleSection)
                    newSectionLength = qMax(stretchSectionLength, lastSectionSize);
                else
                    newSectionLength = stretchSectionLength;
                if (pixelReminder > 0) {
                    newSectionLength += 1;
                    --pixelReminder;
                }
                section_sizes.removeFirst();
            } else {
                newSectionLength = section_sizes.front();
                section_sizes.removeFirst();
            }
        }

        //Q_ASSERT(newSectionLength > 0);
        if ((previousSectionResizeMode != newSectionResizeMode
            || previousSectionLength != newSectionLength) && i > 0) {
            int spanLength = (i - spanStartSection) * previousSectionLength;
            createSectionSpan(spanStartSection, i - 1, spanLength, previousSectionResizeMode);
            //Q_ASSERT(headerLength() == length);
            spanStartSection = i;
        }

        if (newSectionLength != oldSectionLength)
            emit q->sectionResized(logicalIndex(i), oldSectionLength, newSectionLength);

        previousSectionLength = newSectionLength;
        previousSectionResizeMode = newSectionResizeMode;
    }

    createSectionSpan(spanStartSection, sectionCount - 1,
                      (sectionCount - spanStartSection) * previousSectionLength,
                      previousSectionResizeMode);
    //Q_ASSERT(headerLength() == length);
    resizeRecursionBlock = false;
    viewport->update();
}

void QHeaderViewPrivate::createSectionSpan(int start, int end, int size, QHeaderView::ResizeMode mode)
{
    // ### the code for merging spans does not merge at all opertuneties
    // ### what if the number of sections is reduced ?

    SectionSpan span(size, (end - start) + 1, mode);
    int start_section = 0;
#ifndef QT_NO_DEBUG
    int initial_section_count = headerSectionCount(); // ### debug code
#endif

    QList<int> spansToRemove;
    for (int i = 0; i < sectionSpans.count(); ++i) {
        int end_section = start_section + sectionSpans.at(i).count - 1;
        int section_count = sectionSpans.at(i).count;
        if (start <= start_section && end > end_section) {
            // the existing span is entirely coveded by the new span
            spansToRemove.append(i);
        } else if (start < start_section && end >= end_section) {
            // the existing span is entirely coveded by the new span
            spansToRemove.append(i);
        } else if (start == start_section && end == end_section) {
            // the new span is covered by an existin span
            length -= sectionSpans.at(i).size;
            length += size;
            sectionSpans[i].size = size;
            sectionSpans[i].resizeMode = mode;
            // ### check if we can merge the section with any of its neighbours
            removeSpans(spansToRemove);
            Q_ASSERT(initial_section_count == headerSectionCount());
            return;
        } else if (start > start_section && end < end_section) {
            if (sectionSpans.at(i).sectionSize() == span.sectionSize()
                && sectionSpans.at(i).resizeMode == span.resizeMode) {
                Q_ASSERT(initial_section_count == headerSectionCount());
                return;
            }
            // the new span is in the middle of the old span, so we have to split it
            length -= sectionSpans.at(i).size;
            int section_size = sectionSpans.at(i).sectionSize();
#ifndef QT_NO_DEBUG
            int span_count = sectionSpans.at(i).count;
#endif
            QHeaderView::ResizeMode span_mode = sectionSpans.at(i).resizeMode;
            // first span
            int first_span_count = start - start_section;
            int first_span_size = section_size * first_span_count;
            sectionSpans[i].count = first_span_count;
            sectionSpans[i].size = first_span_size;
            sectionSpans[i].resizeMode = span_mode;
            length += first_span_size;
            // middle span (the new span)
#ifndef QT_NO_DEBUG
            int mid_span_count = span.count;
#endif
            int mid_span_size = span.size;
            sectionSpans.insert(i + 1, span);
            length += mid_span_size;
            // last span
            int last_span_count = end_section - end;
            int last_span_size = section_size * last_span_count;
            sectionSpans.insert(i + 2, SectionSpan(last_span_size, last_span_count, span_mode));
            length += last_span_size;
            Q_ASSERT(span_count == first_span_count + mid_span_count + last_span_count);
            removeSpans(spansToRemove);
            Q_ASSERT(initial_section_count == headerSectionCount());
            return;
        } else if (start > start_section && start <= end_section && end >= end_section) {
            // the new span covers the last part of the existing span
            length -= sectionSpans.at(i).size;
            int removed_count = (end_section - start + 1);
            int span_count = sectionSpans.at(i).count - removed_count;
            int section_size = sectionSpans.at(i).sectionSize();
            int span_size = section_size * span_count;
            sectionSpans[i].count = span_count;
            sectionSpans[i].size = span_size;
            length += span_size;
            if (end == end_section) {
                sectionSpans.insert(i + 1, span); // insert after
                length += span.size;
                removeSpans(spansToRemove);
                Q_ASSERT(initial_section_count == headerSectionCount());
                return;
            }
        } else if (end < end_section && end >= start_section && start <= start_section) {
            // the new span covers the first part of the existing span
            length -= sectionSpans.at(i).size;
            int removed_count = (end - start_section + 1);
            int section_size = sectionSpans.at(i).sectionSize();
            int span_count = sectionSpans.at(i).count - removed_count;
            int span_size = section_size * span_count;
            sectionSpans[i].count = span_count;
            sectionSpans[i].size = span_size;
            length += span_size;
            sectionSpans.insert(i, span); // insert before
            length += span.size;
            removeSpans(spansToRemove);
            Q_ASSERT(initial_section_count == headerSectionCount());
            return;
        }
        start_section += section_count;
    }

    // ### adding and removing _ sections_  in addition to spans
    // ### add some more checks here

    if (spansToRemove.isEmpty()) {
        if (!sectionSpans.isEmpty()
            && sectionSpans.last().sectionSize() == span.sectionSize()
            && sectionSpans.last().resizeMode == span.resizeMode) {
            length += span.size;
            int last = sectionSpans.count() - 1;
            sectionSpans[last].count += span.count;
            sectionSpans[last].size += span.size;
            sectionSpans[last].resizeMode = span.resizeMode;
        } else {
            length += span.size;
            sectionSpans.append(span);
        }
    } else {
        removeSpans(spansToRemove);
        length += span.size;
        sectionSpans.insert(spansToRemove.first(), span);
        //Q_ASSERT(initial_section_count == headerSectionCount());
    }
}

void QHeaderViewPrivate::removeSectionsFromSpans(int start, int end)
{
    // remove sections
    int start_section = 0;
    QList<int> spansToRemove;
    for (int i = 0; i < sectionSpans.count(); ++i) {
        int end_section = start_section + sectionSpans.at(i).count - 1;
        int section_size = sectionSpans.at(i).sectionSize();
        int section_count = sectionSpans.at(i).count;
        if (start <= start_section && end >= end_section) {
            // the change covers the entire span
            spansToRemove.append(i);
            if (end == end_section)
                break;
        } else if (start > start_section && end < end_section) {
            // all the removed sections are inside the span
            int change = (end - start + 1);
            sectionSpans[i].count -= change;
            sectionSpans[i].size = section_size * sectionSpans.at(i).count;
            length -= (change * section_size);
            break;
        } else if (start >= start_section && start <= end_section) {
            // the some of the removed sections are inside the span,at the end
            int change = qMin(end_section - start + 1, end - start + 1);
            sectionSpans[i].count -= change;
            sectionSpans[i].size = section_size * sectionSpans.at(i).count;
            start += change;
            length -= (change * section_size);
            // the change affects several spans
        } else if (end >= start_section && end <= end_section) {
            // the some of the removed sections are inside the span, at the beginning
            int change = qMin((end - start_section + 1), end - start + 1);
            sectionSpans[i].count -= change;
            sectionSpans[i].size = section_size * sectionSpans.at(i).count;
            length -= (change * section_size);
            break;
        }
        start_section += section_count;
    }

    for (int i = spansToRemove.count() - 1; i >= 0; --i) {
        int s = spansToRemove.at(i);
        length -= sectionSpans.at(s).size;
        sectionSpans.remove(s);
        // ### merge remaining spans
    }
}

void QHeaderViewPrivate::clear()
{
    if (state != NoClear) {
    length = 0;
    sectionCount = 0;
    visualIndices.clear();
    logicalIndices.clear();
    sectionSelected.clear();
    sectionHidden.clear();
    hiddenSectionSize.clear();
    sectionSpans.clear();
    }
}

void QHeaderViewPrivate::flipSortIndicator(int section)
{
    Q_Q(QHeaderView);
    Qt::SortOrder sortOrder;
    if (sortIndicatorSection == section) {
        sortOrder = (sortIndicatorOrder == Qt::DescendingOrder) ? Qt::AscendingOrder : Qt::DescendingOrder;
    } else {
        const QVariant value = model->headerData(section, orientation, Qt::InitialSortOrderRole);
        if (value.canConvert(QVariant::Int))
            sortOrder = static_cast<Qt::SortOrder>(value.toInt());
        else
            sortOrder = Qt::AscendingOrder;
    }
    q->setSortIndicator(section, sortOrder);
}

void QHeaderViewPrivate::cascadingResize(int visual, int newSize)
{
    Q_Q(QHeaderView);
    const int minimumSize = q->minimumSectionSize();
    const int oldSize = headerSectionSize(visual);
    int delta = newSize - oldSize;

    if (delta > 0) { // larger
        bool sectionResized = false;

        // restore old section sizes
        for (int i = firstCascadingSection; i < visual; ++i) {
            if (cascadingSectionSize.contains(i)) {
                int currentSectionSize = headerSectionSize(i);
                int originalSectionSize = cascadingSectionSize.value(i);
                if (currentSectionSize < originalSectionSize) {
                    int newSectionSize = currentSectionSize + delta;
                    resizeSectionSpan(i, currentSectionSize, newSectionSize);
                    if (newSectionSize >= originalSectionSize && false)
                        cascadingSectionSize.remove(i); // the section is now restored
                    sectionResized = true;
                    break;
                }
            }

        }

        // resize the section
        if (!sectionResized) {
            newSize = qMax(newSize, minimumSize);
            if (oldSize != newSize)
                resizeSectionSpan(visual, oldSize, newSize);
        }

        // cascade the section size change
        for (int i = visual + 1; i < sectionCount; ++i) {
            if (!sectionIsCascadable(i))
                continue;
            int currentSectionSize = headerSectionSize(i);
            if (currentSectionSize <= minimumSize)
                continue;
            int newSectionSize = qMax(currentSectionSize - delta, minimumSize);
            //qDebug() << "### cascading to" << i << newSectionSize - currentSectionSize << delta;
            resizeSectionSpan(i, currentSectionSize, newSectionSize);
            saveCascadingSectionSize(i, currentSectionSize);
            delta = delta - (currentSectionSize - newSectionSize);
            //qDebug() << "new delta" << delta;
            //if (newSectionSize != minimumSize)
            if (delta <= 0)
                break;
        }
    } else { // smaller
        bool sectionResized = false;

        // restore old section sizes
        for (int i = lastCascadingSection; i > visual; --i) {
            if (!cascadingSectionSize.contains(i))
                continue;
            int currentSectionSize = headerSectionSize(i);
            int originalSectionSize = cascadingSectionSize.value(i);
            if (currentSectionSize >= originalSectionSize)
                continue;
            int newSectionSize = currentSectionSize - delta;
            resizeSectionSpan(i, currentSectionSize, newSectionSize);
            if (newSectionSize >= originalSectionSize && false) {
                //qDebug() << "section" << i << "restored to" << originalSectionSize;
                cascadingSectionSize.remove(i); // the section is now restored
            }
            sectionResized = true;
            break;
        }

        // resize the section
        resizeSectionSpan(visual, oldSize, qMax(newSize, minimumSize));

        // cascade the section size change
        if (delta < 0 && newSize < minimumSize) {
            for (int i = visual - 1; i >= 0; --i) {
                if (!sectionIsCascadable(i))
                    continue;
                int sectionSize = headerSectionSize(i);
                if (sectionSize <= minimumSize)
                    continue;
                resizeSectionSpan(i, sectionSize, qMax(sectionSize + delta, minimumSize));
                saveCascadingSectionSize(i, sectionSize);
                break;
            }
        }

        // let the next section get the space from the resized section
        if (!sectionResized) {
            for (int i = visual + 1; i < sectionCount; ++i) {
                if (!sectionIsCascadable(i))
                    continue;
                int currentSectionSize = headerSectionSize(i);
                int newSectionSize = qMax(currentSectionSize - delta, minimumSize);
                resizeSectionSpan(i, currentSectionSize, newSectionSize);
                break;
            }
        }
    }

    if (hasAutoResizeSections())
        doDelayedResizeSections();

    viewport->update();
}

void QHeaderViewPrivate::setDefaultSectionSize(int size)
{
    Q_Q(QHeaderView);
    defaultSectionSize = size;
    int currentVisualIndex = 0;
    for (int i = 0; i < sectionSpans.count(); ++i) {
        QHeaderViewPrivate::SectionSpan &span = sectionSpans[i];
        if (span.size > 0) {
            //we resize it if it is not hidden (ie size > 0)
            const int newSize = span.count * size;
            if (newSize != span.size) {
                length += newSize - span.size; //the whole length is changed
                const int oldSectionSize = span.sectionSize();
                span.size = span.count * size;
                for (int i = currentVisualIndex; i < currentVisualIndex + span.count; ++i) {
                    emit q->sectionResized(logicalIndex(i), oldSectionSize, size);
                }
            }
        }
        currentVisualIndex += span.count;
    }
}

void QHeaderViewPrivate::resizeSectionSpan(int visualIndex, int oldSize, int newSize)
{
    Q_Q(QHeaderView);
    QHeaderView::ResizeMode mode = headerSectionResizeMode(visualIndex);
    createSectionSpan(visualIndex, visualIndex, newSize, mode);
    emit q->sectionResized(logicalIndex(visualIndex), oldSize, newSize);
}

int QHeaderViewPrivate::headerSectionSize(int visual) const
{
    // ### stupid iteration
    int section_start = 0;
    const int sectionSpansCount = sectionSpans.count();
    for (int i = 0; i < sectionSpansCount; ++i) {
        const QHeaderViewPrivate::SectionSpan &currentSection = sectionSpans.at(i);
        int section_end = section_start + currentSection.count - 1;
        if (visual >= section_start && visual <= section_end)
            return currentSection.sectionSize();
        section_start = section_end + 1;
    }
    return -1;
}

int QHeaderViewPrivate::headerSectionPosition(int visual) const
{
    // ### stupid iteration
    int section_start = 0;
    int span_position = 0;
    const int sectionSpansCount = sectionSpans.count();
    for (int i = 0; i < sectionSpansCount; ++i) {
        const QHeaderViewPrivate::SectionSpan &currentSection = sectionSpans.at(i);
        int section_end = section_start + currentSection.count - 1;
        if (visual >= section_start && visual <= section_end)
            return span_position + (visual - section_start) * currentSection.sectionSize();
        section_start = section_end + 1;
        span_position += currentSection.size;
    }
    return -1;
}

int QHeaderViewPrivate::headerVisualIndexAt(int position) const
{
    // ### stupid iteration
    int span_start_section = 0;
    int span_position = 0;
    const int sectionSpansCount = sectionSpans.count();
    for (int i = 0; i < sectionSpansCount; ++i) {
        const QHeaderViewPrivate::SectionSpan &currentSection = sectionSpans.at(i);
        int next_span_start_section = span_start_section + currentSection.count;
        int next_span_position = span_position + currentSection.size;
        if (position == span_position)
            return span_start_section; // spans with no size
        if (position > span_position && position < next_span_position) {
            int position_in_span = position - span_position;
            return span_start_section + (position_in_span / currentSection.sectionSize());
        }
        span_start_section = next_span_start_section;
        span_position = next_span_position;
    }
    return -1;
}

void QHeaderViewPrivate::setHeaderSectionResizeMode(int visual, QHeaderView::ResizeMode mode)
{
    int size = headerSectionSize(visual);
    createSectionSpan(visual, visual, size, mode);
}

QHeaderView::ResizeMode QHeaderViewPrivate::headerSectionResizeMode(int visual) const
{
    int span = sectionSpanIndex(visual);
    if (span == -1)
        return globalResizeMode;
    return sectionSpans.at(span).resizeMode;
}

void QHeaderViewPrivate::setGlobalHeaderResizeMode(QHeaderView::ResizeMode mode)
{
    globalResizeMode = mode;
    for (int i = 0; i < sectionSpans.count(); ++i)
        sectionSpans[i].resizeMode = mode;
}

int QHeaderViewPrivate::viewSectionSizeHint(int logical) const
{
    if (QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent)) {
        return (orientation == Qt::Horizontal
                ? view->sizeHintForColumn(logical)
                : view->sizeHintForRow(logical));
    }
    return 0;
}

int QHeaderViewPrivate::adjustedVisualIndex(int visualIndex) const
{
    if (hiddenSectionSize.count() > 0) {
        int adjustedVisualIndex = visualIndex;
        int currentVisualIndex = 0;
        for (int i = 0; i < sectionSpans.count(); ++i) {
            if (sectionSpans.at(i).size == 0)
                adjustedVisualIndex += sectionSpans.at(i).count;
            else
                currentVisualIndex += sectionSpans.at(i).count;
            if (currentVisualIndex >= visualIndex)
                break;
        }
        visualIndex = adjustedVisualIndex;
    }
    return visualIndex;
}

#ifndef QT_NO_DATASTREAM
void QHeaderViewPrivate::write(QDataStream &out) const
{
    out << int(orientation);
    out << int(sortIndicatorOrder);
    out << sortIndicatorSection;
    out << sortIndicatorShown;

    out << visualIndices;
    out << logicalIndices;

    out << sectionHidden;
    out << hiddenSectionSize;

    out << length;
    out << sectionCount;
    out << movableSections;
    out << clickableSections;
    out << highlightSelected;
    out << stretchLastSection;
    out << cascadingResizing;
    out << stretchSections;
    out << contentsSections;
    out << defaultSectionSize;
    out << minimumSectionSize;

    out << int(defaultAlignment);
    out << int(globalResizeMode);

    out << sectionSpans;
}

bool QHeaderViewPrivate::read(QDataStream &in)
{
    int orient, order, align, global;
    in >> orient;
    orientation = (Qt::Orientation)orient;

    in >> order;
    sortIndicatorOrder = (Qt::SortOrder)order;

    in >> sortIndicatorSection;
    in >> sortIndicatorShown;

    in >> visualIndices;
    in >> logicalIndices;

    in >> sectionHidden;
    in >> hiddenSectionSize;

    in >> length;
    in >> sectionCount;
    in >> movableSections;
    in >> clickableSections;
    in >> highlightSelected;
    in >> stretchLastSection;
    in >> cascadingResizing;
    in >> stretchSections;
    in >> contentsSections;
    in >> defaultSectionSize;
    in >> minimumSectionSize;

    in >> align;
    defaultAlignment = Qt::Alignment(align);

    in >> global;
    globalResizeMode = (QHeaderView::ResizeMode)global;

    in >> sectionSpans;

    return true;
}

#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#endif // QT_NO_ITEMVIEWS

#include "moc_qheaderview.cpp"
