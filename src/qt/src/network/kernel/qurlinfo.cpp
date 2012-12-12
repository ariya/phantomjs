/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qurlinfo.h"

#ifndef QT_NO_URLINFO

#include "qurl.h"
#include "qdir.h"
#include <limits.h>

QT_BEGIN_NAMESPACE

class QUrlInfoPrivate
{
public:
    QUrlInfoPrivate() :
        permissions(0),
        size(0),
        isDir(false),
        isFile(true),
        isSymLink(false),
        isWritable(true),
        isReadable(true),
        isExecutable(false)
    {}

    QString name;
    int permissions;
    QString owner;
    QString group;
    qint64 size;

    QDateTime lastModified;
    QDateTime lastRead;
    bool isDir;
    bool isFile;
    bool isSymLink;
    bool isWritable;
    bool isReadable;
    bool isExecutable;
};


/*!
    \class QUrlInfo
    \brief The QUrlInfo class stores information about URLs.

    \ingroup io
    \ingroup network

    The information about a URL that can be retrieved includes name(),
    permissions(), owner(), group(), size(), lastModified(),
    lastRead(), isDir(), isFile(), isSymLink(), isWritable(),
    isReadable() and isExecutable().

    You can create your own QUrlInfo objects passing in all the
    relevant information in the constructor, and you can modify a
    QUrlInfo; for each getter mentioned above there is an equivalent
    setter. Note that setting values does not affect the underlying
    resource that the QUrlInfo provides information about; for example
    if you call setWritable(true) on a read-only resource the only
    thing changed is the QUrlInfo object, not the resource.

    \sa QUrl, {FTP Example}
*/

/*!
    \enum QUrlInfo::PermissionSpec

    This enum is used by the permissions() function to report the
    permissions of a file.

    \value ReadOwner The file is readable by the owner of the file.
    \value WriteOwner The file is writable by the owner of the file.
    \value ExeOwner The file is executable by the owner of the file.
    \value ReadGroup The file is readable by the group.
    \value WriteGroup The file is writable by the group.
    \value ExeGroup The file is executable by the group.
    \value ReadOther The file is readable by anyone.
    \value WriteOther The file is writable by anyone.
    \value ExeOther The file is executable by anyone.
*/

/*!
    Constructs an invalid QUrlInfo object with default values.

    \sa isValid()
*/

QUrlInfo::QUrlInfo()
{
    d = 0;
}

/*!
    Copy constructor, copies \a ui to this URL info object.
*/

QUrlInfo::QUrlInfo(const QUrlInfo &ui)
{
    if (ui.d) {
        d = new QUrlInfoPrivate;
        *d = *ui.d;
    } else {
        d = 0;
    }
}

/*!
    Constructs a QUrlInfo object by specifying all the URL's
    information.

    The information that is passed is the \a name, file \a
    permissions, \a owner and \a group and the file's \a size. Also
    passed is the \a lastModified date/time and the \a lastRead
    date/time. Flags are also passed, specifically, \a isDir, \a
    isFile, \a isSymLink, \a isWritable, \a isReadable and \a
    isExecutable.
*/

QUrlInfo::QUrlInfo(const QString &name, int permissions, const QString &owner,
                    const QString &group, qint64 size, const QDateTime &lastModified,
                    const QDateTime &lastRead, bool isDir, bool isFile, bool isSymLink,
                    bool isWritable, bool isReadable, bool isExecutable)
{
    d = new QUrlInfoPrivate;
    d->name = name;
    d->permissions = permissions;
    d->owner = owner;
    d->group = group;
    d->size = size;
    d->lastModified = lastModified;
    d->lastRead = lastRead;
    d->isDir = isDir;
    d->isFile = isFile;
    d->isSymLink = isSymLink;
    d->isWritable = isWritable;
    d->isReadable = isReadable;
    d->isExecutable = isExecutable;
}


/*!
    Constructs a QUrlInfo object by specifying all the URL's
    information.

    The information that is passed is the \a url, file \a
    permissions, \a owner and \a group and the file's \a size. Also
    passed is the \a lastModified date/time and the \a lastRead
    date/time. Flags are also passed, specifically, \a isDir, \a
    isFile, \a isSymLink, \a isWritable, \a isReadable and \a
    isExecutable.
*/

QUrlInfo::QUrlInfo(const QUrl &url, int permissions, const QString &owner,
                    const QString &group, qint64 size, const QDateTime &lastModified,
                    const QDateTime &lastRead, bool isDir, bool isFile, bool isSymLink,
                    bool isWritable, bool isReadable, bool isExecutable)
{
    d = new QUrlInfoPrivate;
    d->name = QFileInfo(url.path()).fileName();
    d->permissions = permissions;
    d->owner = owner;
    d->group = group;
    d->size = size;
    d->lastModified = lastModified;
    d->lastRead = lastRead;
    d->isDir = isDir;
    d->isFile = isFile;
    d->isSymLink = isSymLink;
    d->isWritable = isWritable;
    d->isReadable = isReadable;
    d->isExecutable = isExecutable;
}


/*!
    Sets the name of the URL to \a name. The name is the full text,
    for example, "http://qt.nokia.com/doc/qurlinfo.html".

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setName(const QString &name)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->name = name;
}


/*!
    If \a b is true then the URL is set to be a directory; if \a b is
    false then the URL is set not to be a directory (which normally
    means it is a file). (Note that a URL can refer to both a file and
    a directory even though most file systems do not support this.)

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setDir(bool b)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->isDir = b;
}


/*!
    If \a b is true then the URL is set to be a file; if \b is false
    then the URL is set not to be a file (which normally means it is a
    directory). (Note that a URL can refer to both a file and a
    directory even though most file systems do not support this.)

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setFile(bool b)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->isFile = b;
}


/*!
    Specifies that the URL refers to a symbolic link if \a b is true
    and that it does not if \a b is false.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setSymLink(bool b)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->isSymLink = b;
}


/*!
    Specifies that the URL is writable if \a b is true and not
    writable if \a b is false.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setWritable(bool b)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->isWritable = b;
}


/*!
    Specifies that the URL is readable if \a b is true and not
    readable if \a b is false.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setReadable(bool b)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->isReadable = b;
}

/*!
    Specifies that the owner of the URL is called \a s.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setOwner(const QString &s)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->owner = s;
}

/*!
    Specifies that the owning group of the URL is called \a s.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setGroup(const QString &s)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->group = s;
}

/*!
    Specifies the \a size of the URL.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setSize(qint64 size)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->size = size;
}

/*!
    Specifies that the URL has access permissions \a p.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setPermissions(int p)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->permissions = p;
}

/*!
    Specifies that the object the URL refers to was last modified at
    \a dt.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setLastModified(const QDateTime &dt)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->lastModified = dt;
}

/*!
    \since 4.4

    Specifies that the object the URL refers to was last read at
    \a dt.

    If you call this function for an invalid URL info, this function
    turns it into a valid one.

    \sa isValid()
*/

void QUrlInfo::setLastRead(const QDateTime &dt)
{
    if (!d)
        d = new QUrlInfoPrivate;
    d->lastRead = dt;
}

/*!
    Destroys the URL info object.
*/

QUrlInfo::~QUrlInfo()
{
    delete d;
}

/*!
    Assigns the values of \a ui to this QUrlInfo object.
*/

QUrlInfo &QUrlInfo::operator=(const QUrlInfo &ui)
{
    if (ui.d) {
        if (!d)
            d= new QUrlInfoPrivate;
        *d = *ui.d;
    } else {
        delete d;
        d = 0;
    }
    return *this;
}

/*!
    Returns the file name of the URL.

    \sa isValid()
*/

QString QUrlInfo::name() const
{
    if (!d)
        return QString();
    return d->name;
}

/*!
    Returns the permissions of the URL. You can use the \c PermissionSpec flags
    to test for certain permissions.

    \sa isValid()
*/

int QUrlInfo::permissions() const
{
    if (!d)
        return 0;
    return d->permissions;
}

/*!
    Returns the owner of the URL.

    \sa isValid()
*/

QString QUrlInfo::owner() const
{
    if (!d)
        return QString();
    return d->owner;
}

/*!
    Returns the group of the URL.

    \sa isValid()
*/

QString QUrlInfo::group() const
{
    if (!d)
        return QString();
    return d->group;
}

/*!
    Returns the size of the URL.

    \sa isValid()
*/

qint64 QUrlInfo::size() const
{
    if (!d)
        return 0;
    return d->size;
}

/*!
    Returns the last modification date of the URL.

    \sa isValid()
*/

QDateTime QUrlInfo::lastModified() const
{
    if (!d)
        return QDateTime();
    return d->lastModified;
}

/*!
    Returns the date when the URL was last read.

    \sa isValid()
*/

QDateTime QUrlInfo::lastRead() const
{
    if (!d)
        return QDateTime();
    return d->lastRead;
}

/*!
    Returns true if the URL is a directory; otherwise returns false.

    \sa isValid()
*/

bool QUrlInfo::isDir() const
{
    if (!d)
        return false;
    return d->isDir;
}

/*!
    Returns true if the URL is a file; otherwise returns false.

    \sa isValid()
*/

bool QUrlInfo::isFile() const
{
    if (!d)
        return false;
    return d->isFile;
}

/*!
    Returns true if the URL is a symbolic link; otherwise returns false.

    \sa isValid()
*/

bool QUrlInfo::isSymLink() const
{
    if (!d)
        return false;
    return d->isSymLink;
}

/*!
    Returns true if the URL is writable; otherwise returns false.

    \sa isValid()
*/

bool QUrlInfo::isWritable() const
{
    if (!d)
        return false;
    return d->isWritable;
}

/*!
    Returns true if the URL is readable; otherwise returns false.

    \sa isValid()
*/

bool QUrlInfo::isReadable() const
{
    if (!d)
        return false;
    return d->isReadable;
}

/*!
    Returns true if the URL is executable; otherwise returns false.

    \sa isValid()
*/

bool QUrlInfo::isExecutable() const
{
    if (!d)
        return false;
    return d->isExecutable;
}

/*!
    Returns true if \a i1 is greater than \a i2; otherwise returns
    false. The objects are compared by the value, which is specified
    by \a sortBy. This must be one of QDir::Name, QDir::Time or
    QDir::Size.
*/

bool QUrlInfo::greaterThan(const QUrlInfo &i1, const QUrlInfo &i2,
                            int sortBy)
{
    switch (sortBy) {
    case QDir::Name:
        return i1.name() > i2.name();
    case QDir::Time:
        return i1.lastModified() > i2.lastModified();
    case QDir::Size:
        return i1.size() > i2.size();
    default:
        return false;
    }
}

/*!
    Returns true if \a i1 is less than \a i2; otherwise returns false.
    The objects are compared by the value, which is specified by \a
    sortBy. This must be one of QDir::Name, QDir::Time or QDir::Size.
*/

bool QUrlInfo::lessThan(const QUrlInfo &i1, const QUrlInfo &i2,
                         int sortBy)
{
    return !greaterThan(i1, i2, sortBy);
}

/*!
    Returns true if \a i1 equals to \a i2; otherwise returns false.
    The objects are compared by the value, which is specified by \a
    sortBy. This must be one of QDir::Name, QDir::Time or QDir::Size.
*/

bool QUrlInfo::equal(const QUrlInfo &i1, const QUrlInfo &i2,
                      int sortBy)
{
    switch (sortBy) {
    case QDir::Name:
        return i1.name() == i2.name();
    case QDir::Time:
        return i1.lastModified() == i2.lastModified();
    case QDir::Size:
        return i1.size() == i2.size();
    default:
        return false;
    }
}

/*!
    Returns true if this QUrlInfo is equal to \a other; otherwise
    returns false.

    \sa lessThan(), equal()
*/

bool QUrlInfo::operator==(const QUrlInfo &other) const
{
    if (!d)
        return other.d == 0;
    if (!other.d)
        return false;

    return (d->name == other.d->name &&
            d->permissions == other.d->permissions &&
            d->owner == other.d->owner &&
            d->group == other.d->group &&
            d->size == other.d->size &&
            d->lastModified == other.d->lastModified &&
            d->lastRead == other.d->lastRead &&
            d->isDir == other.d->isDir &&
            d->isFile == other.d->isFile &&
            d->isSymLink == other.d->isSymLink &&
            d->isWritable == other.d->isWritable &&
            d->isReadable == other.d->isReadable &&
            d->isExecutable == other.d->isExecutable);
}

/*!
    \fn bool QUrlInfo::operator!=(const QUrlInfo &other) const
    \since 4.2

    Returns true if this QUrlInfo is not equal to \a other; otherwise
    returns false.

    \sa lessThan(), equal()
*/

/*!
    Returns true if the URL info is valid; otherwise returns false.
    Valid means that the QUrlInfo contains real information.

    You should always check if the URL info is valid before relying on
    the values.
*/
bool QUrlInfo::isValid() const
{
    return d != 0;
}

QT_END_NAMESPACE

#endif // QT_NO_URLINFO
