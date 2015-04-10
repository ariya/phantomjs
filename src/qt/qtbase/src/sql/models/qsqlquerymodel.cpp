/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSql module of the Qt Toolkit.
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

#include "qsqlquerymodel.h"
#include "qsqlquerymodel_p.h"

#include <qdebug.h>
#include <qsqldriver.h>
#include <qsqlfield.h>

QT_BEGIN_NAMESPACE

#define QSQL_PREFETCH 255

void QSqlQueryModelPrivate::prefetch(int limit)
{
    Q_Q(QSqlQueryModel);

    if (atEnd || limit <= bottom.row() || bottom.column() == -1)
        return;

    QModelIndex newBottom;
    const int oldBottomRow = qMax(bottom.row(), 0);

    // try to seek directly
    if (query.seek(limit)) {
        newBottom = q->createIndex(limit, bottom.column());
    } else {
        // have to seek back to our old position for MS Access
        int i = oldBottomRow;
        if (query.seek(i)) {
            while (query.next())
                ++i;
            newBottom = q->createIndex(i, bottom.column());
        } else {
            // empty or invalid query
            newBottom = q->createIndex(-1, bottom.column());
        }
        atEnd = true; // this is the end.
    }
    if (newBottom.row() >= 0 && newBottom.row() > bottom.row()) {
        q->beginInsertRows(QModelIndex(), bottom.row() + 1, newBottom.row());
        bottom = newBottom;
        q->endInsertRows();
    } else {
        bottom = newBottom;
    }
}

QSqlQueryModelPrivate::~QSqlQueryModelPrivate()
{
}

void QSqlQueryModelPrivate::initColOffsets(int size)
{
    colOffsets.resize(size);
    memset(colOffsets.data(), 0, colOffsets.size() * sizeof(int));
}

int QSqlQueryModelPrivate::columnInQuery(int modelColumn) const
{
    if (modelColumn < 0 || modelColumn >= rec.count() || !rec.isGenerated(modelColumn) || modelColumn >= colOffsets.size())
        return -1;
    return modelColumn - colOffsets[modelColumn];
}

/*!
    \class QSqlQueryModel
    \brief The QSqlQueryModel class provides a read-only data model for SQL
    result sets.

    \ingroup database
    \inmodule QtSql

    QSqlQueryModel is a high-level interface for executing SQL
    statements and traversing the result set. It is built on top of
    the lower-level QSqlQuery and can be used to provide data to
    view classes such as QTableView. For example:

    \snippet sqldatabase/sqldatabase.cpp 16

    We set the model's query, then we set up the labels displayed in
    the view header.

    QSqlQueryModel can also be used to access a database
    programmatically, without binding it to a view:

    \snippet sqldatabase/sqldatabase.cpp 21

    The code snippet above extracts the \c salary field from record 4 in
    the result set of the query \c{SELECT * from employee}. Assuming
    that \c salary is column 2, we can rewrite the last line as follows:

    \snippet sqldatabase/sqldatabase.cpp 22

    The model is read-only by default. To make it read-write, you
    must subclass it and reimplement setData() and flags(). Another
    option is to use QSqlTableModel, which provides a read-write
    model based on a single database table.

    The \l{querymodel} example illustrates how to use
    QSqlQueryModel to display the result of a query. It also shows
    how to subclass QSqlQueryModel to customize the contents of the
    data before showing it to the user, and how to create a
    read-write model based on QSqlQueryModel.

    If the database doesn't return the number of selected rows in
    a query, the model will fetch rows incrementally.
    See fetchMore() for more information.

    \sa QSqlTableModel, QSqlRelationalTableModel, QSqlQuery,
        {Model/View Programming}, {Query Model Example}
*/

/*!
    Creates an empty QSqlQueryModel with the given \a parent.
 */
QSqlQueryModel::QSqlQueryModel(QObject *parent)
    : QAbstractTableModel(*new QSqlQueryModelPrivate, parent)
{
}

/*! \internal
 */
QSqlQueryModel::QSqlQueryModel(QSqlQueryModelPrivate &dd, QObject *parent)
    : QAbstractTableModel(dd, parent)
{
}

/*!
    Destroys the object and frees any allocated resources.

    \sa clear()
*/
QSqlQueryModel::~QSqlQueryModel()
{
}

/*!
    \since 4.1

    Fetches more rows from a database.
    This only affects databases that don't report back the size of a query
    (see QSqlDriver::hasFeature()).

    To force fetching of the entire result set, you can use the following:

    \snippet code/src_sql_models_qsqlquerymodel.cpp 0

    \a parent should always be an invalid QModelIndex.

    \sa canFetchMore()
*/
void QSqlQueryModel::fetchMore(const QModelIndex &parent)
{
    Q_D(QSqlQueryModel);
    if (parent.isValid())
        return;
    d->prefetch(qMax(d->bottom.row(), 0) + QSQL_PREFETCH);
}

/*!
    \since 4.1

    Returns \c true if it is possible to read more rows from the database.
    This only affects databases that don't report back the size of a query
    (see QSqlDriver::hasFeature()).

    \a parent should always be an invalid QModelIndex.

    \sa fetchMore()
 */
bool QSqlQueryModel::canFetchMore(const QModelIndex &parent) const
{
    Q_D(const QSqlQueryModel);
    return (!parent.isValid() && !d->atEnd);
}

/*! \internal
 */
void QSqlQueryModel::beginInsertRows(const QModelIndex &parent, int first, int last)
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::beginInsertRows(parent, first, last);
}

/*! \internal
 */
void QSqlQueryModel::endInsertRows()
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::endInsertRows();
}

/*! \internal
 */
void QSqlQueryModel::beginRemoveRows(const QModelIndex &parent, int first, int last)
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::beginRemoveRows(parent, first, last);
}

/*! \internal
 */
void QSqlQueryModel::endRemoveRows()
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::endRemoveRows();
}

/*! \internal
 */
void QSqlQueryModel::beginInsertColumns(const QModelIndex &parent, int first, int last)
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::beginInsertColumns(parent, first, last);
}

/*! \internal
 */
void QSqlQueryModel::endInsertColumns()
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::endInsertColumns();
}

/*! \internal
 */
void QSqlQueryModel::beginRemoveColumns(const QModelIndex &parent, int first, int last)
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::beginRemoveColumns(parent, first, last);
}

/*! \internal
 */
void QSqlQueryModel::endRemoveColumns()
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::endRemoveColumns();
}

/*! \internal
 */
void QSqlQueryModel::beginResetModel()
{
    Q_D(QSqlQueryModel);
    if (!d->nestedResetLevel)
        QAbstractTableModel::beginResetModel();
    ++d->nestedResetLevel;
}

/*! \internal
 */
void QSqlQueryModel::endResetModel()
{
    Q_D(QSqlQueryModel);
    --d->nestedResetLevel;
    if (!d->nestedResetLevel)
        QAbstractTableModel::endResetModel();
}

/*! \fn int QSqlQueryModel::rowCount(const QModelIndex &parent) const
    \since 4.1

    If the database supports returning the size of a query
    (see QSqlDriver::hasFeature()), the number of rows of the current
    query is returned. Otherwise, returns the number of rows
    currently cached on the client.

    \a parent should always be an invalid QModelIndex.

    \sa canFetchMore(), QSqlDriver::hasFeature()
 */
int QSqlQueryModel::rowCount(const QModelIndex &index) const
{
    Q_D(const QSqlQueryModel);
    return index.isValid() ? 0 : d->bottom.row() + 1;
}

/*! \reimp
 */
int QSqlQueryModel::columnCount(const QModelIndex &index) const
{
    Q_D(const QSqlQueryModel);
    return index.isValid() ? 0 : d->rec.count();
}

/*!
    Returns the value for the specified \a item and \a role.

    If \a item is out of bounds or if an error occurred, an invalid
    QVariant is returned.

    \sa lastError()
*/
QVariant QSqlQueryModel::data(const QModelIndex &item, int role) const
{
    Q_D(const QSqlQueryModel);
    if (!item.isValid())
        return QVariant();

    QVariant v;
    if (role & ~(Qt::DisplayRole | Qt::EditRole))
        return v;

    if (!d->rec.isGenerated(item.column()))
        return v;
    QModelIndex dItem = indexInQuery(item);
    if (dItem.row() > d->bottom.row())
        const_cast<QSqlQueryModelPrivate *>(d)->prefetch(dItem.row());

    if (!d->query.seek(dItem.row())) {
        d->error = d->query.lastError();
        return v;
    }

    return d->query.value(dItem.column());
}

/*!
    Returns the header data for the given \a role in the \a section
    of the header with the specified \a orientation.
*/
QVariant QSqlQueryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QSqlQueryModel);
    if (orientation == Qt::Horizontal) {
        QVariant val = d->headers.value(section).value(role);
        if (role == Qt::DisplayRole && !val.isValid())
            val = d->headers.value(section).value(Qt::EditRole);
        if (val.isValid())
            return val;
        if (role == Qt::DisplayRole && d->rec.count() > section && d->columnInQuery(section) != -1)
            return d->rec.fieldName(section);
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

/*!
    This virtual function is called whenever the query changes. The
    default implementation does nothing.

    query() returns the new query.

    \sa query(), setQuery()
 */
void QSqlQueryModel::queryChange()
{
    // do nothing
}

/*!
    Resets the model and sets the data provider to be the given \a
    query. Note that the query must be active and must not be
    isForwardOnly().

    lastError() can be used to retrieve verbose information if there
    was an error setting the query.

    \note Calling setQuery() will remove any inserted columns.

    \sa query(), QSqlQuery::isActive(), QSqlQuery::setForwardOnly(), lastError()
*/
void QSqlQueryModel::setQuery(const QSqlQuery &query)
{
    Q_D(QSqlQueryModel);
    beginResetModel();

    QSqlRecord newRec = query.record();
    bool columnsChanged = (newRec != d->rec);

    if (d->colOffsets.size() != newRec.count() || columnsChanged)
        d->initColOffsets(newRec.count());

    d->bottom = QModelIndex();
    d->error = QSqlError();
    d->query = query;
    d->rec = newRec;
    d->atEnd = true;

    if (query.isForwardOnly()) {
        d->error = QSqlError(QLatin1String("Forward-only queries "
                                           "cannot be used in a data model"),
                             QString(), QSqlError::ConnectionError);
        endResetModel();
        return;
    }

    if (!query.isActive()) {
        d->error = query.lastError();
        endResetModel();
        return;
    }

    if (query.driver()->hasFeature(QSqlDriver::QuerySize) && d->query.size() > 0) {
        d->bottom = createIndex(d->query.size() - 1, d->rec.count() - 1);
    } else {
        d->bottom = createIndex(-1, d->rec.count() - 1);
        d->atEnd = false;
    }


    // fetchMore does the rowsInserted stuff for incremental models
    fetchMore();

    endResetModel();
    queryChange();
}

/*! \overload

    Executes the query \a query for the given database connection \a
    db. If no database (or an invalid database) is specified, the
    default connection is used.

    lastError() can be used to retrieve verbose information if there
    was an error setting the query.

    Example:
    \snippet code/src_sql_models_qsqlquerymodel.cpp 1

    \sa query(), queryChange(), lastError()
*/
void QSqlQueryModel::setQuery(const QString &query, const QSqlDatabase &db)
{
    setQuery(QSqlQuery(query, db));
}

/*!
    Clears the model and releases any acquired resource.
*/
void QSqlQueryModel::clear()
{
    Q_D(QSqlQueryModel);
    d->error = QSqlError();
    d->atEnd = true;
    d->query.clear();
    d->rec.clear();
    d->colOffsets.clear();
    d->bottom = QModelIndex();
    d->headers.clear();
}

/*!
    Sets the caption for a horizontal header for the specified \a role to
    \a value. This is useful if the model is used to
    display data in a view (e.g., QTableView).

    Returns \c true if \a orientation is Qt::Horizontal and
    the \a section refers to a valid section; otherwise returns
    false.

    Note that this function cannot be used to modify values in the
    database since the model is read-only.

    \sa data()
 */
bool QSqlQueryModel::setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant &value, int role)
{
    Q_D(QSqlQueryModel);
    if (orientation != Qt::Horizontal || section < 0 || columnCount() <= section)
        return false;

    if (d->headers.size() <= section)
        d->headers.resize(qMax(section + 1, 16));
    d->headers[section][role] = value;
    emit headerDataChanged(orientation, section, section);
    return true;
}

/*!
    Returns the QSqlQuery associated with this model.

    \sa setQuery()
*/
QSqlQuery QSqlQueryModel::query() const
{
    Q_D(const QSqlQueryModel);
    return d->query;
}

/*!
    Returns information about the last error that occurred on the
    database.

    \sa query()
*/
QSqlError QSqlQueryModel::lastError() const
{
    Q_D(const QSqlQueryModel);
    return d->error;
}

/*!
   Protected function which allows derived classes to set the value of
   the last error that occurred on the database to \a error.

   \sa lastError()
*/
void QSqlQueryModel::setLastError(const QSqlError &error)
{
    Q_D(QSqlQueryModel);
    d->error = error;
}

/*!
    Returns the record containing information about the fields of the
    current query. If \a row is the index of a valid row, the record
    will be populated with values from that row.

    If the model is not initialized, an empty record will be
    returned.

    \sa QSqlRecord::isEmpty()
*/
QSqlRecord QSqlQueryModel::record(int row) const
{
    Q_D(const QSqlQueryModel);
    if (row < 0)
        return d->rec;

    QSqlRecord rec = d->rec;
    for (int i = 0; i < rec.count(); ++i)
        rec.setValue(i, data(createIndex(row, i), Qt::EditRole));
    return rec;
}

/*! \overload

    Returns an empty record containing information about the fields
    of the current query.

    If the model is not initialized, an empty record will be
    returned.

    \sa QSqlRecord::isEmpty()
 */
QSqlRecord QSqlQueryModel::record() const
{
    Q_D(const QSqlQueryModel);
    return d->rec;
}

/*!
    Inserts \a count columns into the model at position \a column. The
    \a parent parameter must always be an invalid QModelIndex, since
    the model does not support parent-child relationships.

    Returns \c true if \a column is within bounds; otherwise returns \c false.

    By default, inserted columns are empty. To fill them with data,
    reimplement data() and handle any inserted column separately:

    \snippet sqldatabase/sqldatabase.cpp 23

    \sa removeColumns()
*/
bool QSqlQueryModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QSqlQueryModel);
    if (count <= 0 || parent.isValid() || column < 0 || column > d->rec.count())
        return false;

    beginInsertColumns(parent, column, column + count - 1);
    for (int c = 0; c < count; ++c) {
        QSqlField field;
        field.setReadOnly(true);
        field.setGenerated(false);
        d->rec.insert(column, field);
        if (d->colOffsets.size() < d->rec.count()) {
            int nVal = d->colOffsets.isEmpty() ? 0 : d->colOffsets[d->colOffsets.size() - 1];
            d->colOffsets.append(nVal);
            Q_ASSERT(d->colOffsets.size() >= d->rec.count());
        }
        for (int i = column + 1; i < d->colOffsets.count(); ++i)
            ++d->colOffsets[i];
    }
    endInsertColumns();
    return true;
}

/*!
    Removes \a count columns from the model starting from position \a
    column. The \a parent parameter must always be an invalid
    QModelIndex, since the model does not support parent-child
    relationships.

    Removing columns effectively hides them. It does not affect the
    underlying QSqlQuery.

    Returns \c true if the columns were removed; otherwise returns \c false.
 */
bool QSqlQueryModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QSqlQueryModel);
    if (count <= 0 || parent.isValid() || column < 0 || column >= d->rec.count())
        return false;

    beginRemoveColumns(parent, column, column + count - 1);

    int i;
    for (i = 0; i < count; ++i)
        d->rec.remove(column);
    for (i = column; i < d->colOffsets.count(); ++i)
        d->colOffsets[i] -= count;

    endRemoveColumns();
    return true;
}

/*!
    Returns the index of the value in the database result set for the
    given \a item in the model.

    The return value is identical to \a item if no columns or rows
    have been inserted, removed, or moved around.

    Returns an invalid model index if \a item is out of bounds or if
    \a item does not point to a value in the result set.

    \sa QSqlTableModel::indexInQuery(), insertColumns(), removeColumns()
*/
QModelIndex QSqlQueryModel::indexInQuery(const QModelIndex &item) const
{
    Q_D(const QSqlQueryModel);
    int modelColumn = d->columnInQuery(item.column());
    if (modelColumn < 0)
        return QModelIndex();
    return createIndex(item.row(), modelColumn, item.internalPointer());
}

QT_END_NAMESPACE
