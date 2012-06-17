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

#ifndef QHEADERVIEW_P_H
#define QHEADERVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qabstractitemview_p.h"

#ifndef QT_NO_ITEMVIEWS

#include "QtCore/qbitarray.h"
#include "QtGui/qapplication.h"
#include "QtGui/qlabel.h"

QT_BEGIN_NAMESPACE

class QHeaderViewPrivate: public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QHeaderView)

public:
    enum StateVersion { VersionMarker = 0xff };

    QHeaderViewPrivate()
        : state(NoState),
          offset(0),
          sortIndicatorOrder(Qt::DescendingOrder),
          sortIndicatorSection(0),
          sortIndicatorShown(false),
          lastPos(-1),
          firstPos(-1),
          originalSize(-1),
          section(-1),
          target(-1),
          pressed(-1),
          hover(-1),
          length(0),
          sectionCount(0),
          movableSections(false),
          clickableSections(false),
          highlightSelected(false),
          stretchLastSection(false),
          cascadingResizing(false),
          resizeRecursionBlock(false),
          stretchSections(0),
          contentsSections(0),
          minimumSectionSize(-1),
          lastSectionSize(0),
          sectionIndicatorOffset(0),
          sectionIndicator(0),
          globalResizeMode(QHeaderView::Interactive)
    {}


    int lastVisibleVisualIndex() const;
    int sectionHandleAt(int position);
    void setupSectionIndicator(int section, int position);
    void updateSectionIndicator(int section, int position);
    void updateHiddenSections(int logicalFirst, int logicalLast);
    void resizeSections(QHeaderView::ResizeMode globalMode, bool useGlobalMode = false);
    void _q_sectionsRemoved(const QModelIndex &,int,int);
    void _q_layoutAboutToBeChanged();
    void _q_layoutChanged();

    bool isSectionSelected(int section) const;

    inline bool rowIntersectsSelection(int row) const {
        return (selectionModel ? selectionModel->rowIntersectsSelection(row, root) : false);
    }

    inline bool columnIntersectsSelection(int column) const {
        return (selectionModel ? selectionModel->columnIntersectsSelection(column, root) : false);
    }

    inline bool sectionIntersectsSelection(int logical) const {
        return (orientation == Qt::Horizontal ? columnIntersectsSelection(logical) : rowIntersectsSelection(logical));
    }

    inline bool isRowSelected(int row) const {
        return (selectionModel ? selectionModel->isRowSelected(row, root) : false);
    }

    inline bool isColumnSelected(int column) const {
        return (selectionModel ? selectionModel->isColumnSelected(column, root) : false);
    }

    inline void prepareSectionSelected() {
        if (!selectionModel || !selectionModel->hasSelection())
            sectionSelected.clear();
        else if (sectionSelected.count() != sectionCount * 2)
            sectionSelected.fill(false, sectionCount * 2);
        else sectionSelected.fill(false);
    }

    inline bool reverse() const {
        return orientation == Qt::Horizontal && q_func()->isRightToLeft();
    }

    inline int logicalIndex(int visualIndex) const {
        return logicalIndices.isEmpty() ? visualIndex : logicalIndices.at(visualIndex);
    }

    inline int visualIndex(int logicalIndex) const {
        return visualIndices.isEmpty() ? logicalIndex : visualIndices.at(logicalIndex);
    }

    inline void setDefaultValues(Qt::Orientation o) {
        orientation = o;
        defaultSectionSize = (o == Qt::Horizontal ? 100
                              : qMax(q_func()->minimumSectionSize(), 30));
        defaultAlignment = (o == Qt::Horizontal
                            ? Qt::Alignment(Qt::AlignCenter)
                            : Qt::AlignLeft|Qt::AlignVCenter);
    }

    inline bool isVisualIndexHidden(int visual) const {
        return !sectionHidden.isEmpty() && sectionHidden.at(visual);
    }

    inline void setVisualIndexHidden(int visual, bool hidden) {
        if (!sectionHidden.isEmpty()) sectionHidden.setBit(visual, hidden);
    }

    inline bool hasAutoResizeSections() const {
        return stretchSections || stretchLastSection || contentsSections;
    }

    QStyleOptionHeader getStyleOption() const;

    inline void invalidateCachedSizeHint() const {
        cachedSizeHint = QSize();
    }

    inline void initializeIndexMapping() const {
        if (visualIndices.count() != sectionCount
            || logicalIndices.count() != sectionCount) {
            visualIndices.resize(sectionCount);
            logicalIndices.resize(sectionCount);
            for (int s = 0; s < sectionCount; ++s) {
                visualIndices[s] = s;
                logicalIndices[s] = s;
            }
        }
    }

    inline void clearCascadingSections() {
        firstCascadingSection = sectionCount;
        lastCascadingSection = 0;
        cascadingSectionSize.clear();
    }

    inline void saveCascadingSectionSize(int visual, int size) {
        if (!cascadingSectionSize.contains(visual)) {
            cascadingSectionSize.insert(visual, size);
            firstCascadingSection = qMin(firstCascadingSection, visual);
            lastCascadingSection = qMax(lastCascadingSection, visual);
        }
    }

    inline bool sectionIsCascadable(int visual) const {
        return headerSectionResizeMode(visual) == QHeaderView::Interactive;
    }

    inline int modelSectionCount() const {
        return (orientation == Qt::Horizontal
                ? model->columnCount(root)
                : model->rowCount(root));
    }

    inline bool modelIsEmpty() const {
        return (model->rowCount(root) == 0 || model->columnCount(root) == 0);
    }

    inline void doDelayedResizeSections() {
        if (!delayedResize.isActive())
            delayedResize.start(0, q_func());
    }

    inline void executePostedResize() const {
        if (delayedResize.isActive() && state == NoState) {
            const_cast<QHeaderView*>(q_func())->resizeSections();
        }
    }

    void clear();
    void flipSortIndicator(int section);
    void cascadingResize(int visual, int newSize);

    enum State { NoState, ResizeSection, MoveSection, SelectSections, NoClear } state;

    int offset;
    Qt::Orientation orientation;
    Qt::SortOrder sortIndicatorOrder;
    int sortIndicatorSection;
    bool sortIndicatorShown;

    mutable QVector<int> visualIndices; // visualIndex = visualIndices.at(logicalIndex)
    mutable QVector<int> logicalIndices; // logicalIndex = row or column in the model
    mutable QBitArray sectionSelected; // from logical index to bit
    mutable QBitArray sectionHidden; // from visual index to bit
    mutable QHash<int, int> hiddenSectionSize; // from logical index to section size
    mutable QHash<int, int> cascadingSectionSize; // from visual index to section size
    mutable QSize cachedSizeHint;
    mutable QBasicTimer delayedResize;

    int firstCascadingSection;
    int lastCascadingSection;

    int lastPos;
    int firstPos;
    int originalSize;
    int section; // used for resizing and moving sections
    int target;
    int pressed;
    int hover;

    int length;
    int sectionCount;
    bool movableSections;
    bool clickableSections;
    bool highlightSelected;
    bool stretchLastSection;
    bool cascadingResizing;
    bool resizeRecursionBlock;
    int stretchSections;
    int contentsSections;
    int defaultSectionSize;
    int minimumSectionSize;
    int lastSectionSize; // $$$
    int sectionIndicatorOffset;
    Qt::Alignment defaultAlignment;
    QLabel *sectionIndicator;
    QHeaderView::ResizeMode globalResizeMode;
    QList<QPersistentModelIndex> persistentHiddenSections;

    // header section spans

    struct SectionSpan {
        int size;
        int count;
        QHeaderView::ResizeMode resizeMode;
        inline SectionSpan() : size(0), count(0), resizeMode(QHeaderView::Interactive) {}
        inline SectionSpan(int length, int sections, QHeaderView::ResizeMode mode)
            : size(length), count(sections), resizeMode(mode) {}
        inline int sectionSize() const { return (count > 0 ? size / count : 0); }
#ifndef QT_NO_DATASTREAM
        inline void write(QDataStream &out) const
        { out << size; out << count; out << (int)resizeMode; }
        inline void read(QDataStream &in)
        { in >> size; in >> count; int m; in >> m; resizeMode = (QHeaderView::ResizeMode)m; }
#endif
    };

    QVector<SectionSpan> sectionSpans;

    void createSectionSpan(int start, int end, int size, QHeaderView::ResizeMode mode);
    void removeSectionsFromSpans(int start, int end);
    void resizeSectionSpan(int visualIndex, int oldSize, int newSize);
    void setDefaultSectionSize(int size);

    inline int headerSectionCount() const { // for debugging
        int count = 0;
        for (int i = 0; i < sectionSpans.count(); ++i)
            count += sectionSpans.at(i).count;
        return count;
    }

    inline int headerLength() const { // for debugging
        int len = 0;
        for (int i = 0; i < sectionSpans.count(); ++i)
            len += sectionSpans.at(i).size;
        return len;
    }

    inline void removeSpans(const QList<int> &spans) {
        for (int i = spans.count() - 1; i >= 0; --i) {
            length -= sectionSpans.at(spans.at(i)).size;
            sectionSpans.remove(spans.at(i));
        }
    }

    inline int sectionSpanIndex(int visual) const {
        int section_start = 0;
        for (int i = 0; i < sectionSpans.count(); ++i) {
            int section_end = section_start + sectionSpans.at(i).count - 1;
            if (visual >= section_start && visual <= section_end)
                return i;
            section_start = section_end + 1;
        }
        return -1;
    }

    int headerSectionSize(int visual) const;
    int headerSectionPosition(int visual) const;
    int headerVisualIndexAt(int position) const;

    // resize mode
    void setHeaderSectionResizeMode(int visual, QHeaderView::ResizeMode mode);
    QHeaderView::ResizeMode headerSectionResizeMode(int visual) const;
    void setGlobalHeaderResizeMode(QHeaderView::ResizeMode mode);

    // other
    int viewSectionSizeHint(int logical) const;
    int adjustedVisualIndex(int visualIndex) const;

#ifndef QT_NO_DATASTREAM
    void write(QDataStream &out) const;
    bool read(QDataStream &in);
#endif

};

QT_END_NAMESPACE

#endif // QT_NO_ITEMVIEWS

#endif // QHEADERVIEW_P_H
