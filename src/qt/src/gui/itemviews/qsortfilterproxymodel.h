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

#ifndef QSORTFILTERPROXYMODEL_H
#define QSORTFILTERPROXYMODEL_H

#include <QtGui/qabstractproxymodel.h>

#ifndef QT_NO_SORTFILTERPROXYMODEL

#include <QtCore/qregexp.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QSortFilterProxyModelPrivate;
class QSortFilterProxyModelLessThan;
class QSortFilterProxyModelGreaterThan;

class Q_GUI_EXPORT QSortFilterProxyModel : public QAbstractProxyModel
{
    friend class QSortFilterProxyModelLessThan;
    friend class QSortFilterProxyModelGreaterThan;

    Q_OBJECT
    Q_PROPERTY(QRegExp filterRegExp READ filterRegExp WRITE setFilterRegExp)
    Q_PROPERTY(int filterKeyColumn READ filterKeyColumn WRITE setFilterKeyColumn)
    Q_PROPERTY(bool dynamicSortFilter READ dynamicSortFilter WRITE setDynamicSortFilter)
    Q_PROPERTY(Qt::CaseSensitivity filterCaseSensitivity READ filterCaseSensitivity WRITE setFilterCaseSensitivity)
    Q_PROPERTY(Qt::CaseSensitivity sortCaseSensitivity READ sortCaseSensitivity WRITE setSortCaseSensitivity)
    Q_PROPERTY(bool isSortLocaleAware READ isSortLocaleAware WRITE setSortLocaleAware)
    Q_PROPERTY(int sortRole READ sortRole WRITE setSortRole)
    Q_PROPERTY(int filterRole READ filterRole WRITE setFilterRole)

public:
    QSortFilterProxyModel(QObject *parent = 0);
    ~QSortFilterProxyModel();

    void setSourceModel(QAbstractItemModel *sourceModel);

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    QItemSelection mapSelectionToSource(const QItemSelection &proxySelection) const;
    QItemSelection mapSelectionFromSource(const QItemSelection &sourceSelection) const;

    QRegExp filterRegExp() const;
    void setFilterRegExp(const QRegExp &regExp);

    int filterKeyColumn() const;
    void setFilterKeyColumn(int column);

    Qt::CaseSensitivity filterCaseSensitivity() const;
    void setFilterCaseSensitivity(Qt::CaseSensitivity cs);

    Qt::CaseSensitivity sortCaseSensitivity() const;
    void setSortCaseSensitivity(Qt::CaseSensitivity cs);

    bool isSortLocaleAware() const;
    void setSortLocaleAware(bool on);

    int sortColumn() const;
    Qt::SortOrder sortOrder() const;

    bool dynamicSortFilter() const;
    void setDynamicSortFilter(bool enable);

    int sortRole() const;
    void setSortRole(int role);

    int filterRole() const;
    void setFilterRole(int role);

public Q_SLOTS:
    void setFilterRegExp(const QString &pattern);
    void setFilterWildcard(const QString &pattern);
    void setFilterFixedString(const QString &pattern);
    void clear();
    void invalidate();

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    virtual bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

    void filterChanged();
    void invalidateFilter();

public:
#ifdef Q_NO_USING_KEYWORD
    inline QObject *parent() const { return QObject::parent(); }
#else
    using QObject::parent;
#endif

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setHeaderData(int section, Qt::Orientation orientation,
            const QVariant &value, int role = Qt::EditRole);

    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex());

    void fetchMore(const QModelIndex &parent);
    bool canFetchMore(const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QModelIndex buddy(const QModelIndex &index) const;
    QModelIndexList match(const QModelIndex &start, int role,
                          const QVariant &value, int hits = 1,
                          Qt::MatchFlags flags =
                          Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
    QSize span(const QModelIndex &index) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;
private:
    Q_DECLARE_PRIVATE(QSortFilterProxyModel)
    Q_DISABLE_COPY(QSortFilterProxyModel)

    Q_PRIVATE_SLOT(d_func(), void _q_sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceHeaderDataChanged(Qt::Orientation orientation, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceAboutToBeReset())
    Q_PRIVATE_SLOT(d_func(), void _q_sourceReset())
    Q_PRIVATE_SLOT(d_func(), void _q_sourceLayoutAboutToBeChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_sourceLayoutChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceRowsInserted(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceRowsRemoved(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceColumnsAboutToBeInserted(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceColumnsInserted(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_sourceColumnsRemoved(const QModelIndex &source_parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_clearMapping())
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_SORTFILTERPROXYMODEL

#endif // QSORTFILTERPROXYMODEL_H
