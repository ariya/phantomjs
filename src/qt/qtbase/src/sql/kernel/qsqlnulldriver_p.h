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

#ifndef QSQLNULLDRIVER_P_H
#define QSQLNULLDRIVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include "QtCore/qvariant.h"
#include "QtSql/qsqldriver.h"
#include "QtSql/qsqlerror.h"
#include "QtSql/qsqlresult.h"

QT_BEGIN_NAMESPACE

class QSqlNullResult : public QSqlResult
{
public:
    inline explicit QSqlNullResult(const QSqlDriver* d): QSqlResult(d)
    { QSqlResult::setLastError(
            QSqlError(QLatin1String("Driver not loaded"), QLatin1String("Driver not loaded"), QSqlError::ConnectionError)); }
protected:
    inline QVariant data(int) { return QVariant(); }
    inline bool reset (const QString&) { return false; }
    inline bool fetch(int) { return false; }
    inline bool fetchFirst() { return false; }
    inline bool fetchLast() { return false; }
    inline bool isNull(int) { return false; }
    inline int size()  { return -1; }
    inline int numRowsAffected() { return 0; }

    inline void setAt(int) {}
    inline void setActive(bool) {}
    inline void setLastError(const QSqlError&) {}
    inline void setQuery(const QString&) {}
    inline void setSelect(bool) {}
    inline void setForwardOnly(bool) {}

    inline bool exec() { return false; }
    inline bool prepare(const QString&) { return false; }
    inline bool savePrepare(const QString&) { return false; }
    inline void bindValue(int, const QVariant&, QSql::ParamType) {}
    inline void bindValue(const QString&, const QVariant&, QSql::ParamType) {}
};

class QSqlNullDriver : public QSqlDriver
{
public:
    inline QSqlNullDriver(): QSqlDriver()
    { QSqlDriver::setLastError(
            QSqlError(QLatin1String("Driver not loaded"), QLatin1String("Driver not loaded"), QSqlError::ConnectionError)); }
    inline bool hasFeature(DriverFeature) const { return false; }
    inline bool open(const QString &, const QString & , const QString & ,
              const QString &, int, const QString&)
    { return false; }
    inline void close() {}
    inline QSqlResult *createResult() const { return new QSqlNullResult(this); }

protected:
    inline void setOpen(bool) {}
    inline void setOpenError(bool) {}
    inline void setLastError(const QSqlError&) {}
};

QT_END_NAMESPACE

#endif // QSQLNULLDRIVER_P_H
