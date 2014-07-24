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

#include "qsql_tds_p.h"

#include <qglobal.h>
#ifdef Q_OS_WIN32    // We assume that MS SQL Server is used. Set Q_USE_SYBASE to force Sybase.
// Conflicting declarations of LPCBYTE in sqlfront.h and winscard.h
#define _WINSCARD_H_
#include <windows.h>
#else
#define Q_USE_SYBASE
#endif

#include <qvariant.h>
#include <qdatetime.h>
#include <qhash.h>
#include <qregexp.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlquery.h>
#include <QtSql/private/qsqlcachedresult_p.h>
#include <QtSql/private/qsqldriver_p.h>
#include <qstringlist.h>
#include <qvector.h>

#include <stdlib.h>

Q_DECLARE_OPAQUE_POINTER(LOGINREC*)
Q_DECLARE_OPAQUE_POINTER(DBPROCESS*)

QT_BEGIN_NAMESPACE

#ifdef DBNTWIN32
#define QMSGHANDLE DBMSGHANDLE_PROC
#define QERRHANDLE DBERRHANDLE_PROC
#define QTDSCHAR SQLCHAR
#define QTDSDATETIME4 SQLDATETIM4
#define QTDSDATETIME SQLDATETIME
#define QTDSDATETIME_N SQLDATETIMN
#define QTDSDECIMAL SQLDECIMAL
#define QTDSFLT4 SQLFLT4
#define QTDSFLT8 SQLFLT8
#define QTDSFLT8_N SQLFLTN
#define QTDSINT1 SQLINT1
#define QTDSINT2 SQLINT2
#define QTDSINT4 SQLINT4
#define QTDSINT4_N SQLINTN
#define QTDSMONEY4 SQLMONEY4
#define QTDSMONEY SQLMONEY
#define QTDSMONEY_N SQLMONEYN
#define QTDSNUMERIC SQLNUMERIC
#define QTDSTEXT SQLTEXT
#define QTDSVARCHAR SQLVARCHAR
#define QTDSBIT SQLBIT
#define QTDSBINARY SQLBINARY
#define QTDSVARBINARY SQLVARBINARY
#define QTDSIMAGE SQLIMAGE
#else
#define QMSGHANDLE MHANDLEFUNC
#define QERRHANDLE EHANDLEFUNC
#define QTDSCHAR SYBCHAR
#define QTDSDATETIME4 SYBDATETIME4
#define QTDSDATETIME SYBDATETIME
#define QTDSDATETIME_N SYBDATETIMN
#define QTDSDECIMAL SYBDECIMAL
#define QTDSFLT8 SYBFLT8
#define QTDSFLT8_N SYBFLTN
#define QTDSFLT4 SYBREAL
#define QTDSINT1 SYBINT1
#define QTDSINT2 SYBINT2
#define QTDSINT4 SYBINT4
#define QTDSINT4_N SYBINTN
#define QTDSMONEY4 SYBMONEY4
#define QTDSMONEY SYBMONEY
#define QTDSMONEY_N SYBMONEYN
#define QTDSNUMERIC SYBNUMERIC
#define QTDSTEXT SYBTEXT
#define QTDSVARCHAR SYBVARCHAR
#define QTDSBIT SYBBIT
#define QTDSBINARY SYBBINARY
#define QTDSVARBINARY SYBVARBINARY
#define QTDSIMAGE SYBIMAGE
// magic numbers not defined anywhere in Sybase headers
#define QTDSDECIMAL_2 55
#define QTDSNUMERIC_2 63
#endif  //DBNTWIN32

#define TDS_CURSOR_SIZE 50

// workaround for FreeTDS
#ifndef CS_PUBLIC
#define CS_PUBLIC
#endif

QSqlError qMakeError(const QString& err, QSqlError::ErrorType type, int errNo = -1)
{
    return QSqlError(QLatin1String("QTDS: ") + err, QString(), type, errNo);
}

class QTDSDriverPrivate : public QSqlDriverPrivate
{
public:
    QTDSDriverPrivate() : QSqlDriverPrivate(), login(0), initialized(false) { dbmsType = Sybase; }
    LOGINREC* login;  // login information
    QString hostName;
    QString db;
    bool initialized;
};

struct QTDSColumnData
{
    void *data;
    DBINT nullbind;
};
Q_DECLARE_TYPEINFO(QTDSColumnData, Q_MOVABLE_TYPE);

class QTDSResultPrivate;

class QTDSResult : public QSqlCachedResult
{
public:
    explicit QTDSResult(const QTDSDriver* db);
    ~QTDSResult();
    QVariant handle() const;

protected:
    void cleanup();
    bool reset (const QString& query);
    int size();
    int numRowsAffected();
    bool gotoNext(QSqlCachedResult::ValueCache &values, int index);
    QSqlRecord record() const;

private:
    QTDSResultPrivate* d;
};

class QTDSResultPrivate
{
public:
    QTDSResultPrivate():login(0), dbproc(0) {}
    LOGINREC* login;  // login information
    DBPROCESS* dbproc; // connection from app to server
    QSqlError lastError;
    void addErrorMsg(QString& errMsg) { errorMsgs.append(errMsg); }
    QString getErrorMsgs() { return errorMsgs.join(QLatin1String("\n")); }
    void clearErrorMsgs() { errorMsgs.clear(); }
    QVector<QTDSColumnData> buffer;
    QSqlRecord rec;

private:
    QStringList errorMsgs;
};

typedef QHash<DBPROCESS *, QTDSResultPrivate *> QTDSErrorHash;
Q_GLOBAL_STATIC(QTDSErrorHash, errs)

extern "C" {
static int CS_PUBLIC qTdsMsgHandler (DBPROCESS* dbproc,
                            DBINT msgno,
                            int msgstate,
                            int severity,
                            char* msgtext,
                            char* srvname,
                            char* /*procname*/,
                            int line)
{
    QTDSResultPrivate* p = errs()->value(dbproc);

    if (!p) {
//        ### umm... temporary disabled since this throws a lot of warnings...
//        qWarning("QTDSDriver warning (%d): [%s] from server [%s]", msgstate, msgtext, srvname);
        return INT_CANCEL;
    }

    if (severity > 0) {
        QString errMsg = QString::fromLatin1("%1 (Msg %2, Level %3, State %4, Server %5, Line %6)")
                         .arg(QString::fromLatin1(msgtext))
                         .arg(msgno)
                         .arg(severity)
                         .arg(msgstate)
                         .arg(QString::fromLatin1(srvname))
                         .arg(line);
        p->addErrorMsg(errMsg);
        if (severity > 10) {
            // Severe messages are really errors in the sense of lastError
            errMsg = p->getErrorMsgs();
            p->lastError = qMakeError(errMsg, QSqlError::UnknownError, msgno);
            p->clearErrorMsgs();
        }
    }

    return INT_CANCEL;
}

static int CS_PUBLIC qTdsErrHandler(DBPROCESS* dbproc,
                                int /*severity*/,
                                int dberr,
                                int /*oserr*/,
                                char* dberrstr,
                                char* oserrstr)
{
    QTDSResultPrivate* p = errs()->value(dbproc);
    if (!p) {
        qWarning("QTDSDriver error (%d): [%s] [%s]", dberr, dberrstr, oserrstr);
        return INT_CANCEL;
    }
    /*
     * If the process is dead or NULL and
     * we are not in the middle of logging in...
     */
    if((dbproc == NULL || DBDEAD(dbproc))) {
        qWarning("QTDSDriver error (%d): [%s] [%s]", dberr, dberrstr, oserrstr);
        return INT_CANCEL;
    }


    QString errMsg = QString::fromLatin1("%1 %2\n").arg(QLatin1String(dberrstr)).arg(
                                QLatin1String(oserrstr));
    errMsg += p->getErrorMsgs();
    p->lastError = qMakeError(errMsg, QSqlError::UnknownError, dberr);
    p->clearErrorMsgs();

    return INT_CANCEL ;
}

} //extern "C"


QVariant::Type qDecodeTDSType(int type)
{
    QVariant::Type t = QVariant::Invalid;
    switch (type) {
    case QTDSCHAR:
    case QTDSTEXT:
    case QTDSVARCHAR:
        t = QVariant::String;
        break;
    case QTDSINT1:
    case QTDSINT2:
    case QTDSINT4:
    case QTDSINT4_N:
    case QTDSBIT:
        t = QVariant::Int;
        break;
    case QTDSFLT4:
    case QTDSFLT8:
    case QTDSFLT8_N:
    case QTDSMONEY4:
    case QTDSMONEY:
    case QTDSDECIMAL:
    case QTDSNUMERIC:
#ifdef QTDSNUMERIC_2
    case QTDSNUMERIC_2:
#endif
#ifdef QTDSDECIMAL_2
    case QTDSDECIMAL_2:
#endif
    case QTDSMONEY_N:
        t = QVariant::Double;
        break;
    case QTDSDATETIME4:
    case QTDSDATETIME:
    case QTDSDATETIME_N:
        t = QVariant::DateTime;
        break;
    case QTDSBINARY:
    case QTDSVARBINARY:
    case QTDSIMAGE:
        t = QVariant::ByteArray;
        break;
    default:
        t = QVariant::Invalid;
        break;
    }
    return t;
}

QVariant::Type qFieldType(QTDSResultPrivate* d, int i)
{
    QVariant::Type type = qDecodeTDSType(dbcoltype(d->dbproc, i+1));
    return type;
}


QTDSResult::QTDSResult(const QTDSDriver* db)
    : QSqlCachedResult(db)
{
    d = new QTDSResultPrivate();
    d->login = db->d_func()->login;

    d->dbproc = dbopen(d->login, const_cast<char*>(db->d_func()->hostName.toLatin1().constData()));
    if (!d->dbproc)
        return;
    if (dbuse(d->dbproc, const_cast<char*>(db->d_func()->db.toLatin1().constData())) == FAIL)
        return;

    // insert d in error handler dict
    errs()->insert(d->dbproc, d);
    dbcmd(d->dbproc, "set quoted_identifier on");
    dbsqlexec(d->dbproc);
}

QTDSResult::~QTDSResult()
{
    cleanup();
    if (d->dbproc)
        dbclose(d->dbproc);
    errs()->remove(d->dbproc);
    delete d;
}

void QTDSResult::cleanup()
{
    d->clearErrorMsgs();
    d->rec.clear();
    for (int i = 0; i < d->buffer.size(); ++i)
        free(d->buffer.at(i).data);
    d->buffer.clear();
    // "can" stands for "cancel"... very clever.
    dbcanquery(d->dbproc);
    dbfreebuf(d->dbproc);

    QSqlCachedResult::cleanup();
}

QVariant QTDSResult::handle() const
{
    return QVariant(qRegisterMetaType<DBPROCESS *>("DBPROCESS*"), &d->dbproc);
}

static inline bool qIsNull(const QTDSColumnData &p)
{
    return p.nullbind == -1;
}

bool QTDSResult::gotoNext(QSqlCachedResult::ValueCache &values, int index)
{
    STATUS stat = dbnextrow(d->dbproc);
    if (stat == NO_MORE_ROWS) {
        setAt(QSql::AfterLastRow);
        return false;
    }
    if ((stat == FAIL) || (stat == BUF_FULL)) {
        setLastError(d->lastError);
        return false;
    }

    if (index < 0)
        return true;

    for (int i = 0; i < d->rec.count(); ++i) {
        int idx = index + i;
        switch (d->rec.field(i).type()) {
            case QVariant::DateTime:
                if (qIsNull(d->buffer.at(i))) {
                    values[idx] = QVariant(QVariant::DateTime);
                } else {
                    DBDATETIME *bdt = (DBDATETIME*) d->buffer.at(i).data;
                    QDate date = QDate::fromString(QLatin1String("1900-01-01"), Qt::ISODate);
                    QTime time = QTime::fromString(QLatin1String("00:00:00"), Qt::ISODate);
                    values[idx] = QDateTime(date.addDays(bdt->dtdays), time.addMSecs(int(bdt->dttime / 0.3)));
                }
                break;
            case QVariant::Int:
                if (qIsNull(d->buffer.at(i)))
                    values[idx] = QVariant(QVariant::Int);
                else
                    values[idx] = *((int*)d->buffer.at(i).data);
                break;
            case QVariant::Double:
            case QVariant::String:
                if (qIsNull(d->buffer.at(i)))
                    values[idx] = QVariant(QVariant::String);
                else
                    values[idx] = QString::fromLocal8Bit((const char*)d->buffer.at(i).data).trimmed();
                break;
            case QVariant::ByteArray: {
                if (qIsNull(d->buffer.at(i)))
                    values[idx] = QVariant(QVariant::ByteArray);
                else
                    values[idx] = QByteArray((const char*)d->buffer.at(i).data);
                break;
            }
            default:
                // should never happen, and we already fired
                // a warning while binding.
                values[idx] = QVariant();
                break;
        }
    }

    return true;
}

bool QTDSResult::reset (const QString& query)
{
    cleanup();
    if (!driver() || !driver()-> isOpen() || driver()->isOpenError())
        return false;
    setActive(false);
    setAt(QSql::BeforeFirstRow);
    if (dbcmd(d->dbproc, const_cast<char*>(query.toLocal8Bit().constData())) == FAIL) {
        setLastError(d->lastError);
        return false;
    }

    if (dbsqlexec(d->dbproc) == FAIL) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }
    if (dbresults(d->dbproc) != SUCCEED) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }

    setSelect((DBCMDROW(d->dbproc) == SUCCEED)); // decide whether or not we are dealing with a SELECT query
    int numCols = dbnumcols(d->dbproc);
    if (numCols > 0) {
        d->buffer.resize(numCols);
        init(numCols);
    }
    for (int i = 0; i < numCols; ++i) {
        int dbType = dbcoltype(d->dbproc, i+1);
        QVariant::Type vType = qDecodeTDSType(dbType);
        QSqlField f(QString::fromLatin1(dbcolname(d->dbproc, i+1)), vType);
        f.setSqlType(dbType);
        f.setLength(dbcollen(d->dbproc, i+1));
        d->rec.append(f);

        RETCODE ret = -1;
        void* p = 0;
        switch (vType) {
        case QVariant::Int:
            p = malloc(4);
            ret = dbbind(d->dbproc, i+1, INTBIND, (DBINT) 4, (unsigned char *)p);
            break;
        case QVariant::Double:
            // use string binding to prevent loss of precision
            p = malloc(50);
            ret = dbbind(d->dbproc, i+1, STRINGBIND, 50, (unsigned char *)p);
            break;
        case QVariant::String:
            p = malloc(dbcollen(d->dbproc, i+1) + 1);
            ret = dbbind(d->dbproc, i+1, STRINGBIND, DBINT(dbcollen(d->dbproc, i+1) + 1), (unsigned char *)p);
            break;
        case QVariant::DateTime:
            p = malloc(8);
            ret = dbbind(d->dbproc, i+1, DATETIMEBIND, (DBINT) 8, (unsigned char *)p);
            break;
        case QVariant::ByteArray:
            p = malloc(dbcollen(d->dbproc, i+1) + 1);
            ret = dbbind(d->dbproc, i+1, BINARYBIND, DBINT(dbcollen(d->dbproc, i+1) + 1), (unsigned char *)p);
            break;
        default: //don't bind the field since we do not support it
            qWarning("QTDSResult::reset: Unsupported type for field \"%s\"", dbcolname(d->dbproc, i+1));
            break;
        }
        if (ret == SUCCEED) {
            d->buffer[i].data = p;
            ret = dbnullbind(d->dbproc, i+1, &d->buffer[i].nullbind);
        } else {
            d->buffer[i].data = 0;
            d->buffer[i].nullbind = 0;
            free(p);
        }
        if ((ret != SUCCEED) && (ret != -1)) {
            setLastError(d->lastError);
            return false;
        }
    }

    setActive(true);
    return true;
}

int QTDSResult::size()
{
    return -1;
}

int QTDSResult::numRowsAffected()
{
#ifdef DBNTWIN32
    if (dbiscount(d->dbproc)) {
        return DBCOUNT(d->dbproc);
    }
    return -1;
#else
    return DBCOUNT(d->dbproc);
#endif
}

QSqlRecord QTDSResult::record() const
{
    return d->rec;
}

///////////////////////////////////////////////////////////////////

QTDSDriver::QTDSDriver(QObject* parent)
    : QSqlDriver(*new QTDSDriverPrivate, parent)
{
    init();
}

QTDSDriver::QTDSDriver(LOGINREC* rec, const QString& host, const QString &db, QObject* parent)
    : QSqlDriver(*new QTDSDriverPrivate, parent)
{
    Q_D(QTDSDriver);
    init();
    d->login = rec;
    d->hostName = host;
    d->db = db;
    if (rec) {
        setOpen(true);
        setOpenError(false);
    }
}

QVariant QTDSDriver::handle() const
{
    Q_D(const QTDSDriver);
    return QVariant(qRegisterMetaType<LOGINREC *>("LOGINREC*"), &d->login);
}

void QTDSDriver::init()
{
    Q_D(QTDSDriver);
    d->initialized = (dbinit() == SUCCEED);
    // the following two code-lines will fail compilation on some FreeTDS versions
    // just comment them out if you have FreeTDS (you won't get any errors and warnings then)
    dberrhandle((QERRHANDLE)qTdsErrHandler);
    dbmsghandle((QMSGHANDLE)qTdsMsgHandler);
}

QTDSDriver::~QTDSDriver()
{
    dberrhandle(0);
    dbmsghandle(0);
    // dbexit also calls dbclose if necessary
    dbexit();
}

bool QTDSDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case Transactions:
    case QuerySize:
    case Unicode:
    case SimpleLocking:
    case EventNotifications:
    case MultipleResultSets:
        return false;
    case BLOB:
        return true;
    default:
        return false;
    }
}

bool QTDSDriver::open(const QString & db,
                       const QString & user,
                       const QString & password,
                       const QString & host,
                       int /*port*/,
                       const QString& /*connOpts*/)
{
    Q_D(QTDSDriver);
    if (isOpen())
        close();
    if (!d->initialized) {
        setOpenError(true);
        return false;
    }
    d->login = dblogin();
    if (!d->login) {
        setOpenError(true);
        return false;
    }
    DBSETLPWD(d->login, const_cast<char*>(password.toLocal8Bit().constData()));
    DBSETLUSER(d->login, const_cast<char*>(user.toLocal8Bit().constData()));

    // Now, try to open and use the database. If this fails, return false.
    DBPROCESS* dbproc;

    dbproc = dbopen(d->login, const_cast<char*>(host.toLatin1().constData()));
    if (!dbproc) {
        setLastError(qMakeError(tr("Unable to open connection"), QSqlError::ConnectionError, -1));
        setOpenError(true);
        return false;
    }
    if (dbuse(dbproc, const_cast<char*>(db.toLatin1().constData())) == FAIL) {
        setLastError(qMakeError(tr("Unable to use database"), QSqlError::ConnectionError, -1));
        setOpenError(true);
        return false;
    }
    dbclose( dbproc );

    setOpen(true);
    setOpenError(false);
    d->hostName = host;
    d->db = db;
    return true;
}

void QTDSDriver::close()
{
    Q_D(QTDSDriver);
    if (isOpen()) {
#ifdef Q_USE_SYBASE
        dbloginfree(d->login);
#else
        dbfreelogin(d->login);
#endif
        d->login = 0;
        setOpen(false);
        setOpenError(false);
    }
}

QSqlResult *QTDSDriver::createResult() const
{
    return new QTDSResult(this);
}

bool QTDSDriver::beginTransaction()
{
    return false;
/*
    if (!isOpen()) {
        qWarning("QTDSDriver::beginTransaction: Database not open");
        return false;
    }
    if (dbcmd(d->dbproc, "BEGIN TRANSACTION") == FAIL) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }
    if (dbsqlexec(d->dbproc) == FAIL) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }
    while(dbresults(d->dbproc) == NO_MORE_RESULTS) {}
    dbfreebuf(d->dbproc);
    inTransaction = true;
    return true;
*/
}

bool QTDSDriver::commitTransaction()
{
    return false;
/*
    if (!isOpen()) {
        qWarning("QTDSDriver::commitTransaction: Database not open");
        return false;
    }
    if (dbcmd(d->dbproc, "COMMIT TRANSACTION") == FAIL) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }
    if (dbsqlexec(d->dbproc) == FAIL) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }
    while(dbresults(d->dbproc) == NO_MORE_RESULTS) {}
    dbfreebuf(d->dbproc);
    inTransaction = false;
    return true;
*/
}

bool QTDSDriver::rollbackTransaction()
{
    return false;
/*
    if (!isOpen()) {
        qWarning("QTDSDriver::rollbackTransaction: Database not open");
        return false;
    }
    if (dbcmd(d->dbproc, "ROLLBACK TRANSACTION") == FAIL) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }
    if (dbsqlexec(d->dbproc) == FAIL) {
        setLastError(d->lastError);
        dbfreebuf(d->dbproc);
        return false;
    }
    while(dbresults(d->dbproc) == NO_MORE_RESULTS) {}
    dbfreebuf(d->dbproc);
    inTransaction = false;
    return true;
*/
}

QSqlRecord QTDSDriver::record(const QString& tablename) const
{
    QSqlRecord info;
    if (!isOpen())
        return info;
    QSqlQuery t(createResult());
    t.setForwardOnly(true);

    QString table = tablename;
    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);

    QString stmt (QLatin1String("select name, type, length, prec from syscolumns "
                   "where id = (select id from sysobjects where name = '%1')"));
    t.exec(stmt.arg(table));
    while (t.next()) {
        QSqlField f(t.value(0).toString().simplified(), qDecodeTDSType(t.value(1).toInt()));
        f.setLength(t.value(2).toInt());
        f.setPrecision(t.value(3).toInt());
        f.setSqlType(t.value(1).toInt());
        info.append(f);
    }
    return info;
}

QStringList QTDSDriver::tables(QSql::TableType type) const
{
    QStringList list;

    if (!isOpen())
        return list;

    QStringList typeFilter;

    if (type & QSql::Tables)
        typeFilter += QLatin1String("type='U'");
    if (type & QSql::SystemTables)
        typeFilter += QLatin1String("type='S'");
    if (type & QSql::Views)
        typeFilter += QLatin1String("type='V'");

    if (typeFilter.isEmpty())
        return list;

    QSqlQuery t(createResult());
    t.setForwardOnly(true);
    t.exec(QLatin1String("select name from sysobjects where ") + typeFilter.join(QLatin1String(" or ")));
    while (t.next())
        list.append(t.value(0).toString().simplified());

    return list;
}

QString QTDSDriver::formatValue(const QSqlField &field,
                                  bool trim) const
{
    QString r;
    if (field.isNull())
        r = QLatin1String("NULL");
    else if (field.type() == QVariant::DateTime) {
        if (field.value().toDateTime().isValid()){
            r = field.value().toDateTime().toString(QLatin1String("yyyyMMdd hh:mm:ss"));
            r.prepend(QLatin1String("'"));
            r.append(QLatin1String("'"));
        } else
            r = QLatin1String("NULL");
    } else if (field.type() == QVariant::ByteArray) {
        QByteArray ba = field.value().toByteArray();
        QString res;
        static const char hexchars[] = "0123456789abcdef";
        for (int i = 0; i < ba.size(); ++i) {
            uchar s = (uchar) ba[i];
            res += QLatin1Char(hexchars[s >> 4]);
            res += QLatin1Char(hexchars[s & 0x0f]);
        }
        r = QLatin1String("0x") + res;
    } else {
        r = QSqlDriver::formatValue(field, trim);
    }
    return r;
}

QSqlIndex QTDSDriver::primaryIndex(const QString& tablename) const
{
    QSqlRecord rec = record(tablename);

    QString table = tablename;
    if (isIdentifierEscaped(table, QSqlDriver::TableName))
        table = stripDelimiters(table, QSqlDriver::TableName);

    QSqlIndex idx(table);
    if ((!isOpen()) || (table.isEmpty()))
        return QSqlIndex();

    QSqlQuery t(createResult());
    t.setForwardOnly(true);
    t.exec(QString::fromLatin1("sp_helpindex '%1'").arg(table));
    if (t.next()) {
        QStringList fNames = t.value(2).toString().simplified().split(QLatin1Char(','));
        QRegExp regx(QLatin1String("\\s*(\\S+)(?:\\s+(DESC|desc))?\\s*"));
        for(QStringList::Iterator it = fNames.begin(); it != fNames.end(); ++it) {
            regx.indexIn(*it);
            QSqlField f(regx.cap(1), rec.field(regx.cap(1)).type());
            if (regx.cap(2).toLower() == QLatin1String("desc")) {
                idx.append(f, true);
            } else {
                idx.append(f, false);
            }
        }
        idx.setName(t.value(0).toString().simplified());
    }
    return idx;
}

QString QTDSDriver::escapeIdentifier(const QString &identifier, IdentifierType type) const
{
    Q_UNUSED(type)
    QString res = identifier;
    if(!identifier.isEmpty() && !identifier.startsWith(QLatin1Char('"')) && !identifier.endsWith(QLatin1Char('"')) ) {
        res.replace(QLatin1Char('"'), QLatin1String("\"\""));
        res.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
        res.replace(QLatin1Char('.'), QLatin1String("\".\""));
    }
    return res;
}

QT_END_NAMESPACE
