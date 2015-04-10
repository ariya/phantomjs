/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qplatformdefs.h"
#include "qdir.h"
#include "qdir_p.h"
#include "qabstractfileengine_p.h"
#include "qfsfileengine_p.h"
#ifndef QT_NO_DEBUG_STREAM
#include "qdebug.h"
#endif
#include "qdiriterator.h"
#include "qdatetime.h"
#include "qstring.h"
#include "qregexp.h"
#include "qvector.h"
#include "qvarlengtharray.h"
#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"
#include "qfilesystemengine_p.h"
#include <qstringbuilder.h>

#ifdef QT_BUILD_CORE_LIB
#  include "qresource.h"
#  include "private/qcoreglobaldata_p.h"
#endif

#include <algorithm>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WIN)
static QString driveSpec(const QString &path)
{
    if (path.size() < 2)
        return QString();
    char c = path.at(0).toLatin1();
    if (c < 'a' && c > 'z' && c < 'A' && c > 'Z')
        return QString();
    if (path.at(1).toLatin1() != ':')
        return QString();
    return path.mid(0, 2);
}
#endif

//************* QDirPrivate
QDirPrivate::QDirPrivate(const QString &path, const QStringList &nameFilters_, QDir::SortFlags sort_, QDir::Filters filters_)
    : QSharedData()
    , fileListsInitialized(false)
    , nameFilters(nameFilters_)
    , sort(sort_)
    , filters(filters_)
{
    setPath(path.isEmpty() ? QString::fromLatin1(".") : path);

    bool empty = nameFilters.isEmpty();
    if (!empty) {
        empty = true;
        for (int i = 0; i < nameFilters.size(); ++i) {
            if (!nameFilters.at(i).isEmpty()) {
                empty = false;
                break;
            }
        }
    }
    if (empty)
        nameFilters = QStringList(QString::fromLatin1("*"));
}

QDirPrivate::QDirPrivate(const QDirPrivate &copy)
    : QSharedData(copy)
    , fileListsInitialized(false)
    , nameFilters(copy.nameFilters)
    , sort(copy.sort)
    , filters(copy.filters)
    , dirEntry(copy.dirEntry)
    , metaData(copy.metaData)
{
}

bool QDirPrivate::exists() const
{
    if (fileEngine.isNull()) {
        QFileSystemEngine::fillMetaData(dirEntry, metaData,
                QFileSystemMetaData::ExistsAttribute | QFileSystemMetaData::DirectoryType); // always stat
        return metaData.exists() && metaData.isDirectory();
    }
    const QAbstractFileEngine::FileFlags info =
        fileEngine->fileFlags(QAbstractFileEngine::DirectoryType
                                       | QAbstractFileEngine::ExistsFlag
                                       | QAbstractFileEngine::Refresh);
    if (!(info & QAbstractFileEngine::DirectoryType))
        return false;
    return info & QAbstractFileEngine::ExistsFlag;
}

// static
inline QChar QDirPrivate::getFilterSepChar(const QString &nameFilter)
{
    QChar sep(QLatin1Char(';'));
    int i = nameFilter.indexOf(sep, 0);
    if (i == -1 && nameFilter.indexOf(QLatin1Char(' '), 0) != -1)
        sep = QChar(QLatin1Char(' '));
    return sep;
}

// static
inline QStringList QDirPrivate::splitFilters(const QString &nameFilter, QChar sep)
{
    if (sep == 0)
        sep = getFilterSepChar(nameFilter);
    QStringList ret = nameFilter.split(sep);
    for (int i = 0; i < ret.count(); ++i)
        ret[i] = ret[i].trimmed();
    return ret;
}

inline void QDirPrivate::setPath(const QString &path)
{
    QString p = QDir::fromNativeSeparators(path);
    if (p.endsWith(QLatin1Char('/'))
            && p.length() > 1
#if defined(Q_OS_WIN)
        && (!(p.length() == 3 && p.at(1).unicode() == ':' && p.at(0).isLetter()))
#endif
    ) {
            p.truncate(p.length() - 1);
    }

    dirEntry = QFileSystemEntry(p, QFileSystemEntry::FromInternalPath());
    metaData.clear();
    initFileEngine();
    clearFileLists();
    absoluteDirEntry = QFileSystemEntry();
}

inline void QDirPrivate::clearFileLists()
{
    fileListsInitialized = false;
    files.clear();
    fileInfos.clear();
}

inline void QDirPrivate::resolveAbsoluteEntry() const
{
    if (!absoluteDirEntry.isEmpty() || dirEntry.isEmpty())
        return;

    QString absoluteName;
    if (fileEngine.isNull()) {
        if (!dirEntry.isRelative() && dirEntry.isClean()) {
            absoluteDirEntry = dirEntry;
            return;
        }

        absoluteName = QFileSystemEngine::absoluteName(dirEntry).filePath();
    } else {
        absoluteName = fileEngine->fileName(QAbstractFileEngine::AbsoluteName);
    }

    absoluteDirEntry = QFileSystemEntry(QDir::cleanPath(absoluteName), QFileSystemEntry::FromInternalPath());
}

/* For sorting */
struct QDirSortItem
{
    mutable QString filename_cache;
    mutable QString suffix_cache;
    QFileInfo item;
};


class QDirSortItemComparator
{
    int qt_cmp_si_sort_flags;
public:
    QDirSortItemComparator(int flags) : qt_cmp_si_sort_flags(flags) {}
    bool operator()(const QDirSortItem &, const QDirSortItem &) const;
};

bool QDirSortItemComparator::operator()(const QDirSortItem &n1, const QDirSortItem &n2) const
{
    const QDirSortItem* f1 = &n1;
    const QDirSortItem* f2 = &n2;

    if ((qt_cmp_si_sort_flags & QDir::DirsFirst) && (f1->item.isDir() != f2->item.isDir()))
        return f1->item.isDir();
    if ((qt_cmp_si_sort_flags & QDir::DirsLast) && (f1->item.isDir() != f2->item.isDir()))
        return !f1->item.isDir();

    int r = 0;
    int sortBy = (qt_cmp_si_sort_flags & QDir::SortByMask)
                 | (qt_cmp_si_sort_flags & QDir::Type);

    switch (sortBy) {
      case QDir::Time: {
        QDateTime firstModified = f1->item.lastModified();
        QDateTime secondModified = f2->item.lastModified();

        // QDateTime by default will do all sorts of conversions on these to
        // find timezones, which is incredibly expensive. As we aren't
        // presenting these to the user, we don't care (at all) about the
        // local timezone, so force them to UTC to avoid that conversion.
        firstModified.setTimeSpec(Qt::UTC);
        secondModified.setTimeSpec(Qt::UTC);

        r = firstModified.secsTo(secondModified);
        break;
      }
      case QDir::Size:
          r = int(qBound<qint64>(-1, f2->item.size() - f1->item.size(), 1));
        break;
      case QDir::Type:
      {
        bool ic = qt_cmp_si_sort_flags & QDir::IgnoreCase;

        if (f1->suffix_cache.isNull())
            f1->suffix_cache = ic ? f1->item.suffix().toLower()
                               : f1->item.suffix();
        if (f2->suffix_cache.isNull())
            f2->suffix_cache = ic ? f2->item.suffix().toLower()
                               : f2->item.suffix();

        r = qt_cmp_si_sort_flags & QDir::LocaleAware
            ? f1->suffix_cache.localeAwareCompare(f2->suffix_cache)
            : f1->suffix_cache.compare(f2->suffix_cache);
      }
        break;
      default:
        ;
    }

    if (r == 0 && sortBy != QDir::Unsorted) {
        // Still not sorted - sort by name
        bool ic = qt_cmp_si_sort_flags & QDir::IgnoreCase;

        if (f1->filename_cache.isNull())
            f1->filename_cache = ic ? f1->item.fileName().toLower()
                                    : f1->item.fileName();
        if (f2->filename_cache.isNull())
            f2->filename_cache = ic ? f2->item.fileName().toLower()
                                    : f2->item.fileName();

        r = qt_cmp_si_sort_flags & QDir::LocaleAware
            ? f1->filename_cache.localeAwareCompare(f2->filename_cache)
            : f1->filename_cache.compare(f2->filename_cache);
    }
    if (qt_cmp_si_sort_flags & QDir::Reversed)
        return r > 0;
    return r < 0;
}

inline void QDirPrivate::sortFileList(QDir::SortFlags sort, QFileInfoList &l,
                                      QStringList *names, QFileInfoList *infos)
{
    // names and infos are always empty lists or 0 here
    int n = l.size();
    if (n > 0) {
        if (n == 1 || (sort & QDir::SortByMask) == QDir::Unsorted) {
            if (infos)
                *infos = l;
            if (names) {
                for (int i = 0; i < n; ++i)
                    names->append(l.at(i).fileName());
            }
        } else {
            QScopedArrayPointer<QDirSortItem> si(new QDirSortItem[n]);
            for (int i = 0; i < n; ++i)
                si[i].item = l.at(i);
            std::sort(si.data(), si.data() + n, QDirSortItemComparator(sort));
            // put them back in the list(s)
            if (infos) {
                for (int i = 0; i < n; ++i)
                    infos->append(si[i].item);
            }
            if (names) {
                for (int i = 0; i < n; ++i)
                    names->append(si[i].item.fileName());
            }
        }
    }
}
inline void QDirPrivate::initFileLists(const QDir &dir) const
{
    if (!fileListsInitialized) {
        QFileInfoList l;
        QDirIterator it(dir);
        while (it.hasNext()) {
            it.next();
            l.append(it.fileInfo());
        }
        sortFileList(sort, l, &files, &fileInfos);
        fileListsInitialized = true;
    }
}

inline void QDirPrivate::initFileEngine()
{
    fileEngine.reset(QFileSystemEngine::resolveEntryAndCreateLegacyEngine(dirEntry, metaData));
}

/*!
    \class QDir
    \inmodule QtCore
    \brief The QDir class provides access to directory structures and their contents.

    \ingroup io
    \ingroup shared
    \reentrant


    A QDir is used to manipulate path names, access information
    regarding paths and files, and manipulate the underlying file
    system. It can also be used to access Qt's \l{resource system}.

    Qt uses "/" as a universal directory separator in the same way
    that "/" is used as a path separator in URLs. If you always use
    "/" as a directory separator, Qt will translate your paths to
    conform to the underlying operating system.

    A QDir can point to a file using either a relative or an absolute
    path. Absolute paths begin with the directory separator
    (optionally preceded by a drive specification under Windows).
    Relative file names begin with a directory name or a file name and
    specify a path relative to the current directory.

    Examples of absolute paths:

    \snippet code/src_corelib_io_qdir.cpp 0

    On Windows, the second example above will be translated to
    \c{C:\Documents and Settings} when used to access files.

    Examples of relative paths:

    \snippet code/src_corelib_io_qdir.cpp 1

    You can use the isRelative() or isAbsolute() functions to check if
    a QDir is using a relative or an absolute file path. Call
    makeAbsolute() to convert a relative QDir to an absolute one.

    \section1 Navigation and Directory Operations

    A directory's path can be obtained with the path() function, and
    a new path set with the setPath() function. The absolute path to
    a directory is found by calling absolutePath().

    The name of a directory is found using the dirName() function. This
    typically returns the last element in the absolute path that specifies
    the location of the directory. However, it can also return "." if
    the QDir represents the current directory.

    \snippet code/src_corelib_io_qdir.cpp 2

    The path for a directory can also be changed with the cd() and cdUp()
    functions, both of which operate like familiar shell commands.
    When cd() is called with the name of an existing directory, the QDir
    object changes directory so that it represents that directory instead.
    The cdUp() function changes the directory of the QDir object so that
    it refers to its parent directory; i.e. cd("..") is equivalent to
    cdUp().

    Directories can be created with mkdir(), renamed with rename(), and
    removed with rmdir().

    You can test for the presence of a directory with a given name by
    using exists(), and the properties of a directory can be tested with
    isReadable(), isAbsolute(), isRelative(), and isRoot().

    The refresh() function re-reads the directory's data from disk.

    \section1 Files and Directory Contents

    Directories contain a number of entries, representing files,
    directories, and symbolic links. The number of entries in a
    directory is returned by count().
    A string list of the names of all the entries in a directory can be
    obtained with entryList(). If you need information about each
    entry, use entryInfoList() to obtain a list of QFileInfo objects.

    Paths to files and directories within a directory can be
    constructed using filePath() and absoluteFilePath().
    The filePath() function returns a path to the specified file
    or directory relative to the path of the QDir object;
    absoluteFilePath() returns an absolute path to the specified
    file or directory. Neither of these functions checks for the
    existence of files or directory; they only construct paths.

    \snippet code/src_corelib_io_qdir.cpp 3

    Files can be removed by using the remove() function. Directories
    cannot be removed in the same way as files; use rmdir() to remove
    them instead.

    It is possible to reduce the number of entries returned by
    entryList() and entryInfoList() by applying filters to a QDir object.
    You can apply a name filter to specify a pattern with wildcards that
    file names need to match, an attribute filter that selects properties
    of entries and can distinguish between files and directories, and a
    sort order.

    Name filters are lists of strings that are passed to setNameFilters().
    Attribute filters consist of a bitwise OR combination of Filters, and
    these are specified when calling setFilter().
    The sort order is specified using setSorting() with a bitwise OR
    combination of SortFlags.

    You can test to see if a filename matches a filter using the match()
    function.

    Filter and sort order flags may also be specified when calling
    entryList() and entryInfoList() in order to override previously defined
    behavior.

    \section1 The Current Directory and Other Special Paths

    Access to some common directories is provided with a number of static
    functions that return QDir objects. There are also corresponding functions
    for these that return strings:

    \table
    \header \li QDir      \li QString         \li Return Value
    \row    \li current() \li currentPath()   \li The application's working directory
    \row    \li home()    \li homePath()      \li The user's home directory
    \row    \li root()    \li rootPath()      \li The root directory
    \row    \li temp()    \li tempPath()      \li The system's temporary directory
    \endtable

    The setCurrent() static function can also be used to set the application's
    working directory.

    If you want to find the directory containing the application's executable,
    see \l{QCoreApplication::applicationDirPath()}.

    The drives() static function provides a list of root directories for each
    device that contains a filing system. On Unix systems this returns a list
    containing a single root directory "/"; on Windows the list will usually
    contain \c{C:/}, and possibly other drive letters such as \c{D:/}, depending
    on the configuration of the user's system.

    \section1 Path Manipulation and Strings

    Paths containing "." elements that reference the current directory at that
    point in the path, ".." elements that reference the parent directory, and
    symbolic links can be reduced to a canonical form using the canonicalPath()
    function.

    Paths can also be simplified by using cleanPath() to remove redundant "/"
    and ".." elements.

    It is sometimes necessary to be able to show a path in the native
    representation for the user's platform. The static toNativeSeparators()
    function returns a copy of the specified path in which each directory
    separator is replaced by the appropriate separator for the underlying
    operating system.

    \section1 Examples

    Check if a directory exists:

    \snippet code/src_corelib_io_qdir.cpp 4

    (We could also use the static convenience function
    QFile::exists().)

    Traversing directories and reading a file:

    \snippet code/src_corelib_io_qdir.cpp 5

    A program that lists all the files in the current directory
    (excluding symbolic links), sorted by size, smallest first:

    \snippet qdir-listfiles/main.cpp 0

    \sa QFileInfo, QFile, QFileDialog, QCoreApplication::applicationDirPath(), {Find Files Example}
*/

/*!
    \fn QDir &QDir::operator=(QDir &&other)

    Move-assigns \a other to this QDir instance.

    \since 5.2
*/

/*!
    \internal
*/
QDir::QDir(QDirPrivate &p) : d_ptr(&p)
{
}

/*!
    Constructs a QDir pointing to the given directory \a path. If path
    is empty the program's working directory, ("."), is used.

    \sa currentPath()
*/
QDir::QDir(const QString &path) : d_ptr(new QDirPrivate(path))
{
}

/*!
    Constructs a QDir with path \a path, that filters its entries by
    name using \a nameFilter and by attributes using \a filters. It
    also sorts the names using \a sort.

    The default \a nameFilter is an empty string, which excludes
    nothing; the default \a filters is \l AllEntries, which also means
    exclude nothing. The default \a sort is \l Name | \l IgnoreCase,
    i.e. sort by name case-insensitively.

    If \a path is an empty string, QDir uses "." (the current
    directory). If \a nameFilter is an empty string, QDir uses the
    name filter "*" (all files).

    Note that \a path need not exist.

    \sa exists(), setPath(), setNameFilters(), setFilter(), setSorting()
*/
QDir::QDir(const QString &path, const QString &nameFilter,
           SortFlags sort, Filters filters)
    : d_ptr(new QDirPrivate(path, QDir::nameFiltersFromString(nameFilter), sort, filters))
{
}

/*!
    Constructs a QDir object that is a copy of the QDir object for
    directory \a dir.

    \sa operator=()
*/
QDir::QDir(const QDir &dir)
    : d_ptr(dir.d_ptr)
{
}

/*!
    Destroys the QDir object frees up its resources. This has no
    effect on the underlying directory in the file system.
*/
QDir::~QDir()
{
}

/*!
    Sets the path of the directory to \a path. The path is cleaned of
    redundant ".", ".." and of multiple separators. No check is made
    to see whether a directory with this path actually exists; but you
    can check for yourself using exists().

    The path can be either absolute or relative. Absolute paths begin
    with the directory separator "/" (optionally preceded by a drive
    specification under Windows). Relative file names begin with a
    directory name or a file name and specify a path relative to the
    current directory. An example of an absolute path is the string
    "/tmp/quartz", a relative path might look like "src/fatlib".

    \sa path(), absolutePath(), exists(), cleanPath(), dirName(),
      absoluteFilePath(), isRelative(), makeAbsolute()
*/
void QDir::setPath(const QString &path)
{
    d_ptr->setPath(path);
}

/*!
    Returns the path. This may contain symbolic links, but never
    contains redundant ".", ".." or multiple separators.

    The returned path can be either absolute or relative (see
    setPath()).

    \sa setPath(), absolutePath(), exists(), cleanPath(), dirName(),
    absoluteFilePath(), toNativeSeparators(), makeAbsolute()
*/
QString QDir::path() const
{
    const QDirPrivate* d = d_ptr.constData();
    return d->dirEntry.filePath();
}

/*!
    Returns the absolute path (a path that starts with "/" or with a
    drive specification), which may contain symbolic links, but never
    contains redundant ".", ".." or multiple separators.

    \sa setPath(), canonicalPath(), exists(), cleanPath(),
    dirName(), absoluteFilePath()
*/
QString QDir::absolutePath() const
{
    const QDirPrivate* d = d_ptr.constData();
    d->resolveAbsoluteEntry();
    return d->absoluteDirEntry.filePath();
}

/*!
    Returns the canonical path, i.e. a path without symbolic links or
    redundant "." or ".." elements.

    On systems that do not have symbolic links this function will
    always return the same string that absolutePath() returns. If the
    canonical path does not exist (normally due to dangling symbolic
    links) canonicalPath() returns an empty string.

    Example:

    \snippet code/src_corelib_io_qdir.cpp 6

    \sa path(), absolutePath(), exists(), cleanPath(), dirName(),
        absoluteFilePath()
*/
QString QDir::canonicalPath() const
{
    const QDirPrivate* d = d_ptr.constData();
    if (d->fileEngine.isNull()) {
        QFileSystemEntry answer = QFileSystemEngine::canonicalName(d->dirEntry, d->metaData);
        return answer.filePath();
    }
    return d->fileEngine->fileName(QAbstractFileEngine::CanonicalName);
}

/*!
    Returns the name of the directory; this is \e not the same as the
    path, e.g. a directory with the name "mail", might have the path
    "/var/spool/mail". If the directory has no name (e.g. it is the
    root directory) an empty string is returned.

    No check is made to ensure that a directory with this name
    actually exists; but see exists().

    \sa path(), filePath(), absolutePath(), absoluteFilePath()
*/
QString QDir::dirName() const
{
    const QDirPrivate* d = d_ptr.constData();
    return d->dirEntry.fileName();
}

/*!
    Returns the path name of a file in the directory. Does \e not
    check if the file actually exists in the directory; but see
    exists(). If the QDir is relative the returned path name will also
    be relative. Redundant multiple separators or "." and ".."
    directories in \a fileName are not removed (see cleanPath()).

    \sa dirName(), absoluteFilePath(), isRelative(), canonicalPath()
*/
QString QDir::filePath(const QString &fileName) const
{
    const QDirPrivate* d = d_ptr.constData();
    if (isAbsolutePath(fileName))
        return QString(fileName);

    QString ret = d->dirEntry.filePath();
    if (!fileName.isEmpty()) {
        if (!ret.isEmpty() && ret[(int)ret.length()-1] != QLatin1Char('/') && fileName[0] != QLatin1Char('/'))
            ret += QLatin1Char('/');
        ret += fileName;
    }
    return ret;
}

/*!
    Returns the absolute path name of a file in the directory. Does \e
    not check if the file actually exists in the directory; but see
    exists(). Redundant multiple separators or "." and ".."
    directories in \a fileName are not removed (see cleanPath()).

    \sa relativeFilePath(), filePath(), canonicalPath()
*/
QString QDir::absoluteFilePath(const QString &fileName) const
{
    const QDirPrivate* d = d_ptr.constData();
    if (isAbsolutePath(fileName))
        return fileName;

    d->resolveAbsoluteEntry();
    const QString absoluteDirPath = d->absoluteDirEntry.filePath();
    if (fileName.isEmpty())
        return absoluteDirPath;
    if (!absoluteDirPath.endsWith(QLatin1Char('/')))
        return absoluteDirPath % QLatin1Char('/') % fileName;
    return absoluteDirPath % fileName;
}

/*!
    Returns the path to \a fileName relative to the directory.

    \snippet code/src_corelib_io_qdir.cpp 7

    \sa absoluteFilePath(), filePath(), canonicalPath()
*/
QString QDir::relativeFilePath(const QString &fileName) const
{
    QString dir = cleanPath(absolutePath());
    QString file = cleanPath(fileName);

    if (isRelativePath(file) || isRelativePath(dir))
        return file;

#ifdef Q_OS_WIN
    QString dirDrive = driveSpec(dir);
    QString fileDrive = driveSpec(file);

    bool fileDriveMissing = false;
    if (fileDrive.isEmpty()) {
        fileDrive = dirDrive;
        fileDriveMissing = true;
    }

    if (fileDrive.toLower() != dirDrive.toLower()
        || (file.startsWith(QLatin1String("//"))
        && !dir.startsWith(QLatin1String("//"))))
        return file;

    dir.remove(0, dirDrive.size());
    if (!fileDriveMissing)
        file.remove(0, fileDrive.size());
#endif

    QString result;
    QStringList dirElts = dir.split(QLatin1Char('/'), QString::SkipEmptyParts);
    QStringList fileElts = file.split(QLatin1Char('/'), QString::SkipEmptyParts);

    int i = 0;
    while (i < dirElts.size() && i < fileElts.size() &&
#if defined(Q_OS_WIN)
           dirElts.at(i).toLower() == fileElts.at(i).toLower())
#else
           dirElts.at(i) == fileElts.at(i))
#endif
        ++i;

    for (int j = 0; j < dirElts.size() - i; ++j)
        result += QLatin1String("../");

    for (int j = i; j < fileElts.size(); ++j) {
        result += fileElts.at(j);
        if (j < fileElts.size() - 1)
            result += QLatin1Char('/');
    }

    return result;
}

/*!
    \since 4.2

    Returns \a pathName with the '/' separators converted to
    separators that are appropriate for the underlying operating
    system.

    On Windows, toNativeSeparators("c:/winnt/system32") returns
    "c:\\winnt\\system32".

    The returned string may be the same as the argument on some
    operating systems, for example on Unix.

    \sa fromNativeSeparators(), separator()
*/
QString QDir::toNativeSeparators(const QString &pathName)
{
#if defined(Q_OS_WIN)
    int i = pathName.indexOf(QLatin1Char('/'));
    if (i != -1) {
        QString n(pathName);

        QChar * const data = n.data();
        data[i++] = QLatin1Char('\\');

        for (; i < n.length(); ++i) {
            if (data[i] == QLatin1Char('/'))
                data[i] = QLatin1Char('\\');
        }

        return n;
    }
#endif
    return pathName;
}

/*!
    \since 4.2

    Returns \a pathName using '/' as file separator. On Windows,
    for instance, fromNativeSeparators("\c{c:\\winnt\\system32}") returns
    "c:/winnt/system32".

    The returned string may be the same as the argument on some
    operating systems, for example on Unix.

    \sa toNativeSeparators(), separator()
*/
QString QDir::fromNativeSeparators(const QString &pathName)
{
#if defined(Q_OS_WIN)
    int i = pathName.indexOf(QLatin1Char('\\'));
    if (i != -1) {
        QString n(pathName);

        QChar * const data = n.data();
        data[i++] = QLatin1Char('/');

        for (; i < n.length(); ++i) {
            if (data[i] == QLatin1Char('\\'))
                data[i] = QLatin1Char('/');
        }

        return n;
    }
#endif
    return pathName;
}

/*!
    Changes the QDir's directory to \a dirName.

    Returns \c true if the new directory exists;
    otherwise returns \c false. Note that the logical cd() operation is
    not performed if the new directory does not exist.

    Calling cd("..") is equivalent to calling cdUp().

    \sa cdUp(), isReadable(), exists(), path()
*/
bool QDir::cd(const QString &dirName)
{
    // Don't detach just yet.
    const QDirPrivate * const d = d_ptr.constData();

    if (dirName.isEmpty() || dirName == QLatin1String("."))
        return true;
    QString newPath;
    if (isAbsolutePath(dirName)) {
        newPath = cleanPath(dirName);
    } else {
        if (isRoot())
            newPath = d->dirEntry.filePath();
        else
            newPath = d->dirEntry.filePath() % QLatin1Char('/');
        newPath += dirName;
        if (dirName.indexOf(QLatin1Char('/')) >= 0
            || dirName == QLatin1String("..")
            || d->dirEntry.filePath() == QLatin1String(".")) {
            newPath = cleanPath(newPath);
#if defined (Q_OS_UNIX)
            //After cleanPath() if path is "/.." or starts with "/../" it means trying to cd above root.
            if (newPath.startsWith(QLatin1String("/../")) || newPath == QLatin1String("/.."))
#else
            /*
              cleanPath() already took care of replacing '\' with '/'.
              We can't use startsWith here because the letter of the drive is unknown.
              After cleanPath() if path is "[A-Z]:/.." or starts with "[A-Z]:/../" it means trying to cd above root.
             */

            if (newPath.midRef(1, 4) == QLatin1String(":/..") && (newPath.length() == 5 || newPath.at(5) == QLatin1Char('/')))
#endif
                return false;
            /*
              If newPath starts with .., we convert it to absolute to
              avoid infinite looping on

                  QDir dir(".");
                  while (dir.cdUp())
                      ;
            */
            if (newPath.startsWith(QLatin1String(".."))) {
                newPath = QFileInfo(newPath).absoluteFilePath();
            }
        }
    }

    QScopedPointer<QDirPrivate> dir(new QDirPrivate(*d_ptr.constData()));
    dir->setPath(newPath);
    if (!dir->exists())
        return false;

    d_ptr = dir.take();
    return true;
}

/*!
    Changes directory by moving one directory up from the QDir's
    current directory.

    Returns \c true if the new directory exists;
    otherwise returns \c false. Note that the logical cdUp() operation is
    not performed if the new directory does not exist.

    \sa cd(), isReadable(), exists(), path()
*/
bool QDir::cdUp()
{
    return cd(QString::fromLatin1(".."));
}

/*!
    Returns the string list set by setNameFilters()
*/
QStringList QDir::nameFilters() const
{
    const QDirPrivate* d = d_ptr.constData();
    return d->nameFilters;
}

/*!
    Sets the name filters used by entryList() and entryInfoList() to the
    list of filters specified by \a nameFilters.

    Each name filter is a wildcard (globbing) filter that understands
    \c{*} and \c{?} wildcards. (See \l{QRegExp wildcard matching}.)

    For example, the following code sets three name filters on a QDir
    to ensure that only files with extensions typically used for C++
    source files are listed:

    \snippet qdir-namefilters/main.cpp 0

    \sa nameFilters(), setFilter()
*/
void QDir::setNameFilters(const QStringList &nameFilters)
{
    QDirPrivate* d = d_ptr.data();
    d->initFileEngine();
    d->clearFileLists();

    d->nameFilters = nameFilters;
}

/*!
    \obsolete

    Use QDir::addSearchPath() with a prefix instead.

    Adds \a path to the search paths searched in to find resources
    that are not specified with an absolute path. The default search
    path is to search only in the root (\c{:/}).

    \sa {The Qt Resource System}
*/
void QDir::addResourceSearchPath(const QString &path)
{
#ifdef QT_BUILD_CORE_LIB
    QResource::addSearchPath(path);
#else
    Q_UNUSED(path)
#endif
}

#ifdef QT_BUILD_CORE_LIB
/*!
    \since 4.3

    Sets or replaces Qt's search paths for file names with the prefix \a prefix
    to \a searchPaths.

    To specify a prefix for a file name, prepend the prefix followed by a single
    colon (e.g., "images:undo.png", "xmldocs:books.xml"). \a prefix can only
    contain letters or numbers (e.g., it cannot contain a colon, nor a slash).

    Qt uses this search path to locate files with a known prefix. The search
    path entries are tested in order, starting with the first entry.

    \snippet code/src_corelib_io_qdir.cpp 8

    File name prefix must be at least 2 characters long to avoid conflicts with
    Windows drive letters.

    Search paths may contain paths to \l{The Qt Resource System}.
*/
void QDir::setSearchPaths(const QString &prefix, const QStringList &searchPaths)
{
    if (prefix.length() < 2) {
        qWarning("QDir::setSearchPaths: Prefix must be longer than 1 character");
        return;
    }

    for (int i = 0; i < prefix.count(); ++i) {
        if (!prefix.at(i).isLetterOrNumber()) {
            qWarning("QDir::setSearchPaths: Prefix can only contain letters or numbers");
            return;
        }
    }

    QWriteLocker lock(&QCoreGlobalData::instance()->dirSearchPathsLock);
    QMap<QString, QStringList> &paths = QCoreGlobalData::instance()->dirSearchPaths;
    if (searchPaths.isEmpty()) {
        paths.remove(prefix);
    } else {
        paths.insert(prefix, searchPaths);
    }
}

/*!
    \since 4.3

    Adds \a path to the search path for \a prefix.

    \sa setSearchPaths()
*/
void QDir::addSearchPath(const QString &prefix, const QString &path)
{
    if (path.isEmpty())
        return;

    QWriteLocker lock(&QCoreGlobalData::instance()->dirSearchPathsLock);
    QCoreGlobalData::instance()->dirSearchPaths[prefix] += path;
}

/*!
    \since 4.3

    Returns the search paths for \a prefix.

    \sa setSearchPaths(), addSearchPath()
*/
QStringList QDir::searchPaths(const QString &prefix)
{
    QReadLocker lock(&QCoreGlobalData::instance()->dirSearchPathsLock);
    return QCoreGlobalData::instance()->dirSearchPaths.value(prefix);
}

#endif // QT_BUILD_CORE_LIB

/*!
    Returns the value set by setFilter()
*/
QDir::Filters QDir::filter() const
{
    const QDirPrivate* d = d_ptr.constData();
    return d->filters;
}

/*!
    \enum QDir::Filter

    This enum describes the filtering options available to QDir; e.g.
    for entryList() and entryInfoList(). The filter value is specified
    by combining values from the following list using the bitwise OR
    operator:

    \value Dirs    List directories that match the filters.
    \value AllDirs  List all directories; i.e. don't apply the filters
                    to directory names.
    \value Files   List files.
    \value Drives  List disk drives (ignored under Unix).
    \value NoSymLinks  Do not list symbolic links (ignored by operating
                       systems that don't support symbolic links).
    \value NoDotAndDotDot Do not list the special entries "." and "..".
    \value NoDot       Do not list the special entry ".".
    \value NoDotDot    Do not list the special entry "..".
    \value AllEntries  List directories, files, drives and symlinks (this does not list
                broken symlinks unless you specify System).
    \value Readable    List files for which the application has read
                       access. The Readable value needs to be combined
                       with Dirs or Files.
    \value Writable    List files for which the application has write
                       access. The Writable value needs to be combined
                       with Dirs or Files.
    \value Executable  List files for which the application has
                       execute access. The Executable value needs to be
                       combined with Dirs or Files.
    \value Modified  Only list files that have been modified (ignored
                     on Unix).
    \value Hidden  List hidden files (on Unix, files starting with a ".").
    \value System  List system files (on Unix, FIFOs, sockets and
                   device files are included; on Windows, \c {.lnk}
                   files are included)
    \value CaseSensitive  The filter should be case sensitive.

    \omitvalue TypeMask
    \omitvalue AccessMask
    \omitvalue PermissionMask
    \omitvalue NoFilter

    Functions that use Filter enum values to filter lists of files
    and directories will include symbolic links to files and directories
    unless you set the NoSymLinks value.

    A default constructed QDir will not filter out files based on
    their permissions, so entryList() and entryInfoList() will return
    all files that are readable, writable, executable, or any
    combination of the three.  This makes the default easy to write,
    and at the same time useful.

    For example, setting the \c Readable, \c Writable, and \c Files
    flags allows all files to be listed for which the application has read
    access, write access or both. If the \c Dirs and \c Drives flags are
    also included in this combination then all drives, directories, all
    files that the application can read, write, or execute, and symlinks
    to such files/directories can be listed.

    To retrieve the permissons for a directory, use the
    entryInfoList() function to get the associated QFileInfo objects
    and then use the QFileInfo::permissons() to obtain the permissions
    and ownership for each file.
*/

/*!
    Sets the filter used by entryList() and entryInfoList() to \a
    filters. The filter is used to specify the kind of files that
    should be returned by entryList() and entryInfoList(). See
    \l{QDir::Filter}.

    \sa filter(), setNameFilters()
*/
void QDir::setFilter(Filters filters)
{
    QDirPrivate* d = d_ptr.data();
    d->initFileEngine();
    d->clearFileLists();

    d->filters = filters;
}

/*!
    Returns the value set by setSorting()

    \sa setSorting(), SortFlag
*/
QDir::SortFlags QDir::sorting() const
{
    const QDirPrivate* d = d_ptr.constData();
    return d->sort;
}

/*!
    \enum QDir::SortFlag

    This enum describes the sort options available to QDir, e.g. for
    entryList() and entryInfoList(). The sort value is specified by
    OR-ing together values from the following list:

    \value Name  Sort by name.
    \value Time  Sort by time (modification time).
    \value Size  Sort by file size.
    \value Type  Sort by file type (extension).
    \value Unsorted  Do not sort.
    \value NoSort Not sorted by default.

    \value DirsFirst  Put the directories first, then the files.
    \value DirsLast Put the files first, then the directories.
    \value Reversed  Reverse the sort order.
    \value IgnoreCase  Sort case-insensitively.
    \value LocaleAware Sort items appropriately using the current locale settings.

    \omitvalue SortByMask

    You can only specify one of the first four.

    If you specify both DirsFirst and Reversed, directories are
    still put first, but in reverse order; the files will be listed
    after the directories, again in reverse order.
*/

/*!
    Sets the sort order used by entryList() and entryInfoList().

    The \a sort is specified by OR-ing values from the enum
    \l{QDir::SortFlag}.

    \sa sorting(), SortFlag
*/
void QDir::setSorting(SortFlags sort)
{
    QDirPrivate* d = d_ptr.data();
    d->initFileEngine();
    d->clearFileLists();

    d->sort = sort;
}

/*!
    Returns the total number of directories and files in the directory.

    Equivalent to entryList().count().

    \sa operator[](), entryList()
*/
uint QDir::count() const
{
    const QDirPrivate* d = d_ptr.constData();
    d->initFileLists(*this);
    return d->files.count();
}

/*!
    Returns the file name at position \a pos in the list of file
    names. Equivalent to entryList().at(index).
    \a pos must be a valid index position in the list (i.e., 0 <= pos < count()).

    \sa count(), entryList()
*/
QString QDir::operator[](int pos) const
{
    const QDirPrivate* d = d_ptr.constData();
    d->initFileLists(*this);
    return d->files[pos];
}

/*!
    \overload

    Returns a list of the names of all the files and directories in
    the directory, ordered according to the name and attribute filters
    previously set with setNameFilters() and setFilter(), and sorted according
    to the flags set with setSorting().

    The attribute filter and sorting specifications can be overridden using the
    \a filters and \a sort arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \note To list symlinks that point to non existing files, \l System must be
     passed to the filter.

    \sa entryInfoList(), setNameFilters(), setSorting(), setFilter()
*/
QStringList QDir::entryList(Filters filters, SortFlags sort) const
{
    const QDirPrivate* d = d_ptr.constData();
    return entryList(d->nameFilters, filters, sort);
}


/*!
    \overload

    Returns a list of QFileInfo objects for all the files and directories in
    the directory, ordered according to the name and attribute filters
    previously set with setNameFilters() and setFilter(), and sorted according
    to the flags set with setSorting().

    The attribute filter and sorting specifications can be overridden using the
    \a filters and \a sort arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \sa entryList(), setNameFilters(), setSorting(), setFilter(), isReadable(), exists()
*/
QFileInfoList QDir::entryInfoList(Filters filters, SortFlags sort) const
{
    const QDirPrivate* d = d_ptr.constData();
    return entryInfoList(d->nameFilters, filters, sort);
}

/*!
    Returns a list of the names of all the files and
    directories in the directory, ordered according to the name
    and attribute filters previously set with setNameFilters()
    and setFilter(), and sorted according to the flags set with
    setSorting().

    The name filter, file attribute filter, and sorting specification
    can be overridden using the \a nameFilters, \a filters, and \a sort
    arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \sa entryInfoList(), setNameFilters(), setSorting(), setFilter()
*/
QStringList QDir::entryList(const QStringList &nameFilters, Filters filters,
                            SortFlags sort) const
{
    const QDirPrivate* d = d_ptr.constData();

    if (filters == NoFilter)
        filters = d->filters;
    if (sort == NoSort)
        sort = d->sort;

    if (filters == d->filters && sort == d->sort && nameFilters == d->nameFilters) {
        d->initFileLists(*this);
        return d->files;
    }

    QFileInfoList l;
    QDirIterator it(d->dirEntry.filePath(), nameFilters, filters);
    while (it.hasNext()) {
        it.next();
        l.append(it.fileInfo());
    }
    QStringList ret;
    d->sortFileList(sort, l, &ret, 0);
    return ret;
}

/*!
    Returns a list of QFileInfo objects for all the files and
    directories in the directory, ordered according to the name
    and attribute filters previously set with setNameFilters()
    and setFilter(), and sorted according to the flags set with
    setSorting().

    The name filter, file attribute filter, and sorting specification
    can be overridden using the \a nameFilters, \a filters, and \a sort
    arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \sa entryList(), setNameFilters(), setSorting(), setFilter(), isReadable(), exists()
*/
QFileInfoList QDir::entryInfoList(const QStringList &nameFilters, Filters filters,
                                  SortFlags sort) const
{
    const QDirPrivate* d = d_ptr.constData();

    if (filters == NoFilter)
        filters = d->filters;
    if (sort == NoSort)
        sort = d->sort;

    if (filters == d->filters && sort == d->sort && nameFilters == d->nameFilters) {
        d->initFileLists(*this);
        return d->fileInfos;
    }

    QFileInfoList l;
    QDirIterator it(d->dirEntry.filePath(), nameFilters, filters);
    while (it.hasNext()) {
        it.next();
        l.append(it.fileInfo());
    }
    QFileInfoList ret;
    d->sortFileList(sort, l, 0, &ret);
    return ret;
}

/*!
    Creates a sub-directory called \a dirName.

    Returns \c true on success; otherwise returns \c false.

    If the directory already exists when this function is called, it will return false.

    \sa rmdir()
*/
bool QDir::mkdir(const QString &dirName) const
{
    const QDirPrivate* d = d_ptr.constData();

    if (dirName.isEmpty()) {
        qWarning("QDir::mkdir: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirName);
    if (d->fileEngine.isNull())
        return QFileSystemEngine::createDirectory(QFileSystemEntry(fn), false);
    return d->fileEngine->mkdir(fn, false);
}

/*!
    Removes the directory specified by \a dirName.

    The directory must be empty for rmdir() to succeed.

    Returns \c true if successful; otherwise returns \c false.

    \sa mkdir()
*/
bool QDir::rmdir(const QString &dirName) const
{
    const QDirPrivate* d = d_ptr.constData();

    if (dirName.isEmpty()) {
        qWarning("QDir::rmdir: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirName);
    if (d->fileEngine.isNull())
        return QFileSystemEngine::removeDirectory(QFileSystemEntry(fn), false);

    return d->fileEngine->rmdir(fn, false);
}

/*!
    Creates the directory path \a dirPath.

    The function will create all parent directories necessary to
    create the directory.

    Returns \c true if successful; otherwise returns \c false.

    If the path already exists when this function is called, it will return true.

    \sa rmpath()
*/
bool QDir::mkpath(const QString &dirPath) const
{
    const QDirPrivate* d = d_ptr.constData();

    if (dirPath.isEmpty()) {
        qWarning("QDir::mkpath: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirPath);
    if (d->fileEngine.isNull())
        return QFileSystemEngine::createDirectory(QFileSystemEntry(fn), true);
    return d->fileEngine->mkdir(fn, true);
}

/*!
    Removes the directory path \a dirPath.

    The function will remove all parent directories in \a dirPath,
    provided that they are empty. This is the opposite of
    mkpath(dirPath).

    Returns \c true if successful; otherwise returns \c false.

    \sa mkpath()
*/
bool QDir::rmpath(const QString &dirPath) const
{
    const QDirPrivate* d = d_ptr.constData();

    if (dirPath.isEmpty()) {
        qWarning("QDir::rmpath: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirPath);
    if (d->fileEngine.isNull())
        return QFileSystemEngine::removeDirectory(QFileSystemEntry(fn), true);
    return d->fileEngine->rmdir(fn, true);
}

/*!
    \since 5.0
    Removes the directory, including all its contents.

    Returns \c true if successful, otherwise false.

    If a file or directory cannot be removed, removeRecursively() keeps going
    and attempts to delete as many files and sub-directories as possible,
    then returns \c false.

    If the directory was already removed, the method returns \c true
    (expected result already reached).

    Note: this function is meant for removing a small application-internal
    directory (such as a temporary directory), but not user-visible
    directories. For user-visible operations, it is rather recommended
    to report errors more precisely to the user, to offer solutions
    in case of errors, to show progress during the deletion since it
    could take several minutes, etc.
*/
bool QDir::removeRecursively()
{
    if (!d_ptr->exists())
        return true;

    bool success = true;
    const QString dirPath = path();
    // not empty -- we must empty it first
    QDirIterator di(dirPath, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    while (di.hasNext()) {
        di.next();
        const QFileInfo& fi = di.fileInfo();
        bool ok;
        if (fi.isDir() && !fi.isSymLink())
            ok = QDir(di.filePath()).removeRecursively(); // recursive
        else
            ok = QFile::remove(di.filePath());
        if (!ok)
            success = false;
    }

    if (success)
        success = rmdir(absolutePath());

    return success;
}

/*!
    Returns \c true if the directory is readable \e and we can open files
    by name; otherwise returns \c false.

    \warning A false value from this function is not a guarantee that
    files in the directory are not accessible.

    \sa QFileInfo::isReadable()
*/
bool QDir::isReadable() const
{
    const QDirPrivate* d = d_ptr.constData();

    if (d->fileEngine.isNull()) {
        if (!d->metaData.hasFlags(QFileSystemMetaData::UserReadPermission))
            QFileSystemEngine::fillMetaData(d->dirEntry, d->metaData, QFileSystemMetaData::UserReadPermission);

        return (d->metaData.permissions() & QFile::ReadUser) != 0;
    }

    const QAbstractFileEngine::FileFlags info =
        d->fileEngine->fileFlags(QAbstractFileEngine::DirectoryType
                                       | QAbstractFileEngine::PermsMask);
    if (!(info & QAbstractFileEngine::DirectoryType))
        return false;
    return info & QAbstractFileEngine::ReadUserPerm;
}

/*!
    \overload

    Returns \c true if the directory exists; otherwise returns \c false.
    (If a file with the same name is found this function will return false).

    The overload of this function that accepts an argument is used to test
    for the presence of files and directories within a directory.

    \sa QFileInfo::exists(), QFile::exists()
*/
bool QDir::exists() const
{
    return d_ptr->exists();
}

/*!
    Returns \c true if the directory is the root directory; otherwise
    returns \c false.

    Note: If the directory is a symbolic link to the root directory
    this function returns \c false. If you want to test for this use
    canonicalPath(), e.g.

    \snippet code/src_corelib_io_qdir.cpp 9

    \sa root(), rootPath()
*/
bool QDir::isRoot() const
{
    if (d_ptr->fileEngine.isNull())
        return d_ptr->dirEntry.isRoot();
    return d_ptr->fileEngine->fileFlags(QAbstractFileEngine::FlagsMask) & QAbstractFileEngine::RootFlag;
}

/*!
    \fn bool QDir::isAbsolute() const

    Returns \c true if the directory's path is absolute; otherwise
    returns \c false. See isAbsolutePath().

    \sa isRelative(), makeAbsolute(), cleanPath()
*/

/*!
   \fn bool QDir::isAbsolutePath(const QString &)

    Returns \c true if \a path is absolute; returns \c false if it is
    relative.

    \sa isAbsolute(), isRelativePath(), makeAbsolute(), cleanPath()
*/

/*!
    Returns \c true if the directory path is relative; otherwise returns
    false. (Under Unix a path is relative if it does not start with a
    "/").

    \sa makeAbsolute(), isAbsolute(), isAbsolutePath(), cleanPath()
*/
bool QDir::isRelative() const
{
    if (d_ptr->fileEngine.isNull())
        return d_ptr->dirEntry.isRelative();
    return d_ptr->fileEngine->isRelativePath();
}


/*!
    Converts the directory path to an absolute path. If it is already
    absolute nothing happens. Returns \c true if the conversion
    succeeded; otherwise returns \c false.

    \sa isAbsolute(), isAbsolutePath(), isRelative(), cleanPath()
*/
bool QDir::makeAbsolute()
{
    const QDirPrivate *d = d_ptr.constData();
    QScopedPointer<QDirPrivate> dir;
    if (!d->fileEngine.isNull()) {
        QString absolutePath = d->fileEngine->fileName(QAbstractFileEngine::AbsoluteName);
        if (QDir::isRelativePath(absolutePath))
            return false;

        dir.reset(new QDirPrivate(*d_ptr.constData()));
        dir->setPath(absolutePath);
    } else { // native FS
        d->resolveAbsoluteEntry();
        dir.reset(new QDirPrivate(*d_ptr.constData()));
        dir->setPath(d->absoluteDirEntry.filePath());
    }
    d_ptr = dir.take(); // actually detach
    return true;
}

/*!
    Returns \c true if directory \a dir and this directory have the same
    path and their sort and filter settings are the same; otherwise
    returns \c false.

    Example:

    \snippet code/src_corelib_io_qdir.cpp 10
*/
bool QDir::operator==(const QDir &dir) const
{
    const QDirPrivate *d = d_ptr.constData();
    const QDirPrivate *other = dir.d_ptr.constData();

    if (d == other)
        return true;
    Qt::CaseSensitivity sensitive;
    if (d->fileEngine.isNull() || other->fileEngine.isNull()) {
        if (d->fileEngine.data() != other->fileEngine.data()) // one is native, the other is a custom file-engine
            return false;

        sensitive = QFileSystemEngine::isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    } else {
        if (d->fileEngine->caseSensitive() != other->fileEngine->caseSensitive())
            return false;
        sensitive = d->fileEngine->caseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }

    if (d->filters == other->filters
       && d->sort == other->sort
       && d->nameFilters == other->nameFilters) {

        // Assume directories are the same if path is the same
        if (d->dirEntry.filePath() == other->dirEntry.filePath())
            return true;

        if (exists()) {
            if (!dir.exists())
                return false; //can't be equal if only one exists
            // Both exist, fallback to expensive canonical path computation
            return canonicalPath().compare(dir.canonicalPath(), sensitive) == 0;
        } else {
            if (dir.exists())
                return false; //can't be equal if only one exists
            // Neither exists, compare absolute paths rather than canonical (which would be empty strings)
            d->resolveAbsoluteEntry();
            other->resolveAbsoluteEntry();
            return d->absoluteDirEntry.filePath().compare(other->absoluteDirEntry.filePath(), sensitive) == 0;
        }
    }
    return false;
}

/*!
    Makes a copy of the \a dir object and assigns it to this QDir
    object.
*/
QDir &QDir::operator=(const QDir &dir)
{
    d_ptr = dir.d_ptr;
    return *this;
}

/*!
    \overload
    \obsolete

    Sets the directory path to the given \a path.

    Use setPath() instead.
*/
QDir &QDir::operator=(const QString &path)
{
    d_ptr->setPath(path);
    return *this;
}

/*!
    \fn void QDir::swap(QDir &other)
    \since 5.0

    Swaps this QDir instance with \a other. This function is very fast
    and never fails.
*/

/*!
    \fn bool QDir::operator!=(const QDir &dir) const

    Returns \c true if directory \a dir and this directory have different
    paths or different sort or filter settings; otherwise returns
    false.

    Example:

    \snippet code/src_corelib_io_qdir.cpp 11
*/

/*!
    Removes the file, \a fileName.

    Returns \c true if the file is removed successfully; otherwise
    returns \c false.
*/
bool QDir::remove(const QString &fileName)
{
    if (fileName.isEmpty()) {
        qWarning("QDir::remove: Empty or null file name");
        return false;
    }
    return QFile::remove(filePath(fileName));
}

/*!
    Renames a file or directory from \a oldName to \a newName, and returns
    true if successful; otherwise returns \c false.

    On most file systems, rename() fails only if \a oldName does not
    exist, or if a file with the new name already exists.
    However, there are also other reasons why rename() can
    fail. For example, on at least one file system rename() fails if
    \a newName points to an open file.

    If \a oldName is a file (not a directory) that can't be renamed
    right away, Qt will try to copy \a oldName to \a newName and remove
    \a oldName.

    \sa QFile::rename()
*/
bool QDir::rename(const QString &oldName, const QString &newName)
{
    if (oldName.isEmpty() || newName.isEmpty()) {
        qWarning("QDir::rename: Empty or null file name(s)");
        return false;
    }

    QFile file(filePath(oldName));
    if (!file.exists())
        return false;
    return file.rename(filePath(newName));
}

/*!
    Returns \c true if the file called \a name exists; otherwise returns
    false.

    Unless \a name contains an absolute file path, the file name is assumed
    to be relative to the directory itself, so this function is typically used
    to check for the presence of files within a directory.

    \sa QFileInfo::exists(), QFile::exists()
*/
bool QDir::exists(const QString &name) const
{
    if (name.isEmpty()) {
        qWarning("QDir::exists: Empty or null file name");
        return false;
    }
    return QFile::exists(filePath(name));
}

/*!
    Returns a list of the root directories on this system.

    On Windows this returns a list of QFileInfo objects containing "C:/",
    "D:/", etc. On other operating systems, it returns a list containing
    just one root directory (i.e. "/").

    \sa root(), rootPath()
*/
QFileInfoList QDir::drives()
{
#ifdef QT_NO_FSFILEENGINE
    return QFileInfoList();
#else
    return QFSFileEngine::drives();
#endif
}

/*!
    Returns the native directory separator: "/" under Unix (including
    Mac OS X) and "\\" under Windows.

    You do not need to use this function to build file paths. If you
    always use "/", Qt will translate your paths to conform to the
    underlying operating system. If you want to display paths to the
    user using their operating system's separator use
    toNativeSeparators().
*/
QChar QDir::separator()
{
#if defined(Q_OS_WIN)
    return QLatin1Char('\\');
#else
    return QLatin1Char('/');
#endif
}

/*!
    Sets the application's current working directory to \a path.
    Returns \c true if the directory was successfully changed; otherwise
    returns \c false.

    \sa current(), currentPath(), home(), root(), temp()
*/
bool QDir::setCurrent(const QString &path)
{
    return QFileSystemEngine::setCurrentPath(QFileSystemEntry(path));
}

/*!
    \fn QDir QDir::current()

    Returns the application's current directory.

    The directory is constructed using the absolute path of the current directory,
    ensuring that its path() will be the same as its absolutePath().

    \sa currentPath(), setCurrent(), home(), root(), temp()
*/

/*!
    Returns the absolute path of the application's current directory. The
    current directory is the last directory set with QDir::setCurrent() or, if
    that was never called, the directory at which this application was started
    at by the parent process.

    \sa current(), setCurrent(), homePath(), rootPath(), tempPath(), QCoreApplication::applicationDirPath()
*/
QString QDir::currentPath()
{
    return QFileSystemEngine::currentPath().filePath();
}

/*!
    \fn QDir QDir::home()

    Returns the user's home directory.

    The directory is constructed using the absolute path of the home directory,
    ensuring that its path() will be the same as its absolutePath().

    See homePath() for details.

    \sa drives(), current(), root(), temp()
*/

/*!
    Returns the absolute path of the user's home directory.

    Under Windows this function will return the directory of the
    current user's profile. Typically, this is:

    \snippet code/src_corelib_io_qdir.cpp 12

    Use the toNativeSeparators() function to convert the separators to
    the ones that are appropriate for the underlying operating system.

    If the directory of the current user's profile does not exist or
    cannot be retrieved, the following alternatives will be checked (in
    the given order) until an existing and available path is found:

    \list 1
    \li The path specified by the \c USERPROFILE environment variable.
    \li The path formed by concatenating the \c HOMEDRIVE and \c HOMEPATH
    environment variables.
    \li The path specified by the \c HOME environment variable.
    \li The path returned by the rootPath() function (which uses the \c SystemDrive
    environment variable)
    \li  The \c{C:/} directory.
    \endlist

    Under non-Windows operating systems the \c HOME environment
    variable is used if it exists, otherwise the path returned by the
    rootPath().

    \sa home(), currentPath(), rootPath(), tempPath()
*/
QString QDir::homePath()
{
    return QFileSystemEngine::homePath();
}

/*!
    \fn QDir QDir::temp()

    Returns the system's temporary directory.

    The directory is constructed using the absolute path of the temporary directory,
    ensuring that its path() will be the same as its absolutePath().

    See tempPath() for details.

    \sa drives(), current(), home(), root()
*/

/*!
    Returns the absolute path of the system's temporary directory.

    On Unix/Linux systems this is the path in the \c TMPDIR environment
    variable or \c{/tmp} if \c TMPDIR is not defined. On Windows this is
    usually the path in the \c TEMP or \c TMP environment
    variable.
    The path returned by this method doesn't end with a directory separator
    unless it is the root directory (of a drive).

    \sa temp(), currentPath(), homePath(), rootPath()
*/
QString QDir::tempPath()
{
    return QFileSystemEngine::tempPath();
}

/*!
    \fn QDir QDir::root()

    Returns the root directory.

    The directory is constructed using the absolute path of the root directory,
    ensuring that its path() will be the same as its absolutePath().

    See rootPath() for details.

    \sa drives(), current(), home(), temp()
*/

/*!
    Returns the absolute path of the root directory.

    For Unix operating systems this returns "/". For Windows file
    systems this normally returns "c:/".

    \sa root(), drives(), currentPath(), homePath(), tempPath()
*/
QString QDir::rootPath()
{
    return QFileSystemEngine::rootPath();
}

#ifndef QT_NO_REGEXP
/*!
    \overload

    Returns \c true if the \a fileName matches any of the wildcard (glob)
    patterns in the list of \a filters; otherwise returns \c false. The
    matching is case insensitive.

    \sa {QRegExp wildcard matching}, QRegExp::exactMatch(), entryList(), entryInfoList()
*/
bool QDir::match(const QStringList &filters, const QString &fileName)
{
    for (QStringList::ConstIterator sit = filters.constBegin(); sit != filters.constEnd(); ++sit) {
        QRegExp rx(*sit, Qt::CaseInsensitive, QRegExp::Wildcard);
        if (rx.exactMatch(fileName))
            return true;
    }
    return false;
}

/*!
    Returns \c true if the \a fileName matches the wildcard (glob)
    pattern \a filter; otherwise returns \c false. The \a filter may
    contain multiple patterns separated by spaces or semicolons.
    The matching is case insensitive.

    \sa {QRegExp wildcard matching}, QRegExp::exactMatch(), entryList(), entryInfoList()
*/
bool QDir::match(const QString &filter, const QString &fileName)
{
    return match(nameFiltersFromString(filter), fileName);
}
#endif // QT_NO_REGEXP

/*!
    Returns \a path with redundant directory separators removed,
    and "."s and ".."s resolved (as far as possible).

    This method is shared with QUrl, so it doesn't deal with QDir::separator(),
    nor does it remove the trailing slash, if any.
*/
QString qt_normalizePathSegments(const QString &name, bool allowUncPaths)
{
    int used = 0, levels = 0;
    const int len = name.length();
    QVarLengthArray<QChar> outVector(len);
    QChar *out = outVector.data();

    const QChar *p = name.unicode();
    for (int i = 0, last = -1, iwrite = 0; i < len; ++i) {
        if (p[i] == QLatin1Char('/')) {
            while (i+1 < len && p[i+1] == QLatin1Char('/')) {
                if (allowUncPaths && i == 0)
                    break;
                i++;
            }
            bool eaten = false;
            if (i+1 < len && p[i+1] == QLatin1Char('.')) {
                int dotcount = 1;
                if (i+2 < len && p[i+2] == QLatin1Char('.'))
                    dotcount++;
                if (i == len - dotcount - 1) {
                    if (dotcount == 1) {
                        break;
                    } else if (levels) {
                        if (last == -1) {
                            for (int i2 = iwrite-1; i2 >= 0; i2--) {
                                if (out[i2] == QLatin1Char('/')) {
                                    last = i2;
                                    break;
                                }
                            }
                        }
                        used -= iwrite - last - 1;
                        break;
                    }
                } else if (p[i+dotcount+1] == QLatin1Char('/')) {
                    if (dotcount == 2 && levels) {
                        if (last == -1 || iwrite - last == 1) {
                            for (int i2 = (last == -1) ? (iwrite-1) : (last-1); i2 >= 0; i2--) {
                                if (out[i2] == QLatin1Char('/')) {
                                    eaten = true;
                                    last = i2;
                                    break;
                                }
                            }
                        } else {
                            eaten = true;
                        }
                        if (eaten) {
                            levels--;
                            used -= iwrite - last;
                            iwrite = last;
                            last = -1;
                        }
                    } else if (dotcount == 2 && i > 0 && p[i - 1] != QLatin1Char('.')) {
                        eaten = true;
                        used -= iwrite - qMax(0, last);
                        iwrite = qMax(0, last);
                        last = -1;
                        ++i;
                    } else if (dotcount == 1) {
                        eaten = true;
                    }
                    if (eaten)
                        i += dotcount;
                } else {
                    levels++;
                }
            } else if (last != -1 && iwrite - last == 1) {
#if defined(Q_OS_WIN)
                eaten = (iwrite > 2);
#else
                eaten = true;
#endif
                last = -1;
            } else {
                levels++;
            }
            if (!eaten)
                last = i - (i - iwrite);
            else
                continue;
        } else if (!i && p[i] == QLatin1Char('.')) {
            int dotcount = 1;
            if (len >= 1 && p[1] == QLatin1Char('.'))
                dotcount++;
            if (len >= dotcount && p[dotcount] == QLatin1Char('/')) {
                if (dotcount == 1) {
                    i++;
                    while (i+1 < len-1 && p[i+1] == QLatin1Char('/'))
                        i++;
                    continue;
                }
            }
        }
        out[iwrite++] = p[i];
        used++;
    }

    QString ret = (used == len ? name : QString(out, used));
    return ret;
}

/*!
    Returns \a path with directory separators normalized (converted to "/") and
    redundant ones removed, and "."s and ".."s resolved (as far as possible).

    Symbolic links are kept. This function does not return the
    canonical path, but rather the simplest version of the input.
    For example, "./local" becomes "local", "local/../bin" becomes
    "bin" and "/local/usr/../bin" becomes "/local/bin".

    \sa absolutePath(), canonicalPath()
*/
QString QDir::cleanPath(const QString &path)
{
    if (path.isEmpty())
        return path;
    QString name = path;
    QChar dir_separator = separator();
    if (dir_separator != QLatin1Char('/'))
       name.replace(dir_separator, QLatin1Char('/'));

    bool allowUncPaths = false;
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT) //allow unc paths
    allowUncPaths = true;
#endif

    QString ret = qt_normalizePathSegments(name, allowUncPaths);

    // Strip away last slash except for root directories
    if (ret.length() > 1 && ret.endsWith(QLatin1Char('/'))) {
#if defined (Q_OS_WIN)
        if (!(ret.length() == 3 && ret.at(1) == QLatin1Char(':')))
#endif
            ret.chop(1);
    }

    return ret;
}

/*!
    Returns \c true if \a path is relative; returns \c false if it is
    absolute.

    \sa isRelative(), isAbsolutePath(), makeAbsolute()
*/
bool QDir::isRelativePath(const QString &path)
{
    return QFileInfo(path).isRelative();
}

/*!
    Refreshes the directory information.
*/
void QDir::refresh() const
{
    QDirPrivate *d = const_cast<QDir*>(this)->d_ptr.data();
    d->metaData.clear();
    d->initFileEngine();
    d->clearFileLists();
}

/*!
    \internal
*/
QDirPrivate* QDir::d_func()
{
    return d_ptr.data();
}

/*!
    \internal

    Returns a list of name filters from the given \a nameFilter. (If
    there is more than one filter, each pair of filters is separated
    by a space or by a semicolon.)
*/
QStringList QDir::nameFiltersFromString(const QString &nameFilter)
{
    return QDirPrivate::splitFilters(nameFilter);
}

/*!
    \macro void Q_INIT_RESOURCE(name)
    \relates QDir

    Initializes the resources specified by the \c .qrc file with the
    specified base \a name. Normally, when resources are built as part
    of the application, the resources are loaded automatically at
    startup. The Q_INIT_RESOURCE() macro is necessary on some platforms
    for resources stored in a static library.

    For example, if your application's resources are listed in a file
    called \c myapp.qrc, you can ensure that the resources are
    initialized at startup by adding this line to your \c main()
    function:

    \snippet code/src_corelib_io_qdir.cpp 13

    If the file name contains characters that cannot be part of a valid C++ function name
    (such as '-'), they have to be replaced by the underscore character ('_').

    Note: This macro cannot be used in a namespace. It should be called from
    main(). If that is not possible, the following workaround can be used
    to init the resource \c myapp from the function \c{MyNamespace::myFunction}:

    \snippet code/src_corelib_io_qdir.cpp 14

    \sa Q_CLEANUP_RESOURCE(), {The Qt Resource System}
*/

/*!
    \since 4.1
    \macro void Q_CLEANUP_RESOURCE(name)
    \relates QDir

    Unloads the resources specified by the \c .qrc file with the base
    name \a name.

    Normally, Qt resources are unloaded automatically when the
    application terminates, but if the resources are located in a
    plugin that is being unloaded, call Q_CLEANUP_RESOURCE() to force
    removal of your resources.

    Note: This macro cannot be used in a namespace. Please see the
    Q_INIT_RESOURCE documentation for a workaround.

    Example:

    \snippet code/src_corelib_io_qdir.cpp 15

    \sa Q_INIT_RESOURCE(), {The Qt Resource System}
*/


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, QDir::Filters filters)
{
    QStringList flags;
    if (filters == QDir::NoFilter) {
        flags << QLatin1String("NoFilter");
    } else {
        if (filters & QDir::Dirs) flags << QLatin1String("Dirs");
        if (filters & QDir::AllDirs) flags << QLatin1String("AllDirs");
        if (filters & QDir::Files) flags << QLatin1String("Files");
        if (filters & QDir::Drives) flags << QLatin1String("Drives");
        if (filters & QDir::NoSymLinks) flags << QLatin1String("NoSymLinks");
        if (filters & QDir::NoDot) flags << QLatin1String("NoDot");
        if (filters & QDir::NoDotDot) flags << QLatin1String("NoDotDot");
        if ((filters & QDir::AllEntries) == QDir::AllEntries) flags << QLatin1String("AllEntries");
        if (filters & QDir::Readable) flags << QLatin1String("Readable");
        if (filters & QDir::Writable) flags << QLatin1String("Writable");
        if (filters & QDir::Executable) flags << QLatin1String("Executable");
        if (filters & QDir::Modified) flags << QLatin1String("Modified");
        if (filters & QDir::Hidden) flags << QLatin1String("Hidden");
        if (filters & QDir::System) flags << QLatin1String("System");
        if (filters & QDir::CaseSensitive) flags << QLatin1String("CaseSensitive");
    }
    debug << "QDir::Filters(" << qPrintable(flags.join(QLatin1Char('|'))) << ')';
    return debug;
}

static QDebug operator<<(QDebug debug, QDir::SortFlags sorting)
{
    if (sorting == QDir::NoSort) {
        debug << "QDir::SortFlags(NoSort)";
    } else {
        QString type;
        if ((sorting & 3) == QDir::Name) type = QLatin1String("Name");
        if ((sorting & 3) == QDir::Time) type = QLatin1String("Time");
        if ((sorting & 3) == QDir::Size) type = QLatin1String("Size");
        if ((sorting & 3) == QDir::Unsorted) type = QLatin1String("Unsorted");

        QStringList flags;
        if (sorting & QDir::DirsFirst) flags << QLatin1String("DirsFirst");
        if (sorting & QDir::DirsLast) flags << QLatin1String("DirsLast");
        if (sorting & QDir::IgnoreCase) flags << QLatin1String("IgnoreCase");
        if (sorting & QDir::LocaleAware) flags << QLatin1String("LocaleAware");
        if (sorting & QDir::Type) flags << QLatin1String("Type");
        debug << "QDir::SortFlags(" << qPrintable(type)
              << '|'
              << qPrintable(flags.join(QLatin1Char('|'))) << ')';
    }
    return debug;
}

QDebug operator<<(QDebug debug, const QDir &dir)
{
    debug.maybeSpace() << "QDir(" << dir.path()
                       << ", nameFilters = {"
                       << qPrintable(dir.nameFilters().join(QLatin1Char(',')))
                       << "}, "
                       << dir.sorting()
                       << ','
                       << dir.filter()
                       << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
