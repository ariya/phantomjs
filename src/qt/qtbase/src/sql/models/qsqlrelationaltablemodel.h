/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSQLRELATIONALTABLEMODEL_H
#define QSQLRELATIONALTABLEMODEL_H

#include <QtSql/qsqltablemodel.h>

QT_BEGIN_NAMESPACE


class Q_SQL_EXPORT QSqlRelation
{
public:
    QSqlRelation() {}
    QSqlRelation(const QString &aTableName, const QString &indexCol,
               const QString &displayCol)
        : tName(aTableName), iColumn(indexCol), dColumn(displayCol) {}
    inline QString tableName() const
    { return tName; }
    inline QString indexColumn() const
    { return iColumn; }
    inline QString displayColumn() const
    { return dColumn; }
    inline bool isValid() const
    { return !(tName.isEmpty() || iColumn.isEmpty() || dColumn.isEmpty()); }
private:
    QString tName, iColumn, dColumn;
};

class QSqlRelationalTableModelPrivate;

class Q_SQL_EXPORT QSqlRelationalTableModel: public QSqlTableModel
{
    Q_OBJECT

public:
    enum JoinMode {
        InnerJoin,
        LeftJoin
    };

    explicit QSqlRelationalTableModel(QObject *parent = 0,
                                      QSqlDatabase db = QSqlDatabase());
    virtual ~QSqlRelationalTableModel();

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &item, const QVariant &value, int role = Qt::EditRole);
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex());

    void clear();
    bool select();

    void setTable(const QString &tableName);
    virtual void setRelation(int column, const QSqlRelation &relation);
    QSqlRelation relation(int column) const;
    virtual QSqlTableModel *relationModel(int column) const;
    void setJoinMode( QSqlRelationalTableModel::JoinMode joinMode );

public Q_SLOTS:
    void revertRow(int row);

protected:
    QString selectStatement() const;
    bool updateRowInTable(int row, const QSqlRecord &values);
    bool insertRowIntoTable(const QSqlRecord &values);
    QString orderByClause() const;

private:
    Q_DECLARE_PRIVATE(QSqlRelationalTableModel)
};

QT_END_NAMESPACE

#endif // QSQLRELATIONALTABLEMODEL_H
