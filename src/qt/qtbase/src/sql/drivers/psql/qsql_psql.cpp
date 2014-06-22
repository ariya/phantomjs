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

#include "qsql_psql_p.h"

#include <qcoreapplication.h>
#include <qvariant.h>
#include <qdatetime.h>
#include <qregexp.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlrecord.h>
#include <qsqlquery.h>
#include <qsocketnotifier.h>
#include <qstringlist.h>
#include <qmutex.h>
#include <QtSql/private/qsqlresult_p.h>
#include <QtSql/private/qsqldriver_p.h>

#include <libpq-fe.h>
#include <pg_config.h>

#include <stdlib.h>
#include <math.h>
// below code taken from an example at http://www.gnu.org/software/hello/manual/autoconf/Function-Portability.html
#ifndef isnan
    # define isnan(x) \
        (sizeof (x) == sizeof (long double) ? isnan_ld (x) \
        : sizeof (x) == sizeof (double) ? isnan_d (x) \
        : isnan_f (x))
    static inline int isnan_f  (float       x) { return x != x; }
    static inline int isnan_d  (double      x) { return x != x; }
    static inline int isnan_ld (long double x) { return x != x; }
#endif

#ifndef isinf
    # define isinf(x) \
        (sizeof (x) == sizeof (long double) ? isinf_ld (x) \
        : sizeof (x) == sizeof (double) ? isinf_d (x) \
        : isinf_f (x))
    static inline int isinf_f  (float       x) { return isnan (x - x); }
    static inline int isinf_d  (double      x) { return isnan (x - x); }
    static inline int isinf_ld (long double x) { return isnan (x - x); }
#endif


// workaround for postgres defining their OIDs in a private header file
#define QBOOLOID 16
#define QINT8OID 20
#define QINT2OID 21
#define QINT4OID 23
#define QNUMERICOID 1700
#define QFLOAT4OID 700
#define QFLOAT8OID 701
#define QABSTIMEOID 702
#define QRELTIMEOID 703
#define QDATEOID 1082
#define QTIMEOID 1083
#define QTIMETZOID 1266
#define QTIMESTAMPOID 1114
#define QTIMESTAMPTZOID 1184
#define QOIDOID 2278
#define QBYTEAOID 17
#define QREGPROCOID 24
#define QXIDOID 28
#define QCIDOID 29

/* This is a compile time switch - if PQfreemem is declared, the compiler will use that one,
   otherwise it'll run in this template */
template <typename T>
inline void PQfreemem(T *t, int = 0) { free(t); }

Q_DECLARE_OPAQUE_POINTER(PGconn*)
Q_DECLARE_METATYPE(PGconn*)

Q_DECLARE_OPAQUE_POINTER(PGresult*)
Q_DECLARE_METATYPE(PGresult*)

QT_BEGIN_NAMESPACE

inline void qPQfreemem(void *buffer)
{
    PQfreemem(buffer);
}

class QPSQLDriverPrivate : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QPSQLDriver)
public:
    QPSQLDriverPrivate() : QSqlDriverPrivate(),
        connection(0),
        isUtf8(false),
        pro(QPSQLDriver::Version6),
        sn(0),
        pendingNotifyCheck(false),
        hasBackslashEscape(false)
    { dbmsType = PostgreSQL; }

    PGconn *connection;
    bool isUtf8;
    QPSQLDriver::Protocol pro;
    QSocketNotifier *sn;
    QStringList seid;
    mutable bool pendingNotifyCheck;
    bool hasBackslashEscape;

    void appendTables(QStringList &tl, QSqlQuery &t, QChar type);
    PGresult * exec(const char * stmt) const;
    PGresult * exec(const QString & stmt) const;
    QPSQLDriver::Protocol getPSQLVersion();
    bool setEncodingUtf8();
    void setDatestyle();
    void detectBackslashEscape();
};

void QPSQLDriverPrivate::appendTables(QStringList &tl, QSqlQuery &t, QChar type)
{
    QString query;
    if (pro >= QPSQLDriver::Version73) {
        query = QString::fromLatin1("select pg_class.relname, pg_namespace.nspname from pg_class "
                  "left join pg_namespace on (pg_class.relnamespace = pg_namespace.oid) "
                  "where (pg_class.relkind = '%1') and (pg_class.relname !~ '^Inv') "
                  "and (pg_class.relname !~ '^pg_') "
                  "and (pg_namespace.nspname != 'information_schema') ").arg(type);
    } else {
        query = QString::fromLatin1("select relname, null from pg_class where (relkind = '%1') "
                  "and (relname !~ '^Inv') "
                  "and (relname !~ '^pg_') ").arg(type);
    }
    t.exec(query);
    while (t.next()) {
        QString schema = t.value(1).toString();
        if (schema.isEmpty() || schema == QLatin1String("public"))
            tl.append(t.value(0).toString());
        else
            tl.append(t.value(0).toString().prepend(QLatin1Char('.')).prepend(schema));
    }
}

PGresult * QPSQLDriverPrivate::exec(const char * stmt) const
{
    Q_Q(const QPSQLDriver);
    PGresult *result = PQexec(connection, stmt);
    if (seid.size() && !pendingNotifyCheck) {
        pendingNotifyCheck = true;
        QMetaObject::invokeMethod(const_cast<QPSQLDriver*>(q), "_q_handleNotification", Qt::QueuedConnection, Q_ARG(int,0));
    }
    return result;
}

PGresult * QPSQLDriverPrivate::exec(const QString & stmt) const
{
    return exec(isUtf8 ? stmt.toUtf8().constData() : stmt.toLocal8Bit().constData());
}

class QPSQLResultPrivate : public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QPSQLResult)
public:
    QPSQLResultPrivate()
      : QSqlResultPrivate(),
        result(0),
        currentSize(-1),
        preparedQueriesEnabled(false)
    { }

    QString fieldSerial(int i) const { return QLatin1Char('$') + QString::number(i + 1); }
    void deallocatePreparedStmt();
    const QPSQLDriverPrivate * privDriver() const
    {
        Q_Q(const QPSQLResult);
        return reinterpret_cast<const QPSQLDriver *>(q->driver())->d_func();
    }

    PGresult *result;
    int currentSize;
    bool preparedQueriesEnabled;
    QString preparedStmtId;

    bool processResults();
};

static QSqlError qMakeError(const QString& err, QSqlError::ErrorType type,
                            const QPSQLDriverPrivate *p, PGresult* result = 0)
{
    const char *s = PQerrorMessage(p->connection);
    QString msg = p->isUtf8 ? QString::fromUtf8(s) : QString::fromLocal8Bit(s);
    QString errorCode;
    if (result) {
      errorCode = QString::fromLatin1(PQresultErrorField(result, PG_DIAG_SQLSTATE));
      msg += QString::fromLatin1("(%1)").arg(errorCode);
    }
    return QSqlError(QLatin1String("QPSQL: ") + err, msg, type, errorCode);
}

bool QPSQLResultPrivate::processResults()
{
    Q_Q(QPSQLResult);
    if (!result)
        return false;

    int status = PQresultStatus(result);
    if (status == PGRES_TUPLES_OK) {
        q->setSelect(true);
        q->setActive(true);
        currentSize = PQntuples(result);
        return true;
    } else if (status == PGRES_COMMAND_OK) {
        q->setSelect(false);
        q->setActive(true);
        currentSize = -1;
        return true;
    }
    q->setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                    "Unable to create query"), QSqlError::StatementError, privDriver(), result));
    return false;
}

static QVariant::Type qDecodePSQLType(int t)
{
    QVariant::Type type = QVariant::Invalid;
    switch (t) {
    case QBOOLOID:
        type = QVariant::Bool;
        break;
    case QINT8OID:
        type = QVariant::LongLong;
        break;
    case QINT2OID:
    case QINT4OID:
    case QOIDOID:
    case QREGPROCOID:
    case QXIDOID:
    case QCIDOID:
        type = QVariant::Int;
        break;
    case QNUMERICOID:
    case QFLOAT4OID:
    case QFLOAT8OID:
        type = QVariant::Double;
        break;
    case QABSTIMEOID:
    case QRELTIMEOID:
    case QDATEOID:
        type = QVariant::Date;
        break;
    case QTIMEOID:
    case QTIMETZOID:
        type = QVariant::Time;
        break;
    case QTIMESTAMPOID:
    case QTIMESTAMPTZOID:
        type = QVariant::DateTime;
        break;
    case QBYTEAOID:
        type = QVariant::ByteArray;
        break;
    default:
        type = QVariant::String;
        break;
    }
    return type;
}

void QPSQLResultPrivate::deallocatePreparedStmt()
{
    const QString stmt = QLatin1String("DEALLOCATE ") + preparedStmtId;
    PGresult *result = privDriver()->exec(stmt);

    if (PQresultStatus(result) != PGRES_COMMAND_OK)
        qWarning("Unable to free statement: %s", PQerrorMessage(privDriver()->connection));
    PQclear(result);
    preparedStmtId.clear();
}

QPSQLResult::QPSQLResult(const QPSQLDriver* db)
    : QSqlResult(*new QPSQLResultPrivate, db)
{
    Q_D(QPSQLResult);
    d->preparedQueriesEnabled = db->hasFeature(QSqlDriver::PreparedQueries);
}

QPSQLResult::~QPSQLResult()
{
    Q_D(QPSQLResult);
    cleanup();

    if (d->preparedQueriesEnabled && !d->preparedStmtId.isNull())
        d->deallocatePreparedStmt();
}

QVariant QPSQLResult::handle() const
{
    Q_D(const QPSQLResult);
    return QVariant::fromValue(d->result);
}

void QPSQLResult::cleanup()
{
    Q_D(QPSQLResult);
    if (d->result)
        PQclear(d->result);
    d->result = 0;
    setAt(QSql::BeforeFirstRow);
    d->currentSize = -1;
    setActive(false);
}

bool QPSQLResult::fetch(int i)
{
    Q_D(const QPSQLResult);
    if (!isActive())
        return false;
    if (i < 0)
        return false;
    if (i >= d->currentSize)
        return false;
    if (at() == i)
        return true;
    setAt(i);
    return true;
}

bool QPSQLResult::fetchFirst()
{
    return fetch(0);
}

bool QPSQLResult::fetchLast()
{
    Q_D(const QPSQLResult);
    return fetch(PQntuples(d->result) - 1);
}

QVariant QPSQLResult::data(int i)
{
    Q_D(const QPSQLResult);
    if (i >= PQnfields(d->result)) {
        qWarning("QPSQLResult::data: column %d out of range", i);
        return QVariant();
    }
    int ptype = PQftype(d->result, i);
    QVariant::Type type = qDecodePSQLType(ptype);
    const char *val = PQgetvalue(d->result, at(), i);
    if (PQgetisnull(d->result, at(), i))
        return QVariant(type);
    switch (type) {
    case QVariant::Bool:
        return QVariant((bool)(val[0] == 't'));
    case QVariant::String:
        return d->privDriver()->isUtf8 ? QString::fromUtf8(val) : QString::fromLatin1(val);
    case QVariant::LongLong:
        if (val[0] == '-')
            return QString::fromLatin1(val).toLongLong();
        else
            return QString::fromLatin1(val).toULongLong();
    case QVariant::Int:
        return atoi(val);
    case QVariant::Double:
        if (ptype == QNUMERICOID) {
            if (numericalPrecisionPolicy() != QSql::HighPrecision) {
                QVariant retval;
                bool convert;
                double dbl=QString::fromLatin1(val).toDouble(&convert);
                if (numericalPrecisionPolicy() == QSql::LowPrecisionInt64)
                    retval = (qlonglong)dbl;
                else if (numericalPrecisionPolicy() == QSql::LowPrecisionInt32)
                    retval = (int)dbl;
                else if (numericalPrecisionPolicy() == QSql::LowPrecisionDouble)
                    retval = dbl;
                if (!convert)
                    return QVariant();
                return retval;
            }
            return QString::fromLatin1(val);
        }
        return QString::fromLatin1(val).toDouble();
    case QVariant::Date:
        if (val[0] == '\0') {
            return QVariant(QDate());
        } else {
#ifndef QT_NO_DATESTRING
            return QVariant(QDate::fromString(QString::fromLatin1(val), Qt::ISODate));
#else
            return QVariant(QString::fromLatin1(val));
#endif
        }
    case QVariant::Time: {
        const QString str = QString::fromLatin1(val);
#ifndef QT_NO_DATESTRING
        if (str.isEmpty())
            return QVariant(QTime());
        if (str.at(str.length() - 3) == QLatin1Char('+') || str.at(str.length() - 3) == QLatin1Char('-'))
            // strip the timezone
            // TODO: fix this when timestamp support comes into QDateTime
            return QVariant(QTime::fromString(str.left(str.length() - 3), Qt::ISODate));
        return QVariant(QTime::fromString(str, Qt::ISODate));
#else
        return QVariant(str);
#endif
    }
    case QVariant::DateTime: {
        QString dtval = QString::fromLatin1(val);
#ifndef QT_NO_DATESTRING
        if (dtval.length() < 10)
            return QVariant(QDateTime());
        // remove the timezone
        // TODO: fix this when timestamp support comes into QDateTime
        if (dtval.at(dtval.length() - 3) == QLatin1Char('+') || dtval.at(dtval.length() - 3) == QLatin1Char('-'))
            dtval.chop(3);
        // milliseconds are sometimes returned with 2 digits only
        if (dtval.at(dtval.length() - 3).isPunct())
            dtval += QLatin1Char('0');
        if (dtval.isEmpty())
            return QVariant(QDateTime());
        else
            return QVariant(QDateTime::fromString(dtval, Qt::ISODate));
#else
        return QVariant(dtval);
#endif
    }
    case QVariant::ByteArray: {
        size_t len;
        unsigned char *data = PQunescapeBytea((unsigned char*)val, &len);
        QByteArray ba((const char*)data, len);
        qPQfreemem(data);
        return QVariant(ba);
    }
    default:
    case QVariant::Invalid:
        qWarning("QPSQLResult::data: unknown data type");
    }
    return QVariant();
}

bool QPSQLResult::isNull(int field)
{
    Q_D(const QPSQLResult);
    PQgetvalue(d->result, at(), field);
    return PQgetisnull(d->result, at(), field);
}

bool QPSQLResult::reset (const QString& query)
{
    Q_D(QPSQLResult);
    cleanup();
    if (!driver())
        return false;
    if (!driver()->isOpen() || driver()->isOpenError())
        return false;
    d->result = d->privDriver()->exec(query);
    return d->processResults();
}

int QPSQLResult::size()
{
    Q_D(const QPSQLResult);
    return d->currentSize;
}

int QPSQLResult::numRowsAffected()
{
    Q_D(const QPSQLResult);
    return QString::fromLatin1(PQcmdTuples(d->result)).toInt();
}

QVariant QPSQLResult::lastInsertId() const
{
    Q_D(const QPSQLResult);
    if (d->privDriver()->pro >= QPSQLDriver::Version81) {
        QSqlQuery qry(driver()->createResult());
        // Most recent sequence value obtained from nextval
        if (qry.exec(QLatin1String("SELECT lastval();")) && qry.next())
            return qry.value(0);
    } else if (isActive()) {
        Oid id = PQoidValue(d->result);
        if (id != InvalidOid)
            return QVariant(id);
    }
    return QVariant();
}

QSqlRecord QPSQLResult::record() const
{
    Q_D(const QPSQLResult);
    QSqlRecord info;
    if (!isActive() || !isSelect())
        return info;

    int count = PQnfields(d->result);
    for (int i = 0; i < count; ++i) {
        QSqlField f;
        if (d->privDriver()->isUtf8)
            f.setName(QString::fromUtf8(PQfname(d->result, i)));
        else
            f.setName(QString::fromLocal8Bit(PQfname(d->result, i)));
        f.setType(qDecodePSQLType(PQftype(d->result, i)));
        int len = PQfsize(d->result, i);
        int precision = PQfmod(d->result, i);
        // swap length and precision if length == -1
        if (len == -1 && precision > -1) {
            len = precision - 4;
            precision = -1;
        }
        f.setLength(len);
        f.setPrecision(precision);
        f.setSqlType(PQftype(d->result, i));
        info.append(f);
    }
    return info;
}

void QPSQLResult::virtual_hook(int id, void *data)
{
    Q_ASSERT(data);

    QSqlResult::virtual_hook(id, data);
}

static QString qCreateParamString(const QVector<QVariant> boundValues, const QSqlDriver *driver)
{
    if (boundValues.isEmpty())
        return QString();

    QString params;
    QSqlField f;
    for (int i = 0; i < boundValues.count(); ++i) {
        const QVariant &val = boundValues.at(i);

        f.setType(val.type());
        if (val.isNull())
            f.clear();
        else
            f.setValue(val);
        if(!params.isNull())
            params.append(QLatin1String(", "));
        params.append(driver->formatValue(f));
    }
    return params;
}

Q_GLOBAL_STATIC(QMutex, qMutex)
QString qMakePreparedStmtId()
{
    qMutex()->lock();
    static unsigned int qPreparedStmtCount = 0;
    QString id = QLatin1String("qpsqlpstmt_") + QString::number(++qPreparedStmtCount, 16);
    qMutex()->unlock();
    return id;
}

bool QPSQLResult::prepare(const QString &query)
{
    Q_D(QPSQLResult);
    if (!d->preparedQueriesEnabled)
        return QSqlResult::prepare(query);

    cleanup();

    if (!d->preparedStmtId.isEmpty())
        d->deallocatePreparedStmt();

    const QString stmtId = qMakePreparedStmtId();
    const QString stmt = QString::fromLatin1("PREPARE %1 AS ").arg(stmtId).append(d->positionalToNamedBinding(query));

    PGresult *result = d->privDriver()->exec(stmt);

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                                "Unable to prepare statement"), QSqlError::StatementError, d->privDriver(), result));
        PQclear(result);
        d->preparedStmtId.clear();
        return false;
    }

    PQclear(result);
    d->preparedStmtId = stmtId;
    return true;
}

bool QPSQLResult::exec()
{
    Q_D(QPSQLResult);
    if (!d->preparedQueriesEnabled)
        return QSqlResult::exec();

    cleanup();

    QString stmt;
    const QString params = qCreateParamString(boundValues(), driver());
    if (params.isEmpty())
        stmt = QString::fromLatin1("EXECUTE %1").arg(d->preparedStmtId);
    else
        stmt = QString::fromLatin1("EXECUTE %1 (%2)").arg(d->preparedStmtId).arg(params);

    d->result = d->privDriver()->exec(stmt);

    return d->processResults();
}

///////////////////////////////////////////////////////////////////

bool QPSQLDriverPrivate::setEncodingUtf8()
{
    PGresult* result = exec("SET CLIENT_ENCODING TO 'UNICODE'");
    int status = PQresultStatus(result);
    PQclear(result);
    return status == PGRES_COMMAND_OK;
}

void QPSQLDriverPrivate::setDatestyle()
{
    PGresult* result = exec("SET DATESTYLE TO 'ISO'");
    int status =  PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
        qWarning("%s", PQerrorMessage(connection));
    PQclear(result);
}

void QPSQLDriverPrivate::detectBackslashEscape()
{
    // standard_conforming_strings option introduced in 8.2
    // http://www.postgresql.org/docs/8.2/static/runtime-config-compatible.html
    if (pro < QPSQLDriver::Version82) {
        hasBackslashEscape = true;
    } else {
        hasBackslashEscape = false;
        PGresult* result = exec(QLatin1Literal("SELECT '\\\\' x"));
        int status = PQresultStatus(result);
        if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK)
            if (QString::fromLatin1(PQgetvalue(result, 0, 0)) == QLatin1Literal("\\"))
                hasBackslashEscape = true;
        PQclear(result);
    }
}

static QPSQLDriver::Protocol qMakePSQLVersion(int vMaj, int vMin)
{
    switch (vMaj) {
    case 6:
        return QPSQLDriver::Version6;
    case 7:
    {
        switch (vMin) {
        case 1:
            return QPSQLDriver::Version71;
        case 3:
            return QPSQLDriver::Version73;
        case 4:
            return QPSQLDriver::Version74;
        default:
            return QPSQLDriver::Version7;
        }
        break;
    }
    case 8:
    {
        switch (vMin) {
        case 1:
            return QPSQLDriver::Version81;
        case 2:
            return QPSQLDriver::Version82;
        case 3:
            return QPSQLDriver::Version83;
        case 4:
            return QPSQLDriver::Version84;
        default:
            return QPSQLDriver::Version8;
        }
        break;
    }
    case 9:
        return QPSQLDriver::Version9;
        break;
    default:
        break;
    }
    return QPSQLDriver::VersionUnknown;
}

QPSQLDriver::Protocol QPSQLDriverPrivate::getPSQLVersion()
{
    QPSQLDriver::Protocol serverVersion = QPSQLDriver::Version6;
    PGresult* result = exec("select version()");
    int status = PQresultStatus(result);
    if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) {
        QString val = QString::fromLatin1(PQgetvalue(result, 0, 0));

        QRegExp rx(QLatin1String("(\\d+)\\.(\\d+)"));
        rx.setMinimal(true); // enforce non-greedy RegExp

        if (rx.indexIn(val) != -1) {
            int vMaj = rx.cap(1).toInt();
            int vMin = rx.cap(2).toInt();
            serverVersion = qMakePSQLVersion(vMaj, vMin);
#if defined(PG_MAJORVERSION)
            if (rx.indexIn(QLatin1String(PG_MAJORVERSION)) != -1)
#elif defined(PG_VERSION)
            if (rx.indexIn(QLatin1String(PG_VERSION)) != -1)
#else
            if (0)
#endif
            {
                vMaj = rx.cap(1).toInt();
                vMin = rx.cap(2).toInt();
                QPSQLDriver::Protocol clientVersion = qMakePSQLVersion(vMaj, vMin);

                if (serverVersion >= QPSQLDriver::Version9 && clientVersion < QPSQLDriver::Version9) {
                    //Client version before QPSQLDriver::Version9 only supports escape mode for bytea type,
                    //but bytea format is set to hex by default in PSQL 9 and above. So need to force the
                    //server use the old escape mode when connects to the new server with old client library.
                    result = exec("SET bytea_output=escape; ");
                    status = PQresultStatus(result);
                } else if (serverVersion == QPSQLDriver::VersionUnknown) {
                    serverVersion = clientVersion;
                    if (serverVersion != QPSQLDriver::VersionUnknown)
                        qWarning("The server version of this PostgreSQL is unknown, falling back to the client version.");
                }
            }
        }
    }
    PQclear(result);

    //keep the old behavior unchanged
    if (serverVersion == QPSQLDriver::VersionUnknown)
        serverVersion = QPSQLDriver::Version6;

    if (serverVersion < QPSQLDriver::Version71) {
        qWarning("This version of PostgreSQL is not supported and may not work.");
    }

    return serverVersion;
}

QPSQLDriver::QPSQLDriver(QObject *parent)
    : QSqlDriver(*new QPSQLDriverPrivate, parent)
{
}

QPSQLDriver::QPSQLDriver(PGconn *conn, QObject *parent)
    : QSqlDriver(*new QPSQLDriverPrivate, parent)
{
    Q_D(QPSQLDriver);
    d->connection = conn;
    if (conn) {
        d->pro = d->getPSQLVersion();
        d->detectBackslashEscape();
        setOpen(true);
        setOpenError(false);
    }
}

QPSQLDriver::~QPSQLDriver()
{
    Q_D(QPSQLDriver);
    if (d->connection)
        PQfinish(d->connection);
}

QVariant QPSQLDriver::handle() const
{
    Q_D(const QPSQLDriver);
    return QVariant::fromValue(d->connection);
}

bool QPSQLDriver::hasFeature(DriverFeature f) const
{
    Q_D(const QPSQLDriver);
    switch (f) {
    case Transactions:
    case QuerySize:
    case LastInsertId:
    case LowPrecisionNumbers:
    case EventNotifications:
        return true;
    case PreparedQueries:
    case PositionalPlaceholders:
        return d->pro >= QPSQLDriver::Version82;
    case BatchOperations:
    case NamedPlaceholders:
    case SimpleLocking:
    case FinishQuery:
    case MultipleResultSets:
    case CancelQuery:
        return false;
    case BLOB:
        return d->pro >= QPSQLDriver::Version71;
    case Unicode:
        return d->isUtf8;
    }
    return false;
}

/*
   Quote a string for inclusion into the connection string
   \ -> \\
   ' -> \'
   surround string by single quotes
 */
static QString qQuote(QString s)
{
    s.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    s.replace(QLatin1Char('\''), QLatin1String("\\'"));
    s.append(QLatin1Char('\'')).prepend(QLatin1Char('\''));
    return s;
}

bool QPSQLDriver::open(const QString & db,
                        const QString & user,
                        const QString & password,
                        const QString & host,
                        int port,
                        const QString& connOpts)
{
    Q_D(QPSQLDriver);
    if (isOpen())
        close();
    QString connectString;
    if (!host.isEmpty())
        connectString.append(QLatin1String("host=")).append(qQuote(host));
    if (!db.isEmpty())
        connectString.append(QLatin1String(" dbname=")).append(qQuote(db));
    if (!user.isEmpty())
        connectString.append(QLatin1String(" user=")).append(qQuote(user));
    if (!password.isEmpty())
        connectString.append(QLatin1String(" password=")).append(qQuote(password));
    if (port != -1)
        connectString.append(QLatin1String(" port=")).append(qQuote(QString::number(port)));

    // add any connect options - the server will handle error detection
    if (!connOpts.isEmpty()) {
        QString opt = connOpts;
        opt.replace(QLatin1Char(';'), QLatin1Char(' '), Qt::CaseInsensitive);
        connectString.append(QLatin1Char(' ')).append(opt);
    }

    d->connection = PQconnectdb(connectString.toLocal8Bit().constData());
    if (PQstatus(d->connection) == CONNECTION_BAD) {
        setLastError(qMakeError(tr("Unable to connect"), QSqlError::ConnectionError, d));
        setOpenError(true);
        PQfinish(d->connection);
        d->connection = 0;
        return false;
    }

    d->pro = d->getPSQLVersion();
    d->detectBackslashEscape();
    d->isUtf8 = d->setEncodingUtf8();
    d->setDatestyle();

    setOpen(true);
    setOpenError(false);
    return true;
}

void QPSQLDriver::close()
{
    Q_D(QPSQLDriver);
    if (isOpen()) {

        d->seid.clear();
        if (d->sn) {
            disconnect(d->sn, SIGNAL(activated(int)), this, SLOT(_q_handleNotification(int)));
            delete d->sn;
            d->sn = 0;
        }

        if (d->connection)
            PQfinish(d->connection);
        d->connection = 0;
        setOpen(false);
        setOpenError(false);
    }
}

QSqlResult *QPSQLDriver::createResult() const
{
    return new QPSQLResult(this);
}

bool QPSQLDriver::beginTransaction()
{
    Q_D(const QPSQLDriver);
    if (!isOpen()) {
        qWarning("QPSQLDriver::beginTransaction: Database not open");
        return false;
    }
    PGresult* res = d->exec("BEGIN");
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(tr("Could not begin transaction"),
                                QSqlError::TransactionError, d, res));
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

bool QPSQLDriver::commitTransaction()
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qWarning("QPSQLDriver::commitTransaction: Database not open");
        return false;
    }
    PGresult* res = d->exec("COMMIT");

    bool transaction_failed = false;

    // XXX
    // This hack is used to tell if the transaction has succeeded for the protocol versions of
    // PostgreSQL below. For 7.x and other protocol versions we are left in the dark.
    // This hack can dissapear once there is an API to query this sort of information.
    if (d->pro == QPSQLDriver::Version8 ||
        d->pro == QPSQLDriver::Version81 ||
        d->pro == QPSQLDriver::Version82 ||
        d->pro == QPSQLDriver::Version83 ||
        d->pro == QPSQLDriver::Version84 ||
        d->pro == QPSQLDriver::Version9) {
        transaction_failed = qstrcmp(PQcmdStatus(res), "ROLLBACK") == 0;
    }

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK || transaction_failed) {
        setLastError(qMakeError(tr("Could not commit transaction"),
                                QSqlError::TransactionError, d, res));
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

bool QPSQLDriver::rollbackTransaction()
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qWarning("QPSQLDriver::rollbackTransaction: Database not open");
        return false;
    }
    PGresult* res = d->exec("ROLLBACK");
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(tr("Could not rollback transaction"),
                                QSqlError::TransactionError, d, res));
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

QStringList QPSQLDriver::tables(QSql::TableType type) const
{
    Q_D(const QPSQLDriver);
    QStringList tl;
    if (!isOpen())
        return tl;
    QSqlQuery t(createResult());
    t.setForwardOnly(true);

    if (type & QSql::Tables)
        const_cast<QPSQLDriverPrivate*>(d)->appendTables(tl, t, QLatin1Char('r'));
    if (type & QSql::Views)
        const_cast<QPSQLDriverPrivate*>(d)->appendTables(tl, t, QLatin1Char('v'));
    if (type & QSql::SystemTables) {
        t.exec(QLatin1String("select relname from pg_class where (relkind = 'r') "
                "and (relname like 'pg_%') "));
        while (t.next())
            tl.append(t.value(0).toString());
    }

    return tl;
}

static void qSplitTableName(QString &tablename, QString &schema)
{
    int dot = tablename.indexOf(QLatin1Char('.'));
    if (dot == -1)
        return;
    schema = tablename.left(dot);
    tablename = tablename.mid(dot + 1);
}

QSqlIndex QPSQLDriver::primaryIndex(const QString& tablename) const
{
    Q_D(const QPSQLDriver);
    QSqlIndex idx(tablename);
    if (!isOpen())
        return idx;
    QSqlQuery i(createResult());
    QString stmt;

    QString tbl = tablename;
    QString schema;
    qSplitTableName(tbl, schema);

    if (isIdentifierEscaped(tbl, QSqlDriver::TableName))
        tbl = stripDelimiters(tbl, QSqlDriver::TableName);
    else
        tbl = tbl.toLower();

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = schema.toLower();

    switch(d->pro) {
    case QPSQLDriver::Version6:
        stmt = QLatin1String("select pg_att1.attname, int(pg_att1.atttypid), pg_cl.relname "
                "from pg_attribute pg_att1, pg_attribute pg_att2, pg_class pg_cl, pg_index pg_ind "
                "where pg_cl.relname = '%1_pkey' "
                "and pg_cl.oid = pg_ind.indexrelid "
                "and pg_att2.attrelid = pg_ind.indexrelid "
                "and pg_att1.attrelid = pg_ind.indrelid "
                "and pg_att1.attnum = pg_ind.indkey[pg_att2.attnum-1] "
                "order by pg_att2.attnum");
        break;
    case QPSQLDriver::Version7:
    case QPSQLDriver::Version71:
        stmt = QLatin1String("select pg_att1.attname, pg_att1.atttypid::int, pg_cl.relname "
                "from pg_attribute pg_att1, pg_attribute pg_att2, pg_class pg_cl, pg_index pg_ind "
                "where pg_cl.relname = '%1_pkey' "
                "and pg_cl.oid = pg_ind.indexrelid "
                "and pg_att2.attrelid = pg_ind.indexrelid "
                "and pg_att1.attrelid = pg_ind.indrelid "
                "and pg_att1.attnum = pg_ind.indkey[pg_att2.attnum-1] "
                "order by pg_att2.attnum");
        break;
    case QPSQLDriver::Version73:
    case QPSQLDriver::Version74:
    case QPSQLDriver::Version8:
    case QPSQLDriver::Version81:
    case QPSQLDriver::Version82:
    case QPSQLDriver::Version83:
    case QPSQLDriver::Version84:
    case QPSQLDriver::Version9:
        stmt = QLatin1String("SELECT pg_attribute.attname, pg_attribute.atttypid::int, "
                "pg_class.relname "
                "FROM pg_attribute, pg_class "
                "WHERE %1 pg_class.oid IN "
                "(SELECT indexrelid FROM pg_index WHERE indisprimary = true AND indrelid IN "
                " (SELECT oid FROM pg_class WHERE relname = '%2')) "
                "AND pg_attribute.attrelid = pg_class.oid "
                "AND pg_attribute.attisdropped = false "
                "ORDER BY pg_attribute.attnum");
        if (schema.isEmpty())
            stmt = stmt.arg(QLatin1String("pg_table_is_visible(pg_class.oid) AND"));
        else
            stmt = stmt.arg(QString::fromLatin1("pg_class.relnamespace = (select oid from "
                   "pg_namespace where pg_namespace.nspname = '%1') AND ").arg(schema));
        break;
    case QPSQLDriver::VersionUnknown:
        qFatal("PSQL version is unknown");
        break;
    }

    i.exec(stmt.arg(tbl));
    while (i.isActive() && i.next()) {
        QSqlField f(i.value(0).toString(), qDecodePSQLType(i.value(1).toInt()));
        idx.append(f);
        idx.setName(i.value(2).toString());
    }
    return idx;
}

QSqlRecord QPSQLDriver::record(const QString& tablename) const
{
    Q_D(const QPSQLDriver);
    QSqlRecord info;
    if (!isOpen())
        return info;

    QString tbl = tablename;
    QString schema;
    qSplitTableName(tbl, schema);

    if (isIdentifierEscaped(tbl, QSqlDriver::TableName))
        tbl = stripDelimiters(tbl, QSqlDriver::TableName);
    else
        tbl = tbl.toLower();

    if (isIdentifierEscaped(schema, QSqlDriver::TableName))
        schema = stripDelimiters(schema, QSqlDriver::TableName);
    else
        schema = schema.toLower();

    QString stmt;
    switch(d->pro) {
    case QPSQLDriver::Version6:
        stmt = QLatin1String("select pg_attribute.attname, int(pg_attribute.atttypid), "
                "pg_attribute.attnotnull, pg_attribute.attlen, pg_attribute.atttypmod, "
                "int(pg_attribute.attrelid), pg_attribute.attnum "
                "from pg_class, pg_attribute "
                "where pg_class.relname = '%1' "
                "and pg_attribute.attnum > 0 "
                "and pg_attribute.attrelid = pg_class.oid ");
        break;
    case QPSQLDriver::Version7:
        stmt = QLatin1String("select pg_attribute.attname, pg_attribute.atttypid::int, "
                "pg_attribute.attnotnull, pg_attribute.attlen, pg_attribute.atttypmod, "
                "pg_attribute.attrelid::int, pg_attribute.attnum "
                "from pg_class, pg_attribute "
                "where pg_class.relname = '%1' "
                "and pg_attribute.attnum > 0 "
                "and pg_attribute.attrelid = pg_class.oid ");
        break;
    case QPSQLDriver::Version71:
        stmt = QLatin1String("select pg_attribute.attname, pg_attribute.atttypid::int, "
                "pg_attribute.attnotnull, pg_attribute.attlen, pg_attribute.atttypmod, "
                "pg_attrdef.adsrc "
                "from pg_class, pg_attribute "
                "left join pg_attrdef on (pg_attrdef.adrelid = "
                "pg_attribute.attrelid and pg_attrdef.adnum = pg_attribute.attnum) "
                "where pg_class.relname = '%1' "
                "and pg_attribute.attnum > 0 "
                "and pg_attribute.attrelid = pg_class.oid "
                "order by pg_attribute.attnum ");
        break;
    case QPSQLDriver::Version73:
    case QPSQLDriver::Version74:
    case QPSQLDriver::Version8:
    case QPSQLDriver::Version81:
    case QPSQLDriver::Version82:
    case QPSQLDriver::Version83:
    case QPSQLDriver::Version84:
    case QPSQLDriver::Version9:
        stmt = QLatin1String("select pg_attribute.attname, pg_attribute.atttypid::int, "
                "pg_attribute.attnotnull, pg_attribute.attlen, pg_attribute.atttypmod, "
                "pg_attrdef.adsrc "
                "from pg_class, pg_attribute "
                "left join pg_attrdef on (pg_attrdef.adrelid = "
                "pg_attribute.attrelid and pg_attrdef.adnum = pg_attribute.attnum) "
                "where %1 "
                "and pg_class.relname = '%2' "
                "and pg_attribute.attnum > 0 "
                "and pg_attribute.attrelid = pg_class.oid "
                "and pg_attribute.attisdropped = false "
                "order by pg_attribute.attnum ");
        if (schema.isEmpty())
            stmt = stmt.arg(QLatin1String("pg_table_is_visible(pg_class.oid)"));
        else
            stmt = stmt.arg(QString::fromLatin1("pg_class.relnamespace = (select oid from "
                   "pg_namespace where pg_namespace.nspname = '%1')").arg(schema));
        break;
    case QPSQLDriver::VersionUnknown:
        qFatal("PSQL version is unknown");
        break;
    }

    QSqlQuery query(createResult());
    query.exec(stmt.arg(tbl));
    if (d->pro >= QPSQLDriver::Version71) {
        while (query.next()) {
            int len = query.value(3).toInt();
            int precision = query.value(4).toInt();
            // swap length and precision if length == -1
            if (len == -1 && precision > -1) {
                len = precision - 4;
                precision = -1;
            }
            QString defVal = query.value(5).toString();
            if (!defVal.isEmpty() && defVal.at(0) == QLatin1Char('\''))
                defVal = defVal.mid(1, defVal.length() - 2);
            QSqlField f(query.value(0).toString(), qDecodePSQLType(query.value(1).toInt()));
            f.setRequired(query.value(2).toBool());
            f.setLength(len);
            f.setPrecision(precision);
            f.setDefaultValue(defVal);
            f.setSqlType(query.value(1).toInt());
            info.append(f);
        }
    } else {
        // Postgres < 7.1 cannot handle outer joins
        while (query.next()) {
            QString defVal;
            QString stmt2 = QLatin1String("select pg_attrdef.adsrc from pg_attrdef where "
                            "pg_attrdef.adrelid = %1 and pg_attrdef.adnum = %2 ");
            QSqlQuery query2(createResult());
            query2.exec(stmt2.arg(query.value(5).toInt()).arg(query.value(6).toInt()));
            if (query2.isActive() && query2.next())
                defVal = query2.value(0).toString();
            if (!defVal.isEmpty() && defVal.at(0) == QLatin1Char('\''))
                defVal = defVal.mid(1, defVal.length() - 2);
            int len = query.value(3).toInt();
            int precision = query.value(4).toInt();
            // swap length and precision if length == -1
            if (len == -1 && precision > -1) {
                len = precision - 4;
                precision = -1;
            }
            QSqlField f(query.value(0).toString(), qDecodePSQLType(query.value(1).toInt()));
            f.setRequired(query.value(2).toBool());
            f.setLength(len);
            f.setPrecision(precision);
            f.setDefaultValue(defVal);
            f.setSqlType(query.value(1).toInt());
            info.append(f);
        }
    }

    return info;
}

QString QPSQLDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    Q_D(const QPSQLDriver);
    QString r;
    if (field.isNull()) {
        r = QLatin1String("NULL");
    } else {
        switch (field.type()) {
        case QVariant::DateTime:
#ifndef QT_NO_DATESTRING
            if (field.value().toDateTime().isValid()) {
                QDate dt = field.value().toDateTime().date();
                QTime tm = field.value().toDateTime().time();
                // msecs need to be right aligned otherwise psql interprets them wrong
                r = QLatin1Char('\'') + QString::number(dt.year()) + QLatin1Char('-')
                          + QString::number(dt.month()).rightJustified(2, QLatin1Char('0')) + QLatin1Char('-')
                          + QString::number(dt.day()).rightJustified(2, QLatin1Char('0')) + QLatin1Char(' ')
                          + tm.toString() + QLatin1Char('.')
                          + QString::number(tm.msec()).rightJustified(3, QLatin1Char('0'))
                          + QLatin1Char('\'');
            } else {
                r = QLatin1String("NULL");
            }
#else
            r = QLatin1String("NULL");
#endif // QT_NO_DATESTRING
            break;
        case QVariant::Time:
#ifndef QT_NO_DATESTRING
            if (field.value().toTime().isValid()) {
                r = QLatin1Char('\'') + field.value().toTime().toString(QLatin1String("hh:mm:ss.zzz")) + QLatin1Char('\'');
            } else
#endif
            {
                r = QLatin1String("NULL");
            }
            break;
        case QVariant::String:
            r = QSqlDriver::formatValue(field, trimStrings);
            if (d->hasBackslashEscape)
                r.replace(QLatin1String("\\"), QLatin1String("\\\\"));
            break;
        case QVariant::Bool:
            if (field.value().toBool())
                r = QLatin1String("TRUE");
            else
                r = QLatin1String("FALSE");
            break;
        case QVariant::ByteArray: {
            QByteArray ba(field.value().toByteArray());
            size_t len;
#if defined PG_VERSION_NUM && PG_VERSION_NUM-0 >= 80200
            unsigned char *data = PQescapeByteaConn(d->connection, (unsigned char*)ba.constData(), ba.size(), &len);
#else
            unsigned char *data = PQescapeBytea((unsigned char*)ba.constData(), ba.size(), &len);
#endif
            r += QLatin1Char('\'');
            r += QLatin1String((const char*)data);
            r += QLatin1Char('\'');
            qPQfreemem(data);
            break;
        }
        case QVariant::Double: {
            double val = field.value().toDouble();
            if (isnan(val))
                r = QLatin1String("'NaN'");
            else {
                int res = isinf(val);
                if (res == 1)
                    r = QLatin1String("'Infinity'");
                else if (res == -1)
                    r = QLatin1String("'-Infinity'");
                else
                    r = QSqlDriver::formatValue(field, trimStrings);
            }
            break;
        }
        default:
            r = QSqlDriver::formatValue(field, trimStrings);
            break;
        }
    }
    return r;
}

QString QPSQLDriver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    QString res = identifier;
    if(!identifier.isEmpty() && !identifier.startsWith(QLatin1Char('"')) && !identifier.endsWith(QLatin1Char('"')) ) {
        res.replace(QLatin1Char('"'), QLatin1String("\"\""));
        res.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
        res.replace(QLatin1Char('.'), QLatin1String("\".\""));
    }
    return res;
}

bool QPSQLDriver::isOpen() const
{
    Q_D(const QPSQLDriver);
    return PQstatus(d->connection) == CONNECTION_OK;
}

QPSQLDriver::Protocol QPSQLDriver::protocol() const
{
    Q_D(const QPSQLDriver);
    return d->pro;
}

bool QPSQLDriver::subscribeToNotification(const QString &name)
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qWarning("QPSQLDriver::subscribeToNotificationImplementation: database not open.");
        return false;
    }

    if (d->seid.contains(name)) {
        qWarning("QPSQLDriver::subscribeToNotificationImplementation: already subscribing to '%s'.",
            qPrintable(name));
        return false;
    }

    int socket = PQsocket(d->connection);
    if (socket) {
        // Add the name to the list of subscriptions here so that QSQLDriverPrivate::exec knows
        // to check for notifications immediately after executing the LISTEN
        d->seid << name;
        QString query = QLatin1String("LISTEN ") + escapeIdentifier(name, QSqlDriver::TableName);
        PGresult *result = d->exec(query);
        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            setLastError(qMakeError(tr("Unable to subscribe"), QSqlError::StatementError, d, result));
            return false;
        }

        if (!d->sn) {
            d->sn = new QSocketNotifier(socket, QSocketNotifier::Read);
            connect(d->sn, SIGNAL(activated(int)), this, SLOT(_q_handleNotification(int)));
        }
    } else {
        qWarning("QPSQLDriver::subscribeToNotificationImplementation: PQsocket didn't return a valid socket to listen on");
        return false;
    }

    return true;
}

bool QPSQLDriver::unsubscribeFromNotification(const QString &name)
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qWarning("QPSQLDriver::unsubscribeFromNotificationImplementation: database not open.");
        return false;
    }

    if (!d->seid.contains(name)) {
        qWarning("QPSQLDriver::unsubscribeFromNotificationImplementation: not subscribed to '%s'.",
            qPrintable(name));
        return false;
    }

    QString query = QLatin1String("UNLISTEN ") + escapeIdentifier(name, QSqlDriver::TableName);
    PGresult *result = d->exec(query);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(tr("Unable to unsubscribe"), QSqlError::StatementError, d, result));
        return false;
    }

    d->seid.removeAll(name);

    if (d->seid.isEmpty()) {
        disconnect(d->sn, SIGNAL(activated(int)), this, SLOT(_q_handleNotification(int)));
        delete d->sn;
        d->sn = 0;
    }

    return true;
}

QStringList QPSQLDriver::subscribedToNotifications() const
{
    Q_D(const QPSQLDriver);
    return d->seid;
}

void QPSQLDriver::_q_handleNotification(int)
{
    Q_D(QPSQLDriver);
    d->pendingNotifyCheck = false;
    PQconsumeInput(d->connection);

    PGnotify *notify = 0;
    while((notify = PQnotifies(d->connection)) != 0) {
        QString name(QLatin1String(notify->relname));
        if (d->seid.contains(name)) {
            QString payload;
#if defined PG_VERSION_NUM && PG_VERSION_NUM-0 >= 70400
            if (notify->extra)
                payload = d->isUtf8 ? QString::fromUtf8(notify->extra) : QString::fromLatin1(notify->extra);
#endif
            emit notification(name);
            QSqlDriver::NotificationSource source = (notify->be_pid == PQbackendPID(d->connection)) ? QSqlDriver::SelfSource : QSqlDriver::OtherSource;
            emit notification(name, source, payload);
        }
        else
            qWarning("QPSQLDriver: received notification for '%s' which isn't subscribed to.",
                    qPrintable(name));

        qPQfreemem(notify);
    }
}

QT_END_NAMESPACE
