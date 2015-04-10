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

#include "qsql_db2_p.h"
#include <QtSql/private/qsqldriver_p.h>
#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qsqlfield.h>
#include <qsqlerror.h>
#include <qsqlindex.h>
#include <qsqlrecord.h>
#include <qstringlist.h>
#include <qvarlengtharray.h>
#include <qvector.h>
#include <QDebug>

#if defined(Q_CC_BOR)
// DB2's sqlsystm.h (included through sqlcli1.h) defines the SQL_BIGINT_TYPE
// and SQL_BIGUINT_TYPE to wrong the types for Borland; so do the defines to
// the right type before including the header
#define SQL_BIGINT_TYPE qint64
#define SQL_BIGUINT_TYPE quint64
#endif

#define UNICODE

#include <sqlcli1.h>

#include <string.h>

QT_BEGIN_NAMESPACE

static const int COLNAMESIZE = 255;
static const SQLSMALLINT qParamType[4] = { SQL_PARAM_INPUT, SQL_PARAM_INPUT, SQL_PARAM_OUTPUT, SQL_PARAM_INPUT_OUTPUT };

class QDB2DriverPrivate : public QSqlDriverPrivate
{
public:
    QDB2DriverPrivate() : QSqlDriverPrivate(), hEnv(0), hDbc(0) { dbmsType = DB2; }
    SQLHANDLE hEnv;
    SQLHANDLE hDbc;
    QString user;
};

class QDB2ResultPrivate
{
public:
    QDB2ResultPrivate(const QDB2DriverPrivate* d): dp(d), hStmt(0)
    {}
    ~QDB2ResultPrivate()
    {
        emptyValueCache();
    }
    void clearValueCache()
    {
        for (int i = 0; i < valueCache.count(); ++i) {
            delete valueCache[i];
            valueCache[i] = NULL;
        }
    }
    void emptyValueCache()
    {
        clearValueCache();
        valueCache.clear();
    }

    const QDB2DriverPrivate* dp;
    SQLHANDLE hStmt;
    QSqlRecord recInf;
    QVector<QVariant*> valueCache;
};

static QString qFromTChar(SQLTCHAR* str)
{
    return QString((const QChar *)str);
}

// dangerous!! (but fast). Don't use in functions that
// require out parameters!
static SQLTCHAR* qToTChar(const QString& str)
{
    return (SQLTCHAR*)str.utf16();
}

static QString qWarnDB2Handle(int handleType, SQLHANDLE handle)
{
    SQLINTEGER nativeCode;
    SQLSMALLINT msgLen;
    SQLRETURN r = SQL_ERROR;
    SQLTCHAR state[SQL_SQLSTATE_SIZE + 1];
    SQLTCHAR description[SQL_MAX_MESSAGE_LENGTH];
    r = SQLGetDiagRec(handleType,
                       handle,
                       1,
                       (SQLTCHAR*) state,
                       &nativeCode,
                       (SQLTCHAR*) description,
                       SQL_MAX_MESSAGE_LENGTH - 1, /* in bytes, not in characters */
                       &msgLen);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        return QString(qFromTChar(description));
    return QString();
}

static QString qDB2Warn(const QDB2DriverPrivate* d)
{
    return (qWarnDB2Handle(SQL_HANDLE_ENV, d->hEnv) + QLatin1Char(' ')
             + qWarnDB2Handle(SQL_HANDLE_DBC, d->hDbc));
}

static QString qDB2Warn(const QDB2ResultPrivate* d)
{
    return (qWarnDB2Handle(SQL_HANDLE_ENV, d->dp->hEnv) + QLatin1Char(' ')
             + qWarnDB2Handle(SQL_HANDLE_DBC, d->dp->hDbc)
             + qWarnDB2Handle(SQL_HANDLE_STMT, d->hStmt));
}

static void qSqlWarning(const QString& message, const QDB2DriverPrivate* d)
{
    qWarning("%s\tError: %s", message.toLocal8Bit().constData(),
                              qDB2Warn(d).toLocal8Bit().constData());
}

static void qSqlWarning(const QString& message, const QDB2ResultPrivate* d)
{
    qWarning("%s\tError: %s", message.toLocal8Bit().constData(),
                              qDB2Warn(d).toLocal8Bit().constData());
}

static QSqlError qMakeError(const QString& err, QSqlError::ErrorType type,
                            const QDB2DriverPrivate* p)
{
    return QSqlError(QLatin1String("QDB2: ") + err, qDB2Warn(p), type);
}

static QSqlError qMakeError(const QString& err, QSqlError::ErrorType type,
                            const QDB2ResultPrivate* p)
{
    return QSqlError(QLatin1String("QDB2: ") + err, qDB2Warn(p), type);
}

static QVariant::Type qDecodeDB2Type(SQLSMALLINT sqltype)
{
    QVariant::Type type = QVariant::Invalid;
    switch (sqltype) {
    case SQL_REAL:
    case SQL_FLOAT:
    case SQL_DOUBLE:
    case SQL_DECIMAL:
    case SQL_NUMERIC:
        type = QVariant::Double;
        break;
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIT:
    case SQL_TINYINT:
        type = QVariant::Int;
        break;
    case SQL_BIGINT:
        type = QVariant::LongLong;
        break;
    case SQL_BLOB:
    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
    case SQL_CLOB:
    case SQL_DBCLOB:
        type = QVariant::ByteArray;
        break;
    case SQL_DATE:
    case SQL_TYPE_DATE:
        type = QVariant::Date;
        break;
    case SQL_TIME:
    case SQL_TYPE_TIME:
        type = QVariant::Time;
        break;
    case SQL_TIMESTAMP:
    case SQL_TYPE_TIMESTAMP:
        type = QVariant::DateTime;
        break;
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
        type = QVariant::String;
        break;
    default:
        type = QVariant::ByteArray;
        break;
    }
    return type;
}

static QSqlField qMakeFieldInfo(const QDB2ResultPrivate* d, int i)
{
    SQLSMALLINT colNameLen;
    SQLSMALLINT colType;
    SQLUINTEGER colSize;
    SQLSMALLINT colScale;
    SQLSMALLINT nullable;
    SQLRETURN r = SQL_ERROR;
    SQLTCHAR colName[COLNAMESIZE];
    r = SQLDescribeCol(d->hStmt,
                        i+1,
                        colName,
                        (SQLSMALLINT) COLNAMESIZE,
                        &colNameLen,
                        &colType,
                        &colSize,
                        &colScale,
                        &nullable);

    if (r != SQL_SUCCESS) {
        qSqlWarning(QString::fromLatin1("qMakeFieldInfo: Unable to describe column %1").arg(i), d);
        return QSqlField();
    }
    QSqlField f(qFromTChar(colName), qDecodeDB2Type(colType));
    // nullable can be SQL_NO_NULLS, SQL_NULLABLE or SQL_NULLABLE_UNKNOWN
    if (nullable == SQL_NO_NULLS)
        f.setRequired(true);
    else if (nullable == SQL_NULLABLE)
        f.setRequired(false);
    // else required is unknown
    f.setLength(colSize == 0 ? -1 : int(colSize));
    f.setPrecision(colScale == 0 ? -1 : int(colScale));
    f.setSqlType(int(colType));
    return f;
}

static int qGetIntData(SQLHANDLE hStmt, int column, bool& isNull)
{
    SQLINTEGER intbuf;
    isNull = false;
    SQLINTEGER lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column + 1,
                              SQL_C_SLONG,
                              (SQLPOINTER) &intbuf,
                              0,
                              &lengthIndicator);
    if ((r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) || lengthIndicator == SQL_NULL_DATA) {
        isNull = true;
        return 0;
    }
    return int(intbuf);
}

static double qGetDoubleData(SQLHANDLE hStmt, int column, bool& isNull)
{
    SQLDOUBLE dblbuf;
    isNull = false;
    SQLINTEGER lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column+1,
                              SQL_C_DOUBLE,
                              (SQLPOINTER) &dblbuf,
                              0,
                              &lengthIndicator);
    if ((r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) || lengthIndicator == SQL_NULL_DATA) {
        isNull = true;
        return 0.0;
    }

    return (double) dblbuf;
}

static SQLBIGINT qGetBigIntData(SQLHANDLE hStmt, int column, bool& isNull)
{
    SQLBIGINT lngbuf = Q_INT64_C(0);
    isNull = false;
    SQLINTEGER lengthIndicator = 0;
    SQLRETURN r = SQLGetData(hStmt,
                              column+1,
                              SQL_C_SBIGINT,
                              (SQLPOINTER) &lngbuf,
                              0,
                              &lengthIndicator);
    if ((r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) || lengthIndicator == SQL_NULL_DATA)
        isNull = true;

    return lngbuf;
}

static QString qGetStringData(SQLHANDLE hStmt, int column, int colSize, bool& isNull)
{
    QString     fieldVal;
    SQLRETURN   r = SQL_ERROR;
    SQLINTEGER  lengthIndicator = 0;

    if (colSize <= 0)
        colSize = 255;
    else if (colSize > 65536) // limit buffer size to 64 KB
        colSize = 65536;
    else
        colSize++; // make sure there is room for more than the 0 termination
    SQLTCHAR* buf = new SQLTCHAR[colSize];

    while (true) {
        r = SQLGetData(hStmt,
                        column + 1,
                        SQL_C_WCHAR,
                        (SQLPOINTER)buf,
                        colSize * sizeof(SQLTCHAR),
                        &lengthIndicator);
        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
            if (lengthIndicator == SQL_NULL_DATA || lengthIndicator == SQL_NO_TOTAL) {
                fieldVal.clear();
                isNull = true;
                break;
            }
            fieldVal += qFromTChar(buf);
        } else if (r == SQL_NO_DATA) {
            break;
        } else {
            qWarning("qGetStringData: Error while fetching data (%d)", r);
            fieldVal.clear();
            break;
        }
    }
    delete[] buf;
    return fieldVal;
}

static QByteArray qGetBinaryData(SQLHANDLE hStmt, int column, SQLINTEGER& lengthIndicator, bool& isNull)
{
    QByteArray fieldVal;
    SQLSMALLINT colNameLen;
    SQLSMALLINT colType;
    SQLUINTEGER colSize;
    SQLSMALLINT colScale;
    SQLSMALLINT nullable;
    SQLRETURN r = SQL_ERROR;

    SQLTCHAR colName[COLNAMESIZE];
    r = SQLDescribeCol(hStmt,
                        column+1,
                        colName,
                        COLNAMESIZE,
                        &colNameLen,
                        &colType,
                        &colSize,
                        &colScale,
                        &nullable);
    if (r != SQL_SUCCESS)
        qWarning("qGetBinaryData: Unable to describe column %d", column);
    // SQLDescribeCol may return 0 if size cannot be determined
    if (!colSize)
        colSize = 255;
    else if (colSize > 65536) // read the field in 64 KB chunks
        colSize = 65536;
    char * buf = new char[colSize];
    while (true) {
        r = SQLGetData(hStmt,
                        column+1,
                        colType == SQL_DBCLOB ? SQL_C_CHAR : SQL_C_BINARY,
                        (SQLPOINTER) buf,
                        colSize,
                        &lengthIndicator);
        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
            if (lengthIndicator == SQL_NULL_DATA) {
                isNull = true;
                break;
            } else {
                int rSize;
                r == SQL_SUCCESS ? rSize = lengthIndicator : rSize = colSize;
                if (lengthIndicator == SQL_NO_TOTAL) // size cannot be determined
                    rSize = colSize;
                fieldVal.append(QByteArray(buf, rSize));
                if (r == SQL_SUCCESS) // the whole field was read in one chunk
                    break;
            }
        } else {
            break;
        }
    }
    delete [] buf;
    return fieldVal;
}

static void qSplitTableQualifier(const QString & qualifier, QString * catalog,
                                  QString * schema, QString * table)
{
    if (!catalog || !schema || !table)
        return;
    QStringList l = qualifier.split(QLatin1Char('.'));
    if (l.count() > 3)
        return; // can't possibly be a valid table qualifier
    int i = 0, n = l.count();
    if (n == 1) {
        *table = qualifier;
    } else {
        for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
            if (n == 3) {
                if (i == 0)
                    *catalog = *it;
                else if (i == 1)
                    *schema = *it;
                else if (i == 2)
                    *table = *it;
            } else if (n == 2) {
                if (i == 0)
                    *schema = *it;
                else if (i == 1)
                    *table = *it;
            }
            i++;
        }
    }
}

// creates a QSqlField from a valid hStmt generated
// by SQLColumns. The hStmt has to point to a valid position.
static QSqlField qMakeFieldInfo(const SQLHANDLE hStmt)
{
    bool isNull;
    int type = qGetIntData(hStmt, 4, isNull);
    QSqlField f(qGetStringData(hStmt, 3, -1, isNull), qDecodeDB2Type(type));
    int required = qGetIntData(hStmt, 10, isNull); // nullable-flag
    // required can be SQL_NO_NULLS, SQL_NULLABLE or SQL_NULLABLE_UNKNOWN
    if (required == SQL_NO_NULLS)
        f.setRequired(true);
    else if (required == SQL_NULLABLE)
        f.setRequired(false);
    // else we don't know.
    f.setLength(qGetIntData(hStmt, 6, isNull)); // column size
    f.setPrecision(qGetIntData(hStmt, 8, isNull)); // precision
    f.setSqlType(type);
    return f;
}

static bool qMakeStatement(QDB2ResultPrivate* d, bool forwardOnly, bool setForwardOnly = true)
{
    SQLRETURN r;
    if (!d->hStmt) {
        r = SQLAllocHandle(SQL_HANDLE_STMT,
                            d->dp->hDbc,
                            &d->hStmt);
        if (r != SQL_SUCCESS) {
            qSqlWarning(QLatin1String("QDB2Result::reset: Unable to allocate statement handle"), d);
            return false;
        }
    } else {
        r = SQLFreeStmt(d->hStmt, SQL_CLOSE);
        if (r != SQL_SUCCESS) {
            qSqlWarning(QLatin1String("QDB2Result::reset: Unable to close statement handle"), d);
            return false;
        }
    }

    if (!setForwardOnly)
        return true;

    if (forwardOnly) {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER) SQL_CURSOR_FORWARD_ONLY,
                            SQL_IS_UINTEGER);
    } else {
        r = SQLSetStmtAttr(d->hStmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER) SQL_CURSOR_STATIC,
                            SQL_IS_UINTEGER);
    }
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning(QString::fromLatin1("QDB2Result::reset: Unable to set %1 attribute.").arg(
                     forwardOnly ? QLatin1String("SQL_CURSOR_FORWARD_ONLY")
                                 : QLatin1String("SQL_CURSOR_STATIC")), d);
        return false;
    }
    return true;
}

QVariant QDB2Result::handle() const
{
    return QVariant(qRegisterMetaType<SQLHANDLE>("SQLHANDLE"), &d->hStmt);
}

/************************************/

QDB2Result::QDB2Result(const QDB2Driver* dr, const QDB2DriverPrivate* dp)
    : QSqlResult(dr)
{
    d = new QDB2ResultPrivate(dp);
}

QDB2Result::~QDB2Result()
{
    if (d->hStmt) {
        SQLRETURN r = SQLFreeHandle(SQL_HANDLE_STMT, d->hStmt);
        if (r != SQL_SUCCESS)
            qSqlWarning(QLatin1String("QDB2Driver: Unable to free statement handle ")
                        + QString::number(r), d);
    }
    delete d;
}

bool QDB2Result::reset (const QString& query)
{
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    SQLRETURN r;

    d->recInf.clear();
    d->emptyValueCache();

    if (!qMakeStatement(d, isForwardOnly()))
        return false;

    r = SQLExecDirect(d->hStmt,
                       qToTChar(query),
                       (SQLINTEGER) query.length());
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                "Unable to execute statement"), QSqlError::StatementError, d));
        return false;
    }
    SQLSMALLINT count;
    r = SQLNumResultCols(d->hStmt, &count);
    if (count) {
        setSelect(true);
        for (int i = 0; i < count; ++i) {
            d->recInf.append(qMakeFieldInfo(d, i));
        }
    } else {
        setSelect(false);
    }
    d->valueCache.resize(count);
    d->valueCache.fill(NULL);
    setActive(true);
    return true;
}

bool QDB2Result::prepare(const QString& query)
{
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    SQLRETURN r;

    d->recInf.clear();
    d->emptyValueCache();

    if (!qMakeStatement(d, isForwardOnly()))
        return false;

    r = SQLPrepare(d->hStmt,
                    qToTChar(query),
                    (SQLINTEGER) query.length());

    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                     "Unable to prepare statement"), QSqlError::StatementError, d));
        return false;
    }
    return true;
}

bool QDB2Result::exec()
{
    QList<QByteArray> tmpStorage; // holds temporary ptrs
    QVarLengthArray<SQLINTEGER, 32> indicators(boundValues().count());

    memset(indicators.data(), 0, indicators.size() * sizeof(SQLINTEGER));
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    SQLRETURN r;

    d->recInf.clear();
    d->emptyValueCache();

    if (!qMakeStatement(d, isForwardOnly(), false))
        return false;


    QVector<QVariant> &values = boundValues();
    int i;
    for (i = 0; i < values.count(); ++i) {
        // bind parameters - only positional binding allowed
        SQLINTEGER *ind = &indicators[i];
        if (values.at(i).isNull())
            *ind = SQL_NULL_DATA;
        if (bindValueType(i) & QSql::Out)
            values[i].detach();

        switch (values.at(i).type()) {
            case QVariant::Date: {
                QByteArray ba;
                ba.resize(sizeof(DATE_STRUCT));
                DATE_STRUCT *dt = (DATE_STRUCT *)ba.constData();
                QDate qdt = values.at(i).toDate();
                dt->year = qdt.year();
                dt->month = qdt.month();
                dt->day = qdt.day();
                r = SQLBindParameter(d->hStmt,
                                     i + 1,
                                     qParamType[bindValueType(i) & 3],
                                     SQL_C_DATE,
                                     SQL_DATE,
                                     0,
                                     0,
                                     (void *) dt,
                                     0,
                                     *ind == SQL_NULL_DATA ? ind : NULL);
                tmpStorage.append(ba);
                break; }
            case QVariant::Time: {
                QByteArray ba;
                ba.resize(sizeof(TIME_STRUCT));
                TIME_STRUCT *dt = (TIME_STRUCT *)ba.constData();
                QTime qdt = values.at(i).toTime();
                dt->hour = qdt.hour();
                dt->minute = qdt.minute();
                dt->second = qdt.second();
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_TIME,
                                      SQL_TIME,
                                      0,
                                      0,
                                      (void *) dt,
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                tmpStorage.append(ba);
                break; }
            case QVariant::DateTime: {
                QByteArray ba;
                ba.resize(sizeof(TIMESTAMP_STRUCT));
                TIMESTAMP_STRUCT * dt = (TIMESTAMP_STRUCT *)ba.constData();
                QDateTime qdt = values.at(i).toDateTime();
                dt->year = qdt.date().year();
                dt->month = qdt.date().month();
                dt->day = qdt.date().day();
                dt->hour = qdt.time().hour();
                dt->minute = qdt.time().minute();
                dt->second = qdt.time().second();
                dt->fraction = qdt.time().msec() * 1000000;
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_TIMESTAMP,
                                      SQL_TIMESTAMP,
                                      0,
                                      0,
                                      (void *) dt,
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                tmpStorage.append(ba);
                break; }
            case QVariant::Int:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_SLONG,
                                      SQL_INTEGER,
                                      0,
                                      0,
                                      (void *)values.at(i).constData(),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QVariant::Double:
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_DOUBLE,
                                      SQL_DOUBLE,
                                      0,
                                      0,
                                      (void *)values.at(i).constData(),
                                      0,
                                      *ind == SQL_NULL_DATA ? ind : NULL);
                break;
            case QVariant::ByteArray: {
                int len = values.at(i).toByteArray().size();
                if (*ind != SQL_NULL_DATA)
                    *ind = len;
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_BINARY,
                                      SQL_LONGVARBINARY,
                                      len,
                                      0,
                                      (void *)values.at(i).toByteArray().constData(),
                                      len,
                                      ind);
                break; }
            case QVariant::String:
            {
                QString str(values.at(i).toString());
                if (*ind != SQL_NULL_DATA)
                    *ind = str.length() * sizeof(QChar);
                if (bindValueType(i) & QSql::Out) {
                    QByteArray ba((char*)str.utf16(), str.capacity() * sizeof(QChar));
                    r = SQLBindParameter(d->hStmt,
                                        i + 1,
                                        qParamType[bindValueType(i) & 3],
                                        SQL_C_WCHAR,
                                        SQL_WVARCHAR,
                                        str.length(),
                                        0,
                                        (void *)ba.constData(),
                                        ba.size(),
                                        ind);
                    tmpStorage.append(ba);
                } else {
                    void *data = (void*)str.utf16();
                    int len = str.length();
                    r = SQLBindParameter(d->hStmt,
                                        i + 1,
                                        qParamType[bindValueType(i) & 3],
                                        SQL_C_WCHAR,
                                        SQL_WVARCHAR,
                                        len,
                                        0,
                                        data,
                                        len * sizeof(QChar),
                                        ind);
                }
                break;
            }
            default: {
                QByteArray ba = values.at(i).toString().toLatin1();
                int len = ba.length() + 1;
                if (*ind != SQL_NULL_DATA)
                    *ind = ba.length();
                r = SQLBindParameter(d->hStmt,
                                      i + 1,
                                      qParamType[bindValueType(i) & 3],
                                      SQL_C_CHAR,
                                      SQL_VARCHAR,
                                      len,
                                      0,
                                      (void *) ba.constData(),
                                      len,
                                      ind);
                tmpStorage.append(ba);
                break; }
        }
        if (r != SQL_SUCCESS) {
            qWarning("QDB2Result::exec: unable to bind variable: %s",
                     qDB2Warn(d).toLocal8Bit().constData());
            setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                    "Unable to bind variable"), QSqlError::StatementError, d));
            return false;
        }
    }

    r = SQLExecute(d->hStmt);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qWarning("QDB2Result::exec: Unable to execute statement: %s",
                 qDB2Warn(d).toLocal8Bit().constData());
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                "Unable to execute statement"), QSqlError::StatementError, d));
        return false;
    }
    SQLSMALLINT count;
    r = SQLNumResultCols(d->hStmt, &count);
    if (count) {
        setSelect(true);
        for (int i = 0; i < count; ++i) {
            d->recInf.append(qMakeFieldInfo(d, i));
        }
    } else {
        setSelect(false);
    }
    setActive(true);
    d->valueCache.resize(count);
    d->valueCache.fill(NULL);

    //get out parameters
    if (!hasOutValues())
        return true;

    for (i = 0; i < values.count(); ++i) {
        switch (values[i].type()) {
            case QVariant::Date: {
                DATE_STRUCT ds = *((DATE_STRUCT *)tmpStorage.takeFirst().constData());
                values[i] = QVariant(QDate(ds.year, ds.month, ds.day));
                break; }
            case QVariant::Time: {
                TIME_STRUCT dt = *((TIME_STRUCT *)tmpStorage.takeFirst().constData());
                values[i] = QVariant(QTime(dt.hour, dt.minute, dt.second));
                break; }
            case QVariant::DateTime: {
                TIMESTAMP_STRUCT dt = *((TIMESTAMP_STRUCT *)tmpStorage.takeFirst().constData());
                values[i] = QVariant(QDateTime(QDate(dt.year, dt.month, dt.day),
                              QTime(dt.hour, dt.minute, dt.second, dt.fraction / 1000000)));
                break; }
            case QVariant::Int:
            case QVariant::Double:
            case QVariant::ByteArray:
                break;
            case QVariant::String:
                if (bindValueType(i) & QSql::Out)
                    values[i] = QString((const QChar *)tmpStorage.takeFirst().constData());
                break;
            default: {
                values[i] = QString::fromLatin1(tmpStorage.takeFirst().constData());
                break; }
        }
        if (indicators[i] == SQL_NULL_DATA)
            values[i] = QVariant(values[i].type());
    }
    return true;
}

bool QDB2Result::fetch(int i)
{
    if (isForwardOnly() && i < at())
        return false;
    if (i == at())
        return true;
    d->clearValueCache();
    int actualIdx = i + 1;
    if (actualIdx <= 0) {
        setAt(QSql::BeforeFirstRow);
        return false;
    }
    SQLRETURN r;
    if (isForwardOnly()) {
        bool ok = true;
        while (ok && i > at())
            ok = fetchNext();
        return ok;
    } else {
        r = SQLFetchScroll(d->hStmt,
                            SQL_FETCH_ABSOLUTE,
                            actualIdx);
    }
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO && r != SQL_NO_DATA) {
        setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                "Unable to fetch record %1").arg(i), QSqlError::StatementError, d));
        return false;
    }
    else if (r == SQL_NO_DATA)
        return false;
    setAt(i);
    return true;
}

bool QDB2Result::fetchNext()
{
    SQLRETURN r;
    d->clearValueCache();
    r = SQLFetchScroll(d->hStmt,
                       SQL_FETCH_NEXT,
                       0);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        if (r != SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QDB2Result",
                                    "Unable to fetch next"), QSqlError::StatementError, d));
        return false;
    }
    setAt(at() + 1);
    return true;
}

bool QDB2Result::fetchFirst()
{
    if (isForwardOnly() && at() != QSql::BeforeFirstRow)
        return false;
    if (isForwardOnly())
        return fetchNext();
    d->clearValueCache();
    SQLRETURN r;
    r = SQLFetchScroll(d->hStmt,
                       SQL_FETCH_FIRST,
                       0);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        if(r!= SQL_NO_DATA)
            setLastError(qMakeError(QCoreApplication::translate("QDB2Result", "Unable to fetch first"),
                                    QSqlError::StatementError, d));
        return false;
    }
    setAt(0);
    return true;
}

bool QDB2Result::fetchLast()
{
    d->clearValueCache();

    int i = at();
    if (i == QSql::AfterLastRow) {
        if (isForwardOnly()) {
            return false;
        } else {
            if (!fetch(0))
                return false;
            i = at();
        }
    }

    while (fetchNext())
        ++i;

    if (i == QSql::BeforeFirstRow) {
        setAt(QSql::AfterLastRow);
        return false;
    }

    if (!isForwardOnly())
        return fetch(i);

    setAt(i);
    return true;
}


QVariant QDB2Result::data(int field)
{
    if (field >= d->recInf.count()) {
        qWarning("QDB2Result::data: column %d out of range", field);
        return QVariant();
    }
    SQLRETURN r = 0;
    SQLINTEGER lengthIndicator = 0;
    bool isNull = false;
    const QSqlField info = d->recInf.field(field);

    if (!info.isValid() || field >= d->valueCache.size())
        return QVariant();

    if (d->valueCache[field])
        return *d->valueCache[field];


    QVariant* v = 0;
    switch (info.type()) {
        case QVariant::LongLong:
            v = new QVariant((qint64) qGetBigIntData(d->hStmt, field, isNull));
            break;
        case QVariant::Int:
            v = new QVariant(qGetIntData(d->hStmt, field, isNull));
            break;
        case QVariant::Date: {
            DATE_STRUCT dbuf;
            r = SQLGetData(d->hStmt,
                            field + 1,
                            SQL_C_DATE,
                            (SQLPOINTER) &dbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA)) {
                v = new QVariant(QDate(dbuf.year, dbuf.month, dbuf.day));
            } else {
                v = new QVariant(QDate());
                isNull = true;
            }
            break; }
        case QVariant::Time: {
            TIME_STRUCT tbuf;
            r = SQLGetData(d->hStmt,
                            field + 1,
                            SQL_C_TIME,
                            (SQLPOINTER) &tbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA)) {
                v = new QVariant(QTime(tbuf.hour, tbuf.minute, tbuf.second));
            } else {
                v = new QVariant(QTime());
                isNull = true;
            }
            break; }
        case QVariant::DateTime: {
            TIMESTAMP_STRUCT dtbuf;
            r = SQLGetData(d->hStmt,
                            field + 1,
                            SQL_C_TIMESTAMP,
                            (SQLPOINTER) &dtbuf,
                            0,
                            &lengthIndicator);
            if ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (lengthIndicator != SQL_NULL_DATA)) {
                v = new QVariant(QDateTime(QDate(dtbuf.year, dtbuf.month, dtbuf.day),
                                             QTime(dtbuf.hour, dtbuf.minute, dtbuf.second, dtbuf.fraction / 1000000)));
            } else {
                v = new QVariant(QDateTime());
                isNull = true;
            }
            break; }
        case QVariant::ByteArray:
            v = new QVariant(qGetBinaryData(d->hStmt, field, lengthIndicator, isNull));
            break;
        case QVariant::Double:
            {
            switch(numericalPrecisionPolicy()) {
                case QSql::LowPrecisionInt32:
                    v = new QVariant(qGetIntData(d->hStmt, field, isNull));
                    break;
                case QSql::LowPrecisionInt64:
                    v = new QVariant((qint64) qGetBigIntData(d->hStmt, field, isNull));
                    break;
                case QSql::LowPrecisionDouble:
                    v = new QVariant(qGetDoubleData(d->hStmt, field, isNull));
                    break;
                case QSql::HighPrecision:
                default:
                    // length + 1 for the comma
                    v = new QVariant(qGetStringData(d->hStmt, field, info.length() + 1, isNull));
                    break;
            }
            break;
            }
        case QVariant::String:
        default:
            v = new QVariant(qGetStringData(d->hStmt, field, info.length(), isNull));
            break;
    }
    if (isNull)
        *v = QVariant(info.type());
    d->valueCache[field] = v;
    return *v;
}

bool QDB2Result::isNull(int i)
{
    if (i >= d->valueCache.size())
        return true;

    if (d->valueCache[i])
        return d->valueCache[i]->isNull();
    return data(i).isNull();
}

int QDB2Result::numRowsAffected()
{
    SQLINTEGER affectedRowCount = 0;
    SQLRETURN r = SQLRowCount(d->hStmt, &affectedRowCount);
    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        return affectedRowCount;
    else
        qSqlWarning(QLatin1String("QDB2Result::numRowsAffected: Unable to count affected rows"), d);
    return -1;
}

int QDB2Result::size()
{
    return -1;
}

QSqlRecord QDB2Result::record() const
{
    if (isActive())
        return d->recInf;
    return QSqlRecord();
}

bool QDB2Result::nextResult()
{
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    d->recInf.clear();
    d->emptyValueCache();
    setSelect(false);

    SQLRETURN r = SQLMoreResults(d->hStmt);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        if (r != SQL_NO_DATA) {
            setLastError(qMakeError(QCoreApplication::translate("QODBCResult",
                "Unable to fetch last"), QSqlError::ConnectionError, d));
        }
        return false;
    }

    SQLSMALLINT fieldCount;
    r = SQLNumResultCols(d->hStmt, &fieldCount);
    setSelect(fieldCount > 0);
    for (int i = 0; i < fieldCount; ++i)
        d->recInf.append(qMakeFieldInfo(d, i));

    d->valueCache.resize(fieldCount);
    d->valueCache.fill(NULL);
    setActive(true);

    return true;
}

void QDB2Result::virtual_hook(int id, void *data)
{
    QSqlResult::virtual_hook(id, data);
}

void QDB2Result::detachFromResultSet()
{
    if (d->hStmt)
        SQLCloseCursor(d->hStmt);
}

/************************************/

QDB2Driver::QDB2Driver(QObject* parent)
    : QSqlDriver(*new QDB2DriverPrivate, parent)
{
}

QDB2Driver::QDB2Driver(Qt::HANDLE env, Qt::HANDLE con, QObject* parent)
    : QSqlDriver(*new QDB2DriverPrivate, parent)
{
    Q_D(QDB2Driver);
    d->hEnv = (SQLHANDLE)env;
    d->hDbc = (SQLHANDLE)con;
    if (env && con) {
        setOpen(true);
        setOpenError(false);
    }
}

QDB2Driver::~QDB2Driver()
{
    close();
}

bool QDB2Driver::open(const QString& db, const QString& user, const QString& password, const QString& host, int port,
                       const QString& connOpts)
{
    Q_D(QDB2Driver);
    if (isOpen())
      close();
    SQLRETURN r;
    r = SQLAllocHandle(SQL_HANDLE_ENV,
                        SQL_NULL_HANDLE,
                        &d->hEnv);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning(QLatin1String("QDB2Driver::open: Unable to allocate environment"), d);
        setOpenError(true);
        return false;
    }

    r = SQLAllocHandle(SQL_HANDLE_DBC,
                        d->hEnv,
                        &d->hDbc);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning(QLatin1String("QDB2Driver::open: Unable to allocate connection"), d);
        setOpenError(true);
        return false;
    }

    QString protocol;
    // Set connection attributes
    const QStringList opts(connOpts.split(QLatin1Char(';'), QString::SkipEmptyParts));
    for (int i = 0; i < opts.count(); ++i) {
        const QString tmp(opts.at(i));
        int idx;
        if ((idx = tmp.indexOf(QLatin1Char('='))) == -1) {
            qWarning("QDB2Driver::open: Illegal connect option value '%s'",
                     tmp.toLocal8Bit().constData());
            continue;
        }

        const QString opt(tmp.left(idx));
        const QString val(tmp.mid(idx + 1).simplified());

        SQLUINTEGER v = 0;
        r = SQL_SUCCESS;
        if (opt == QLatin1String("SQL_ATTR_ACCESS_MODE")) {
            if (val == QLatin1String("SQL_MODE_READ_ONLY")) {
                v = SQL_MODE_READ_ONLY;
            } else if (val == QLatin1String("SQL_MODE_READ_WRITE")) {
                v = SQL_MODE_READ_WRITE;
            } else {
                qWarning("QDB2Driver::open: Unknown option value '%s'",
                         tmp.toLocal8Bit().constData());
                continue;
            }
            r = SQLSetConnectAttr(d->hDbc, SQL_ATTR_ACCESS_MODE, (SQLPOINTER) v, 0);
        } else if (opt == QLatin1String("SQL_ATTR_LOGIN_TIMEOUT")) {
            v = val.toUInt();
            r = SQLSetConnectAttr(d->hDbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER) v, 0);
        } else if (opt.compare(QLatin1String("PROTOCOL"), Qt::CaseInsensitive) == 0) {
                        protocol = tmp;
        }
        else {
            qWarning("QDB2Driver::open: Unknown connection attribute '%s'",
                      tmp.toLocal8Bit().constData());
        }
        if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
            qSqlWarning(QString::fromLatin1("QDB2Driver::open: "
                           "Unable to set connection attribute '%1'").arg(opt), d);
    }

    if (protocol.isEmpty())
        protocol = QLatin1String("PROTOCOL=TCPIP");

    if (port < 0 )
        port = 50000;

    QString connQStr;
    connQStr =  protocol + QLatin1String(";DATABASE=") + db + QLatin1String(";HOSTNAME=") + host
        + QLatin1String(";PORT=") + QString::number(port) + QLatin1String(";UID=") + user
        + QLatin1String(";PWD=") + password;


    SQLTCHAR connOut[SQL_MAX_OPTION_STRING_LENGTH];
    SQLSMALLINT cb;

    r = SQLDriverConnect(d->hDbc,
                          NULL,
                          qToTChar(connQStr),
                          (SQLSMALLINT) connQStr.length(),
                          connOut,
                          SQL_MAX_OPTION_STRING_LENGTH,
                          &cb,
                          SQL_DRIVER_NOPROMPT);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        setLastError(qMakeError(tr("Unable to connect"),
                                QSqlError::ConnectionError, d));
        setOpenError(true);
        return false;
    }

    d->user = user;
    setOpen(true);
    setOpenError(false);
    return true;
}

void QDB2Driver::close()
{
    Q_D(QDB2Driver);
    SQLRETURN r;
    if (d->hDbc) {
        // Open statements/descriptors handles are automatically cleaned up by SQLDisconnect
        if (isOpen()) {
            r = SQLDisconnect(d->hDbc);
            if (r != SQL_SUCCESS)
                qSqlWarning(QLatin1String("QDB2Driver::close: Unable to disconnect datasource"), d);
        }
        r = SQLFreeHandle(SQL_HANDLE_DBC, d->hDbc);
        if (r != SQL_SUCCESS)
            qSqlWarning(QLatin1String("QDB2Driver::close: Unable to free connection handle"), d);
        d->hDbc = 0;
    }

    if (d->hEnv) {
        r = SQLFreeHandle(SQL_HANDLE_ENV, d->hEnv);
        if (r != SQL_SUCCESS)
            qSqlWarning(QLatin1String("QDB2Driver::close: Unable to free environment handle"), d);
        d->hEnv = 0;
    }
    setOpen(false);
    setOpenError(false);
}

QSqlResult *QDB2Driver::createResult() const
{
    Q_D(const QDB2Driver);
    return new QDB2Result(this, d);
}

QSqlRecord QDB2Driver::record(const QString& tableName) const
{
    Q_D(const QDB2Driver);
    QSqlRecord fil;
    if (!isOpen())
        return fil;

    SQLHANDLE hStmt;
    QString catalog, schema, table;
    qSplitTableQualifier(tableName, &catalog, &schema, &table);
    if (schema.isEmpty())
        schema = d->user;

    if (isIdentifierEscaped(catalog, QSqlDriver::TableName))
        catalog = stripDelimiters(catalog, QSqlDriver::TableName);
    else
        catalog = catalog.toUpper();

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = schema.toUpper();

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = table.toUpper();

    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning(QLatin1String("QDB2Driver::record: Unable to allocate handle"), d);
        return fil;
    }

    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER) SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);


    //Aside: szSchemaName and szTableName parameters of SQLColumns
    //are case sensitive search patterns, so no escaping is used.
    r =  SQLColumns(hStmt,
                     NULL,
                     0,
                     qToTChar(schema),
                     schema.length(),
                     qToTChar(table),
                     table.length(),
                     NULL,
                     0);

    if (r != SQL_SUCCESS)
        qSqlWarning(QLatin1String("QDB2Driver::record: Unable to execute column list"), d);
    r = SQLFetchScroll(hStmt,
                        SQL_FETCH_NEXT,
                        0);
    while (r == SQL_SUCCESS) {
        fil.append(qMakeFieldInfo(hStmt));
        r = SQLFetchScroll(hStmt,
                            SQL_FETCH_NEXT,
                            0);
    }

    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r != SQL_SUCCESS)
        qSqlWarning(QLatin1String("QDB2Driver: Unable to free statement handle ")
                    + QString::number(r), d);

    return fil;
}

QStringList QDB2Driver::tables(QSql::TableType type) const
{
    Q_D(const QDB2Driver);
    QStringList tl;
    if (!isOpen())
        return tl;

    SQLHANDLE hStmt;

    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) {
        qSqlWarning(QLatin1String("QDB2Driver::tables: Unable to allocate handle"), d);
        return tl;
    }
    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);

    QString tableType;
    if (type & QSql::Tables)
        tableType += QLatin1String("TABLE,");
    if (type & QSql::Views)
        tableType += QLatin1String("VIEW,");
    if (type & QSql::SystemTables)
        tableType += QLatin1String("SYSTEM TABLE,");
    if (tableType.isEmpty())
        return tl;
    tableType.chop(1);

    r = SQLTables(hStmt,
                   NULL,
                   0,
                   NULL,
                   0,
                   NULL,
                   0,
                   qToTChar(tableType),
                   tableType.length());

    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
        qSqlWarning(QLatin1String("QDB2Driver::tables: Unable to execute table list"), d);
    r = SQLFetchScroll(hStmt,
                        SQL_FETCH_NEXT,
                        0);
    while (r == SQL_SUCCESS) {
        bool isNull;
        QString fieldVal = qGetStringData(hStmt, 2, -1, isNull);
        QString userVal = qGetStringData(hStmt, 1, -1, isNull);
        QString user = d->user;
        if ( isIdentifierEscaped(user, QSqlDriver::TableName))
            user = stripDelimiters(user, QSqlDriver::TableName);
        else
            user = user.toUpper();

        if (userVal != user)
            fieldVal = userVal + QLatin1Char('.') + fieldVal;
        tl.append(fieldVal);
        r = SQLFetchScroll(hStmt,
                            SQL_FETCH_NEXT,
                            0);
    }

    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r != SQL_SUCCESS)
        qSqlWarning(QLatin1String("QDB2Driver::tables: Unable to free statement handle ")
                    + QString::number(r), d);
    return tl;
}

QSqlIndex QDB2Driver::primaryIndex(const QString& tablename) const
{
    Q_D(const QDB2Driver);
    QSqlIndex index(tablename);
    if (!isOpen())
        return index;
    QSqlRecord rec = record(tablename);

    SQLHANDLE hStmt;
    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
                                  d->hDbc,
                                  &hStmt);
    if (r != SQL_SUCCESS) {
        qSqlWarning(QLatin1String("QDB2Driver::primaryIndex: Unable to list primary key"), d);
        return index;
    }
    QString catalog, schema, table;
    qSplitTableQualifier(tablename, &catalog, &schema, &table);

    if (isIdentifierEscaped(catalog, QSqlDriver::TableName))
        catalog = stripDelimiters(catalog, QSqlDriver::TableName);
    else
        catalog = catalog.toUpper();

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = schema.toUpper();

    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);
    else
        table = table.toUpper();

    r = SQLSetStmtAttr(hStmt,
                        SQL_ATTR_CURSOR_TYPE,
                        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                        SQL_IS_UINTEGER);

    r = SQLPrimaryKeys(hStmt,
                        NULL,
                        0,
                        qToTChar(schema),
                        schema.length(),
                        qToTChar(table),
                        table.length());
    r = SQLFetchScroll(hStmt,
                        SQL_FETCH_NEXT,
                        0);

    bool isNull;
    QString cName, idxName;
    // Store all fields in a StringList because the driver can't detail fields in this FETCH loop
    while (r == SQL_SUCCESS) {
        cName = qGetStringData(hStmt, 3, -1, isNull); // column name
        idxName = qGetStringData(hStmt, 5, -1, isNull); // pk index name
        index.append(rec.field(cName));
        index.setName(idxName);
        r = SQLFetchScroll(hStmt,
                            SQL_FETCH_NEXT,
                            0);
    }
    r = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (r!= SQL_SUCCESS)
        qSqlWarning(QLatin1String("QDB2Driver: Unable to free statement handle ")
                    + QString::number(r), d);
    return index;
}

bool QDB2Driver::hasFeature(DriverFeature f) const
{
    switch (f) {
        case QuerySize:
        case NamedPlaceholders:
        case BatchOperations:
        case LastInsertId:
        case SimpleLocking:
        case EventNotifications:
            return false;
        case BLOB:
        case Transactions:
        case MultipleResultSets:
        case PreparedQueries:
        case PositionalPlaceholders:
        case LowPrecisionNumbers:
        case FinishQuery:
            return true;
        case Unicode:
            return true;
    }
    return false;
}

bool QDB2Driver::beginTransaction()
{
    if (!isOpen()) {
        qWarning("QDB2Driver::beginTransaction: Database not open");
        return false;
    }
    return setAutoCommit(false);
}

bool QDB2Driver::commitTransaction()
{
    Q_D(QDB2Driver);
    if (!isOpen()) {
        qWarning("QDB2Driver::commitTransaction: Database not open");
        return false;
    }
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC,
                              d->hDbc,
                              SQL_COMMIT);
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to commit transaction"),
                     QSqlError::TransactionError, d));
        return false;
    }
    return setAutoCommit(true);
}

bool QDB2Driver::rollbackTransaction()
{
    Q_D(QDB2Driver);
    if (!isOpen()) {
        qWarning("QDB2Driver::rollbackTransaction: Database not open");
        return false;
    }
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC,
                              d->hDbc,
                              SQL_ROLLBACK);
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to rollback transaction"),
                                QSqlError::TransactionError, d));
        return false;
    }
    return setAutoCommit(true);
}

bool QDB2Driver::setAutoCommit(bool autoCommit)
{
    Q_D(QDB2Driver);
    SQLUINTEGER ac = autoCommit ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
    SQLRETURN r  = SQLSetConnectAttr(d->hDbc,
                                      SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)ac,
                                      sizeof(ac));
    if (r != SQL_SUCCESS) {
        setLastError(qMakeError(tr("Unable to set autocommit"),
                                QSqlError::TransactionError, d));
        return false;
    }
    return true;
}

QString QDB2Driver::formatValue(const QSqlField &field, bool trimStrings) const
{
    if (field.isNull())
        return QLatin1String("NULL");

    switch (field.type()) {
        case QVariant::DateTime: {
            // Use an escape sequence for the datetime fields
            if (field.value().toDateTime().isValid()) {
                QDate dt = field.value().toDateTime().date();
                QTime tm = field.value().toDateTime().time();
                // Dateformat has to be "yyyy-MM-dd hh:mm:ss", with leading zeroes if month or day < 10
                return QLatin1Char('\'') + QString::number(dt.year()) + QLatin1Char('-')
                       + QString::number(dt.month()) + QLatin1Char('-')
                       + QString::number(dt.day()) + QLatin1Char('-')
                       + QString::number(tm.hour()) + QLatin1Char('.')
                       + QString::number(tm.minute()).rightJustified(2, QLatin1Char('0'), true)
                       + QLatin1Char('.')
                       + QString::number(tm.second()).rightJustified(2, QLatin1Char('0'), true)
                       + QLatin1Char('.')
                       + QString::number(tm.msec() * 1000).rightJustified(6, QLatin1Char('0'), true)
                       + QLatin1Char('\'');
                } else {
                    return QLatin1String("NULL");
                }
        }
        case QVariant::ByteArray: {
            QByteArray ba = field.value().toByteArray();
            QString res = QString::fromLatin1("BLOB(X'");
            static const char hexchars[] = "0123456789abcdef";
            for (int i = 0; i < ba.size(); ++i) {
                uchar s = (uchar) ba[i];
                res += QLatin1Char(hexchars[s >> 4]);
                res += QLatin1Char(hexchars[s & 0x0f]);
            }
            res += QLatin1String("')");
            return res;
        }
        default:
            return QSqlDriver::formatValue(field, trimStrings);
    }
}

QVariant QDB2Driver::handle() const
{
    Q_D(const QDB2Driver);
    return QVariant(qRegisterMetaType<SQLHANDLE>("SQLHANDLE"), &d->hDbc);
}

QString QDB2Driver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    QString res = identifier;
    if(!identifier.isEmpty() && !identifier.startsWith(QLatin1Char('"')) && !identifier.endsWith(QLatin1Char('"')) ) {
        res.replace(QLatin1Char('"'), QLatin1String("\"\""));
        res.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
        res.replace(QLatin1Char('.'), QLatin1String("\".\""));
    }
    return res;
}

QT_END_NAMESPACE
