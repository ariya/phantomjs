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

#ifndef QSQL_DB2_H
#define QSQL_DB2_H

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

#include <QtCore/qglobal.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_DB2
#else
#define Q_EXPORT_SQLDRIVER_DB2 Q_SQL_EXPORT
#endif

#include <QtSql/qsqlresult.h>
#include <QtSql/qsqldriver.h>

QT_BEGIN_NAMESPACE

class QDB2Driver;
class QDB2DriverPrivate;
class QDB2ResultPrivate;
class QSqlRecord;

class QDB2Result : public QSqlResult
{
public:
    QDB2Result(const QDB2Driver* dr, const QDB2DriverPrivate* dp);
    ~QDB2Result();
    bool prepare(const QString& query);
    bool exec();
    QVariant handle() const;

protected:
    QVariant data(int field);
    bool reset (const QString& query);
    bool fetch(int i);
    bool fetchNext();
    bool fetchFirst();
    bool fetchLast();
    bool isNull(int i);
    int size();
    int numRowsAffected();
    QSqlRecord record() const;
    void virtual_hook(int id, void *data);
    void detachFromResultSet();
    bool nextResult();

private:
    QDB2ResultPrivate* d;
};

class Q_EXPORT_SQLDRIVER_DB2 QDB2Driver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QDB2Driver)
    Q_OBJECT
public:
    explicit QDB2Driver(QObject* parent = 0);
    QDB2Driver(Qt::HANDLE env, Qt::HANDLE con, QObject* parent = 0);
    ~QDB2Driver();
    bool hasFeature(DriverFeature) const;
    void close();
    QSqlRecord record(const QString& tableName) const;
    QStringList tables(QSql::TableType type) const;
    QSqlResult *createResult() const;
    QSqlIndex primaryIndex(const QString& tablename) const;
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    QString formatValue(const QSqlField &field, bool trimStrings) const;
    QVariant handle() const;
    bool open(const QString& db,
               const QString& user,
               const QString& password,
               const QString& host,
               int port,
               const QString& connOpts);
    QString escapeIdentifier(const QString &identifier, IdentifierType type) const;

private:
    bool setAutoCommit(bool autoCommit);
};

QT_END_NAMESPACE

#endif // QSQL_DB2_H
