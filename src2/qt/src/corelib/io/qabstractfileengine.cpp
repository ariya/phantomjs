/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qabstractfileengine.h"
#include "private/qabstractfileengine_p.h"
#ifdef QT_BUILD_CORE_LIB
#include "private/qresource_p.h"
#endif
#include "qdatetime.h"
#include "qreadwritelock.h"
#include "qvariant.h"
// built-in handlers
#include "qfsfileengine.h"
#include "qdiriterator.h"
#include "qstringbuilder.h"

#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>
#include <QtCore/private/qfilesystemengine_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractFileEngineHandler
    \reentrant

    \brief The QAbstractFileEngineHandler class provides a way to register
    custom file engines with your application.

    \ingroup io
    \since 4.1

    QAbstractFileEngineHandler is a factory for creating QAbstractFileEngine
    objects (file engines), which are used internally by QFile, QFileInfo, and
    QDir when working with files and directories.

    When you open a file, Qt chooses a suitable file engine by passing the
    file name from QFile or QDir through an internal list of registered file
    engine handlers. The first handler to recognize the file name is used to
    create the engine. Qt provides internal file engines for working with
    regular files and resources, but you can also register your own
    QAbstractFileEngine subclasses.

    To install an application-specific file engine, you subclass
    QAbstractFileEngineHandler and reimplement create(). When you instantiate
    the handler (e.g. by creating an instance on the stack or on the heap), it
    will automatically register with Qt. (The latest registered handler takes
    precedence over existing handlers.)

    For example:

    \snippet doc/src/snippets/code/src_corelib_io_qabstractfileengine.cpp 0

    When the handler is destroyed, it is automatically removed from Qt.

    The most common approach to registering a handler is to create an instance
    as part of the start-up phase of your application. It is also possible to
    limit the scope of the file engine handler to a particular area of
    interest (e.g. a special file dialog that needs a custom file engine). By
    creating the handler inside a local scope, you can precisely control the
    area in which your engine will be applied without disturbing file
    operations in other parts of your application.

    \sa QAbstractFileEngine, QAbstractFileEngine::create()
*/

static bool qt_file_engine_handlers_in_use = false;

/*
    All application-wide handlers are stored in this list. The mutex must be
    acquired to ensure thread safety.
 */
Q_GLOBAL_STATIC_WITH_ARGS(QReadWriteLock, fileEngineHandlerMutex, (QReadWriteLock::Recursive))
static bool qt_abstractfileenginehandlerlist_shutDown = false;
class QAbstractFileEngineHandlerList : public QList<QAbstractFileEngineHandler *>
{
public:
    ~QAbstractFileEngineHandlerList()
    {
        QWriteLocker locker(fileEngineHandlerMutex());
        qt_abstractfileenginehandlerlist_shutDown = true;
    }
};
Q_GLOBAL_STATIC(QAbstractFileEngineHandlerList, fileEngineHandlers)

/*!
    Constructs a file handler and registers it with Qt. Once created this
    handler's create() function will be called (along with all the other
    handlers) for any paths used. The most recently created handler that
    recognizes the given path (i.e. that returns a QAbstractFileEngine) is
    used for the new path.

    \sa create()
 */
QAbstractFileEngineHandler::QAbstractFileEngineHandler()
{
    QWriteLocker locker(fileEngineHandlerMutex());
    qt_file_engine_handlers_in_use = true;
    fileEngineHandlers()->prepend(this);
}

/*!
    Destroys the file handler. This will automatically unregister the handler
    from Qt.
 */
QAbstractFileEngineHandler::~QAbstractFileEngineHandler()
{
    QWriteLocker locker(fileEngineHandlerMutex());
    // Remove this handler from the handler list only if the list is valid.
    if (!qt_abstractfileenginehandlerlist_shutDown) {
        QAbstractFileEngineHandlerList *handlers = fileEngineHandlers();
        handlers->removeOne(this);
        if (handlers->isEmpty())
            qt_file_engine_handlers_in_use = false;
    }
}

/*
   \ìnternal

   Handles calls to custom file engine handlers.
*/
QAbstractFileEngine *qt_custom_file_engine_handler_create(const QString &path)
{
    QAbstractFileEngine *engine = 0;

    if (qt_file_engine_handlers_in_use) {
        QReadLocker locker(fileEngineHandlerMutex());

        // check for registered handlers that can load the file
        QAbstractFileEngineHandlerList *handlers = fileEngineHandlers();
        for (int i = 0; i < handlers->size(); i++) {
            if ((engine = handlers->at(i)->create(path)))
                break;
        }
    }

    return engine;
}

/*!
    \fn QAbstractFileEngine *QAbstractFileEngineHandler::create(const QString &fileName) const

    Creates a file engine for file \a fileName. Returns 0 if this
    file handler cannot handle \a fileName.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qabstractfileengine.cpp 1

    \sa QAbstractFileEngine::create()
*/

/*!
    Creates and returns a QAbstractFileEngine suitable for processing \a
    fileName.

    You should not need to call this function; use QFile, QFileInfo or
    QDir directly instead.

    If you reimplemnt this function, it should only return file
    engines that knows how to handle \a fileName; otherwise, it should
    return 0.

    \sa QAbstractFileEngineHandler
*/
QAbstractFileEngine *QAbstractFileEngine::create(const QString &fileName)
{
    QFileSystemEntry entry(fileName);
    QFileSystemMetaData metaData;
    QAbstractFileEngine *engine = QFileSystemEngine::resolveEntryAndCreateLegacyEngine(entry, metaData);

#ifndef QT_NO_FSFILEENGINE
    if (!engine)
        // fall back to regular file engine
        return new QFSFileEngine(entry.filePath());
#endif

    return engine;
}

/*!
    \class QAbstractFileEngine
    \reentrant

    \brief The QAbstractFileEngine class provides an abstraction for accessing
    the filesystem.

    \ingroup io
    \since 4.1

    The QDir, QFile, and QFileInfo classes all make use of a
    QAbstractFileEngine internally. If you create your own QAbstractFileEngine
    subclass (and register it with Qt by creating a QAbstractFileEngineHandler
    subclass), your file engine will be used when the path is one that your
    file engine handles.

    A QAbstractFileEngine refers to one file or one directory. If the referent
    is a file, the setFileName(), rename(), and remove() functions are
    applicable. If the referent is a directory the mkdir(), rmdir(), and
    entryList() functions are applicable. In all cases the caseSensitive(),
    isRelativePath(), fileFlags(), ownerId(), owner(), and fileTime()
    functions are applicable.

    A QAbstractFileEngine subclass can be created to do synchronous network I/O
    based file system operations, local file system operations, or to operate
    as a resource system to access file based resources.

   \sa QAbstractFileEngineHandler
*/

/*!
    \enum QAbstractFileEngine::FileName

    These values are used to request a file name in a particular
    format.

    \value DefaultName The same filename that was passed to the
    QAbstractFileEngine.
    \value BaseName The name of the file excluding the path.
    \value PathName The path to the file excluding the base name.
    \value AbsoluteName The absolute path to the file (including
    the base name).
    \value AbsolutePathName The absolute path to the file (excluding
    the base name).
    \value LinkName The full file name of the file that this file is a
    link to. (This will be empty if this file is not a link.)
    \value CanonicalName Often very similar to LinkName. Will return the true path to the file.
    \value CanonicalPathName Same as CanonicalName, excluding the base name.
    \value BundleName Returns the name of the bundle implies BundleType is set.

    \omitvalue NFileNames

    \sa fileName(), setFileName()
*/

/*!
    \enum QAbstractFileEngine::FileFlag

    The permissions and types of a file, suitable for OR'ing together.

    \value ReadOwnerPerm The owner of the file has permission to read
    it.
    \value WriteOwnerPerm The owner of the file has permission to
    write to it.
    \value ExeOwnerPerm The owner of the file has permission to
    execute it.
    \value ReadUserPerm The current user has permission to read the
    file.
    \value WriteUserPerm The current user has permission to write to
    the file.
    \value ExeUserPerm The current user has permission to execute the
    file.
    \value ReadGroupPerm Members of the current user's group have
    permission to read the file.
    \value WriteGroupPerm Members of the current user's group have
    permission to write to the file.
    \value ExeGroupPerm Members of the current user's group have
    permission to execute the file.
    \value ReadOtherPerm All users have permission to read the file.
    \value WriteOtherPerm All users have permission to write to the
    file.
    \value ExeOtherPerm All users have permission to execute the file.

    \value LinkType The file is a link to another file (or link) in
    the file system (i.e. not a file or directory).
    \value FileType The file is a regular file to the file system
    (i.e. not a link or directory)
    \value BundleType The file is a Mac OS X bundle implies DirectoryType
    \value DirectoryType The file is a directory in the file system
    (i.e. not a link or file).

    \value HiddenFlag The file is hidden.
    \value ExistsFlag The file actually exists in the file system.
    \value RootFlag  The file or the file pointed to is the root of the filesystem.
    \value LocalDiskFlag The file resides on the local disk and can be passed to standard file functions.
    \value Refresh Passing this flag will force the file engine to refresh all flags.

    \omitvalue PermsMask
    \omitvalue TypesMask
    \omitvalue FlagsMask
    \omitvalue FileInfoAll

    \sa fileFlags(), setFileName()
*/

/*!
    \enum QAbstractFileEngine::FileTime

    These are used by the fileTime() function.

    \value CreationTime When the file was created.
    \value ModificationTime When the file was most recently modified.
    \value AccessTime When the file was most recently accessed (e.g.
    read or written to).

    \sa setFileName()
*/

/*!
    \enum QAbstractFileEngine::FileOwner

    \value OwnerUser The user who owns the file.
    \value OwnerGroup The group who owns the file.

    \sa owner(), ownerId(), setFileName()
*/

/*!
   Constructs a new QAbstractFileEngine that does not refer to any file or directory.

   \sa setFileName()
 */
QAbstractFileEngine::QAbstractFileEngine() : d_ptr(new QAbstractFileEnginePrivate)
{
    d_ptr->q_ptr = this;
}

/*!
   \internal

   Constructs a QAbstractFileEngine.
 */
QAbstractFileEngine::QAbstractFileEngine(QAbstractFileEnginePrivate &dd) : d_ptr(&dd)
{
    d_ptr->q_ptr = this;
}

/*!
    Destroys the QAbstractFileEngine.
 */
QAbstractFileEngine::~QAbstractFileEngine()
{
}

/*!
    \fn bool QAbstractFileEngine::open(QIODevice::OpenMode mode)

    Opens the file in the specified \a mode. Returns true if the file
    was successfully opened; otherwise returns false.

    The \a mode is an OR combination of QIODevice::OpenMode and
    QIODevice::HandlingMode values.
*/
bool QAbstractFileEngine::open(QIODevice::OpenMode openMode)
{
    Q_UNUSED(openMode);
    return false;
}

/*!
    Closes the file, returning true if successful; otherwise returns false.

    The default implementation always returns false.
*/
bool QAbstractFileEngine::close()
{
    return false;
}

/*!
    Flushes the open file, returning true if successful; otherwise returns
    false.

    The default implementation always returns false.
*/
bool QAbstractFileEngine::flush()
{
    return false;
}

/*!
    Returns the size of the file.
*/
qint64 QAbstractFileEngine::size() const
{
    return 0;
}

/*!
    Returns the current file position.

    This is the position of the data read/write head of the file.
*/
qint64 QAbstractFileEngine::pos() const
{
    return 0;
}

/*!
    \fn bool QAbstractFileEngine::seek(qint64 offset)

    Sets the file position to the given \a offset. Returns true if
    the position was successfully set; otherwise returns false.

    The offset is from the beginning of the file, unless the
    file is sequential.

    \sa isSequential()
*/
bool QAbstractFileEngine::seek(qint64 pos)
{
    Q_UNUSED(pos);
    return false;
}

/*!
    Returns true if the file is a sequential access device; returns
    false if the file is a direct access device.

    Operations involving size() and seek(int) are not valid on
    sequential devices.
*/
bool QAbstractFileEngine::isSequential() const
{
    return false;
}

/*!
    Requests that the file is deleted from the file system. If the
    operation succeeds return true; otherwise return false.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName() rmdir()
 */
bool QAbstractFileEngine::remove()
{
    return false;
}

/*!
    Copies the contents of this file to a file with the name \a newName.
    Returns true on success; otherwise, false is returned.
*/
bool QAbstractFileEngine::copy(const QString &newName)
{
    Q_UNUSED(newName);
    return false;
}

/*!
    Requests that the file be renamed to \a newName in the file
    system. If the operation succeeds return true; otherwise return
    false.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName()
 */
bool QAbstractFileEngine::rename(const QString &newName)
{
    Q_UNUSED(newName);
    return false;
}

/*!
    Creates a link from the file currently specified by fileName() to
    \a newName. What a link is depends on the underlying filesystem
    (be it a shortcut on Windows or a symbolic link on Unix). Returns
    true if successful; otherwise returns false.
*/
bool QAbstractFileEngine::link(const QString &newName)
{
    Q_UNUSED(newName);
    return false;
}

/*!
    Requests that the directory \a dirName be created. If
    \a createParentDirectories is true, then any sub-directories in \a dirName
    that don't exist must be created. If \a createParentDirectories is false then
    any sub-directories in \a dirName must already exist for the function to
    succeed. If the operation succeeds return true; otherwise return
    false.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName() rmdir() isRelativePath()
 */
bool QAbstractFileEngine::mkdir(const QString &dirName, bool createParentDirectories) const
{
    Q_UNUSED(dirName);
    Q_UNUSED(createParentDirectories);
    return false;
}

/*!
    Requests that the directory \a dirName is deleted from the file
    system. When \a recurseParentDirectories is true, then any empty
    parent-directories in \a dirName must also be deleted. If
    \a recurseParentDirectories is false, only the \a dirName leaf-node
    should be deleted. In most file systems a directory cannot be deleted
    using this function if it is non-empty. If the operation succeeds
    return true; otherwise return false.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName() remove() mkdir() isRelativePath()
 */
bool QAbstractFileEngine::rmdir(const QString &dirName, bool recurseParentDirectories) const
{
    Q_UNUSED(dirName);
    Q_UNUSED(recurseParentDirectories);
    return false;
}

/*!
    Requests that the file be set to size \a size. If \a size is larger
    than the current file then it is filled with 0's, if smaller it is
    simply truncated. If the operations succceeds return true; otherwise
    return false;

    This virtual function must be reimplemented by all subclasses.

    \sa size()
*/
bool QAbstractFileEngine::setSize(qint64 size)
{
    Q_UNUSED(size);
    return false;
}

/*!
    Should return true if the underlying file system is case-sensitive;
    otherwise return false.

    This virtual function must be reimplemented by all subclasses.
 */
bool QAbstractFileEngine::caseSensitive() const
{
    return false;
}

/*!
    Return true if the file referred to by this file engine has a
    relative path; otherwise return false.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName()
 */
bool QAbstractFileEngine::isRelativePath() const
{
    return false;
}

/*!
    Requests that a list of all the files matching the \a filters
    list based on the \a filterNames in the file engine's directory
    are returned.

    Should return an empty list if the file engine refers to a file
    rather than a directory, or if the directory is unreadable or does
    not exist or if nothing matches the specifications.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName()
 */
QStringList QAbstractFileEngine::entryList(QDir::Filters filters, const QStringList &filterNames) const
{
    QStringList ret;
    QDirIterator it(fileName(), filterNames, filters);
    while (it.hasNext()) {
        it.next();
        ret << it.fileName();
    }
    return ret;
}

/*!
    This function should return the set of OR'd flags that are true
    for the file engine's file, and that are in the \a type's OR'd
    members.

    In your reimplementation you can use the \a type argument as an
    optimization hint and only return the OR'd set of members that are
    true and that match those in \a type; in other words you can
    ignore any members not mentioned in \a type, thus avoiding some
    potentially expensive lookups or system calls.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName()
*/
QAbstractFileEngine::FileFlags QAbstractFileEngine::fileFlags(FileFlags type) const
{
    Q_UNUSED(type);
    return 0;
}

/*!
    Requests that the file's permissions be set to \a perms. The argument
    perms will be set to the OR-ed together combination of
    QAbstractFileEngine::FileInfo, with only the QAbstractFileEngine::PermsMask being
    honored. If the operations succceeds return true; otherwise return
    false;

    This virtual function must be reimplemented by all subclasses.

    \sa size()
*/
bool QAbstractFileEngine::setPermissions(uint perms)
{
    Q_UNUSED(perms);
    return false;
}

/*!
    Return  the file engine's current file name in the format
    specified by \a file.

    If you don't handle some \c FileName possibilities, return the
    file name set in setFileName() when an unhandled format is
    requested.

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName(), FileName
 */
QString QAbstractFileEngine::fileName(FileName file) const
{
    Q_UNUSED(file);
    return QString();
}

/*!
    If \a owner is \c OwnerUser return the ID of the user who owns
    the file. If \a owner is \c OwnerGroup return the ID of the group
    that own the file. If you can't determine the owner return -2.

    This virtual function must be reimplemented by all subclasses.

    \sa owner() setFileName(), FileOwner
 */
uint QAbstractFileEngine::ownerId(FileOwner owner) const
{
    Q_UNUSED(owner);
    return 0;
}

/*!
    If \a owner is \c OwnerUser return the name of the user who owns
    the file. If \a owner is \c OwnerGroup return the name of the group
    that own the file. If you can't determine the owner return
    QString().

    This virtual function must be reimplemented by all subclasses.

    \sa ownerId() setFileName(), FileOwner
 */
QString QAbstractFileEngine::owner(FileOwner owner) const
{
    Q_UNUSED(owner);
    return QString();
}

/*!
    If \a time is \c CreationTime, return when the file was created.
    If \a time is \c ModificationTime, return when the file was most
    recently modified. If \a time is \c AccessTime, return when the
    file was most recently accessed (e.g. read or written).
    If the time cannot be determined return QDateTime() (an invalid
    date time).

    This virtual function must be reimplemented by all subclasses.

    \sa setFileName(), QDateTime, QDateTime::isValid(), FileTime
 */
QDateTime QAbstractFileEngine::fileTime(FileTime time) const
{
    Q_UNUSED(time);
    return QDateTime();
}

/*!
    Sets the file engine's file name to \a file. This file name is the
    file that the rest of the virtual functions will operate on.

    This virtual function must be reimplemented by all subclasses.

    \sa rename()
 */
void QAbstractFileEngine::setFileName(const QString &file)
{
    Q_UNUSED(file);
}

/*!
    Returns the native file handle for this file engine. This handle must be
    used with care; its value and type are platform specific, and using it
    will most likely lead to non-portable code.
*/
int QAbstractFileEngine::handle() const
{
    return -1;
}

/*!
    \since 4.3

    Returns true if the current position is at the end of the file; otherwise,
    returns false.

    This function bases its behavior on calling extension() with
    AtEndExtension. If the engine does not support this extension, false is
    returned.

    \sa extension(), supportsExtension(), QFile::atEnd()
*/
bool QAbstractFileEngine::atEnd() const
{
    return const_cast<QAbstractFileEngine *>(this)->extension(AtEndExtension);
}

/*!
    \since 4.4

    Maps \a size bytes of the file into memory starting at \a offset.
    Returns a pointer to the memory if successful; otherwise returns false
    if, for example, an error occurs.

    This function bases its behavior on calling extension() with
    MapExtensionOption. If the engine does not support this extension, 0 is
    returned.

    \a flags is currently not used, but could be used in the future.

    \sa unmap(), supportsExtension()
 */

uchar *QAbstractFileEngine::map(qint64 offset, qint64 size, QFile::MemoryMapFlags flags)
{
    MapExtensionOption option;
    option.offset = offset;
    option.size = size;
    option.flags = flags;
    MapExtensionReturn r;
    if (!extension(MapExtension, &option, &r))
        return 0;
    return r.address;
}

/*!
    \since 4.4

    Unmaps the memory \a address.  Returns true if the unmap succeeds; otherwise
    returns false.

    This function bases its behavior on calling extension() with
    UnMapExtensionOption. If the engine does not support this extension, false is
    returned.

    \sa map(), supportsExtension()
 */
bool QAbstractFileEngine::unmap(uchar *address)
{
    UnMapExtensionOption options;
    options.address = address;
    return extension(UnMapExtension, &options);
}

/*!
    \since 4.3
    \class QAbstractFileEngineIterator
    \brief The QAbstractFileEngineIterator class provides an iterator
    interface for custom file engines.

    If all you want is to iterate over entries in a directory, see
    QDirIterator instead. This class is only for custom file engine authors.

    QAbstractFileEngineIterator is a unidirectional single-use virtual
    iterator that plugs into QDirIterator, providing transparent proxy
    iteration for custom file engines.

    You can subclass QAbstractFileEngineIterator to provide an iterator when
    writing your own file engine. To plug the iterator into your file system,
    you simply return an instance of this subclass from a reimplementation of
    QAbstractFileEngine::beginEntryList().

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qabstractfileengine.cpp 2

    QAbstractFileEngineIterator is associated with a path, name filters, and
    entry filters. The path is the directory that the iterator lists entries
    in. The name filters and entry filters are provided for file engines that
    can optimize directory listing at the iterator level (e.g., network file
    systems that need to minimize network traffic), but they can also be
    ignored by the iterator subclass; QAbstractFileEngineIterator already
    provides the required filtering logics in the matchesFilters() function.
    You can call dirName() to get the directory name, nameFilters() to get a
    stringlist of name filters, and filters() to get the entry filters.

    The pure virtual function hasNext() returns true if the current directory
    has at least one more entry (i.e., the directory name is valid and
    accessible, and we have not reached the end of the entry list), and false
    otherwise. Reimplement next() to seek to the next entry.

    The pure virtual function currentFileName() returns the name of the
    current entry without advancing the iterator. The currentFilePath()
    function is provided for convenience; it returns the full path of the
    current entry.

    Here is an example of how to implement an iterator that returns each of
    three fixed entries in sequence.

    \snippet doc/src/snippets/code/src_corelib_io_qabstractfileengine.cpp 3

    Note: QAbstractFileEngineIterator does not deal with QDir::IteratorFlags;
    it simply returns entries for a single directory.

    \sa QDirIterator
*/

/*!
    \enum QAbstractFileEngineIterator::EntryInfoType
    \internal

    This enum describes the different types of information that can be
    requested through the QAbstractFileEngineIterator::entryInfo() function.
*/

/*!
    \typedef QAbstractFileEngine::Iterator
    \since 4.3
    \relates QAbstractFileEngine

    Synonym for QAbstractFileEngineIterator.
*/

class QAbstractFileEngineIteratorPrivate
{
public:
    QString path;
    QDir::Filters filters;
    QStringList nameFilters;
    QFileInfo fileInfo;
};

/*!
    Constructs a QAbstractFileEngineIterator, using the entry filters \a
    filters, and wildcard name filters \a nameFilters.
*/
QAbstractFileEngineIterator::QAbstractFileEngineIterator(QDir::Filters filters,
                                                         const QStringList &nameFilters)
    : d(new QAbstractFileEngineIteratorPrivate)
{
    d->nameFilters = nameFilters;
    d->filters = filters;
}

/*!
    Destroys the QAbstractFileEngineIterator.

    \sa QDirIterator
*/
QAbstractFileEngineIterator::~QAbstractFileEngineIterator()
{
}

/*!
    Returns the path for this iterator. QDirIterator is responsible for
    assigning this path; it cannot change during the iterator's lifetime.

    \sa nameFilters(), filters()
*/
QString QAbstractFileEngineIterator::path() const
{
    return d->path;
}

/*!
    \internal

    Sets the iterator path to \a path. This function is called from within
    QDirIterator.
*/
void QAbstractFileEngineIterator::setPath(const QString &path)
{
    d->path = path;
}

/*!
    Returns the name filters for this iterator.

    \sa QDir::nameFilters(), filters(), path()
*/
QStringList QAbstractFileEngineIterator::nameFilters() const
{
    return d->nameFilters;
}

/*!
    Returns the entry filters for this iterator.

    \sa QDir::filter(), nameFilters(), path()
*/
QDir::Filters QAbstractFileEngineIterator::filters() const
{
    return d->filters;
}

/*!
    \fn QString QAbstractFileEngineIterator::currentFileName() const = 0

    This pure virtual function returns the name of the current directory
    entry, excluding the path.

    \sa currentFilePath()
*/

/*!
    Returns the path to the current directory entry. It's the same as
    prepending path() to the return value of currentFileName().

    \sa currentFileName()
*/
QString QAbstractFileEngineIterator::currentFilePath() const
{
    QString name = currentFileName();
    if (!name.isNull()) {
        QString tmp = path();
        if (!tmp.isEmpty()) {
            if (!tmp.endsWith(QLatin1Char('/')))
                tmp.append(QLatin1Char('/'));
            name.prepend(tmp);
        }
    }
    return name;
}

/*!
    The virtual function returns a QFileInfo for the current directory
    entry. This function is provided for convenience. It can also be slightly
    faster than creating a QFileInfo object yourself, as the object returned
    by this function might contain cached information that QFileInfo otherwise
    would have to access through the file engine.

    \sa currentFileName()
*/
QFileInfo QAbstractFileEngineIterator::currentFileInfo() const
{
    QString path = currentFilePath();
    if (d->fileInfo.filePath() != path)
        d->fileInfo.setFile(path);

    // return a shallow copy
    return d->fileInfo;
}

/*!
    \internal

    Returns the entry info \a type for this iterator's current directory entry
    as a QVariant. If \a type is undefined for this entry, a null QVariant is
    returned.

    \sa QAbstractFileEngine::beginEntryList(), QDir::beginEntryList()
*/
QVariant QAbstractFileEngineIterator::entryInfo(EntryInfoType type) const
{
    Q_UNUSED(type)
    return QVariant();
}

/*!
    \fn virtual QString QAbstractFileEngineIterator::next() = 0

    This pure virtual function advances the iterator to the next directory
    entry, and returns the file path to the current entry.

    This function can optionally make use of nameFilters() and filters() to
    optimize its performance.

    Reimplement this function in a subclass to advance the iterator.

    \sa QDirIterator::next()
*/

/*!
    \fn virtual bool QAbstractFileEngineIterator::hasNext() const = 0

    This pure virtual function returns true if there is at least one more
    entry in the current directory (i.e., the iterator path is valid and
    accessible, and the iterator has not reached the end of the entry list).

    \sa QDirIterator::hasNext()
*/

/*!
    Returns an instance of a QAbstractFileEngineIterator using \a filters for
    entry filtering and \a filterNames for name filtering. This function is
    called by QDirIterator to initiate directory iteration.

    QDirIterator takes ownership of the returned instance, and deletes it when
    it's done.

    \sa QDirIterator
*/
QAbstractFileEngine::Iterator *QAbstractFileEngine::beginEntryList(QDir::Filters filters, const QStringList &filterNames)
{
    Q_UNUSED(filters);
    Q_UNUSED(filterNames);
    return 0;
}

/*!
    \internal
*/
QAbstractFileEngine::Iterator *QAbstractFileEngine::endEntryList()
{
    return 0;
}

/*!
    Reads a number of characters from the file into \a data. At most
    \a maxlen characters will be read.

    Returns -1 if a fatal error occurs, or 0 if there are no bytes to
    read.
*/
qint64 QAbstractFileEngine::read(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return -1;
}

/*!
    Writes \a len bytes from \a data to the file. Returns the number
    of characters written on success; otherwise returns -1.
*/
qint64 QAbstractFileEngine::write(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1;
}

/*!
    This function reads one line, terminated by a '\n' character, from the
    file info \a data. At most \a maxlen characters will be read. The
    end-of-line character is included.
*/
qint64 QAbstractFileEngine::readLine(char *data, qint64 maxlen)
{
    qint64 readSoFar = 0;
    while (readSoFar < maxlen) {
        char c;
        qint64 readResult = read(&c, 1);
        if (readResult <= 0)
            return (readSoFar > 0) ? readSoFar : -1;
        ++readSoFar;
        *data++ = c;
        if (c == '\n')
            return readSoFar;
    }
    return readSoFar;
}

/*!
   \enum QAbstractFileEngine::Extension
   \since 4.3

   This enum describes the types of extensions that the file engine can
   support. Before using these extensions, you must verify that the extension
   is supported (i.e., call supportsExtension()).

   \value AtEndExtension Whether the current file position is at the end of
   the file or not. This extension allows file engines that implement local
   buffering to report end-of-file status without having to check the size of
   the file. It is also useful for sequential files, where the size of the
   file cannot be used to determine whether or not you have reached the end.
   This extension returns true if the file is at the end; otherwise it returns
   false. The input and output arguments to extension() are ignored.

   \value FastReadLineExtension Whether the file engine provides a
   fast implementation for readLine() or not. If readLine() remains
   unimplemented in the file engine, QAbstractFileEngine will provide
   an implementation based on calling read() repeatedly. If
   supportsExtension() returns false for this extension, however,
   QIODevice can provide a faster implementation by making use of its
   internal buffer. For engines that already provide a fast readLine()
   implementation, returning false for this extension can avoid
   unnnecessary double-buffering in QIODevice.

   \value MapExtension Whether the file engine provides the ability to map
   a file to memory.

   \value UnMapExtension Whether the file engine provides the ability to
   unmap memory that was previously mapped.
*/

/*!
   \class QAbstractFileEngine::ExtensionOption
   \since 4.3
   \brief provides an extended input argument to QAbstractFileEngine's
   extension support.

   \sa QAbstractFileEngine::extension()
*/

/*!
   \class QAbstractFileEngine::ExtensionReturn
   \since 4.3
   \brief provides an extended output argument to QAbstractFileEngine's
   extension support.

   \sa QAbstractFileEngine::extension()
*/

/*!
    \since 4.3

    This virtual function can be reimplemented in a QAbstractFileEngine
    subclass to provide support for extensions. The \a option argument is
    provided as input to the extension, and this function can store output
    results in \a output.

    The behavior of this function is determined by \a extension; see the
    Extension documentation for details.

    You can call supportsExtension() to check if an extension is supported by
    the file engine.

    By default, no extensions are supported, and this function returns false.

    \sa supportsExtension(), Extension
*/
bool QAbstractFileEngine::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    Q_UNUSED(extension);
    Q_UNUSED(option);
    Q_UNUSED(output);
    return false;
}

/*!
    \since 4.3

    This virtual function returns true if the file engine supports \a
    extension; otherwise, false is returned. By default, no extensions are
    supported.

    \sa extension()
*/
bool QAbstractFileEngine::supportsExtension(Extension extension) const
{
    Q_UNUSED(extension);
    return false;
}

/*!
  Returns the QFile::FileError that resulted from the last failed
  operation. If QFile::UnspecifiedError is returned, QFile will
  use its own idea of the error status.

  \sa QFile::FileError, errorString()
 */
QFile::FileError QAbstractFileEngine::error() const
{
    Q_D(const QAbstractFileEngine);
    return d->fileError;
}

/*!
  Returns the human-readable message appropriate to the current error
  reported by error(). If no suitable string is available, an
  empty string is returned.

  \sa error()
 */
QString QAbstractFileEngine::errorString() const
{
    Q_D(const QAbstractFileEngine);
    return d->errorString;
}

/*!
    Sets the error type to \a error, and the error string to \a errorString.
    Call this function to set the error values returned by the higher-level
    classes.

    \sa QFile::error(), QIODevice::errorString(), QIODevice::setErrorString()
*/
void QAbstractFileEngine::setError(QFile::FileError error, const QString &errorString)
{
    Q_D(QAbstractFileEngine);
    d->fileError = error;
    d->errorString = errorString;
}

QT_END_NAMESPACE
