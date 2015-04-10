/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "qwebdatabase.h"

#include "qwebdatabase_p.h"
#include "qwebsecurityorigin.h"
#include "qwebsecurityorigin_p.h"
#include "DatabaseDetails.h"
#include "DatabaseManager.h"

using namespace WebCore;

/*!
    \class QWebDatabase
    \since 4.5
    \brief The QWebDatabase class provides access to HTML 5 databases created with JavaScript.

    \inmodule QtWebKit

    The upcoming HTML 5 standard includes support for SQL databases that web sites can create and
    access on a local computer through JavaScript. QWebDatabase is the C++ interface to these
    databases.

    Databases are grouped together in security origins. To get access to all databases defined by
    a security origin, use QWebSecurityOrigin::databases(). Each database has an internal name(),
    as well as a user-friendly name, provided by displayName(). These names are specified when
    creating the database in the JavaScript code.

    WebKit uses SQLite to create and access the local SQL databases. The location of the database
    file in the local file system is returned by fileName(). You can access the database directly
    through the \l{Qt SQL} database module.

    For each database the web site can define an expectedSize(). The current size of the database
    in bytes is returned by size().

    For more information refer to the \l{http://dev.w3.org/html5/webdatabase/}{HTML5 Web SQL Database Draft Standard}.

    \sa QWebSecurityOrigin
*/

/*!
    Constructs a web database from \a other.
*/
QWebDatabase::QWebDatabase(const QWebDatabase& other)
    : d(other.d)
{
}

/*!
    Assigns the \a other web database to this.
*/
QWebDatabase& QWebDatabase::operator=(const QWebDatabase& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns the name of the database.
*/
QString QWebDatabase::name() const
{
    return d->name;
}

/*!
    Returns the name of the database in a format that is suitable for display to the user.
*/
QString QWebDatabase::displayName() const
{
#if ENABLE(SQL_DATABASE)
    DatabaseDetails details = DatabaseManager::manager().detailsForNameAndOrigin(d->name, d->origin.get());
    return details.displayName();
#else
    return QString();
#endif
}

/*!
    Returns the expected size of the database in bytes as defined by the web author.
*/
qint64 QWebDatabase::expectedSize() const
{
#if ENABLE(SQL_DATABASE)
    DatabaseDetails details = DatabaseManager::manager().detailsForNameAndOrigin(d->name, d->origin.get());
    return details.expectedUsage();
#else
    return 0;
#endif
}

/*!
    Returns the current size of the database in bytes.
*/
qint64 QWebDatabase::size() const
{
#if ENABLE(SQL_DATABASE)
    DatabaseDetails details = DatabaseManager::manager().detailsForNameAndOrigin(d->name, d->origin.get());
    return details.currentUsage();
#else
    return 0;
#endif
}

/*!
    \internal
*/
QWebDatabase::QWebDatabase(QWebDatabasePrivate* priv)
{
    d = priv;
}

/*!
    Returns the file name of the web database.

    The name can be used to access the database through the \l{Qt SQL} database module, for example:
    \code
      QWebDatabase webdb = ...
      QSqlDatabase sqldb = QSqlDatabase::addDatabase("QSQLITE", "myconnection");
      sqldb.setDatabaseName(webdb.fileName());
      if (sqldb.open()) {
          QStringList tables = sqldb.tables();
          ...
      }
    \endcode

    \note Concurrent access to a database from multiple threads or processes
    is not very efficient because SQLite is used as WebKit's database backend.
*/
QString QWebDatabase::fileName() const
{
#if ENABLE(SQL_DATABASE)
    return DatabaseManager::manager().fullPathForDatabase(d->origin.get(), d->name, false);
#else
    return QString();
#endif
}

/*!
    Returns the databases's security origin.
*/
QWebSecurityOrigin QWebDatabase::origin() const
{
    QWebSecurityOriginPrivate* priv = new QWebSecurityOriginPrivate(d->origin.get());
    QWebSecurityOrigin origin(priv);
    return origin;
}

/*!
    Removes the database \a db from its security origin. All data stored in the
    database \a db will be destroyed.
*/
void QWebDatabase::removeDatabase(const QWebDatabase& db)
{
#if ENABLE(SQL_DATABASE)
    DatabaseManager::manager().deleteDatabase(db.d->origin.get(), db.d->name);
#endif
}

/*!
  \since 4.6

  Deletes all web databases in the configured offline storage path.

  \sa QWebSettings::setOfflineStoragePath()
*/
void QWebDatabase::removeAllDatabases()
{
#if ENABLE(SQL_DATABASE)
    DatabaseManager::manager().deleteAllDatabases();
#endif
}

/*!
    Destroys the web database object. The data within this database is \b not destroyed.
*/
QWebDatabase::~QWebDatabase()
{
}

