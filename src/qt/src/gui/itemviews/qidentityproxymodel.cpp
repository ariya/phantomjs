/****************************************************************************
**
** Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
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

#include "qidentityproxymodel.h"

#ifndef QT_NO_IDENTITYPROXYMODEL

#include "qitemselectionmodel.h"
#include <private/qabstractproxymodel_p.h>

QT_BEGIN_NAMESPACE

class QIdentityProxyModelPrivate : public QAbstractProxyModelPrivate
{
  QIdentityProxyModelPrivate()
    : ignoreNextLayoutAboutToBeChanged(false),
      ignoreNextLayoutChanged(false)
  {

  }

  Q_DECLARE_PUBLIC(QIdentityProxyModel)

  bool ignoreNextLayoutAboutToBeChanged;
  bool ignoreNextLayoutChanged;
  QList<QPersistentModelIndex> layoutChangePersistentIndexes;
  QModelIndexList proxyIndexes;

  void _q_sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
  void _q_sourceRowsInserted(const QModelIndex &parent, int start, int end);
  void _q_sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
  void _q_sourceRowsRemoved(const QModelIndex &parent, int start, int end);
  void _q_sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);
  void _q_sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);

  void _q_sourceColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
  void _q_sourceColumnsInserted(const QModelIndex &parent, int start, int end);
  void _q_sourceColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
  void _q_sourceColumnsRemoved(const QModelIndex &parent, int start, int end);
  void _q_sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);
  void _q_sourceColumnsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);

  void _q_sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  void _q_sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last);

  void _q_sourceLayoutAboutToBeChanged();
  void _q_sourceLayoutChanged();
  void _q_sourceModelAboutToBeReset();
  void _q_sourceModelReset();

};

/*!
    \since 4.8
    \class QIdentityProxyModel
    \brief The QIdentityProxyModel class proxies its source model unmodified

    \ingroup model-view

    QIdentityProxyModel can be used to forward the structure of a source model exactly, with no sorting, filtering or other transformation.
    This is similar in concept to an identity matrix where A.I = A.

    Because it does no sorting or filtering, this class is most suitable to proxy models which transform the data() of the source model.
    For example, a proxy model could be created to define the font used, or the background colour, or the tooltip etc. This removes the
    need to implement all data handling in the same class that creates the structure of the model, and can also be used to create
    re-usable components.

    This also provides a way to change the data in the case where a source model is supplied by a third party which can not be modified.

    \snippet doc/src/snippets/code/src_gui_itemviews_qidentityproxymodel.cpp 0

    \sa QAbstractProxyModel, {Model/View Programming}, QAbstractItemModel

*/

/*!
    Constructs an identity model with the given \a parent.
*/
QIdentityProxyModel::QIdentityProxyModel(QObject* parent)
  : QAbstractProxyModel(*new QIdentityProxyModelPrivate, parent)
{

}

/*! \internal
 */
QIdentityProxyModel::QIdentityProxyModel(QIdentityProxyModelPrivate &dd, QObject* parent)
  : QAbstractProxyModel(dd, parent)
{

}

/*!
    Destroys this identity model.
*/
QIdentityProxyModel::~QIdentityProxyModel()
{
}

/*!
    \reimp
 */
int QIdentityProxyModel::columnCount(const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    return d->model->columnCount(mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->dropMimeData(data, action, row, column, mapToSource(parent));
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    const QModelIndex sourceParent = mapToSource(parent);
    const QModelIndex sourceIndex = d->model->index(row, column, sourceParent);
    Q_ASSERT(sourceIndex.isValid());
    return mapFromSource(sourceIndex);
}

/*!
    \reimp
 */
bool QIdentityProxyModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->insertColumns(column, count, mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->insertRows(row, count, mapToSource(parent));
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    Q_D(const QIdentityProxyModel);
    if (!d->model || !sourceIndex.isValid())
        return QModelIndex();

    Q_ASSERT(sourceIndex.model() == d->model);
    return createIndex(sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer());
}

/*!
    \reimp
 */
QItemSelection QIdentityProxyModel::mapSelectionFromSource(const QItemSelection& selection) const
{
    Q_D(const QIdentityProxyModel);
    QItemSelection proxySelection;

    if (!d->model)
        return proxySelection;

    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    for ( ; it != end; ++it) {
        Q_ASSERT(it->model() == d->model);
        const QItemSelectionRange range(mapFromSource(it->topLeft()), mapFromSource(it->bottomRight()));
        proxySelection.append(range);
    }

    return proxySelection;
}

/*!
    \reimp
 */
QItemSelection QIdentityProxyModel::mapSelectionToSource(const QItemSelection& selection) const
{
    Q_D(const QIdentityProxyModel);
    QItemSelection sourceSelection;

    if (!d->model)
        return sourceSelection;

    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    for ( ; it != end; ++it) {
        Q_ASSERT(it->model() == this);
        const QItemSelectionRange range(mapToSource(it->topLeft()), mapToSource(it->bottomRight()));
        sourceSelection.append(range);
    }

    return sourceSelection;
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    Q_D(const QIdentityProxyModel);
    if (!d->model || !proxyIndex.isValid())
        return QModelIndex();
    Q_ASSERT(proxyIndex.model() == this);
    return d->model->createIndex(proxyIndex.row(), proxyIndex.column(), proxyIndex.internalPointer());
}

/*!
    \reimp
 */
QModelIndexList QIdentityProxyModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    Q_D(const QIdentityProxyModel);
    Q_ASSERT(start.isValid() ? start.model() == this : true);
    if (!d->model)
        return QModelIndexList();

    const QModelIndexList sourceList = d->model->match(mapToSource(start), role, value, hits, flags);
    QModelIndexList::const_iterator it = sourceList.constBegin();
    const QModelIndexList::const_iterator end = sourceList.constEnd();
    QModelIndexList proxyList;
    for ( ; it != end; ++it)
        proxyList.append(mapFromSource(*it));
    return proxyList;
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::parent(const QModelIndex& child) const
{
    Q_ASSERT(child.isValid() ? child.model() == this : true);
    const QModelIndex sourceIndex = mapToSource(child);
    const QModelIndex sourceParent = sourceIndex.parent();
    return mapFromSource(sourceParent);
}

/*!
    \reimp
 */
bool QIdentityProxyModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->removeColumns(column, count, mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->removeRows(row, count, mapToSource(parent));
}

/*!
    \reimp
 */
int QIdentityProxyModel::rowCount(const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    return d->model->rowCount(mapToSource(parent));
}

/*!
    \reimp
 */
void QIdentityProxyModel::setSourceModel(QAbstractItemModel* newSourceModel)
{
    beginResetModel();

    if (sourceModel()) {
        disconnect(sourceModel(), SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceRowsInserted(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceRowsRemoved(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                   this, SLOT(_q_sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        disconnect(sourceModel(), SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                   this, SLOT(_q_sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceColumnsAboutToBeInserted(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(columnsInserted(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceColumnsInserted(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceColumnsAboutToBeRemoved(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
                   this, SLOT(_q_sourceColumnsRemoved(const QModelIndex &, int, int)));
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                   this, SLOT(_q_sourceColumnsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        disconnect(sourceModel(), SIGNAL(columnsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                   this, SLOT(_q_sourceColumnsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        disconnect(sourceModel(), SIGNAL(modelAboutToBeReset()),
                   this, SLOT(_q_sourceModelAboutToBeReset()));
        disconnect(sourceModel(), SIGNAL(modelReset()),
                   this, SLOT(_q_sourceModelReset()));
        disconnect(sourceModel(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                   this, SLOT(_q_sourceDataChanged(const QModelIndex &, const QModelIndex &)));
        disconnect(sourceModel(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                   this, SLOT(_q_sourceHeaderDataChanged(Qt::Orientation,int,int)));
        disconnect(sourceModel(), SIGNAL(layoutAboutToBeChanged()),
                   this, SLOT(_q_sourceLayoutAboutToBeChanged()));
        disconnect(sourceModel(), SIGNAL(layoutChanged()),
                   this, SLOT(_q_sourceLayoutChanged()));
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (sourceModel()) {
        connect(sourceModel(), SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
                SLOT(_q_sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
                SLOT(_q_sourceRowsInserted(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
                SLOT(_q_sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
                SLOT(_q_sourceRowsRemoved(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                SLOT(_q_sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        connect(sourceModel(), SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                SLOT(_q_sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        connect(sourceModel(), SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
                SLOT(_q_sourceColumnsAboutToBeInserted(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(columnsInserted(const QModelIndex &, int, int)),
                SLOT(_q_sourceColumnsInserted(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
                SLOT(_q_sourceColumnsAboutToBeRemoved(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
                SLOT(_q_sourceColumnsRemoved(const QModelIndex &, int, int)));
        connect(sourceModel(), SIGNAL(columnsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                SLOT(_q_sourceColumnsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        connect(sourceModel(), SIGNAL(columnsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
                SLOT(_q_sourceColumnsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        connect(sourceModel(), SIGNAL(modelAboutToBeReset()),
                SLOT(_q_sourceModelAboutToBeReset()));
        connect(sourceModel(), SIGNAL(modelReset()),
                SLOT(_q_sourceModelReset()));
        connect(sourceModel(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                SLOT(_q_sourceDataChanged(const QModelIndex &, const QModelIndex &)));
        connect(sourceModel(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                SLOT(_q_sourceHeaderDataChanged(Qt::Orientation,int,int)));
        connect(sourceModel(), SIGNAL(layoutAboutToBeChanged()),
                SLOT(_q_sourceLayoutAboutToBeChanged()));
        connect(sourceModel(), SIGNAL(layoutChanged()),
                SLOT(_q_sourceLayoutChanged()));
    }

    endResetModel();
}

void QIdentityProxyModelPrivate::_q_sourceColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginInsertColumns(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginMoveColumns(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destParent), dest);
}

void QIdentityProxyModelPrivate::_q_sourceColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginRemoveColumns(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceColumnsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    q->endInsertColumns();
}

void QIdentityProxyModelPrivate::_q_sourceColumnsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destParent)
    Q_UNUSED(dest)
    q->endMoveColumns();
}

void QIdentityProxyModelPrivate::_q_sourceColumnsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    q->endRemoveColumns();
}

void QIdentityProxyModelPrivate::_q_sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_ASSERT(topLeft.isValid() ? topLeft.model() == model : true);
    Q_ASSERT(bottomRight.isValid() ? bottomRight.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->dataChanged(q->mapFromSource(topLeft), q->mapFromSource(bottomRight));
}

void QIdentityProxyModelPrivate::_q_sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    Q_Q(QIdentityProxyModel);
    q->headerDataChanged(orientation, first, last);
}

void QIdentityProxyModelPrivate::_q_sourceLayoutAboutToBeChanged()
{
    if (ignoreNextLayoutAboutToBeChanged)
        return;

    Q_Q(QIdentityProxyModel);

    foreach(const QPersistentModelIndex &proxyPersistentIndex, q->persistentIndexList()) {
        proxyIndexes << proxyPersistentIndex;
        Q_ASSERT(proxyPersistentIndex.isValid());
        const QPersistentModelIndex srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
        Q_ASSERT(srcPersistentIndex.isValid());
        layoutChangePersistentIndexes << srcPersistentIndex;
    }

    q->layoutAboutToBeChanged();
}

void QIdentityProxyModelPrivate::_q_sourceLayoutChanged()
{
    if (ignoreNextLayoutChanged)
        return;

    Q_Q(QIdentityProxyModel);

    for (int i = 0; i < proxyIndexes.size(); ++i) {
        q->changePersistentIndex(proxyIndexes.at(i), q->mapFromSource(layoutChangePersistentIndexes.at(i)));
    }

    layoutChangePersistentIndexes.clear();
    proxyIndexes.clear();

    q->layoutChanged();
}

void QIdentityProxyModelPrivate::_q_sourceModelAboutToBeReset()
{
    Q_Q(QIdentityProxyModel);
    q->beginResetModel();
}

void QIdentityProxyModelPrivate::_q_sourceModelReset()
{
    Q_Q(QIdentityProxyModel);
    q->endResetModel();
}

void QIdentityProxyModelPrivate::_q_sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginInsertRows(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginMoveRows(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destParent), dest);
}

void QIdentityProxyModelPrivate::_q_sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginRemoveRows(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    q->endInsertRows();
}

void QIdentityProxyModelPrivate::_q_sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destParent)
    Q_UNUSED(dest)
    q->endMoveRows();
}

void QIdentityProxyModelPrivate::_q_sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    q->endRemoveRows();
}

QT_END_NAMESPACE

#include "moc_qidentityproxymodel.cpp"

#endif // QT_NO_IDENTITYPROXYMODEL
