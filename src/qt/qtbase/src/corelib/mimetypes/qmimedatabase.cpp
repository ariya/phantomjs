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

#include <qplatformdefs.h> // always first

#include "qmimedatabase.h"

#include "qmimedatabase_p.h"

#include "qmimeprovider_p.h"
#include "qmimetype_p.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QUrl>
#include <QtCore/QStack>
#include <QtCore/QDebug>

#include <algorithm>
#include <functional>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QMimeDatabasePrivate, staticQMimeDatabase)

QMimeDatabasePrivate *QMimeDatabasePrivate::instance()
{
    return staticQMimeDatabase();
}

QMimeDatabasePrivate::QMimeDatabasePrivate()
    : m_provider(0), m_defaultMimeType(QLatin1String("application/octet-stream"))
{
}

QMimeDatabasePrivate::~QMimeDatabasePrivate()
{
    delete m_provider;
    m_provider = 0;
}

QMimeProviderBase *QMimeDatabasePrivate::provider()
{
    if (!m_provider) {
        QMimeProviderBase *binaryProvider = new QMimeBinaryProvider(this);
        if (binaryProvider->isValid()) {
            m_provider = binaryProvider;
        } else {
            delete binaryProvider;
            m_provider = new QMimeXMLProvider(this);
        }
    }
    return m_provider;
}

void QMimeDatabasePrivate::setProvider(QMimeProviderBase *theProvider)
{
    delete m_provider;
    m_provider = theProvider;
}

/*!
    \internal
    Returns a MIME type or an invalid one if none found
 */
QMimeType QMimeDatabasePrivate::mimeTypeForName(const QString &nameOrAlias)
{
    return provider()->mimeTypeForName(provider()->resolveAlias(nameOrAlias));
}

QStringList QMimeDatabasePrivate::mimeTypeForFileName(const QString &fileName, QString *foundSuffix)
{
    if (fileName.endsWith(QLatin1Char('/')))
        return QStringList() << QLatin1String("inode/directory");

    const QStringList matchingMimeTypes = provider()->findByFileName(QFileInfo(fileName).fileName(), foundSuffix);
    return matchingMimeTypes;
}

static inline bool isTextFile(const QByteArray &data)
{
    // UTF16 byte order marks
    static const char bigEndianBOM[] = "\xFE\xFF";
    static const char littleEndianBOM[] = "\xFF\xFE";
    if (data.startsWith(bigEndianBOM) || data.startsWith(littleEndianBOM))
        return true;

    // Check the first 32 bytes (see shared-mime spec)
    const char *p = data.constData();
    const char *e = p + qMin(32, data.size());
    for ( ; p < e; ++p) {
        if ((unsigned char)(*p) < 32 && *p != 9 && *p !=10 && *p != 13)
            return false;
    }

    return true;
}

QMimeType QMimeDatabasePrivate::findByData(const QByteArray &data, int *accuracyPtr)
{
    if (data.isEmpty()) {
        *accuracyPtr = 100;
        return mimeTypeForName(QLatin1String("application/x-zerosize"));
    }

    *accuracyPtr = 0;
    QMimeType candidate = provider()->findByMagic(data, accuracyPtr);

    if (candidate.isValid())
        return candidate;

    if (isTextFile(data)) {
        *accuracyPtr = 5;
        return mimeTypeForName(QLatin1String("text/plain"));
    }

    return mimeTypeForName(defaultMimeType());
}

QMimeType QMimeDatabasePrivate::mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device, int *accuracyPtr)
{
    // First, glob patterns are evaluated. If there is a match with max weight,
    // this one is selected and we are done. Otherwise, the file contents are
    // evaluated and the match with the highest value (either a magic priority or
    // a glob pattern weight) is selected. Matching starts from max level (most
    // specific) in both cases, even when there is already a suffix matching candidate.
    *accuracyPtr = 0;

    // Pass 1) Try to match on the file name
    QStringList candidatesByName = mimeTypeForFileName(fileName);
    if (candidatesByName.count() == 1) {
        *accuracyPtr = 100;
        const QMimeType mime = mimeTypeForName(candidatesByName.at(0));
        if (mime.isValid())
            return mime;
        candidatesByName.clear();
    }

    // Extension is unknown, or matches multiple mimetypes.
    // Pass 2) Match on content, if we can read the data
    if (device->isOpen()) {

        // Read 16K in one go (QIODEVICE_BUFFERSIZE in qiodevice_p.h).
        // This is much faster than seeking back and forth into QIODevice.
        const QByteArray data = device->peek(16384);

        int magicAccuracy = 0;
        QMimeType candidateByData(findByData(data, &magicAccuracy));

        // Disambiguate conflicting extensions (if magic matching found something)
        if (candidateByData.isValid() && magicAccuracy > 0) {
            // "for glob_match in glob_matches:"
            // "if glob_match is subclass or equal to sniffed_type, use glob_match"
            const QString sniffedMime = candidateByData.name();
            foreach (const QString &m, candidatesByName) {
                if (inherits(m, sniffedMime)) {
                    // We have magic + pattern pointing to this, so it's a pretty good match
                    *accuracyPtr = 100;
                    return mimeTypeForName(m);
                }
            }
            *accuracyPtr = magicAccuracy;
            return candidateByData;
        }
    }

    if (candidatesByName.count() > 1) {
        *accuracyPtr = 20;
        candidatesByName.sort(); // to make it deterministic
        const QMimeType mime = mimeTypeForName(candidatesByName.at(0));
        if (mime.isValid())
            return mime;
    }

    return mimeTypeForName(defaultMimeType());
}

QList<QMimeType> QMimeDatabasePrivate::allMimeTypes()
{
    return provider()->allMimeTypes();
}

bool QMimeDatabasePrivate::inherits(const QString &mime, const QString &parent)
{
    const QString resolvedParent = provider()->resolveAlias(parent);
    //Q_ASSERT(provider()->resolveAlias(mime) == mime);
    QStack<QString> toCheck;
    toCheck.push(mime);
    while (!toCheck.isEmpty()) {
        const QString current = toCheck.pop();
        if (current == resolvedParent)
            return true;
        foreach (const QString &par, provider()->parents(current))
            toCheck.push(par);
    }
    return false;
}

/*!
    \class QMimeDatabase
    \inmodule QtCore
    \brief The QMimeDatabase class maintains a database of MIME types.

    \since 5.0

    The MIME type database is provided by the freedesktop.org shared-mime-info
    project. If the MIME type database cannot be found on the system, as is the case
    on most Windows and Mac OS X systems, Qt will use its own copy of it.

    Applications which want to define custom MIME types need to install an
    XML file into the locations searched for MIME definitions.
    These locations can be queried with
    \code
    QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime/packages"),
                              QStandardPaths::LocateDirectory);
    \endcode
    On a typical Unix system, this will be /usr/share/mime/packages/, but it is also
    possible to extend the list of directories by setting the environment variable
    XDG_DATA_DIRS. For instance adding /opt/myapp/share to XDG_DATA_DIRS will result
    in /opt/myapp/share/mime/packages/ being searched for MIME definitions.

    Here is an example of MIME XML:
    \code
    <?xml version="1.0" encoding="UTF-8"?>
    <mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
      <mime-type type="application/vnd.nokia.qt.qmakeprofile">
        <comment xml:lang="en">Qt qmake Profile</comment>
        <glob pattern="*.pro" weight="50"/>
      </mime-type>
    </mime-info>
    \endcode

    For more details about the syntax of XML MIME definitions, including defining
    "magic" in order to detect MIME types based on data as well, read the
    Shared Mime Info specification at
    http://standards.freedesktop.org/shared-mime-info-spec/shared-mime-info-spec-latest.html

    On Unix systems, a binary cache is used for more performance. This cache is generated
    by the command "update-mime-database path", where path would be /opt/myapp/share/mime
    in the above example. Make sure to run this command when installing the MIME type
    definition file.

    \threadsafe

    \snippet code/src_corelib_mimetype_qmimedatabase.cpp 0

    \sa QMimeType
 */

/*!
    \fn QMimeDatabase::QMimeDatabase();
    Constructs a QMimeDatabase object.

    It is perfectly OK to create an instance of QMimeDatabase every time you need to
    perform a lookup.
    The parsing of mimetypes is done on demand (when shared-mime-info is installed)
    or when the very first instance is constructed (when parsing XML files directly).
 */
QMimeDatabase::QMimeDatabase() :
        d(staticQMimeDatabase())
{
}

/*!
    \fn QMimeDatabase::~QMimeDatabase();
    Destroys the QMimeDatabase object.
 */
QMimeDatabase::~QMimeDatabase()
{
    d = 0;
}

/*!
    \fn QMimeType QMimeDatabase::mimeTypeForName(const QString &nameOrAlias) const;
    Returns a MIME type for \a nameOrAlias or an invalid one if none found.
 */
QMimeType QMimeDatabase::mimeTypeForName(const QString &nameOrAlias) const
{
    QMutexLocker locker(&d->mutex);

    return d->mimeTypeForName(nameOrAlias);
}

/*!
    Returns a MIME type for \a fileInfo.

    A valid MIME type is always returned.

    The default matching algorithm looks at both the file name and the file
    contents, if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
    If \a fileInfo is a Unix symbolic link, the file that it refers to
    will be used instead.
    If the file doesn't match any known pattern or data, the default MIME type
    (application/octet-stream) is returned.

    When \a mode is set to MatchExtension, only the file name is used, not
    the file contents. The file doesn't even have to exist. If the file name
    doesn't match any known pattern, the default MIME type (application/octet-stream)
    is returned.
    If multiple MIME types match this file, the first one (alphabetically) is returned.

    When \a mode is set to MatchContent, and the file is readable, only the
    file contents are used to determine the MIME type. This is equivalent to
    calling mimeTypeForData with a QFile as input device.

    \a fileInfo may refer to an absolute or relative path.

    \sa QMimeType::isDefault(), mimeTypeForData()
*/
QMimeType QMimeDatabase::mimeTypeForFile(const QFileInfo &fileInfo, MatchMode mode) const
{
    QMutexLocker locker(&d->mutex);

    if (fileInfo.isDir())
        return d->mimeTypeForName(QLatin1String("inode/directory"));

    QFile file(fileInfo.absoluteFilePath());

#ifdef Q_OS_UNIX
    // Cannot access statBuf.st_mode from the filesystem engine, so we have to stat again.
    const QByteArray nativeFilePath = QFile::encodeName(file.fileName());
    QT_STATBUF statBuffer;
    if (QT_LSTAT(nativeFilePath.constData(), &statBuffer) == 0) {
        if (S_ISCHR(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/chardevice"));
        if (S_ISBLK(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/blockdevice"));
        if (S_ISFIFO(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/fifo"));
        if (S_ISSOCK(statBuffer.st_mode))
            return d->mimeTypeForName(QLatin1String("inode/socket"));
    }
#endif

    int priority = 0;
    switch (mode) {
    case MatchDefault:
        file.open(QIODevice::ReadOnly); // isOpen() will be tested by method below
        return d->mimeTypeForFileNameAndData(fileInfo.absoluteFilePath(), &file, &priority);
    case MatchExtension:
        locker.unlock();
        return mimeTypeForFile(fileInfo.absoluteFilePath(), mode);
    case MatchContent:
        if (file.open(QIODevice::ReadOnly)) {
            locker.unlock();
            return mimeTypeForData(&file);
        } else {
            return d->mimeTypeForName(d->defaultMimeType());
        }
    default:
        Q_ASSERT(false);
    }
    return d->mimeTypeForName(d->defaultMimeType());
}

/*!
    Returns a MIME type for the file named \a fileName using \a mode.

    \overload
*/
QMimeType QMimeDatabase::mimeTypeForFile(const QString &fileName, MatchMode mode) const
{
    if (mode == MatchExtension) {
        QMutexLocker locker(&d->mutex);
        QStringList matches = d->mimeTypeForFileName(fileName);
        const int matchCount = matches.count();
        if (matchCount == 0) {
            return d->mimeTypeForName(d->defaultMimeType());
        } else if (matchCount == 1) {
            return d->mimeTypeForName(matches.first());
        } else {
            // We have to pick one.
            matches.sort(); // Make it deterministic
            return d->mimeTypeForName(matches.first());
        }
    } else {
        // Implemented as a wrapper around mimeTypeForFile(QFileInfo), so no mutex.
        QFileInfo fileInfo(fileName);
        return mimeTypeForFile(fileInfo, mode);
    }
}

/*!
    Returns the MIME types for the file name \a fileName.

    If the file name doesn't match any known pattern, an empty list is returned.
    If multiple MIME types match this file, they are all returned.

    This function does not try to open the file. To also use the content
    when determining the MIME type, use mimeTypeForFile() or
    mimeTypeForFileNameAndData() instead.

    \sa mimeTypeForFile()
*/
QList<QMimeType> QMimeDatabase::mimeTypesForFileName(const QString &fileName) const
{
    QMutexLocker locker(&d->mutex);

    QStringList matches = d->mimeTypeForFileName(fileName);
    QList<QMimeType> mimes;
    matches.sort(); // Make it deterministic
    foreach (const QString &mime, matches)
        mimes.append(d->mimeTypeForName(mime));
    return mimes;
}
/*!
    Returns the suffix for the file \a fileName, as known by the MIME database.

    This allows to pre-select "tar.bz2" for foo.tar.bz2, but still only
    "txt" for my.file.with.dots.txt.
*/
QString QMimeDatabase::suffixForFileName(const QString &fileName) const
{
    QMutexLocker locker(&d->mutex);
    QString foundSuffix;
    d->mimeTypeForFileName(fileName, &foundSuffix);
    return foundSuffix;
}

/*!
    Returns a MIME type for \a data.

    A valid MIME type is always returned. If \a data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
QMimeType QMimeDatabase::mimeTypeForData(const QByteArray &data) const
{
    QMutexLocker locker(&d->mutex);

    int accuracy = 0;
    return d->findByData(data, &accuracy);
}

/*!
    Returns a MIME type for the data in \a device.

    A valid MIME type is always returned. If the data in \a device doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
QMimeType QMimeDatabase::mimeTypeForData(QIODevice *device) const
{
    QMutexLocker locker(&d->mutex);

    int accuracy = 0;
    const bool openedByUs = !device->isOpen() && device->open(QIODevice::ReadOnly);
    if (device->isOpen()) {
        // Read 16K in one go (QIODEVICE_BUFFERSIZE in qiodevice_p.h).
        // This is much faster than seeking back and forth into QIODevice.
        const QByteArray data = device->peek(16384);
        const QMimeType result = d->findByData(data, &accuracy);
        if (openedByUs)
            device->close();
        return result;
    }
    return d->mimeTypeForName(d->defaultMimeType());
}

/*!
    Returns a MIME type for \a url.

    If the URL is a local file, this calls mimeTypeForFile.

    Otherwise the matching is done based on the file name only,
    except for schemes where file names don't mean much, like HTTP.
    This method always returns the default mimetype for HTTP URLs,
    use QNetworkAccessManager to handle HTTP URLs properly.

    A valid MIME type is always returned. If \a url doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.
*/
QMimeType QMimeDatabase::mimeTypeForUrl(const QUrl &url) const
{
    if (url.isLocalFile())
        return mimeTypeForFile(url.toLocalFile());

    const QString scheme = url.scheme();
    if (scheme.startsWith(QLatin1String("http")))
        return mimeTypeForName(d->defaultMimeType());

    return mimeTypeForFile(url.path());
}

/*!
    Returns a MIME type for the given \a fileName and \a device data.

    This overload can be useful when the file is remote, and we started to
    download some of its data in a device. This allows to do full MIME type
    matching for remote files as well.

    If the device is not open, it will be opened by this function, and closed
    after the MIME type detection is completed.

    A valid MIME type is always returned. If \a device data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.

    This method looks at both the file name and the file contents,
    if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
*/
QMimeType QMimeDatabase::mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device) const
{
    int accuracy = 0;
    const bool openedByUs = !device->isOpen() && device->open(QIODevice::ReadOnly);
    const QMimeType result = d->mimeTypeForFileNameAndData(fileName, device, &accuracy);
    if (openedByUs)
        device->close();
    return result;
}

/*!
    Returns a MIME type for the given \a fileName and device \a data.

    This overload can be useful when the file is remote, and we started to
    download some of its data. This allows to do full MIME type matching for
    remote files as well.

    A valid MIME type is always returned. If \a data doesn't match any
    known MIME type data, the default MIME type (application/octet-stream)
    is returned.

    This method looks at both the file name and the file contents,
    if necessary. The file extension has priority over the contents,
    but the contents will be used if the file extension is unknown, or
    matches multiple MIME types.
*/
QMimeType QMimeDatabase::mimeTypeForFileNameAndData(const QString &fileName, const QByteArray &data) const
{
    QBuffer buffer(const_cast<QByteArray *>(&data));
    buffer.open(QIODevice::ReadOnly);
    int accuracy = 0;
    return d->mimeTypeForFileNameAndData(fileName, &buffer, &accuracy);
}

/*!
    Returns the list of all available MIME types.

    This can be useful for showing all MIME types to the user, for instance
    in a MIME type editor. Do not use unless really necessary in other cases
    though, prefer using the mimeTypeFor* methods for performance reasons.
*/
QList<QMimeType> QMimeDatabase::allMimeTypes() const
{
    QMutexLocker locker(&d->mutex);

    return d->allMimeTypes();
}

/*!
    \enum QMimeDatabase::MatchMode

    This enum specifies how matching a file to a MIME type is performed.

    \value MatchDefault Both the file name and content are used to look for a match

    \value MatchExtension Only the file name is used to look for a match

    \value MatchContent The file content is used to look for a match
*/

QT_END_NAMESPACE
