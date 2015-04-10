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

#include "qsqlrelationaltablemodel.h"

#include "qhash.h"
#include "qstringlist.h"
#include "qsqldatabase.h"
#include "qsqldriver.h"
#include "qsqlerror.h"
#include "qsqlfield.h"
#include "qsqlindex.h"
#include "qsqlquery.h"
#include "qsqlrecord.h"

#include "qsqltablemodel_p.h"

#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QSqlRelationalTableModelSql: public QSqlTableModelSql
{
public:
    inline const static QString relTablePrefix(int i) { return QString::number(i).prepend(QLatin1String("relTblAl_")); }
};

typedef QSqlRelationalTableModelSql Sql;

/*!
    \class QSqlRelation
    \inmodule QtSql
    \brief The QSqlRelation class stores information about an SQL foreign key.

    QSqlRelation is a helper class for QSqlRelationalTableModel. See
    QSqlRelationalTableModel::setRelation() and
    QSqlRelationalTableModel::relation() for details.

    \sa QSqlRelationalTableModel, QSqlRelationalDelegate,
        {Relational Table Model Example}
*/

/*!
    \fn QSqlRelation::QSqlRelation()

    Constructs an invalid QSqlRelation object.

    For such an object, the tableName(), indexColumn(), and
    displayColumn() functions return an empty string.

    \sa isValid()
*/

/*!
    \fn QSqlRelation::QSqlRelation(const QString &tableName, const QString &indexColumn,
                                   const QString &displayColumn)

    Constructs a QSqlRelation object, where \a tableName is the SQL
    table name to which a foreign key refers, \a indexColumn is the
    foreign key, and \a displayColumn is the field that should be
    presented to the user.

    \sa tableName(), indexColumn(), displayColumn()
*/

/*!
    \fn QString QSqlRelation::tableName() const

    Returns the name of the table to which a foreign key refers.
*/

/*!
    \fn QString QSqlRelation::indexColumn() const

    Returns the index column from table tableName() to which a
    foreign key refers.
*/

/*!
    \fn QString QSqlRelation::displayColumn() const

    Returns the column from table tableName() that should be
    presented to the user instead of a foreign key.
*/

/*!
    \fn bool QSqlRelation::isValid() const

    Returns \c true if the QSqlRelation object is valid; otherwise
    returns \c false.
*/

class QRelatedTableModel;

struct QRelation
{
    public:
        QRelation(): model(0), m_parent(0), m_dictInitialized(false) {}
        void init(QSqlRelationalTableModel *parent, const QSqlRelation &relation);

        void populateModel();

        bool isDictionaryInitialized();
        void populateDictionary();
        void clearDictionary();

        void clear();
        bool isValid();

        QSqlRelation rel;
        QRelatedTableModel *model;
        QHash<QString, QVariant> dictionary;//maps keys to display values

    private:
        QSqlRelationalTableModel *m_parent;
        bool m_dictInitialized;
};

class QRelatedTableModel : public QSqlTableModel
{
public:
    QRelatedTableModel(QRelation *rel, QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    bool select();
private:
    bool firstSelect;
    QRelation *relation;
};
/*
    A QRelation must be initialized before it is considered valid.
    Note: population of the model and dictionary are kept separate
          from initialization, and are populated on an as needed basis.
*/
void QRelation::init(QSqlRelationalTableModel *parent, const QSqlRelation &relation)
{
    Q_ASSERT(parent != NULL);
    m_parent = parent;
    rel = relation;
}

void QRelation::populateModel()
{
    if (!isValid())
        return;
    Q_ASSERT(m_parent != NULL);

    if (!model) {
        model = new QRelatedTableModel(this, m_parent, m_parent->database());
        model->setTable(rel.tableName());
        model->select();
    }
}

bool QRelation::isDictionaryInitialized()
{
    return m_dictInitialized;
}

void QRelation::populateDictionary()
{
    if (!isValid())
        return;

    if (model ==  NULL)
        populateModel();

    QSqlRecord record;
    QString indexColumn;
    QString displayColumn;
    for (int i=0; i < model->rowCount(); ++i) {
        record = model->record(i);

        indexColumn = rel.indexColumn();
        if (m_parent->database().driver()->isIdentifierEscaped(indexColumn, QSqlDriver::FieldName))
            indexColumn = m_parent->database().driver()->stripDelimiters(indexColumn, QSqlDriver::FieldName);

        displayColumn = rel.displayColumn();
        if (m_parent->database().driver()->isIdentifierEscaped(displayColumn, QSqlDriver::FieldName))
            displayColumn = m_parent->database().driver()->stripDelimiters(displayColumn, QSqlDriver::FieldName);

        dictionary[record.field(indexColumn).value().toString()] =
            record.field(displayColumn).value();
    }
    m_dictInitialized = true;
}

void QRelation::clearDictionary()
{
    dictionary.clear();
    m_dictInitialized = false;
}

void QRelation::clear()
{
    delete model;
    model = 0;
    clearDictionary();
}

bool QRelation::isValid()
{
    return (rel.isValid() && m_parent != NULL);
}



QRelatedTableModel::QRelatedTableModel(QRelation *rel, QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db), firstSelect(true), relation(rel)
{
}

bool QRelatedTableModel::select()
{
    if (firstSelect) {
        firstSelect = false;
        return QSqlTableModel::select();
    }
    relation->clearDictionary();
    bool res = QSqlTableModel::select();
    if (res)
        relation->populateDictionary();
    return res;
}


class QSqlRelationalTableModelPrivate: public QSqlTableModelPrivate
{
    Q_DECLARE_PUBLIC(QSqlRelationalTableModel)
public:
    QSqlRelationalTableModelPrivate()
        : QSqlTableModelPrivate(),
        joinMode( QSqlRelationalTableModel::InnerJoin )
    {}
    QString fullyQualifiedFieldName(const QString &tableName, const QString &fieldName) const;

    int nameToIndex(const QString &name) const;
    mutable QVector<QRelation> relations;
    QSqlRecord baseRec; // the record without relations
    void clearChanges();
    void clearCache();
    void revertCachedRow(int row);

    void translateFieldNames(QSqlRecord &values) const;
    QSqlRelationalTableModel::JoinMode joinMode;
};

void QSqlRelationalTableModelPrivate::clearChanges()
{
    for (int i = 0; i < relations.count(); ++i) {
        QRelation &rel = relations[i];
        rel.clear();
    }
}

void QSqlRelationalTableModelPrivate::revertCachedRow(int row)
{
    QSqlTableModelPrivate::revertCachedRow(row);
}

int QSqlRelationalTableModelPrivate::nameToIndex(const QString &name) const
{
    const QString fieldname = strippedFieldName(name);
    int idx = baseRec.indexOf(fieldname);
    if (idx == -1) {
        // If the name is an alias we can find it here.
        idx = QSqlTableModelPrivate::nameToIndex(name);
    }
    return idx;
}

/*!
    \reimp
*/
void QSqlRelationalTableModelPrivate::clearCache()
{
    for (int i = 0; i < relations.count(); ++i)
        relations[i].clearDictionary();

    QSqlTableModelPrivate::clearCache();
}

/*!
    \class QSqlRelationalTableModel
    \brief The QSqlRelationalTableModel class provides an editable
    data model for a single database table, with foreign key support.

    \ingroup database
    \inmodule QtSql

    QSqlRelationalTableModel acts like QSqlTableModel, but allows
    columns to be set as foreign keys into other database tables.

    \table
    \row \li \inlineimage noforeignkeys.png
         \li \inlineimage foreignkeys.png
    \endtable

    The screenshot on the left shows a plain QSqlTableModel in a
    QTableView. Foreign keys (\c city and \c country) aren't resolved
    to human-readable values. The screenshot on the right shows a
    QSqlRelationalTableModel, with foreign keys resolved into
    human-readable text strings.

    The following code snippet shows how the QSqlRelationalTableModel
    was set up:

    \snippet relationaltablemodel/relationaltablemodel.cpp 0
    \codeline
    \snippet relationaltablemodel/relationaltablemodel.cpp 1
    \snippet relationaltablemodel/relationaltablemodel.cpp 2

    The setRelation() function calls establish a relationship between
    two tables. The first call specifies that column 2 in table \c
    employee is a foreign key that maps with field \c id of table \c
    city, and that the view should present the \c{city}'s \c name
    field to the user. The second call does something similar with
    column 3.

    If you use a read-write QSqlRelationalTableModel, you probably
    want to use QSqlRelationalDelegate on the view. Unlike the default
    delegate, QSqlRelationalDelegate provides a combobox for fields
    that are foreign keys into other tables. To use the class, simply
    call QAbstractItemView::setItemDelegate() on the view with an
    instance of QSqlRelationalDelegate:

    \snippet relationaltablemodel/relationaltablemodel.cpp 4

    The \l{relationaltablemodel} example illustrates how to use
    QSqlRelationalTableModel in conjunction with
    QSqlRelationalDelegate to provide tables with foreign key
    support.

    \image relationaltable.png

    Notes:

    \list
    \li The table must have a primary key declared.
    \li The table's primary key may not contain a relation to
       another table.
    \li If a relational table contains keys that refer to non-existent
       rows in the referenced table, the rows containing the invalid
       keys will not be exposed through the model. The user or the
       database is responsible for keeping referential integrity.
    \li If a relation's display column name is also used as a column
       name in the relational table, or if it is used as display column
       name in more than one relation it will be aliased. The alias is
       the relation's table name, display column name and a unique id
       joined by an underscore (e.g. tablename_columnname_id).
       QSqlRecord::fieldName() will return the aliased column name.
       All occurrences of the duplicate display column name are aliased when
       duplication is detected, but no aliasing is done to the column
       names in the main table. The aliasing doesn't affect
       QSqlRelation, so QSqlRelation::displayColumn() will return the
       original display column name.
    \li The reference table name is aliased. The alias is the word "relTblAl"
       and the relationed column index joined by an underscore
       (e.g. relTblAl_2). The alias can be used to filter the table
       (For example, setFilter("relTblAl_2='Oslo' OR
       relTblAl_3='USA'")).
    \li When using setData() the role should always be Qt::EditRole,
       and when using data() the role should always be Qt::DisplayRole.
    \endlist

    \sa QSqlRelation, QSqlRelationalDelegate,
        {Relational Table Model Example}
*/


/*!
    Creates an empty QSqlRelationalTableModel and sets the parent to \a parent
    and the database connection to \a db. If \a db is not valid, the
    default database connection will be used.
*/
QSqlRelationalTableModel::QSqlRelationalTableModel(QObject *parent, QSqlDatabase db)
    : QSqlTableModel(*new QSqlRelationalTableModelPrivate, parent, db)
{
}

/*!
    Destroys the object and frees any allocated resources.
*/
QSqlRelationalTableModel::~QSqlRelationalTableModel()
{
}

/*!
    \reimp
*/
QVariant QSqlRelationalTableModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QSqlRelationalTableModel);

    if (role == Qt::DisplayRole && index.column() >= 0 && index.column() < d->relations.count() &&
            d->relations.value(index.column()).isValid()) {
        QRelation &relation = d->relations[index.column()];
        if (!relation.isDictionaryInitialized())
            relation.populateDictionary();

        //only perform a dictionary lookup for the display value
        //when the value at index has been changed or added.
        //At an unmodified index, the underlying model will
        //already have the correct display value.
        if (d->strategy != OnFieldChange) {
            const QSqlTableModelPrivate::ModifiedRow row = d->cache.value(index.row());
            if (row.op() != QSqlTableModelPrivate::None && row.rec().isGenerated(index.column())) {
                if (d->strategy == OnManualSubmit || row.op() != QSqlTableModelPrivate::Delete) {
                    QVariant v = row.rec().value(index.column());
                    if (v.isValid())
                        return relation.dictionary[v.toString()];
                }
            }
        }
    }
    return QSqlTableModel::data(index, role);
}

/*!
    Sets the data for the \a role in the item with the specified \a
    index to the \a value given. Depending on the edit strategy, the
    value might be applied to the database at once, or it may be
    cached in the model.

    Returns \c true if the value could be set, or false on error (for
    example, if \a index is out of bounds).

    For relational columns, \a value must be the index, not the
    display value. The index must also exist in the referenced
    table, otherwise the function returns \c false.

    \sa editStrategy(), data(), submit(), revertRow()
*/
bool QSqlRelationalTableModel::setData(const QModelIndex &index, const QVariant &value,
                                       int role)
{
    Q_D(QSqlRelationalTableModel);
    if ( role == Qt::EditRole && index.column() > 0 && index.column() < d->relations.count()
            && d->relations.value(index.column()).isValid()) {
        QRelation &relation = d->relations[index.column()];
        if (!relation.isDictionaryInitialized())
            relation.populateDictionary();
        if (!relation.dictionary.contains(value.toString()))
            return false;
    }
    return QSqlTableModel::setData(index, value, role);
}

/*!
    Lets the specified \a column be a foreign index specified by \a relation.

    Example:

    \snippet relationaltablemodel/relationaltablemodel.cpp 0
    \codeline
    \snippet relationaltablemodel/relationaltablemodel.cpp 1

    The setRelation() call specifies that column 2 in table \c
    employee is a foreign key that maps with field \c id of table \c
    city, and that the view should present the \c{city}'s \c name
    field to the user.

    Note: The table's primary key may not contain a relation to another table.

    \sa relation()
*/
void QSqlRelationalTableModel::setRelation(int column, const QSqlRelation &relation)
{
    Q_D(QSqlRelationalTableModel);
    if (column < 0)
        return;
    if (d->relations.size() <= column)
        d->relations.resize(column + 1);
    d->relations[column].init(this, relation);
}

/*!
    Returns the relation for the column \a column, or an invalid
    relation if no relation is set.

    \sa setRelation(), QSqlRelation::isValid()
*/
QSqlRelation QSqlRelationalTableModel::relation(int column) const
{
    Q_D(const QSqlRelationalTableModel);
    return d->relations.value(column).rel;
}

QString QSqlRelationalTableModelPrivate::fullyQualifiedFieldName(const QString &tableName,
                                                                 const QString &fieldName) const
{
    QString ret;
    ret.reserve(tableName.size() + fieldName.size() + 1);
    ret.append(tableName).append(QLatin1Char('.')).append(fieldName);

    return ret;
}

/*!
    \reimp
*/
QString QSqlRelationalTableModel::selectStatement() const
{
    Q_D(const QSqlRelationalTableModel);

    if (tableName().isEmpty())
        return QString();
    if (d->relations.isEmpty())
        return QSqlTableModel::selectStatement();

    // Count how many times each field name occurs in the record
    QHash<QString, int> fieldNames;
    QStringList fieldList;
    for (int i = 0; i < d->baseRec.count(); ++i) {
        QSqlRelation relation = d->relations.value(i).rel;
        QString name;
        if (relation.isValid()) {
            // Count the display column name, not the original foreign key
            name = relation.displayColumn();
            if (d->db.driver()->isIdentifierEscaped(name, QSqlDriver::FieldName))
                name = d->db.driver()->stripDelimiters(name, QSqlDriver::FieldName);

            const QSqlRecord rec = database().record(relation.tableName());
            for (int i = 0; i < rec.count(); ++i) {
                if (name.compare(rec.fieldName(i), Qt::CaseInsensitive) == 0) {
                    name = rec.fieldName(i);
                    break;
                }
            }
        }
        else {
            name = d->baseRec.fieldName(i);
        }
        fieldNames[name] = fieldNames.value(name, 0) + 1;
        fieldList.append(name);
    }

    QString fList;
    QString conditions;
    QString from = Sql::from(tableName());
    for (int i = 0; i < d->baseRec.count(); ++i) {
        QSqlRelation relation = d->relations.value(i).rel;
        const QString tableField = d->fullyQualifiedFieldName(tableName(), d->db.driver()->escapeIdentifier(d->baseRec.fieldName(i), QSqlDriver::FieldName));
        if (relation.isValid()) {
            const QString relTableAlias = Sql::relTablePrefix(i);
            QString displayTableField = d->fullyQualifiedFieldName(relTableAlias, relation.displayColumn());

            // Duplicate field names must be aliased
            if (fieldNames.value(fieldList[i]) > 1) {
                QString relTableName = relation.tableName().section(QChar::fromLatin1('.'), -1, -1);
                if (d->db.driver()->isIdentifierEscaped(relTableName, QSqlDriver::TableName))
                    relTableName = d->db.driver()->stripDelimiters(relTableName, QSqlDriver::TableName);
                QString displayColumn = relation.displayColumn();
                if (d->db.driver()->isIdentifierEscaped(displayColumn, QSqlDriver::FieldName))
                    displayColumn = d->db.driver()->stripDelimiters(displayColumn, QSqlDriver::FieldName);
                const QString alias = QString::fromLatin1("%1_%2_%3").arg(relTableName).arg(displayColumn).arg(fieldNames.value(fieldList[i]));
                displayTableField = Sql::as(displayTableField, alias);
                --fieldNames[fieldList[i]];
            }

            fList = Sql::comma(fList, displayTableField);

            // Join related table
            const QString tblexpr = Sql::concat(relation.tableName(), relTableAlias);
            const QString relTableField = d->fullyQualifiedFieldName(relTableAlias, relation.indexColumn());
            const QString cond = Sql::eq(tableField, relTableField);
            if (d->joinMode == QSqlRelationalTableModel::InnerJoin) {
                // FIXME: InnerJoin code is known to be broken.
                // Use LeftJoin mode if you want correct behavior.
                from = Sql::comma(from, tblexpr);
                conditions = Sql::et(conditions, cond);
            } else {
                from = Sql::concat(from, Sql::leftJoin(tblexpr));
                from = Sql::concat(from, Sql::on(cond));
            }
        } else {
            fList = Sql::comma(fList, tableField);
        }
    }

    if (fList.isEmpty())
        return QString();

    const QString stmt = Sql::concat(Sql::select(fList), from);
    const QString where = Sql::where(Sql::et(Sql::paren(conditions), Sql::paren(filter())));
    return Sql::concat(Sql::concat(stmt, where), orderByClause());
}

/*!
    Returns a QSqlTableModel object for accessing the table for which
    \a column is a foreign key, or 0 if there is no relation for the
    given \a column.

    The returned object is owned by the QSqlRelationalTableModel.

    \sa setRelation(), relation()
*/
QSqlTableModel *QSqlRelationalTableModel::relationModel(int column) const
{
    Q_D(const QSqlRelationalTableModel);
    if ( column < 0 || column >= d->relations.count())
        return 0;

    QRelation &relation = const_cast<QSqlRelationalTableModelPrivate *>(d)->relations[column];
    if (!relation.isValid())
        return 0;

    if (!relation.model)
        relation.populateModel();
    return relation.model;
}

/*!
    \reimp
*/
void QSqlRelationalTableModel::revertRow(int row)
{
    QSqlTableModel::revertRow(row);
}

/*!
    \reimp
*/
void QSqlRelationalTableModel::clear()
{
    Q_D(QSqlRelationalTableModel);
    d->clearChanges();
    d->relations.clear();
    QSqlTableModel::clear();
}


/*! \enum QSqlRelationalTableModel::JoinMode

    \value InnerJoin - Inner join mode, return rows when there is at least one match in both tables.
    \value LeftJoin - Left join mode, returns all rows from the left table (table_name1), even if there are no matches in the right table (table_name2).

    \sa QSqlRelationalTableModel::setJoinMode()
    \since 4.8
*/

/*!
    Sets the SQL \a joinMode to show or hide rows with NULL foreign keys.
    In InnerJoin mode (the default) these rows will not be shown: use the
    LeftJoin mode if you want to show them.

    \sa QSqlRelationalTableModel::JoinMode
    \since 4.8
*/
void QSqlRelationalTableModel::setJoinMode( QSqlRelationalTableModel::JoinMode joinMode )
{
    Q_D(QSqlRelationalTableModel);
    d->joinMode = joinMode;
}
/*!
    \reimp
*/
bool QSqlRelationalTableModel::select()
{
    return QSqlTableModel::select();
}

/*!
    \reimp
*/
void QSqlRelationalTableModel::setTable(const QString &table)
{
    Q_D(QSqlRelationalTableModel);

    // memorize the table before applying the relations
    d->baseRec = d->db.record(table);

    QSqlTableModel::setTable(table);
}

/*! \internal
 */
void QSqlRelationalTableModelPrivate::translateFieldNames(QSqlRecord &values) const
{
    for (int i = 0; i < values.count(); ++i) {
        if (relations.value(i).isValid()) {
            QVariant v = values.value(i);
            bool gen = values.isGenerated(i);
            values.replace(i, baseRec.field(i));
            values.setValue(i, v);
            values.setGenerated(i, gen);
        }
    }
}

/*!
    \reimp
*/
bool QSqlRelationalTableModel::updateRowInTable(int row, const QSqlRecord &values)
{
    Q_D(QSqlRelationalTableModel);

    QSqlRecord rec = values;
    d->translateFieldNames(rec);

    return QSqlTableModel::updateRowInTable(row, rec);
}

/*!
    \reimp
*/
bool QSqlRelationalTableModel::insertRowIntoTable(const QSqlRecord &values)
{
    Q_D(QSqlRelationalTableModel);

    QSqlRecord rec = values;
    d->translateFieldNames(rec);

    return QSqlTableModel::insertRowIntoTable(rec);
}

/*!
    \reimp
*/
QString QSqlRelationalTableModel::orderByClause() const
{
    Q_D(const QSqlRelationalTableModel);

    const QSqlRelation rel = d->relations.value(d->sortColumn).rel;
    if (!rel.isValid())
        return QSqlTableModel::orderByClause();

    QString f = d->fullyQualifiedFieldName(Sql::relTablePrefix(d->sortColumn), rel.displayColumn());
    f = d->sortOrder == Qt::AscendingOrder ? Sql::asc(f) : Sql::desc(f);
    return Sql::orderBy(f);
}

/*!
    \reimp
*/
bool QSqlRelationalTableModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QSqlRelationalTableModel);

    if (parent.isValid() || column < 0 || column + count > d->rec.count())
        return false;

    for (int i = 0; i < count; ++i) {
        d->baseRec.remove(column);
        if (d->relations.count() > column)
            d->relations.remove(column);
    }
    return QSqlTableModel::removeColumns(column, count, parent);
}

QT_END_NAMESPACE
