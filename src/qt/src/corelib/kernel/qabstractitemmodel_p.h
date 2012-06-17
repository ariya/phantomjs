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

#ifndef QABSTRACTITEMMODEL_P_H
#define QABSTRACTITEMMODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QAbstractItemModel*.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include "private/qobject_p.h"
#include "QtCore/qstack.h"
#include "QtCore/qset.h"
#include "QtCore/qhash.h"

QT_BEGIN_NAMESPACE

class QPersistentModelIndexData
{
public:
    QPersistentModelIndexData() : model(0) {}
    QPersistentModelIndexData(const QModelIndex &idx) : index(idx), model(idx.model()) {}
    QModelIndex index;
    QAtomicInt ref;
    const QAbstractItemModel *model;
    static QPersistentModelIndexData *create(const QModelIndex &index);
    static void destroy(QPersistentModelIndexData *data);
};

class Q_CORE_EXPORT QAbstractItemModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemModel)

public:
    QAbstractItemModelPrivate() : QObjectPrivate(), supportedDragActions(-1), roleNames(defaultRoleNames()) {}
    void removePersistentIndexData(QPersistentModelIndexData *data);
    void movePersistentIndexes(QVector<QPersistentModelIndexData *> indexes, int change, const QModelIndex &parent, Qt::Orientation orientation);
    void rowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void rowsInserted(const QModelIndex &parent, int first, int last);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void rowsRemoved(const QModelIndex &parent, int first, int last);
    void columnsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void columnsInserted(const QModelIndex &parent, int first, int last);
    void columnsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void columnsRemoved(const QModelIndex &parent, int first, int last);
    static QAbstractItemModel *staticEmptyModel();
    static bool variantLessThan(const QVariant &v1, const QVariant &v2);

    void itemsAboutToBeMoved(const QModelIndex &srcParent, int srcFirst, int srcLast, const QModelIndex &destinationParent, int destinationChild, Qt::Orientation);
    void itemsMoved(const QModelIndex &srcParent, int srcFirst, int srcLast, const QModelIndex &destinationParent, int destinationChild, Qt::Orientation orientation);
    bool allowMove(const QModelIndex &srcParent, int srcFirst, int srcLast, const QModelIndex &destinationParent, int destinationChild, Qt::Orientation orientation);
    
    inline QModelIndex createIndex(int row, int column, void *data = 0) const {
        return q_func()->createIndex(row, column, data);
    }

    inline QModelIndex createIndex(int row, int column, int id) const {
        return q_func()->createIndex(row, column, id);
    }

    inline bool indexValid(const QModelIndex &index) const {
         return (index.row() >= 0) && (index.column() >= 0) && (index.model() == q_func());
    }

    inline void invalidatePersistentIndexes() {
        foreach (QPersistentModelIndexData *data, persistent.indexes) {
            data->index = QModelIndex();
            data->model = 0;
        }
        persistent.indexes.clear();
    }

    /*!
      \internal
      clean the QPersistentModelIndex relative to the index if there is one.
      To be used before an index is invalided
      */
    inline void invalidatePersistentIndex(const QModelIndex &index) {
        QHash<QModelIndex, QPersistentModelIndexData *>::iterator it = persistent.indexes.find(index);
        if(it != persistent.indexes.end()) {
            QPersistentModelIndexData *data = *it;
            persistent.indexes.erase(it);
            data->index = QModelIndex();
            data->model = 0;
        }
    }

    struct Change {
        Change() : first(-1), last(-1) {}
        Change(const Change &c) : parent(c.parent), first(c.first), last(c.last), needsAdjust(c.needsAdjust) {}
        Change(const QModelIndex &p, int f, int l) : parent(p), first(f), last(l), needsAdjust(false) {}
        QModelIndex parent;
        int first, last;


        // In cases such as this:
        // - A
        // - B
        // - C
        // - - D
        // - - E
        // - - F
        //
        // If B is moved to above E, C is the source parent in the signal and its row is 2. When the move is
        // completed however, C is at row 1 and there is no row 2 at the same level in the model at all.
        // The QModelIndex is adjusted to correct that in those cases before reporting it though the
        // rowsMoved signal.
        bool needsAdjust;

        bool isValid() { return first >= 0 && last >= 0; }
    };
    QStack<Change> changes;

    struct Persistent {
        Persistent() {}
        QHash<QModelIndex, QPersistentModelIndexData *> indexes;
        QStack<QVector<QPersistentModelIndexData *> > moved;
        QStack<QVector<QPersistentModelIndexData *> > invalidated;
        void insertMultiAtEnd(const QModelIndex& key, QPersistentModelIndexData *data);
    } persistent;

    Qt::DropActions supportedDragActions;

    QHash<int,QByteArray> roleNames;
    static const QHash<int,QByteArray> &defaultRoleNames();
};

QT_END_NAMESPACE

#endif // QABSTRACTITEMMODEL_P_H
