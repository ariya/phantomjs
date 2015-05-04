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

#ifndef QSQL_OCI_H
#define QSQL_OCI_H

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

#include <QtSql/qsqlresult.h>
#include <QtSql/qsqldriver.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_OCI
#else
#define Q_EXPORT_SQLDRIVER_OCI Q_SQL_EXPORT
#endif

typedef struct OCIEnv OCIEnv;
typedef struct OCISvcCtx OCISvcCtx;

QT_BEGIN_NAMESPACE

class QOCIDriver;
class QOCICols;
class QOCIDriverPrivate;

class Q_EXPORT_SQLDRIVER_OCI QOCIDriver : public QSqlDriver
{
    Q_DECLARE_PRIVATE(QOCIDriver)
    Q_OBJECT
    friend struct QOCIResultPrivate;
    friend class QOCIPrivate;
public:
    explicit QOCIDriver(QObject* parent = 0);
    QOCIDriver(OCIEnv* env, OCISvcCtx* ctx, QObject* parent = 0);
    ~QOCIDriver();
    bool hasFeature(DriverFeature f) const;
    bool open(const QString & db,
              const QString & user,
              const QString & password,
              const QString & host,
              int port,
              const QString& connOpts);
    void close();
    QSqlResult *createResult() const;
    QStringList tables(QSql::TableType) const;
    QSqlRecord record(const QString& tablename) const;
    QSqlIndex primaryIndex(const QString& tablename) const;
    QString formatValue(const QSqlField &field,
                        bool trimStrings) const;
    QVariant handle() const;
    QString escapeIdentifier(const QString &identifier, IdentifierType) const;

protected:
    bool                beginTransaction();
    bool                commitTransaction();
    bool                rollbackTransaction();
};

QT_END_NAMESPACE

#endif // QSQL_OCI_H
