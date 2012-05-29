/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

/*!
    \class QCompleter
    \brief The QCompleter class provides completions based on an item model.
    \since 4.2

    You can use QCompleter to provide auto completions in any Qt
    widget, such as QLineEdit and QComboBox.
    When the user starts typing a word, QCompleter suggests possible ways of
    completing the word, based on a word list. The word list is
    provided as a QAbstractItemModel. (For simple applications, where
    the word list is static, you can pass a QStringList to
    QCompleter's constructor.)

    \tableofcontents

    \section1 Basic Usage

    A QCompleter is used typically with a QLineEdit or QComboBox.
    For example, here's how to provide auto completions from a simple
    word list in a QLineEdit:

    \snippet doc/src/snippets/code/src_gui_util_qcompleter.cpp 0

    A QFileSystemModel can be used to provide auto completion of file names.
    For example:

    \snippet doc/src/snippets/code/src_gui_util_qcompleter.cpp 1

    To set the model on which QCompleter should operate, call
    setModel(). By default, QCompleter will attempt to match the \l
    {completionPrefix}{completion prefix} (i.e., the word that the
    user has started typing) against the Qt::EditRole data stored in
    column 0 in the  model case sensitively. This can be changed
    using setCompletionRole(), setCompletionColumn(), and
    setCaseSensitivity().

    If the model is sorted on the column and role that are used for completion,
    you can call setModelSorting() with either
    QCompleter::CaseSensitivelySortedModel or
    QCompleter::CaseInsensitivelySortedModel as the argument. On large models,
    this can lead to significant performance improvements, because QCompleter
    can then use binary search instead of linear search.

    The model can be a \l{QAbstractListModel}{list model},
    a \l{QAbstractTableModel}{table model}, or a
    \l{QAbstractItemModel}{tree model}. Completion on tree models
    is slightly more involved and is covered in the \l{Handling
    Tree Models} section below.

    The completionMode() determines the mode used to provide completions to
    the user.

    \section1 Iterating Through Completions

    To retrieve a single candidate string, call setCompletionPrefix()
    with the text that needs to be completed and call
    currentCompletion(). You can iterate through the list of
    completions as below:

    \snippet doc/src/snippets/code/src_gui_util_qcompleter.cpp 2

    completionCount() returns the total number of completions for the
    current prefix. completionCount() should be avoided when possible,
    since it requires a scan of the entire model.

    \section1 The Completion Model

    completionModel() return a list model that contains all possible
    completions for the current completion prefix, in the order in which
    they appear in the model. This model can be used to display the current
    completions in a custom view. Calling setCompletionPrefix() automatically
    refreshes the completion model.

    \section1 Handling Tree Models

    QCompleter can look for completions in tree models, assuming
    that any item (or sub-item or sub-sub-item) can be unambiguously
    represented as a string by specifying the path to the item. The
    completion is then performed one level at a time.

    Let's take the example of a user typing in a file system path.
    The model is a (hierarchical) QFileSystemModel. The completion
    occurs for every element in the path. For example, if the current
    text is \c C:\Wind, QCompleter might suggest \c Windows to
    complete the current path element. Similarly, if the current text
    is \c C:\Windows\Sy, QCompleter might suggest \c System.

    For this kind of completion to work, QCompleter needs to be able to
    split the path into a list of strings that are matched at each level.
    For \c C:\Windows\Sy, it needs to be split as "C:", "Windows" and "Sy".
    The default implementation of splitPath(), splits the completionPrefix
    using QDir::separator() if the model is a QFileSystemModel.

    To provide completions, QCompleter needs to know the path from an index.
    This is provided by pathFromIndex(). The default implementation of
    pathFromIndex(), returns the data for the \l{Qt::EditRole}{edit role}
    for list models and the absolute file path if the mode is a QFileSystemModel.

    \sa QAbstractItemModel, QLineEdit, QComboBox, {Completer Example}
*/

#include "qcompleter_p.h"

#ifndef QT_NO_COMPLETER

#include "QtGui/qscrollbar.h"
#include "QtGui/qstringlistmodel.h"
#include "QtGui/qdirmodel.h"
#include "QtGui/qfilesystemmodel.h"
#include "QtGui/qheaderview.h"
#include "QtGui/qlistview.h"
#include "QtGui/qapplication.h"
#include "QtGui/qevent.h"
#include "QtGui/qheaderview.h"
#include "QtGui/qdesktopwidget.h"
#include "QtGui/qlineedit.h"

QT_BEGIN_NAMESPACE

QCompletionModel::QCompletionModel(QCompleterPrivate *c, QObject *parent)
    : QAbstractProxyModel(*new QCompletionModelPrivate, parent),
      c(c), showAll(false)
{
    createEngine();
}

int QCompletionModel::columnCount(const QModelIndex &) const
{
    Q_D(const QCompletionModel);
    return d->model->columnCount();
}

void QCompletionModel::setSourceModel(QAbstractItemModel *source)
{
    bool hadModel = (sourceModel() != 0);

    if (hadModel)
        QObject::disconnect(sourceModel(), 0, this, 0);

    QAbstractProxyModel::setSourceModel(source);

    if (source) {
        // TODO: Optimize updates in the source model
        connect(source, SIGNAL(modelReset()), this, SLOT(invalidate()));
        connect(source, SIGNAL(destroyed()), this, SLOT(modelDestroyed()));
        connect(source, SIGNAL(layoutChanged()), this, SLOT(invalidate()));
        connect(source, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsInserted()));
        connect(source, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(invalidate()));
        connect(source, SIGNAL(columnsInserted(QModelIndex,int,int)), this, SLOT(invalidate()));
        connect(source, SIGNAL(columnsRemoved(QModelIndex,int,int)), this, SLOT(invalidate()));
        connect(source, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(invalidate()));
    }

    invalidate();
}

void QCompletionModel::createEngine()
{
    bool sortedEngine = false;
    switch (c->sorting) {
    case QCompleter::UnsortedModel:
        sortedEngine = false;
        break;
    case QCompleter::CaseSensitivelySortedModel:
        sortedEngine = c->cs == Qt::CaseSensitive;
        break;
    case QCompleter::CaseInsensitivelySortedModel:
        sortedEngine = c->cs == Qt::CaseInsensitive;
        break;
    }

    if (sortedEngine)
        engine.reset(new QSortedModelEngine(c));
    else
        engine.reset(new QUnsortedModelEngine(c));
}

QModelIndex QCompletionModel::mapToSource(const QModelIndex& index) const
{
    Q_D(const QCompletionModel);
    if (!index.isValid())
        return engine->curParent;

    int row;
    QModelIndex parent = engine->curParent;
    if (!showAll) {
        if (!engine->matchCount())
            return QModelIndex();
        Q_ASSERT(index.row() < engine->matchCount());
        QIndexMapper& rootIndices = engine->historyMatch.indices;
        if (index.row() < rootIndices.count()) {
            row = rootIndices[index.row()];
            parent = QModelIndex();
        } else {
            row = engine->curMatch.indices[index.row() - rootIndices.count()];
        }
    } else {
        row = index.row();
    }

    return d->model->index(row, index.column(), parent);
}

QModelIndex QCompletionModel::mapFromSource(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    int row = -1;
    if (!showAll) {
        if (!engine->matchCount())
            return QModelIndex();

        QIndexMapper& rootIndices = engine->historyMatch.indices;
        if (idx.parent().isValid()) {
            if (idx.parent() != engine->curParent)
                return QModelIndex();
        } else {
            row = rootIndices.indexOf(idx.row());
            if (row == -1 && engine->curParent.isValid())
                return QModelIndex(); // source parent and our parent don't match
        }

        if (row == -1) {
            QIndexMapper& indices = engine->curMatch.indices;
            engine->filterOnDemand(idx.row() - indices.last());
            row = indices.indexOf(idx.row()) + rootIndices.count();
        }

        if (row == -1)
            return QModelIndex();
    } else {
        if (idx.parent() != engine->curParent)
            return QModelIndex();
        row = idx.row();
    }

    return createIndex(row, idx.column());
}

bool QCompletionModel::setCurrentRow(int row)
{
    if (row < 0 || !engine->matchCount())
        return false;

    if (row >= engine->matchCount())
        engine->filterOnDemand(row + 1 - engine->matchCount());

    if (row >= engine->matchCount()) // invalid row
        return false;

    engine->curRow = row;
    return true;
}

QModelIndex QCompletionModel::currentIndex(bool sourceIndex) const
{
    if (!engine->matchCount())
        return QModelIndex();

    int row = engine->curRow;
    if (showAll)
        row = engine->curMatch.indices[engine->curRow];

    QModelIndex idx = createIndex(row, c->column);
    if (!sourceIndex)
        return idx;
    return mapToSource(idx);
}

QModelIndex QCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_D(const QCompletionModel);
    if (row < 0 || column < 0 || column >= columnCount(parent) || parent.isValid())
        return QModelIndex();

    if (!showAll) {
        if (!engine->matchCount())
            return QModelIndex();
        if (row >= engine->historyMatch.indices.count()) {
            int want = row + 1 - engine->matchCount();
            if (want > 0)
                engine->filterOnDemand(want);
            if (row >= engine->matchCount())
                return QModelIndex();
        }
    } else {
        if (row >= d->model->rowCount(engine->curParent))
            return QModelIndex();
    }

    return createIndex(row, column);
}

int QCompletionModel::completionCount() const
{
    if (!engine->matchCount())
        return 0;

    engine->filterOnDemand(INT_MAX);
    return engine->matchCount();
}

int QCompletionModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QCompletionModel);
    if (parent.isValid())
        return 0;

    if (showAll) {
        // Show all items below current parent, even if we have no valid matches
        if (engine->curParts.count() != 1  && !engine->matchCount()
            && !engine->curParent.isValid())
            return 0;
        return d->model->rowCount(engine->curParent);
    }

    return completionCount();
}

void QCompletionModel::setFiltered(bool filtered)
{
    if (showAll == !filtered)
        return;
    showAll = !filtered;
    resetModel();
}

bool QCompletionModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const QCompletionModel);
    if (parent.isValid())
        return false;

    if (showAll)
        return d->model->hasChildren(mapToSource(parent));

    if (!engine->matchCount())
        return false;

    return true;
}

QVariant QCompletionModel::data(const QModelIndex& index, int role) const
{
    Q_D(const QCompletionModel);
    return d->model->data(mapToSource(index), role);
}

void QCompletionModel::modelDestroyed()
{
    QAbstractProxyModel::setSourceModel(0); // switch to static empty model
    invalidate();
}

void QCompletionModel::rowsInserted()
{
    invalidate();
    emit rowsAdded();
}

void QCompletionModel::invalidate()
{
    engine->cache.clear();
    filter(engine->curParts);
}

void QCompletionModel::filter(const QStringList& parts)
{
    Q_D(QCompletionModel);
    engine->filter(parts);
    resetModel();

    if (d->model->canFetchMore(engine->curParent))
        d->model->fetchMore(engine->curParent);
}

void QCompletionModel::resetModel()
{
    if (rowCount() == 0) {
        reset();
        return;
    }

    emit layoutAboutToBeChanged();
    QModelIndexList piList = persistentIndexList();
    QModelIndexList empty;
    for (int i = 0; i < piList.size(); i++)
        empty.append(QModelIndex());
    changePersistentIndexList(piList, empty);
    emit layoutChanged();
}

//////////////////////////////////////////////////////////////////////////////
void QCompletionEngine::filter(const QStringList& parts)
{
    const QAbstractItemModel *model = c->proxy->sourceModel();
    curParts = parts;
    if (curParts.isEmpty())
        curParts.append(QString());

    curRow = -1;
    curParent = QModelIndex();
    curMatch = QMatchData();
    historyMatch = filterHistory();

    if (!model)
        return;

    QModelIndex parent;
    for (int i = 0; i < curParts.count() - 1; i++) {
        QString part = curParts[i];
        int emi = filter(part, parent, -1).exactMatchIndex;
        if (emi == -1)
            return;
        parent = model->index(emi, c->column, parent);
    }

    // Note that we set the curParent to a valid parent, even if we have no matches
    // When filtering is disabled, we show all the items under this parent
    curParent = parent;
    if (curParts.last().isEmpty())
        curMatch = QMatchData(QIndexMapper(0, model->rowCount(curParent) - 1), -1, false);
    else
        curMatch = filter(curParts.last(), curParent, 1); // build at least one
    curRow = curMatch.isValid() ? 0 : -1;
}

QMatchData QCompletionEngine::filterHistory()
{
    QAbstractItemModel *source = c->proxy->sourceModel();
    if (curParts.count() <= 1 || c->proxy->showAll || !source)
        return QMatchData();
    bool isDirModel = false;
    bool isFsModel = false;
#ifndef QT_NO_DIRMODEL
    isDirModel = (qobject_cast<QDirModel *>(source) != 0);
#endif
#ifndef QT_NO_FILESYSTEMMODEL
    isFsModel = (qobject_cast<QFileSystemModel *>(source) != 0);
#endif
    QVector<int> v;
    QIndexMapper im(v);
    QMatchData m(im, -1, true);

    for (int i = 0; i < source->rowCount(); i++) {
        QString str = source->index(i, c->column).data().toString();
        if (str.startsWith(c->prefix, c->cs)
#if (!defined(Q_OS_WIN) || defined(Q_OS_WINCE)) && !defined(Q_OS_SYMBIAN)
            && ((!isFsModel && !isDirModel) || QDir::toNativeSeparators(str) != QDir::separator())
#endif
            )
            m.indices.append(i);
    }
    return m;
}

// Returns a match hint from the cache by chopping the search string
bool QCompletionEngine::matchHint(QString part, const QModelIndex& parent, QMatchData *hint)
{
    if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();

    const CacheItem& map = cache[parent];

    QString key = part;
    while (!key.isEmpty()) {
        key.chop(1);
        if (map.contains(key)) {
            *hint = map[key];
            return true;
        }
    }

    return false;
}

bool QCompletionEngine::lookupCache(QString part, const QModelIndex& parent, QMatchData *m)
{
   if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();
   const CacheItem& map = cache[parent];
   if (!map.contains(part))
       return false;
   *m = map[part];
   return true;
}

// When the cache size exceeds 1MB, it clears out about 1/2 of the cache.
void QCompletionEngine::saveInCache(QString part, const QModelIndex& parent, const QMatchData& m)
{
    QMatchData old = cache[parent].take(part);
    cost = cost + m.indices.cost() - old.indices.cost();
    if (cost * sizeof(int) > 1024 * 1024) {
        QMap<QModelIndex, CacheItem>::iterator it1 = cache.begin();
        while (it1 != cache.end()) {
            CacheItem& ci = it1.value();
            int sz = ci.count()/2;
            QMap<QString, QMatchData>::iterator it2 = ci.begin();
            int i = 0;
            while (it2 != ci.end() && i < sz) {
                cost -= it2.value().indices.cost();
                it2 = ci.erase(it2);
                i++;
            }
            if (ci.count() == 0) {
              it1 = cache.erase(it1);
            } else {
              ++it1;
            }
        }
    }

    if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();
    cache[parent][part] = m;
}

///////////////////////////////////////////////////////////////////////////////////
QIndexMapper QSortedModelEngine::indexHint(QString part, const QModelIndex& parent, Qt::SortOrder order)
{
    const QAbstractItemModel *model = c->proxy->sourceModel();

    if (c->cs == Qt::CaseInsensitive)
        part = part.toLower();

    const CacheItem& map = cache[parent];

    // Try to find a lower and upper bound for the search from previous results
    int to = model->rowCount(parent) - 1;
    int from = 0;
    const CacheItem::const_iterator it = map.lowerBound(part);

    // look backward for first valid hint
    for(CacheItem::const_iterator it1 = it; it1-- != map.constBegin();) {
        const QMatchData& value = it1.value();
        if (value.isValid()) {
            if (order == Qt::AscendingOrder) {
                from = value.indices.last() + 1;
            } else {
                to = value.indices.first() - 1;
            }
            break;
        }
    }

    // look forward for first valid hint
    for(CacheItem::const_iterator it2 = it; it2 != map.constEnd(); ++it2) {
        const QMatchData& value = it2.value();
        if (value.isValid() && !it2.key().startsWith(part)) {
            if (order == Qt::AscendingOrder) {
                to = value.indices.first() - 1;
            } else {
                from = value.indices.first() + 1;
            }
            break;
        }
    }

    return QIndexMapper(from, to);
}

Qt::SortOrder QSortedModelEngine::sortOrder(const QModelIndex &parent) const
{
    const QAbstractItemModel *model = c->proxy->sourceModel();

    int rowCount = model->rowCount(parent);
    if (rowCount < 2)
        return Qt::AscendingOrder;
    QString first = model->data(model->index(0, c->column, parent), c->role).toString();
    QString last = model->data(model->index(rowCount - 1, c->column, parent), c->role).toString();
    return QString::compare(first, last, c->cs) <= 0 ? Qt::AscendingOrder : Qt::DescendingOrder;
}

QMatchData QSortedModelEngine::filter(const QString& part, const QModelIndex& parent, int)
{
    const QAbstractItemModel *model = c->proxy->sourceModel();

    QMatchData hint;
    if (lookupCache(part, parent, &hint))
        return hint;

    QIndexMapper indices;
    Qt::SortOrder order = sortOrder(parent);

    if (matchHint(part, parent, &hint)) {
        if (!hint.isValid())
            return QMatchData();
        indices = hint.indices;
    } else {
        indices = indexHint(part, parent, order);
    }

    // binary search the model within 'indices' for 'part' under 'parent'
    int high = indices.to() + 1;
    int low = indices.from() - 1;
    int probe;
    QModelIndex probeIndex;
    QString probeData;

    while (high - low > 1)
    {
        probe = (high + low) / 2;
        probeIndex = model->index(probe, c->column, parent);
        probeData = model->data(probeIndex, c->role).toString();
        const int cmp = QString::compare(probeData, part, c->cs);
        if ((order == Qt::AscendingOrder && cmp >= 0)
            || (order == Qt::DescendingOrder && cmp < 0)) {
            high = probe;
        } else {
            low = probe;
        }
    }

    if ((order == Qt::AscendingOrder && low == indices.to())
        || (order == Qt::DescendingOrder && high == indices.from())) { // not found
        saveInCache(part, parent, QMatchData());
        return QMatchData();
    }

    probeIndex = model->index(order == Qt::AscendingOrder ? low+1 : high-1, c->column, parent);
    probeData = model->data(probeIndex, c->role).toString();
    if (!probeData.startsWith(part, c->cs)) {
        saveInCache(part, parent, QMatchData());
        return QMatchData();
    }

    const bool exactMatch = QString::compare(probeData, part, c->cs) == 0;
    int emi =  exactMatch ? (order == Qt::AscendingOrder ? low+1 : high-1) : -1;

    int from = 0;
    int to = 0;
    if (order == Qt::AscendingOrder) {
        from = low + 1;
        high = indices.to() + 1;
        low = from;
    } else {
        to = high - 1;
        low = indices.from() - 1;
        high = to;
    }

    while (high - low > 1)
    {
        probe = (high + low) / 2;
        probeIndex = model->index(probe, c->column, parent);
        probeData = model->data(probeIndex, c->role).toString();
        const bool startsWith = probeData.startsWith(part, c->cs);
        if ((order == Qt::AscendingOrder && startsWith)
            || (order == Qt::DescendingOrder && !startsWith)) {
            low = probe;
        } else {
            high = probe;
        }
    }

    QMatchData m(order == Qt::AscendingOrder ? QIndexMapper(from, high - 1) : QIndexMapper(low+1, to), emi, false);
    saveInCache(part, parent, m);
    return m;
}

////////////////////////////////////////////////////////////////////////////////////////
int QUnsortedModelEngine::buildIndices(const QString& str, const QModelIndex& parent, int n,
                                      const QIndexMapper& indices, QMatchData* m)
{
    Q_ASSERT(m->partial);
    Q_ASSERT(n != -1 || m->exactMatchIndex == -1);
    const QAbstractItemModel *model = c->proxy->sourceModel();
    int i, count = 0;

    for (i = 0; i < indices.count() && count != n; ++i) {
        QModelIndex idx = model->index(indices[i], c->column, parent);
        QString data = model->data(idx, c->role).toString();
        if (!data.startsWith(str, c->cs) || !(model->flags(idx) & Qt::ItemIsSelectable))
            continue;
        m->indices.append(indices[i]);
        ++count;
        if (m->exactMatchIndex == -1 && QString::compare(data, str, c->cs) == 0) {
            m->exactMatchIndex = indices[i];
            if (n == -1)
                return indices[i];
        }
    }
    return indices[i-1];
}

void QUnsortedModelEngine::filterOnDemand(int n)
{
    Q_ASSERT(matchCount());
    if (!curMatch.partial)
        return;
    Q_ASSERT(n >= -1);
    const QAbstractItemModel *model = c->proxy->sourceModel();
    int lastRow = model->rowCount(curParent) - 1;
    QIndexMapper im(curMatch.indices.last() + 1, lastRow);
    int lastIndex = buildIndices(curParts.last(), curParent, n, im, &curMatch);
    curMatch.partial = (lastRow != lastIndex);
    saveInCache(curParts.last(), curParent, curMatch);
}

QMatchData QUnsortedModelEngine::filter(const QString& part, const QModelIndex& parent, int n)
{
    QMatchData hint;

    QVector<int> v;
    QIndexMapper im(v);
    QMatchData m(im, -1, true);

    const QAbstractItemModel *model = c->proxy->sourceModel();
    bool foundInCache = lookupCache(part, parent, &m);

    if (!foundInCache) {
        if (matchHint(part, parent, &hint) && !hint.isValid())
            return QMatchData();
    }

    if (!foundInCache && !hint.isValid()) {
        const int lastRow = model->rowCount(parent) - 1;
        QIndexMapper all(0, lastRow);
        int lastIndex = buildIndices(part, parent, n, all, &m);
        m.partial = (lastIndex != lastRow);
    } else {
        if (!foundInCache) { // build from hint as much as we can
            buildIndices(part, parent, INT_MAX, hint.indices, &m);
            m.partial = hint.partial;
        }
        if (m.partial && ((n == -1 && m.exactMatchIndex == -1) || (m.indices.count() < n))) {
            // need more and have more
            const int lastRow = model->rowCount(parent) - 1;
            QIndexMapper rest(hint.indices.last() + 1, lastRow);
            int want = n == -1 ? -1 : n - m.indices.count();
            int lastIndex = buildIndices(part, parent, want, rest, &m);
            m.partial = (lastRow != lastIndex);
        }
    }

    saveInCache(part, parent, m);
    return m;
}

///////////////////////////////////////////////////////////////////////////////
QCompleterPrivate::QCompleterPrivate()
: widget(0), proxy(0), popup(0), cs(Qt::CaseSensitive), role(Qt::EditRole), column(0),
  maxVisibleItems(7), sorting(QCompleter::UnsortedModel), wrap(true), eatFocusOut(true),
  hiddenBecauseNoMatch(false)
{
}

void QCompleterPrivate::init(QAbstractItemModel *m)
{
    Q_Q(QCompleter);
    proxy = new QCompletionModel(this, q);
    QObject::connect(proxy, SIGNAL(rowsAdded()), q, SLOT(_q_autoResizePopup()));
    q->setModel(m);
#ifdef QT_NO_LISTVIEW
    q->setCompletionMode(QCompleter::InlineCompletion);
#else
    q->setCompletionMode(QCompleter::PopupCompletion);
#endif // QT_NO_LISTVIEW
}

void QCompleterPrivate::setCurrentIndex(QModelIndex index, bool select)
{
    Q_Q(QCompleter);
    if (!q->popup())
        return;
    if (!select) {
        popup->selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    } else {
        if (!index.isValid())
            popup->selectionModel()->clear();
        else
            popup->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select
                                                            | QItemSelectionModel::Rows);
    }
    index = popup->selectionModel()->currentIndex();
    if (!index.isValid())
        popup->scrollToTop();
    else
        popup->scrollTo(index, QAbstractItemView::PositionAtTop);
}

void QCompleterPrivate::_q_completionSelected(const QItemSelection& selection)
{
    QModelIndex index;
    if (!selection.indexes().isEmpty())
        index = selection.indexes().first();

    _q_complete(index, true);
}

void QCompleterPrivate::_q_complete(QModelIndex index, bool highlighted)
{
    Q_Q(QCompleter);
    QString completion;

    if (!index.isValid() || (!proxy->showAll && (index.row() >= proxy->engine->matchCount()))) {
        completion = prefix;
    } else {
        if (!(index.flags() & Qt::ItemIsEnabled))
            return;
        QModelIndex si = proxy->mapToSource(index);
        si = si.sibling(si.row(), column); // for clicked()
        completion = q->pathFromIndex(si);
#ifndef QT_NO_DIRMODEL
        // add a trailing separator in inline
        if (mode == QCompleter::InlineCompletion) {
            if (qobject_cast<QDirModel *>(proxy->sourceModel()) && QFileInfo(completion).isDir())
                completion += QDir::separator();
        }
#endif
#ifndef QT_NO_FILESYSTEMMODEL
        // add a trailing separator in inline
        if (mode == QCompleter::InlineCompletion) {
            if (qobject_cast<QFileSystemModel *>(proxy->sourceModel()) && QFileInfo(completion).isDir())
                completion += QDir::separator();
        }
#endif
    }

    if (highlighted) {
        emit q->highlighted(index);
        emit q->highlighted(completion);
    } else {
        emit q->activated(index);
        emit q->activated(completion);
    }
}

void QCompleterPrivate::_q_autoResizePopup()
{
    if (!popup || !popup->isVisible())
        return;
    showPopup(popupRect);
}

void QCompleterPrivate::showPopup(const QRect& rect)
{
    const QRect screen = QApplication::desktop()->availableGeometry(widget);
    Qt::LayoutDirection dir = widget->layoutDirection();
    QPoint pos;
    int rh, w;
    int h = (popup->sizeHintForRow(0) * qMin(maxVisibleItems, popup->model()->rowCount()) + 3) + 3;
    QScrollBar *hsb = popup->horizontalScrollBar();
    if (hsb && hsb->isVisible())
        h += popup->horizontalScrollBar()->sizeHint().height();

    if (rect.isValid()) {
        rh = rect.height();
        w = rect.width();
        pos = widget->mapToGlobal(dir == Qt::RightToLeft ? rect.bottomRight() : rect.bottomLeft());
    } else {
        rh = widget->height();
        pos = widget->mapToGlobal(QPoint(0, widget->height() - 2));
        w = widget->width();
    }

    if (w > screen.width())
        w = screen.width();
    if ((pos.x() + w) > (screen.x() + screen.width()))
        pos.setX(screen.x() + screen.width() - w);
    if (pos.x() < screen.x())
        pos.setX(screen.x());

    int top = pos.y() - rh - screen.top() + 2;
    int bottom = screen.bottom() - pos.y();
    h = qMax(h, popup->minimumHeight());
    if (h > bottom) {
        h = qMin(qMax(top, bottom), h);

        if (top > bottom)
            pos.setY(pos.y() - h - rh + 2);
    }

    popup->setGeometry(pos.x(), pos.y(), w, h);

    if (!popup->isVisible())
        popup->show();
}

void QCompleterPrivate::_q_fileSystemModelDirectoryLoaded(const QString &path)
{
    Q_Q(QCompleter);
    // Slot called when QFileSystemModel has finished loading.
    // If we hide the popup because there was no match because the model was not loaded yet,
    // we re-start the completion when we get the results
    if (hiddenBecauseNoMatch
        && prefix.startsWith(path) && prefix != (path + QLatin1Char('/'))
        && widget) {
        q->complete();
    }
}

/*!
    Constructs a completer object with the given \a parent.
*/
QCompleter::QCompleter(QObject *parent)
: QObject(*new QCompleterPrivate(), parent)
{
    Q_D(QCompleter);
    d->init();
}

/*!
    Constructs a completer object with the given \a parent that provides completions
    from the specified \a model.
*/
QCompleter::QCompleter(QAbstractItemModel *model, QObject *parent)
    : QObject(*new QCompleterPrivate(), parent)
{
    Q_D(QCompleter);
    d->init(model);
}

#ifndef QT_NO_STRINGLISTMODEL
/*!
    Constructs a QCompleter object with the given \a parent that uses the specified
    \a list as a source of possible completions.
*/
QCompleter::QCompleter(const QStringList& list, QObject *parent)
: QObject(*new QCompleterPrivate(), parent)
{
    Q_D(QCompleter);
    d->init(new QStringListModel(list, this));
}
#endif // QT_NO_STRINGLISTMODEL

/*!
    Destroys the completer object.
*/
QCompleter::~QCompleter()
{
}

/*!
    Sets the widget for which completion are provided for to \a widget. This
    function is automatically called when a QCompleter is set on a QLineEdit
    using QLineEdit::setCompleter() or on a QComboBox using
    QComboBox::setCompleter(). The widget needs to be set explicitly when
    providing completions for custom widgets.

    \sa widget(), setModel(), setPopup()
 */
void QCompleter::setWidget(QWidget *widget)
{
    Q_D(QCompleter);
    if (d->widget)
        d->widget->removeEventFilter(this);
    d->widget = widget;
    if (d->widget)
        d->widget->installEventFilter(this);
    if (d->popup) {
        d->popup->hide();
        d->popup->setFocusProxy(d->widget);
    }
}

/*!
    Returns the widget for which the completer object is providing completions.

    \sa setWidget()
 */
QWidget *QCompleter::widget() const
{
    Q_D(const QCompleter);
    return d->widget;
}

/*!
    Sets the model which provides completions to \a model. The \a model can
    be list model or a tree model. If a model has been already previously set
    and it has the QCompleter as its parent, it is deleted.

    For convenience, if \a model is a QFileSystemModel, QCompleter switches its
    caseSensitivity to Qt::CaseInsensitive on Windows and Qt::CaseSensitive
    on other platforms.

    \sa completionModel(), modelSorting, {Handling Tree Models}
*/
void QCompleter::setModel(QAbstractItemModel *model)
{
    Q_D(QCompleter);
    QAbstractItemModel *oldModel = d->proxy->sourceModel();
    d->proxy->setSourceModel(model);
    if (d->popup)
        setPopup(d->popup); // set the model and make new connections
    if (oldModel && oldModel->QObject::parent() == this)
        delete oldModel;
#ifndef QT_NO_DIRMODEL
    if (qobject_cast<QDirModel *>(model)) {
#if (defined(Q_OS_WIN) && !defined(Q_OS_WINCE)) || defined(Q_OS_SYMBIAN)
        setCaseSensitivity(Qt::CaseInsensitive);
#else
        setCaseSensitivity(Qt::CaseSensitive);
#endif
    }
#endif // QT_NO_DIRMODEL
#ifndef QT_NO_FILESYSTEMMODEL
    QFileSystemModel *fsModel = qobject_cast<QFileSystemModel *>(model);
    if (fsModel) {
#if (defined(Q_OS_WIN) && !defined(Q_OS_WINCE)) || defined(Q_OS_SYMBIAN)
        setCaseSensitivity(Qt::CaseInsensitive);
#else
        setCaseSensitivity(Qt::CaseSensitive);
#endif
        setCompletionRole(QFileSystemModel::FileNameRole);
        connect(fsModel, SIGNAL(directoryLoaded(QString)), this, SLOT(_q_fileSystemModelDirectoryLoaded(QString)));
    }
#endif // QT_NO_FILESYSTEMMODEL
}

/*!
    Returns the model that provides completion strings.

    \sa completionModel()
*/
QAbstractItemModel *QCompleter::model() const
{
    Q_D(const QCompleter);
    return d->proxy->sourceModel();
}

/*!
    \enum QCompleter::CompletionMode

    This enum specifies how completions are provided to the user.

    \value PopupCompletion            Current completions are displayed in a popup window.
    \value InlineCompletion           Completions appear inline (as selected text).
    \value UnfilteredPopupCompletion  All possible completions are displayed in a popup window with the most likely suggestion indicated as current.

    \sa setCompletionMode()
*/

/*!
    \property QCompleter::completionMode
    \brief how the completions are provided to the user

    The default value is QCompleter::PopupCompletion.
*/
void QCompleter::setCompletionMode(QCompleter::CompletionMode mode)
{
    Q_D(QCompleter);
    d->mode = mode;
    d->proxy->setFiltered(mode != QCompleter::UnfilteredPopupCompletion);

    if (mode == QCompleter::InlineCompletion) {
        if (d->widget)
            d->widget->removeEventFilter(this);
        if (d->popup) {
            d->popup->deleteLater();
            d->popup = 0;
        }
    } else {
        if (d->widget)
            d->widget->installEventFilter(this);
    }
}

QCompleter::CompletionMode QCompleter::completionMode() const
{
    Q_D(const QCompleter);
    return d->mode;
}

/*!
    Sets the popup used to display completions to \a popup. QCompleter takes
    ownership of the view.

    A QListView is automatically created when the completionMode() is set to
    QCompleter::PopupCompletion or QCompleter::UnfilteredPopupCompletion. The
    default popup displays the completionColumn().

    Ensure that this function is called before the view settings are modified.
    This is required since view's properties may require that a model has been
    set on the view (for example, hiding columns in the view requires a model
    to be set on the view).

    \sa popup()
*/
void QCompleter::setPopup(QAbstractItemView *popup)
{
    Q_D(QCompleter);
    Q_ASSERT(popup != 0);
    if (d->popup) {
        QObject::disconnect(d->popup->selectionModel(), 0, this, 0);
        QObject::disconnect(d->popup, 0, this, 0);
    }
    if (d->popup != popup)
        delete d->popup;
    if (popup->model() != d->proxy)
        popup->setModel(d->proxy);
#if defined(Q_OS_MAC) && !defined(QT_MAC_USE_COCOA)
     popup->show();
#else
     popup->hide();
#endif

    Qt::FocusPolicy origPolicy = Qt::NoFocus;
    if (d->widget)
        origPolicy = d->widget->focusPolicy();
    popup->setParent(0, Qt::Popup);
    popup->setFocusPolicy(Qt::NoFocus);
    if (d->widget)
        d->widget->setFocusPolicy(origPolicy);

    popup->setFocusProxy(d->widget);
    popup->installEventFilter(this);
    popup->setItemDelegate(new QCompleterItemDelegate(popup));
#ifndef QT_NO_LISTVIEW
    if (QListView *listView = qobject_cast<QListView *>(popup)) {
        listView->setModelColumn(d->column);
    }
#endif

    QObject::connect(popup, SIGNAL(clicked(QModelIndex)),
                     this, SLOT(_q_complete(QModelIndex)));
    QObject::connect(this, SIGNAL(activated(QModelIndex)),
                     popup, SLOT(hide()));

    QObject::connect(popup->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     this, SLOT(_q_completionSelected(QItemSelection)));
    d->popup = popup;
}

/*!
    Returns the popup used to display completions.

    \sa setPopup()
*/
QAbstractItemView *QCompleter::popup() const
{
    Q_D(const QCompleter);
#ifndef QT_NO_LISTVIEW
    if (!d->popup && completionMode() != QCompleter::InlineCompletion) {
        QListView *listView = new QListView;
        listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        listView->setSelectionBehavior(QAbstractItemView::SelectRows);
        listView->setSelectionMode(QAbstractItemView::SingleSelection);
        listView->setModelColumn(d->column);
        QCompleter *that = const_cast<QCompleter*>(this);
        that->setPopup(listView);
    }
#endif // QT_NO_LISTVIEW
    return d->popup;
}

/*!
  \reimp
*/
bool QCompleter::event(QEvent *ev)
{
    return QObject::event(ev);
}

/*!
  \reimp
*/
bool QCompleter::eventFilter(QObject *o, QEvent *e)
{
    Q_D(QCompleter);

    if (d->eatFocusOut && o == d->widget && e->type() == QEvent::FocusOut) {
        d->hiddenBecauseNoMatch = false;
        if (d->popup && d->popup->isVisible())
            return true;
    }

    if (o != d->popup)
        return QObject::eventFilter(o, e);

    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        QModelIndex curIndex = d->popup->currentIndex();
        QModelIndexList selList = d->popup->selectionModel()->selectedIndexes();

        const int key = ke->key();
        // In UnFilteredPopup mode, select the current item
        if ((key == Qt::Key_Up || key == Qt::Key_Down) && selList.isEmpty() && curIndex.isValid()
            && d->mode == QCompleter::UnfilteredPopupCompletion) {
              d->setCurrentIndex(curIndex);
              return true;
        }

        // Handle popup navigation keys. These are hardcoded because up/down might make the
        // widget do something else (lineedit cursor moves to home/end on mac, for instance)
        switch (key) {
        case Qt::Key_End:
        case Qt::Key_Home:
            if (ke->modifiers() & Qt::ControlModifier)
                return false;
            break;

        case Qt::Key_Up:
            if (!curIndex.isValid()) {
                int rowCount = d->proxy->rowCount();
                QModelIndex lastIndex = d->proxy->index(rowCount - 1, d->column);
                d->setCurrentIndex(lastIndex);
                return true;
            } else if (curIndex.row() == 0) {
                if (d->wrap)
                    d->setCurrentIndex(QModelIndex());
                return true;
            }
            return false;

        case Qt::Key_Down:
            if (!curIndex.isValid()) {
                QModelIndex firstIndex = d->proxy->index(0, d->column);
                d->setCurrentIndex(firstIndex);
                return true;
            } else if (curIndex.row() == d->proxy->rowCount() - 1) {
                if (d->wrap)
                    d->setCurrentIndex(QModelIndex());
                return true;
            }
            return false;

        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            return false;
        }

        // Send the event to the widget. If the widget accepted the event, do nothing
        // If the widget did not accept the event, provide a default implementation
        d->eatFocusOut = false;
        (static_cast<QObject *>(d->widget))->event(ke);
        d->eatFocusOut = true;
        if (!d->widget || e->isAccepted() || !d->popup->isVisible()) {
            // widget lost focus, hide the popup
            if (d->widget && (!d->widget->hasFocus()
#ifdef QT_KEYPAD_NAVIGATION
                || (QApplication::keypadNavigationEnabled() && !d->widget->hasEditFocus())
#endif
                ))
                d->popup->hide();
            if (e->isAccepted())
                return true;
        }

        // default implementation for keys not handled by the widget when popup is open
        switch (key) {
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (!QApplication::keypadNavigationEnabled())
                break;
#endif
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            d->popup->hide();
            if (curIndex.isValid())
                d->_q_complete(curIndex);
            break;

        case Qt::Key_F4:
            if (ke->modifiers() & Qt::AltModifier)
                d->popup->hide();
            break;

        case Qt::Key_Backtab:
        case Qt::Key_Escape:
            d->popup->hide();
            break;

        default:
            break;
        }

        return true;
    }

#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::KeyRelease: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (QApplication::keypadNavigationEnabled() && ke->key() == Qt::Key_Back) {
            // Send the event to the 'widget'. This is what we did for KeyPress, so we need
            // to do the same for KeyRelease, in case the widget's KeyPress event set
            // up something (such as a timer) that is relying on also receiving the
            // key release. I see this as a bug in Qt, and should really set it up for all
            // the affected keys. However, it is difficult to tell how this will affect
            // existing code, and I can't test for every combination!
            d->eatFocusOut = false;
            static_cast<QObject *>(d->widget)->event(ke);
            d->eatFocusOut = true;
        }
        break;
    }
#endif

    case QEvent::MouseButtonPress: {
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled()) {
            // if we've clicked in the widget (or its descendant), let it handle the click
            QWidget *source = qobject_cast<QWidget *>(o);
            if (source) {
                QPoint pos = source->mapToGlobal((static_cast<QMouseEvent *>(e))->pos());
                QWidget *target = QApplication::widgetAt(pos);
                if (target && (d->widget->isAncestorOf(target) ||
                    target == d->widget)) {
                    d->eatFocusOut = false;
                    static_cast<QObject *>(target)->event(e);
                    d->eatFocusOut = true;
                    return true;
                }
            }
        }
#endif
        if (!d->popup->underMouse()) {
            d->popup->hide();
            return true;
        }
        }
        return false;

    case QEvent::InputMethod:
    case QEvent::ShortcutOverride:
        QApplication::sendEvent(d->widget, e);
        break;

    default:
        return false;
    }
    return false;
}

/*!
    For QCompleter::PopupCompletion and QCompletion::UnfilteredPopupCompletion
    modes, calling this function displays the popup displaying the current
    completions. By default, if \a rect is not specified, the popup is displayed
    on the bottom of the widget(). If \a rect is specified the popup is
    displayed on the left edge of the rectangle.

    For QCompleter::InlineCompletion mode, the highlighted() signal is fired
    with the current completion.
*/
void QCompleter::complete(const QRect& rect)
{
    Q_D(QCompleter);
    QModelIndex idx = d->proxy->currentIndex(false);
    d->hiddenBecauseNoMatch = false;
    if (d->mode == QCompleter::InlineCompletion) {
        if (idx.isValid())
            d->_q_complete(idx, true);
        return;
    }

    Q_ASSERT(d->widget != 0);
    if ((d->mode == QCompleter::PopupCompletion && !idx.isValid())
        || (d->mode == QCompleter::UnfilteredPopupCompletion && d->proxy->rowCount() == 0)) {
        if (d->popup)
            d->popup->hide(); // no suggestion, hide
        d->hiddenBecauseNoMatch = true;
        return;
    }

    popup();
    if (d->mode == QCompleter::UnfilteredPopupCompletion)
        d->setCurrentIndex(idx, false);

    d->showPopup(rect);
    d->popupRect = rect;
}

/*!
    Sets the current row to the \a row specified. Returns true if successful;
    otherwise returns false.

    This function may be used along with currentCompletion() to iterate
    through all the possible completions.

    \sa currentCompletion(), completionCount()
*/
bool QCompleter::setCurrentRow(int row)
{
    Q_D(QCompleter);
    return d->proxy->setCurrentRow(row);
}

/*!
    Returns the current row.

    \sa setCurrentRow()
*/
int QCompleter::currentRow() const
{
    Q_D(const QCompleter);
    return d->proxy->currentRow();
}

/*!
    Returns the number of completions for the current prefix. For an unsorted
    model with a large number of items this can be expensive. Use setCurrentRow()
    and currentCompletion() to iterate through all the completions.
*/
int QCompleter::completionCount() const
{
    Q_D(const QCompleter);
    return d->proxy->completionCount();
}

/*!
    \enum QCompleter::ModelSorting

    This enum specifies how the items in the model are sorted.

    \value UnsortedModel                    The model is unsorted.
    \value CaseSensitivelySortedModel       The model is sorted case sensitively.
    \value CaseInsensitivelySortedModel     The model is sorted case insensitively.

    \sa setModelSorting()
*/

/*!
    \property QCompleter::modelSorting
    \brief the way the model is sorted

    By default, no assumptions are made about the order of the items
    in the model that provides the completions.

    If the model's data for the completionColumn() and completionRole() is sorted in
    ascending order, you can set this property to \l CaseSensitivelySortedModel
    or \l CaseInsensitivelySortedModel. On large models, this can lead to
    significant performance improvements because the completer object can
    then use a binary search algorithm instead of linear search algorithm.

    The sort order (i.e ascending or descending order) of the model is determined
    dynamically by inspecting the contents of the model.

    \bold{Note:} The performance improvements described above cannot take place
    when the completer's \l caseSensitivity is different to the case sensitivity
    used by the model's when sorting.

    \sa setCaseSensitivity(), QCompleter::ModelSorting
*/
void QCompleter::setModelSorting(QCompleter::ModelSorting sorting)
{
    Q_D(QCompleter);
    if (d->sorting == sorting)
        return;
    d->sorting = sorting;
    d->proxy->createEngine();
    d->proxy->invalidate();
}

QCompleter::ModelSorting QCompleter::modelSorting() const
{
    Q_D(const QCompleter);
    return d->sorting;
}

/*!
    \property QCompleter::completionColumn
    \brief the column in the model in which completions are searched for.

    If the popup() is a QListView, it is automatically setup to display
    this column.

    By default, the match column is 0.

    \sa completionRole, caseSensitivity
*/
void QCompleter::setCompletionColumn(int column)
{
    Q_D(QCompleter);
    if (d->column == column)
        return;
#ifndef QT_NO_LISTVIEW
    if (QListView *listView = qobject_cast<QListView *>(d->popup))
        listView->setModelColumn(column);
#endif
    d->column = column;
    d->proxy->invalidate();
}

int QCompleter::completionColumn() const
{
    Q_D(const QCompleter);
    return d->column;
}

/*!
    \property QCompleter::completionRole
    \brief the item role to be used to query the contents of items for matching.

    The default role is Qt::EditRole.

    \sa completionColumn, caseSensitivity
*/
void QCompleter::setCompletionRole(int role)
{
    Q_D(QCompleter);
    if (d->role == role)
        return;
    d->role = role;
    d->proxy->invalidate();
}

int QCompleter::completionRole() const
{
    Q_D(const QCompleter);
    return d->role;
}

/*!
    \property QCompleter::wrapAround
    \brief the completions wrap around when navigating through items
    \since 4.3

    The default is true.
*/
void QCompleter::setWrapAround(bool wrap)
{
    Q_D(QCompleter);
    if (d->wrap == wrap)
        return;
    d->wrap = wrap;
}

bool QCompleter::wrapAround() const
{
    Q_D(const QCompleter);
    return d->wrap;
}

/*!
    \property QCompleter::maxVisibleItems
    \brief the maximum allowed size on screen of the completer, measured in items
    \since 4.6

    By default, this property has a value of 7.
*/
int QCompleter::maxVisibleItems() const
{
    Q_D(const QCompleter);
    return d->maxVisibleItems;
}

void QCompleter::setMaxVisibleItems(int maxItems)
{
    Q_D(QCompleter);
    if (maxItems < 0) {
        qWarning("QCompleter::setMaxVisibleItems: "
                 "Invalid max visible items (%d) must be >= 0", maxItems);
        return;
    }
    d->maxVisibleItems = maxItems;
}

/*!
    \property QCompleter::caseSensitivity
    \brief the case sensitivity of the matching

    The default is Qt::CaseSensitive.

    \sa completionColumn, completionRole, modelSorting
*/
void QCompleter::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    Q_D(QCompleter);
    if (d->cs == cs)
        return;
    d->cs = cs;
    d->proxy->createEngine();
    d->proxy->invalidate();
}

Qt::CaseSensitivity QCompleter::caseSensitivity() const
{
    Q_D(const QCompleter);
    return d->cs;
}

/*!
    \property QCompleter::completionPrefix
    \brief the completion prefix used to provide completions.

    The completionModel() is updated to reflect the list of possible
    matches for \a prefix.
*/
void QCompleter::setCompletionPrefix(const QString &prefix)
{
    Q_D(QCompleter);
    d->prefix = prefix;
    d->proxy->filter(splitPath(prefix));
}

QString QCompleter::completionPrefix() const
{
    Q_D(const QCompleter);
    return d->prefix;
}

/*!
    Returns the model index of the current completion in the completionModel().

    \sa setCurrentRow(), currentCompletion(), model()
*/
QModelIndex QCompleter::currentIndex() const
{
    Q_D(const QCompleter);
    return d->proxy->currentIndex(false);
}

/*!
    Returns the current completion string. This includes the \l completionPrefix.
    When used alongside setCurrentRow(), it can be used to iterate through
    all the matches.

    \sa setCurrentRow(), currentIndex()
*/
QString QCompleter::currentCompletion() const
{
    Q_D(const QCompleter);
    return pathFromIndex(d->proxy->currentIndex(true));
}

/*!
    Returns the completion model. The completion model is a read-only list model
    that contains all the possible matches for the current completion prefix.
    The completion model is auto-updated to reflect the current completions.

    \note The return value of this function is defined to be an QAbstractItemModel
    purely for generality. This actual kind of model returned is an instance of an
    QAbstractProxyModel subclass.

    \sa completionPrefix, model()
*/
QAbstractItemModel *QCompleter::completionModel() const
{
    Q_D(const QCompleter);
    return d->proxy;
}

/*!
    Returns the path for the given \a index. The completer object uses this to
    obtain the completion text from the underlying model.

    The default implementation returns the \l{Qt::EditRole}{edit role} of the
    item for list models. It returns the absolute file path if the model is a
    QFileSystemModel.

    \sa splitPath()
*/

QString QCompleter::pathFromIndex(const QModelIndex& index) const
{
    Q_D(const QCompleter);
    if (!index.isValid())
        return QString();

    QAbstractItemModel *sourceModel = d->proxy->sourceModel();
    if (!sourceModel)
        return QString();
    bool isDirModel = false;
    bool isFsModel = false;
#ifndef QT_NO_DIRMODEL
    isDirModel = qobject_cast<QDirModel *>(d->proxy->sourceModel()) != 0;
#endif
#ifndef QT_NO_FILESYSTEMMODEL
    isFsModel = qobject_cast<QFileSystemModel *>(d->proxy->sourceModel()) != 0;
#endif
    if (!isDirModel && !isFsModel)
        return sourceModel->data(index, d->role).toString();

    QModelIndex idx = index;
    QStringList list;
    do {
        QString t;
        if (isDirModel)
            t = sourceModel->data(idx, Qt::EditRole).toString();
#ifndef QT_NO_FILESYSTEMMODEL
        else
            t = sourceModel->data(idx, QFileSystemModel::FileNameRole).toString();
#endif
        list.prepend(t);
        QModelIndex parent = idx.parent();
        idx = parent.sibling(parent.row(), index.column());
    } while (idx.isValid());

#if (!defined(Q_OS_WIN) || defined(Q_OS_WINCE)) && !defined(Q_OS_SYMBIAN)
    if (list.count() == 1) // only the separator or some other text
        return list[0];
    list[0].clear() ; // the join below will provide the separator
#endif

    return list.join(QDir::separator());
}

/*!
    Splits the given \a path into strings that are used to match at each level
    in the model().

    The default implementation of splitPath() splits a file system path based on
    QDir::separator() when the sourceModel() is a QFileSystemModel.

    When used with list models, the first item in the returned list is used for
    matching.

    \sa pathFromIndex(), {Handling Tree Models}
*/
QStringList QCompleter::splitPath(const QString& path) const
{
    bool isDirModel = false;
    bool isFsModel = false;
#ifndef QT_NO_DIRMODEL
    Q_D(const QCompleter);
    isDirModel = qobject_cast<QDirModel *>(d->proxy->sourceModel()) != 0;
#endif
#ifndef QT_NO_FILESYSTEMMODEL
#ifdef QT_NO_DIRMODEL
    Q_D(const QCompleter);
#endif
    isFsModel = qobject_cast<QFileSystemModel *>(d->proxy->sourceModel()) != 0;
#endif

    if ((!isDirModel && !isFsModel) || path.isEmpty())
        return QStringList(completionPrefix());

    QString pathCopy = QDir::toNativeSeparators(path);
    QString sep = QDir::separator();
#if defined(Q_OS_SYMBIAN)
    if (pathCopy == QLatin1String("\\"))
        return QStringList(pathCopy);
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    if (pathCopy == QLatin1String("\\") || pathCopy == QLatin1String("\\\\"))
        return QStringList(pathCopy);
    QString doubleSlash(QLatin1String("\\\\"));
    if (pathCopy.startsWith(doubleSlash))
        pathCopy = pathCopy.mid(2);
    else
        doubleSlash.clear();
#endif

    QRegExp re(QLatin1Char('[') + QRegExp::escape(sep) + QLatin1Char(']'));
    QStringList parts = pathCopy.split(re);

#if defined(Q_OS_SYMBIAN)
    // Do nothing
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    if (!doubleSlash.isEmpty())
        parts[0].prepend(doubleSlash);
#else
    if (pathCopy[0] == sep[0]) // readd the "/" at the beginning as the split removed it
        parts[0] = QDir::fromNativeSeparators(QString(sep[0]));
#endif

    return parts;
}

/*!
    \fn void QCompleter::activated(const QModelIndex& index)

    This signal is sent when an item in the popup() is activated by the user.
    (by clicking or pressing return). The item's \a index in the completionModel()
    is given.

*/

/*!
    \fn void QCompleter::activated(const QString &text)

    This signal is sent when an item in the popup() is activated by the user (by
    clicking or pressing return). The item's \a text is given.

*/

/*!
    \fn void QCompleter::highlighted(const QModelIndex& index)

    This signal is sent when an item in the popup() is highlighted by
    the user. It is also sent if complete() is called with the completionMode()
    set to QCompleter::InlineCompletion. The item's \a index in the completionModel()
    is given.
*/

/*!
    \fn void QCompleter::highlighted(const QString &text)

    This signal is sent when an item in the popup() is highlighted by
    the user. It is also sent if complete() is called with the completionMode()
    set to QCompleter::InlineCompletion. The item's \a text is given.
*/

QT_END_NAMESPACE

#include "moc_qcompleter.cpp"

#endif // QT_NO_COMPLETER
