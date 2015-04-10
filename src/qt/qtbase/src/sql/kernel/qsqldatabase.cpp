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

#include "qsqldatabase.h"
#include "qsqlquery.h"

#ifdef Q_OS_WIN32
// Conflicting declarations of LPCBYTE in sqlfront.h and winscard.h
#define _WINSCARD_H_
#endif

#ifdef QT_SQL_PSQL
#include "../drivers/psql/qsql_psql_p.h"
#endif
#ifdef QT_SQL_MYSQL
#include "../drivers/mysql/qsql_mysql_p.h"
#endif
#ifdef QT_SQL_ODBC
#include "../drivers/odbc/qsql_odbc_p.h"
#endif
#ifdef QT_SQL_OCI
#include "../drivers/oci/qsql_oci_p.h"
#endif
#ifdef QT_SQL_TDS
// conflicting RETCODE typedef between odbc and freetds
#define RETCODE DBRETCODE
#include "../drivers/tds/qsql_tds_p.h"
#undef RETCODE
#endif
#ifdef QT_SQL_DB2
#include "../drivers/db2/qsql_db2_p.h"
#endif
#ifdef QT_SQL_SQLITE
#include "../drivers/sqlite/qsql_sqlite_p.h"
#endif
#ifdef QT_SQL_SQLITE2
#include "../drivers/sqlite2/qsql_sqlite2_p.h"
#endif
#ifdef QT_SQL_IBASE
#undef SQL_FLOAT  // avoid clash with ODBC
#undef SQL_DOUBLE
#undef SQL_TIMESTAMP
#undef SQL_TYPE_TIME
#undef SQL_TYPE_DATE
#undef SQL_DATE
#define SCHAR IBASE_SCHAR  // avoid clash with ODBC (older versions of ibase.h with Firebird)
#include "../drivers/ibase/qsql_ibase_p.h"
#undef SCHAR
#endif

#include "qdebug.h"
#include "qcoreapplication.h"
#include "qreadwritelock.h"
#include "qsqlresult.h"
#include "qsqldriver.h"
#include "qsqldriverplugin.h"
#include "qsqlindex.h"
#include "private/qfactoryloader_p.h"
#include "private/qsqlnulldriver_p.h"
#include "qmutex.h"
#include "qhash.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QSqlDriverFactoryInterface_iid,
                           QLatin1String("/sqldrivers")))
#endif

QT_STATIC_CONST_IMPL char *QSqlDatabase::defaultConnection = "qt_sql_default_connection";

typedef QHash<QString, QSqlDriverCreatorBase*> DriverDict;

class QConnectionDict: public QHash<QString, QSqlDatabase>
{
public:
    inline bool contains_ts(const QString &key)
    {
        QReadLocker locker(&lock);
        return contains(key);
    }
    inline QStringList keys_ts() const
    {
        QReadLocker locker(&lock);
        return keys();
    }

    mutable QReadWriteLock lock;
};
Q_GLOBAL_STATIC(QConnectionDict, dbDict)

class QSqlDatabasePrivate
{
public:
    QSqlDatabasePrivate(QSqlDatabase *d, QSqlDriver *dr = 0):
        ref(1),
        q(d),
        driver(dr),
        port(-1)
    {
        precisionPolicy = QSql::LowPrecisionDouble;
    }
    QSqlDatabasePrivate(const QSqlDatabasePrivate &other);
    ~QSqlDatabasePrivate();
    void init(const QString& type);
    void copy(const QSqlDatabasePrivate *other);
    void disable();

    QAtomicInt ref;
    QSqlDatabase *q;
    QSqlDriver* driver;
    QString dbname;
    QString uname;
    QString pword;
    QString hname;
    QString drvName;
    int port;
    QString connOptions;
    QString connName;
    QSql::NumericalPrecisionPolicy precisionPolicy;

    static QSqlDatabasePrivate *shared_null();
    static QSqlDatabase database(const QString& name, bool open);
    static void addDatabase(const QSqlDatabase &db, const QString & name);
    static void removeDatabase(const QString& name);
    static void invalidateDb(const QSqlDatabase &db, const QString &name, bool doWarn = true);
    static DriverDict &driverDict();
    static void cleanConnections();
};

QSqlDatabasePrivate::QSqlDatabasePrivate(const QSqlDatabasePrivate &other) : ref(1)
{
    q = other.q;
    dbname = other.dbname;
    uname = other.uname;
    pword = other.pword;
    hname = other.hname;
    drvName = other.drvName;
    port = other.port;
    connOptions = other.connOptions;
    driver = other.driver;
    precisionPolicy = other.precisionPolicy;
}

QSqlDatabasePrivate::~QSqlDatabasePrivate()
{
    if (driver != shared_null()->driver)
        delete driver;
}

void QSqlDatabasePrivate::cleanConnections()
{
    QConnectionDict *dict = dbDict();
    Q_ASSERT(dict);
    QWriteLocker locker(&dict->lock);

    QConnectionDict::iterator it = dict->begin();
    while (it != dict->end()) {
        invalidateDb(it.value(), it.key(), false);
        ++it;
    }
    dict->clear();
}

static bool qDriverDictInit = false;
static void cleanDriverDict()
{
    qDeleteAll(QSqlDatabasePrivate::driverDict());
    QSqlDatabasePrivate::driverDict().clear();
    QSqlDatabasePrivate::cleanConnections();
    qDriverDictInit = false;
}

DriverDict &QSqlDatabasePrivate::driverDict()
{
    static DriverDict dict;
    if (!qDriverDictInit) {
        qDriverDictInit = true;
        qAddPostRoutine(cleanDriverDict);
    }
    return dict;
}

QSqlDatabasePrivate *QSqlDatabasePrivate::shared_null()
{
    static QSqlNullDriver dr;
    static QSqlDatabasePrivate n(NULL, &dr);
    return &n;
}

void QSqlDatabasePrivate::invalidateDb(const QSqlDatabase &db, const QString &name, bool doWarn)
{
    if (db.d->ref.load() != 1 && doWarn) {
        qWarning("QSqlDatabasePrivate::removeDatabase: connection '%s' is still in use, "
                 "all queries will cease to work.", name.toLocal8Bit().constData());
        db.d->disable();
        db.d->connName.clear();
    }
}

void QSqlDatabasePrivate::removeDatabase(const QString &name)
{
    QConnectionDict *dict = dbDict();
    Q_ASSERT(dict);
    QWriteLocker locker(&dict->lock);

    if (!dict->contains(name))
        return;

    invalidateDb(dict->take(name), name);
}

void QSqlDatabasePrivate::addDatabase(const QSqlDatabase &db, const QString &name)
{
    QConnectionDict *dict = dbDict();
    Q_ASSERT(dict);
    QWriteLocker locker(&dict->lock);

    if (dict->contains(name)) {
        invalidateDb(dict->take(name), name);
        qWarning("QSqlDatabasePrivate::addDatabase: duplicate connection name '%s', old "
                 "connection removed.", name.toLocal8Bit().data());
    }
    dict->insert(name, db);
    db.d->connName = name;
}

/*! \internal
*/
QSqlDatabase QSqlDatabasePrivate::database(const QString& name, bool open)
{
    const QConnectionDict *dict = dbDict();
    Q_ASSERT(dict);

    dict->lock.lockForRead();
    QSqlDatabase db = dict->value(name);
    dict->lock.unlock();
    if (db.isValid() && !db.isOpen() && open) {
        if (!db.open())
            qWarning() << "QSqlDatabasePrivate::database: unable to open database:" << db.lastError().text();

    }
    return db;
}


/*! \internal
    Copies the connection data from \a other.
*/
void QSqlDatabasePrivate::copy(const QSqlDatabasePrivate *other)
{
    q = other->q;
    dbname = other->dbname;
    uname = other->uname;
    pword = other->pword;
    hname = other->hname;
    drvName = other->drvName;
    port = other->port;
    connOptions = other->connOptions;
    precisionPolicy = other->precisionPolicy;
}

void QSqlDatabasePrivate::disable()
{
    if (driver != shared_null()->driver) {
        delete driver;
        driver = shared_null()->driver;
    }
}

/*!
    \class QSqlDriverCreatorBase
    \brief The QSqlDriverCreatorBase class is the base class for
    SQL driver factories.

    \ingroup database
    \inmodule QtSql

    Reimplement createObject() to return an instance of the specific
    QSqlDriver subclass that you want to provide.

    See QSqlDatabase::registerSqlDriver() for details.

    \sa QSqlDriverCreator
*/

/*!
    \fn QSqlDriverCreatorBase::~QSqlDriverCreatorBase()

    Destroys the SQL driver creator object.
*/

/*!
    \fn QSqlDriver *QSqlDriverCreatorBase::createObject() const

    Reimplement this function to returns a new instance of a
    QSqlDriver subclass.
*/

/*!
    \class QSqlDriverCreator
    \brief The QSqlDriverCreator class is a template class that
    provides a SQL driver factory for a specific driver type.

    \ingroup database
    \inmodule QtSql

    QSqlDriverCreator<T> instantiates objects of type T, where T is a
    QSqlDriver subclass.

    See QSqlDatabase::registerSqlDriver() for details.
*/

/*!
    \fn QSqlDriver *QSqlDriverCreator::createObject() const
    \reimp
*/

/*!
    \class QSqlDatabase
    \brief The QSqlDatabase class represents a connection to
    a database.

    \ingroup database

    \inmodule QtSql

    The QSqlDatabase class provides an interface for accessing a
    database through a connection. An instance of QSqlDatabase
    represents the connection. The connection provides access to the
    database via one of the \l{SQL Database Drivers#Supported
    Databases} {supported database drivers}, which are derived from
    QSqlDriver.  Alternatively, you can subclass your own database
    driver from QSqlDriver. See \l{How to Write Your Own Database
    Driver} for more information.

    Create a connection (i.e., an instance of QSqlDatabase) by calling
    one of the static addDatabase() functions, where you specify
    \l{SQL Database Drivers#Supported Databases} {the driver or type
    of driver} to use (i.e., what kind of database will you access?)
    and a connection name. A connection is known by its own name,
    \e{not} by the name of the database it connects to. You can have
    multiple connections to one database. QSqlDatabase also supports
    the concept of a \e{default} connection, which is the unnamed
    connection. To create the default connection, don't pass the
    connection name argument when you call addDatabase().
    Subsequently, when you call any static member function that takes
    the connection name argument, if you don't pass the connection
    name argument, the default connection is assumed. The following
    snippet shows how to create and open a default connection to a
    PostgreSQL database:

    \snippet sqldatabase/sqldatabase.cpp 0

    Once the QSqlDatabase object has been created, set the connection
    parameters with setDatabaseName(), setUserName(), setPassword(),
    setHostName(), setPort(), and setConnectOptions(). Then call
    open() to activate the physical connection to the database. The
    connection is not usable until you open it.

    The connection defined above will be the \e{default} connection,
    because we didn't give a connection name to \l{QSqlDatabase::}
    {addDatabase()}. Subsequently, you can get the default connection
    by calling database() without the connection name argument:

    \snippet sqldatabase/sqldatabase.cpp 1

    QSqlDatabase is a value class. Changes made to a database
    connection via one instance of QSqlDatabase will affect other
    instances of QSqlDatabase that represent the same connection. Use
    cloneDatabase() to create an independent database connection based
    on an existing one.

    If you create multiple database connections, specify a unique
    connection name for each one, when you call addDatabase(). Use
    database() with a connection name to get that connection. Use
    removeDatabase() with a connection name to remove a connection.
    QSqlDatabase outputs a warning if you try to remove a connection
    referenced by other QSqlDatabase objects. Use contains() to see if
    a given connection name is in the list of connections.

    Once a connection is established, you can call tables() to get the
    list of tables in the database, call primaryIndex() to get a
    table's primary index, and call record() to get meta-information
    about a table's fields (e.g., field names).

    \note QSqlDatabase::exec() is deprecated. Use QSqlQuery::exec()
    instead.

    If the driver supports transactions, use transaction() to start a
    transaction, and commit() or rollback() to complete it. Use
    \l{QSqlDriver::} {hasFeature()} to ask if the driver supports
    transactions. \note When using transactions, you must start the
    transaction before you create your query.

    If an error occurs, lastError() will return information about it.

    Get the names of the available SQL drivers with drivers().  Check
    for the presence of a particular driver with isDriverAvailable().
    If you have created your own custom driver, you must register it
    with registerSqlDriver().

    \sa QSqlDriver, QSqlQuery, {Qt SQL}, {Threads and the SQL Module}
*/

/*! \fn QSqlDatabase QSqlDatabase::addDatabase(const QString &type, const QString &connectionName)
    \threadsafe

    Adds a database to the list of database connections using the
    driver \a type and the connection name \a connectionName. If
    there already exists a database connection called \a
    connectionName, that connection is removed.

    The database connection is referred to by \a connectionName. The
    newly added database connection is returned.

    If \a type is not available or could not be loaded, isValid() returns \c false.

    If \a connectionName is not specified, the new connection becomes
    the default connection for the application, and subsequent calls
    to database() without the connection name argument will return the
    default connection. If a \a connectionName is provided here, use
    database(\a connectionName) to retrieve the connection.

    \warning If you add a connection with the same name as an existing
    connection, the new connection replaces the old one.  If you call
    this function more than once without specifying \a connectionName,
    the default connection will be the one replaced.

    Before using the connection, it must be initialized. e.g., call
    some or all of setDatabaseName(), setUserName(), setPassword(),
    setHostName(), setPort(), and setConnectOptions(), and, finally,
    open().

    \sa database(), removeDatabase(), {Threads and the SQL Module}
*/
QSqlDatabase QSqlDatabase::addDatabase(const QString &type, const QString &connectionName)
{
    QSqlDatabase db(type);
    QSqlDatabasePrivate::addDatabase(db, connectionName);
    return db;
}

/*!
    \threadsafe

    Returns the database connection called \a connectionName. The
    database connection must have been previously added with
    addDatabase(). If \a open is true (the default) and the database
    connection is not already open it is opened now. If no \a
    connectionName is specified the default connection is used. If \a
    connectionName does not exist in the list of databases, an invalid
    connection is returned.

    \sa isOpen(), {Threads and the SQL Module}
*/

QSqlDatabase QSqlDatabase::database(const QString& connectionName, bool open)
{
    return QSqlDatabasePrivate::database(connectionName, open);
}

/*!
    \threadsafe

    Removes the database connection \a connectionName from the list of
    database connections.

    \warning There should be no open queries on the database
    connection when this function is called, otherwise a resource leak
    will occur.

    Example:

    \snippet code/src_sql_kernel_qsqldatabase.cpp 0

    The correct way to do it:

    \snippet code/src_sql_kernel_qsqldatabase.cpp 1

    To remove the default connection, which may have been created with a
    call to addDatabase() not specifying a connection name, you can
    retrieve the default connection name by calling connectionName() on
    the database returned by database(). Note that if a default database
    hasn't been created an invalid database will be returned.

    \sa database(), connectionName(), {Threads and the SQL Module}
*/

void QSqlDatabase::removeDatabase(const QString& connectionName)
{
    QSqlDatabasePrivate::removeDatabase(connectionName);
}

/*!
    Returns a list of all the available database drivers.

    \sa registerSqlDriver()
*/

QStringList QSqlDatabase::drivers()
{
    QStringList list;

#ifdef QT_SQL_PSQL
    list << QLatin1String("QPSQL7");
    list << QLatin1String("QPSQL");
#endif
#ifdef QT_SQL_MYSQL
    list << QLatin1String("QMYSQL3");
    list << QLatin1String("QMYSQL");
#endif
#ifdef QT_SQL_ODBC
    list << QLatin1String("QODBC3");
    list << QLatin1String("QODBC");
#endif
#ifdef QT_SQL_OCI
    list << QLatin1String("QOCI8");
    list << QLatin1String("QOCI");
#endif
#ifdef QT_SQL_TDS
    list << QLatin1String("QTDS7");
    list << QLatin1String("QTDS");
#endif
#ifdef QT_SQL_DB2
    list << QLatin1String("QDB2");
#endif
#ifdef QT_SQL_SQLITE
    list << QLatin1String("QSQLITE");
#endif
#ifdef QT_SQL_SQLITE2
    list << QLatin1String("QSQLITE2");
#endif
#ifdef QT_SQL_IBASE
    list << QLatin1String("QIBASE");
#endif

#ifndef QT_NO_LIBRARY
    if (QFactoryLoader *fl = loader()) {
        typedef QMultiMap<int, QString> PluginKeyMap;
        typedef PluginKeyMap::const_iterator PluginKeyMapConstIterator;

        const PluginKeyMap keyMap = fl->keyMap();
        const PluginKeyMapConstIterator cend = keyMap.constEnd();
        for (PluginKeyMapConstIterator it = keyMap.constBegin(); it != cend; ++it)
            if (!list.contains(it.value()))
                list << it.value();
    }
#endif

    DriverDict dict = QSqlDatabasePrivate::driverDict();
    for (DriverDict::const_iterator i = dict.constBegin(); i != dict.constEnd(); ++i) {
        if (!list.contains(i.key()))
            list << i.key();
    }

    return list;
}

/*!
    This function registers a new SQL driver called \a name, within
    the SQL framework. This is useful if you have a custom SQL driver
    and don't want to compile it as a plugin.

    Example:
    \snippet code/src_sql_kernel_qsqldatabase.cpp 2

    QSqlDatabase takes ownership of the \a creator pointer, so you
    mustn't delete it yourself.

    \sa drivers()
*/
void QSqlDatabase::registerSqlDriver(const QString& name, QSqlDriverCreatorBase *creator)
{
    delete QSqlDatabasePrivate::driverDict().take(name);
    if (creator)
        QSqlDatabasePrivate::driverDict().insert(name, creator);
}

/*!
    \threadsafe

    Returns \c true if the list of database connections contains \a
    connectionName; otherwise returns \c false.

    \sa connectionNames(), database(), {Threads and the SQL Module}
*/

bool QSqlDatabase::contains(const QString& connectionName)
{
    return dbDict()->contains_ts(connectionName);
}

/*!
    \threadsafe

    Returns a list containing the names of all connections.

    \sa contains(), database(), {Threads and the SQL Module}
*/
QStringList QSqlDatabase::connectionNames()
{
    return dbDict()->keys_ts();
}

/*!
    \overload

    Creates a QSqlDatabase connection that uses the driver referred
    to by \a type. If the \a type is not recognized, the database
    connection will have no functionality.

    The currently available driver types are:

    \table
    \header \li Driver Type \li Description
    \row \li QDB2     \li IBM DB2
    \row \li QIBASE   \li Borland InterBase Driver
    \row \li QMYSQL   \li MySQL Driver
    \row \li QOCI     \li Oracle Call Interface Driver
    \row \li QODBC    \li ODBC Driver (includes Microsoft SQL Server)
    \row \li QPSQL    \li PostgreSQL Driver
    \row \li QSQLITE  \li SQLite version 3 or above
    \row \li QSQLITE2 \li SQLite version 2
    \row \li QTDS     \li Sybase Adaptive Server
    \endtable

    Additional third party drivers, including your own custom
    drivers, can be loaded dynamically.

    \sa {SQL Database Drivers}, registerSqlDriver(), drivers()
*/

QSqlDatabase::QSqlDatabase(const QString &type)
{
    d = new QSqlDatabasePrivate(this);
    d->init(type);
}

/*!
    \overload

    Creates a database connection using the given \a driver.
*/

QSqlDatabase::QSqlDatabase(QSqlDriver *driver)
{
    d = new QSqlDatabasePrivate(this, driver);
}

/*!
    Creates an empty, invalid QSqlDatabase object. Use addDatabase(),
    removeDatabase(), and database() to get valid QSqlDatabase
    objects.
*/
QSqlDatabase::QSqlDatabase()
{
    d = QSqlDatabasePrivate::shared_null();
    d->ref.ref();
}

/*!
    Creates a copy of \a other.
*/
QSqlDatabase::QSqlDatabase(const QSqlDatabase &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Assigns \a other to this object.
*/
QSqlDatabase &QSqlDatabase::operator=(const QSqlDatabase &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    \internal

    Create the actual driver instance \a type.
*/

void QSqlDatabasePrivate::init(const QString &type)
{
    drvName = type;

    if (!driver) {
#ifdef QT_SQL_PSQL
        if (type == QLatin1String("QPSQL") || type == QLatin1String("QPSQL7"))
            driver = new QPSQLDriver();
#endif
#ifdef QT_SQL_MYSQL
        if (type == QLatin1String("QMYSQL") || type == QLatin1String("QMYSQL3"))
            driver = new QMYSQLDriver();
#endif
#ifdef QT_SQL_ODBC
        if (type == QLatin1String("QODBC") || type == QLatin1String("QODBC3"))
            driver = new QODBCDriver();
#endif
#ifdef QT_SQL_OCI
        if (type == QLatin1String("QOCI") || type == QLatin1String("QOCI8"))
            driver = new QOCIDriver();
#endif
#ifdef QT_SQL_TDS
        if (type == QLatin1String("QTDS") || type == QLatin1String("QTDS7"))
            driver = new QTDSDriver();
#endif
#ifdef QT_SQL_DB2
        if (type == QLatin1String("QDB2"))
            driver = new QDB2Driver();
#endif
#ifdef QT_SQL_SQLITE
        if (type == QLatin1String("QSQLITE"))
            driver = new QSQLiteDriver();
#endif
#ifdef QT_SQL_SQLITE2
        if (type == QLatin1String("QSQLITE2"))
            driver = new QSQLite2Driver();
#endif
#ifdef QT_SQL_IBASE
        if (type == QLatin1String("QIBASE"))
            driver = new QIBaseDriver();
#endif
    }

    if (!driver) {
        DriverDict dict = QSqlDatabasePrivate::driverDict();
        for (DriverDict::const_iterator it = dict.constBegin();
             it != dict.constEnd() && !driver; ++it) {
            if (type == it.key()) {
                driver = ((QSqlDriverCreatorBase*)(*it))->createObject();
            }
        }
    }

#ifndef QT_NO_LIBRARY
    if (!driver && loader())
        driver = qLoadPlugin<QSqlDriver, QSqlDriverPlugin>(loader(), type);
#endif // QT_NO_LIBRARY

    if (!driver) {
        qWarning("QSqlDatabase: %s driver not loaded", type.toLatin1().data());
        qWarning("QSqlDatabase: available drivers: %s",
                        QSqlDatabase::drivers().join(QLatin1Char(' ')).toLatin1().data());
        if (QCoreApplication::instance() == 0)
            qWarning("QSqlDatabase: an instance of QCoreApplication is required for loading driver plugins");
        driver = shared_null()->driver;
    }
}

/*!
    Destroys the object and frees any allocated resources.

    \sa close()
*/

QSqlDatabase::~QSqlDatabase()
{
    if (!d->ref.deref()) {
        close();
        delete d;
    }
}

/*!
    Executes a SQL statement on the database and returns a QSqlQuery
    object. Use lastError() to retrieve error information. If \a
    query is empty, an empty, invalid query is returned and
    lastError() is not affected.

    \sa QSqlQuery, lastError()
*/

QSqlQuery QSqlDatabase::exec(const QString & query) const
{
    QSqlQuery r(d->driver->createResult());
    if (!query.isEmpty()) {
        r.exec(query);
        d->driver->setLastError(r.lastError());
    }
    return r;
}

/*!
    Opens the database connection using the current connection
    values. Returns \c true on success; otherwise returns \c false. Error
    information can be retrieved using lastError().

    \sa lastError(), setDatabaseName(), setUserName(), setPassword(),
        setHostName(), setPort(), setConnectOptions()
*/

bool QSqlDatabase::open()
{
    return d->driver->open(d->dbname, d->uname, d->pword, d->hname,
                            d->port, d->connOptions);
}

/*!
    \overload

    Opens the database connection using the given \a user name and \a
    password. Returns \c true on success; otherwise returns \c false. Error
    information can be retrieved using the lastError() function.

    This function does not store the password it is given. Instead,
    the password is passed directly to the driver for opening the
    connection and it is then discarded.

    \sa lastError()
*/

bool QSqlDatabase::open(const QString& user, const QString& password)
{
    setUserName(user);
    return d->driver->open(d->dbname, user, password, d->hname,
                            d->port, d->connOptions);
}

/*!
    Closes the database connection, freeing any resources acquired, and
    invalidating any existing QSqlQuery objects that are used with the
    database.

    This will also affect copies of this QSqlDatabase object.

    \sa removeDatabase()
*/

void QSqlDatabase::close()
{
    d->driver->close();
}

/*!
    Returns \c true if the database connection is currently open;
    otherwise returns \c false.
*/

bool QSqlDatabase::isOpen() const
{
    return d->driver->isOpen();
}

/*!
    Returns \c true if there was an error opening the database
    connection; otherwise returns \c false. Error information can be
    retrieved using the lastError() function.
*/

bool QSqlDatabase::isOpenError() const
{
    return d->driver->isOpenError();
}

/*!
  Begins a transaction on the database if the driver supports
  transactions. Returns \c{true} if the operation succeeded.
  Otherwise it returns \c{false}.

  \sa QSqlDriver::hasFeature(), commit(), rollback()
*/
bool QSqlDatabase::transaction()
{
    if (!d->driver->hasFeature(QSqlDriver::Transactions))
        return false;
    return d->driver->beginTransaction();
}

/*!
  Commits a transaction to the database if the driver supports
  transactions and a transaction() has been started. Returns \c{true}
  if the operation succeeded. Otherwise it returns \c{false}.

  \note For some databases, the commit will fail and return \c{false}
  if there is an \l{QSqlQuery::isActive()} {active query} using the
  database for a \c{SELECT}. Make the query \l{QSqlQuery::isActive()}
  {inactive} before doing the commit.

  Call lastError() to get information about errors.

  \sa QSqlQuery::isActive(), QSqlDriver::hasFeature(), rollback()
*/
bool QSqlDatabase::commit()
{
    if (!d->driver->hasFeature(QSqlDriver::Transactions))
        return false;
    return d->driver->commitTransaction();
}

/*!
  Rolls back a transaction on the database, if the driver supports
  transactions and a transaction() has been started. Returns \c{true}
  if the operation succeeded. Otherwise it returns \c{false}.

  \note For some databases, the rollback will fail and return
  \c{false} if there is an \l{QSqlQuery::isActive()} {active query}
  using the database for a \c{SELECT}. Make the query
  \l{QSqlQuery::isActive()} {inactive} before doing the rollback.

  Call lastError() to get information about errors.

  \sa QSqlQuery::isActive(), QSqlDriver::hasFeature(), commit()
*/
bool QSqlDatabase::rollback()
{
    if (!d->driver->hasFeature(QSqlDriver::Transactions))
        return false;
    return d->driver->rollbackTransaction();
}

/*!
    Sets the connection's database name to \a name. To have effect,
    the database name must be set \e{before} the connection is
    \l{open()} {opened}.  Alternatively, you can close() the
    connection, set the database name, and call open() again.  \note
    The \e{database name} is not the \e{connection name}. The
    connection name must be passed to addDatabase() at connection
    object create time.

    For the QOCI (Oracle) driver, the database name is the TNS
    Service Name.

    For the QODBC driver, the \a name can either be a DSN, a DSN
    filename (in which case the file must have a \c .dsn extension),
    or a connection string.

    For example, Microsoft Access users can use the following
    connection string to open an \c .mdb file directly, instead of
    having to create a DSN entry in the ODBC manager:

    \snippet code/src_sql_kernel_qsqldatabase.cpp 3

    There is no default value.

    \sa databaseName(), setUserName(), setPassword(), setHostName(),
        setPort(), setConnectOptions(), open()
*/

void QSqlDatabase::setDatabaseName(const QString& name)
{
    if (isValid())
        d->dbname = name;
}

/*!
    Sets the connection's user name to \a name. To have effect, the
    user name must be set \e{before} the connection is \l{open()}
    {opened}.  Alternatively, you can close() the connection, set the
    user name, and call open() again.

    There is no default value.

    \sa userName(), setDatabaseName(), setPassword(), setHostName(),
        setPort(), setConnectOptions(), open()
*/

void QSqlDatabase::setUserName(const QString& name)
{
    if (isValid())
        d->uname = name;
}

/*!
    Sets the connection's password to \a password. To have effect, the
    password must be set \e{before} the connection is \l{open()}
    {opened}.  Alternatively, you can close() the connection, set the
    password, and call open() again.

    There is no default value.

    \warning This function stores the password in plain text within
    Qt. Use the open() call that takes a password as parameter to
    avoid this behavior.

    \sa password(), setUserName(), setDatabaseName(), setHostName(),
        setPort(), setConnectOptions(), open()
*/

void QSqlDatabase::setPassword(const QString& password)
{
    if (isValid())
        d->pword = password;
}

/*!
    Sets the connection's host name to \a host. To have effect, the
    host name must be set \e{before} the connection is \l{open()}
    {opened}.  Alternatively, you can close() the connection, set the
    host name, and call open() again.

    There is no default value.

    \sa hostName(), setUserName(), setPassword(), setDatabaseName(),
        setPort(), setConnectOptions(), open()
*/

void QSqlDatabase::setHostName(const QString& host)
{
    if (isValid())
        d->hname = host;
}

/*!
    Sets the connection's port number to \a port. To have effect, the
    port number must be set \e{before} the connection is \l{open()}
    {opened}.  Alternatively, you can close() the connection, set the
    port number, and call open() again..

    There is no default value.

    \sa port(), setUserName(), setPassword(), setHostName(),
        setDatabaseName(), setConnectOptions(), open()
*/

void QSqlDatabase::setPort(int port)
{
    if (isValid())
        d->port = port;
}

/*!
    Returns the connection's database name, which may be empty.
    \note The database name is not the connection name.

    \sa setDatabaseName()
*/
QString QSqlDatabase::databaseName() const
{
    return d->dbname;
}

/*!
    Returns the connection's user name; it may be empty.

    \sa setUserName()
*/
QString QSqlDatabase::userName() const
{
    return d->uname;
}

/*!
    Returns the connection's password. If the password was not set
    with setPassword(), and if the password was given in the open()
    call, or if no password was used, an empty string is returned.
*/
QString QSqlDatabase::password() const
{
    return d->pword;
}

/*!
    Returns the connection's host name; it may be empty.

    \sa setHostName()
*/
QString QSqlDatabase::hostName() const
{
    return d->hname;
}

/*!
    Returns the connection's driver name.

    \sa addDatabase(), driver()
*/
QString QSqlDatabase::driverName() const
{
    return d->drvName;
}

/*!
    Returns the connection's port number. The value is undefined if
    the port number has not been set.

    \sa setPort()
*/
int QSqlDatabase::port() const
{
    return d->port;
}

/*!
    Returns the database driver used to access the database
    connection.

    \sa addDatabase(), drivers()
*/

QSqlDriver* QSqlDatabase::driver() const
{
    return d->driver;
}

/*!
    Returns information about the last error that occurred on the
    database.

    Failures that occur in conjunction with an individual query are
    reported by QSqlQuery::lastError().

    \sa QSqlError, QSqlQuery::lastError()
*/

QSqlError QSqlDatabase::lastError() const
{
    return d->driver->lastError();
}


/*!
    Returns a list of the database's tables, system tables and views,
    as specified by the parameter \a type.

    \sa primaryIndex(), record()
*/

QStringList QSqlDatabase::tables(QSql::TableType type) const
{
    return d->driver->tables(type);
}

/*!
    Returns the primary index for table \a tablename. If no primary
    index exists an empty QSqlIndex is returned.

    \sa tables(), record()
*/

QSqlIndex QSqlDatabase::primaryIndex(const QString& tablename) const
{
    return d->driver->primaryIndex(tablename);
}


/*!
    Returns a QSqlRecord populated with the names of all the fields in
    the table (or view) called \a tablename. The order in which the
    fields appear in the record is undefined. If no such table (or
    view) exists, an empty record is returned.
*/

QSqlRecord QSqlDatabase::record(const QString& tablename) const
{
    return d->driver->record(tablename);
}


/*!
    Sets database-specific \a options. This must be done before the
    connection is opened or it has no effect (or you can close() the
    connection, call this function and open() the connection again).

    The format of the \a options string is a semicolon separated list
    of option names or option=value pairs. The options depend on the
    database client used:

    \table
    \header \li ODBC \li MySQL \li PostgreSQL
    \row

    \li
    \list
    \li SQL_ATTR_ACCESS_MODE
    \li SQL_ATTR_LOGIN_TIMEOUT
    \li SQL_ATTR_CONNECTION_TIMEOUT
    \li SQL_ATTR_CURRENT_CATALOG
    \li SQL_ATTR_METADATA_ID
    \li SQL_ATTR_PACKET_SIZE
    \li SQL_ATTR_TRACEFILE
    \li SQL_ATTR_TRACE
    \li SQL_ATTR_CONNECTION_POOLING
    \li SQL_ATTR_ODBC_VERSION
    \endlist

    \li
    \list
    \li CLIENT_COMPRESS
    \li CLIENT_FOUND_ROWS
    \li CLIENT_IGNORE_SPACE
    \li CLIENT_SSL
    \li CLIENT_ODBC
    \li CLIENT_NO_SCHEMA
    \li CLIENT_INTERACTIVE
    \li UNIX_SOCKET
    \li MYSQL_OPT_RECONNECT
    \endlist

    \li
    \list
    \li connect_timeout
    \li options
    \li tty
    \li requiressl
    \li service
    \endlist

    \header \li DB2 \li OCI \li TDS
    \row

    \li
    \list
    \li SQL_ATTR_ACCESS_MODE
    \li SQL_ATTR_LOGIN_TIMEOUT
    \endlist

    \li
    \list
    \li OCI_ATTR_PREFETCH_ROWS
    \li OCI_ATTR_PREFETCH_MEMORY
    \endlist

    \li
    \e none

    \header \li SQLite \li Interbase
    \row

    \li
    \list
    \li QSQLITE_BUSY_TIMEOUT
    \li QSQLITE_OPEN_READONLY
    \li QSQLITE_OPEN_URI
    \li QSQLITE_ENABLE_SHARED_CACHE
    \endlist

    \li
    \list
    \li ISC_DPB_LC_CTYPE
    \li ISC_DPB_SQL_ROLE_NAME
    \endlist

    \endtable

    Examples:
    \snippet code/src_sql_kernel_qsqldatabase.cpp 4

    Refer to the client library documentation for more information
    about the different options.

    \sa connectOptions()
*/

void QSqlDatabase::setConnectOptions(const QString &options)
{
    if (isValid())
        d->connOptions = options;
}

/*!
    Returns the connection options string used for this connection.
    The string may be empty.

    \sa setConnectOptions()
 */
QString QSqlDatabase::connectOptions() const
{
    return d->connOptions;
}

/*!
    Returns \c true if a driver called \a name is available; otherwise
    returns \c false.

    \sa drivers()
*/

bool QSqlDatabase::isDriverAvailable(const QString& name)
{
    return drivers().contains(name);
}

/*! \fn QSqlDatabase QSqlDatabase::addDatabase(QSqlDriver* driver, const QString& connectionName)

    This overload is useful when you want to create a database
    connection with a \l{QSqlDriver} {driver} you instantiated
    yourself. It might be your own database driver, or you might just
    need to instantiate one of the Qt drivers yourself. If you do
    this, it is recommended that you include the driver code in your
    application. For example, you can create a PostgreSQL connection
    with your own QPSQL driver like this:

    \snippet code/src_sql_kernel_qsqldatabase.cpp 5
    \codeline
    \snippet code/src_sql_kernel_qsqldatabase.cpp 6

    The above code sets up a PostgreSQL connection and instantiates a
    QPSQLDriver object. Next, addDatabase() is called to add the
    connection to the known connections so that it can be used by the
    Qt SQL classes. When a driver is instantiated with a connection
    handle (or set of handles), Qt assumes that you have already
    opened the database connection.

    \note We assume that \c qtdir is the directory where Qt is
    installed. This will pull in the code that is needed to use the
    PostgreSQL client library and to instantiate a QPSQLDriver object,
    assuming that you have the PostgreSQL headers somewhere in your
    include search path.

    Remember that you must link your application against the database
    client library. Make sure the client library is in your linker's
    search path, and add lines like these to your \c{.pro} file:

    \snippet code/src_sql_kernel_qsqldatabase.cpp 7

    The method described works for all the supplied drivers.  The only
    difference will be in the driver constructor arguments.  Here is a
    table of the drivers included with Qt, their source code files,
    and their constructor arguments:

    \table
    \header \li Driver \li Class name \li Constructor arguments \li File to include
    \row
    \li QPSQL
    \li QPSQLDriver
    \li PGconn *connection
    \li \c qsql_psql.cpp
    \row
    \li QMYSQL
    \li QMYSQLDriver
    \li MYSQL *connection
    \li \c qsql_mysql.cpp
    \row
    \li QOCI
    \li QOCIDriver
    \li OCIEnv *environment, OCISvcCtx *serviceContext
    \li \c qsql_oci.cpp
    \row
    \li QODBC
    \li QODBCDriver
    \li SQLHANDLE environment, SQLHANDLE connection
    \li \c qsql_odbc.cpp
    \row
    \li QDB2
    \li QDB2
    \li SQLHANDLE environment, SQLHANDLE connection
    \li \c qsql_db2.cpp
    \row
    \li QTDS
    \li QTDSDriver
    \li LOGINREC *loginRecord, DBPROCESS *dbProcess, const QString &hostName
    \li \c qsql_tds.cpp
    \row
    \li QSQLITE
    \li QSQLiteDriver
    \li sqlite *connection
    \li \c qsql_sqlite.cpp
    \row
    \li QIBASE
    \li QIBaseDriver
    \li isc_db_handle connection
    \li \c qsql_ibase.cpp
    \endtable

    The host name (or service name) is needed when constructing the
    QTDSDriver for creating new connections for internal queries. This
    is to prevent blocking when several QSqlQuery objects are used
    simultaneously.

    \warning Adding a database connection with the same connection
    name as an existing connection, causes the existing connection to
    be replaced by the new one.

    \warning The SQL framework takes ownership of the \a driver. It
    must not be deleted. To remove the connection, use
    removeDatabase().

    \sa drivers()
*/
QSqlDatabase QSqlDatabase::addDatabase(QSqlDriver* driver, const QString& connectionName)
{
    QSqlDatabase db(driver);
    QSqlDatabasePrivate::addDatabase(db, connectionName);
    return db;
}

/*!
    Returns \c true if the QSqlDatabase has a valid driver.

    Example:
    \snippet code/src_sql_kernel_qsqldatabase.cpp 8
*/
bool QSqlDatabase::isValid() const
{
    return d->driver && d->driver != d->shared_null()->driver;
}

/*!
    Clones the database connection \a other and stores it as \a
    connectionName. All the settings from the original database, e.g.
    databaseName(), hostName(), etc., are copied across. Does nothing
    if \a other is an invalid database. Returns the newly created
    database connection.

    \note The new connection has not been opened. Before using the new
    connection, you must call open().
*/
QSqlDatabase QSqlDatabase::cloneDatabase(const QSqlDatabase &other, const QString &connectionName)
{
    if (!other.isValid())
        return QSqlDatabase();

    QSqlDatabase db(other.driverName());
    db.d->copy(other.d);
    QSqlDatabasePrivate::addDatabase(db, connectionName);
    return db;
}

/*!
    \since 4.4

    Returns the connection name, which may be empty.  \note The
    connection name is not the \l{databaseName()} {database name}.

    \sa addDatabase()
*/
QString QSqlDatabase::connectionName() const
{
    return d->connName;
}

/*!
    \since 4.6

    Sets the default numerical precision policy used by queries created
    on this database connection to \a precisionPolicy.

    Note: Drivers that don't support fetching numerical values with low
    precision will ignore the precision policy. You can use
    QSqlDriver::hasFeature() to find out whether a driver supports this
    feature.

    Note: Setting the default precision policy to \a precisionPolicy
    doesn't affect any currently active queries.

    \sa QSql::NumericalPrecisionPolicy, numericalPrecisionPolicy(),
        QSqlQuery::setNumericalPrecisionPolicy(), QSqlQuery::numericalPrecisionPolicy()
*/
void QSqlDatabase::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
    if(driver())
        driver()->setNumericalPrecisionPolicy(precisionPolicy);
    d->precisionPolicy = precisionPolicy;
}

/*!
    \since 4.6

    Returns the current default precision policy for the database connection.

    \sa QSql::NumericalPrecisionPolicy, setNumericalPrecisionPolicy(),
        QSqlQuery::numericalPrecisionPolicy(), QSqlQuery::setNumericalPrecisionPolicy()
*/
QSql::NumericalPrecisionPolicy QSqlDatabase::numericalPrecisionPolicy() const
{
    if(driver())
        return driver()->numericalPrecisionPolicy();
    else
        return d->precisionPolicy;
}


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSqlDatabase &d)
{
    if (!d.isValid()) {
        dbg.nospace() << "QSqlDatabase(invalid)";
        return dbg.space();
    }

    dbg.nospace() << "QSqlDatabase(driver=\"" << d.driverName() << "\", database=\""
                  << d.databaseName() << "\", host=\"" << d.hostName() << "\", port=" << d.port()
                  << ", user=\"" << d.userName() << "\", open=" << d.isOpen() << ")";
    return dbg.space();
}
#endif

QT_END_NAMESPACE
