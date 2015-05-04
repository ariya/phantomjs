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

#ifndef QSQLQUERYMODEL_P_H
#define QSQLQUERYMODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qsql*model.h .  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "private/qabstractitemmodel_p.h"
#include "QtSql/qsqlerror.h"
#include "QtSql/qsqlquery.h"
#include "QtSql/qsqlrecord.h"
#include "QtCore/qhash.h"
#include "QtCore/qvarlengtharray.h"
#include "QtCore/qvector.h"

QT_BEGIN_NAMESPACE

class QSqlQueryModelPrivate: public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QSqlQueryModel)
public:
    QSqlQueryModelPrivate() : atEnd(false), nestedResetLevel(0) {}
    ~QSqlQueryModelPrivate();

    void prefetch(int);
    void initColOffsets(int size);
    int columnInQuery(int modelColumn) const;

    mutable QSqlQuery query;
    mutable QSqlError error;
    QModelIndex bottom;
    QSqlRecord rec;
    uint atEnd : 1;
    QVector<QHash<int, QVariant> > headers;
    QVarLengthArray<int, 56> colOffsets; // used to calculate indexInQuery of columns
    int nestedResetLevel;
};

// helpers for building SQL expressions
class QSqlQueryModelSql
{
public:
    // SQL keywords
    inline const static QLatin1String as() { return QLatin1String("AS"); }
    inline const static QLatin1String asc() { return QLatin1String("ASC"); }
    inline const static QLatin1String comma() { return QLatin1String(","); }
    inline const static QLatin1String desc() { return QLatin1String("DESC"); }
    inline const static QLatin1String eq() { return QLatin1String("="); }
    // "and" is a C++ keyword
    inline const static QLatin1String et() { return QLatin1String("AND"); }
    inline const static QLatin1String from() { return QLatin1String("FROM"); }
    inline const static QLatin1String leftJoin() { return QLatin1String("LEFT JOIN"); }
    inline const static QLatin1String on() { return QLatin1String("ON"); }
    inline const static QLatin1String orderBy() { return QLatin1String("ORDER BY"); }
    inline const static QLatin1String parenClose() { return QLatin1String(")"); }
    inline const static QLatin1String parenOpen() { return QLatin1String("("); }
    inline const static QLatin1String select() { return QLatin1String("SELECT"); }
    inline const static QLatin1String sp() { return QLatin1String(" "); }
    inline const static QLatin1String where() { return QLatin1String("WHERE"); }

    // Build expressions based on key words
    inline const static QString as(const QString &a, const QString &b) { return b.isEmpty() ? a : concat(concat(a, as()), b); }
    inline const static QString asc(const QString &s) { return concat(s, asc()); }
    inline const static QString comma(const QString &a, const QString &b) { return a.isEmpty() ? b : b.isEmpty() ? a : QString(a).append(comma()).append(b); }
    inline const static QString concat(const QString &a, const QString &b) { return a.isEmpty() ? b : b.isEmpty() ? a : QString(a).append(sp()).append(b); }
    inline const static QString desc(const QString &s) { return concat(s, desc()); }
    inline const static QString eq(const QString &a, const QString &b) { return QString(a).append(eq()).append(b); }
    inline const static QString et(const QString &a, const QString &b) { return a.isEmpty() ? b : b.isEmpty() ? a : concat(concat(a, et()), b); }
    inline const static QString from(const QString &s) { return concat(from(), s); }
    inline const static QString leftJoin(const QString &s) { return concat(leftJoin(), s); }
    inline const static QString on(const QString &s) { return concat(on(), s); }
    inline const static QString orderBy(const QString &s) { return s.isEmpty() ? s : concat(orderBy(), s); }
    inline const static QString paren(const QString &s) { return s.isEmpty() ? s : parenOpen() + s + parenClose(); }
    inline const static QString select(const QString &s) { return concat(select(), s); }
    inline const static QString where(const QString &s) { return s.isEmpty() ? s : concat(where(), s); }
};

QT_END_NAMESPACE

#endif // QSQLQUERYMODEL_P_H
