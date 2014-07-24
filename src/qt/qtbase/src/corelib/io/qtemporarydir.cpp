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

#include "qtemporarydir.h"

#ifndef QT_NO_TEMPORARYFILE

#include "qdiriterator.h"
#include "qplatformdefs.h"
#include <QDebug>

#if defined(QT_BUILD_CORE_LIB)
#include "qcoreapplication.h"
#endif

#include <stdlib.h> // mkdtemp
#if defined(Q_OS_QNX) || defined(Q_OS_WIN) || defined(Q_OS_ANDROID)
#include <private/qfilesystemengine_p.h>
#endif

QT_BEGIN_NAMESPACE

//************* QTemporaryDirPrivate
class QTemporaryDirPrivate
{
public:
    QTemporaryDirPrivate();
    ~QTemporaryDirPrivate();

    void create(const QString &templateName);

    QString path;
    bool autoRemove;
    bool success;
};

QTemporaryDirPrivate::QTemporaryDirPrivate()
    : autoRemove(true),
      success(false)
{
}

QTemporaryDirPrivate::~QTemporaryDirPrivate()
{
}

static QString defaultTemplateName()
{
    QString baseName;
#if defined(QT_BUILD_CORE_LIB)
    baseName = QCoreApplication::applicationName();
    if (baseName.isEmpty())
#endif
        baseName = QLatin1String("qt_temp");

    return QDir::tempPath() + QLatin1Char('/') + baseName + QLatin1String("-XXXXXX");
}

#if defined(Q_OS_QNX ) || defined(Q_OS_WIN) || defined(Q_OS_ANDROID)

static int nextRand(int &v)
{
    int r = v % 62;
    v /= 62;
    if (v < 62)
        v = qrand();
    return r;
}

static char *q_mkdtemp(char *templateName)
{
    static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    const size_t length = strlen(templateName);

    char *XXXXXX = templateName + length - 6;

    if ((length < 6u) || strncmp(XXXXXX, "XXXXXX", 6))
        return 0;

    for (int i = 0; i < 256; ++i) {
        int v = qrand();

        /* Fill in the random bits.  */
        XXXXXX[0] = letters[nextRand(v)];
        XXXXXX[1] = letters[nextRand(v)];
        XXXXXX[2] = letters[nextRand(v)];
        XXXXXX[3] = letters[nextRand(v)];
        XXXXXX[4] = letters[nextRand(v)];
        XXXXXX[5] = letters[v % 62];

        QString templateNameStr = QFile::decodeName(templateName);

        QFileSystemEntry fileSystemEntry(templateNameStr);
        if (QFileSystemEngine::createDirectory(fileSystemEntry, false)) {
            QSystemError error;
            QFileSystemEngine::setPermissions(fileSystemEntry,
                                              QFile::ReadOwner |
                                              QFile::WriteOwner |
                                              QFile::ExeOwner, error);
            if (error.error() != 0)
                continue;
            return templateName;
        }
    }
    return 0;
}

#else // defined(Q_OS_QNX ) || defined(Q_OS_WIN) || defined(Q_OS_ANDROID)

static char *q_mkdtemp(char *templateName)
{
   return mkdtemp(templateName);
}

#endif

void QTemporaryDirPrivate::create(const QString &templateName)
{
    QByteArray buffer = QFile::encodeName(templateName);
    if (!buffer.endsWith("XXXXXX"))
        buffer += "XXXXXX";
    if (q_mkdtemp(buffer.data())) { // modifies buffer
        success = true;
        path = QFile::decodeName(buffer.constData());
    }
}

//************* QTemporaryDir

/*!
    \class QTemporaryDir
    \inmodule QtCore
    \reentrant
    \brief The QTemporaryDir class creates a unique directory for temporary use.

    \ingroup io


    QTemporaryDir is used to create unique temporary dirs safely.
    The dir itself is created by the constructor. The name of the
    temporary directory is guaranteed to be unique (i.e., you are
    guaranteed to not overwrite an existing dir), and the directory will
    subsequently be removed upon destruction of the QTemporaryDir
    object. The directory name is either auto-generated, or created based
    on a template, which is passed to QTemporaryDir's constructor.

    Example:

    \snippet code/src_corelib_io_qtemporarydir.cpp 0

    It is very important to test that the temporary directory could be
    created, using isValid(). Do not use exists(), since a default-constructed
    QDir represents the current directory, which exists.

    The path to the temporary dir can be found by calling path().

    A temporary directory will have some static part of the name and some
    part that is calculated to be unique. The default path will be
    determined from QCoreApplication::applicationName() (otherwise \c qt_temp) and will
    be placed into the temporary path as returned by QDir::tempPath().
    If you specify your own path, a relative path will not be placed in the
    temporary directory by default, but be relative to the current working directory.
    In all cases, a random string will be appended to the path in order to make it unique.

    \sa QDir::tempPath(), QDir, QTemporaryFile
*/

/*!
    Constructs a QTemporaryDir using as template the application name
    returned by QCoreApplication::applicationName() (otherwise \c qt_temp).
    The directory is stored in the system's temporary directory, QDir::tempPath().

    \sa QDir::tempPath()
*/
QTemporaryDir::QTemporaryDir()
    : d_ptr(new QTemporaryDirPrivate)
{
    d_ptr->create(defaultTemplateName());
}

/*!
    Constructs a QTemporaryFile with a template name of \a templateName.

    If \a templateName is a relative path, the path will be relative to the
    current working directory. You can use QDir::tempPath() to construct \a
    templateName if you want use the system's temporary directory.

    If the \a templateName ends with XXXXXX it will be used as the dynamic portion
    of the directory name, otherwise it will be appended.
    Unlike QTemporaryFile, XXXXXX in the middle of the template string is not supported.

    \sa QDir::tempPath()
*/
QTemporaryDir::QTemporaryDir(const QString &templateName)
    : d_ptr(new QTemporaryDirPrivate)
{
    if (templateName.isEmpty())
        d_ptr->create(defaultTemplateName());
    else
        d_ptr->create(templateName);
}

/*!
    Destroys the temporary directory object.
    If auto remove mode was set, it will automatically delete the directory
    including all its contents.

    \sa autoRemove()
*/
QTemporaryDir::~QTemporaryDir()
{
    if (d_ptr->autoRemove)
        remove();
}

/*!
   Returns \c true if the QTemporaryDir was created successfully.
*/
bool QTemporaryDir::isValid() const
{
    return d_ptr->success;
}

/*!
   Returns the path to the temporary directory.
   Empty if the QTemporaryDir could not be created.
*/
QString QTemporaryDir::path() const
{
    return d_ptr->path;
}

/*!
   Returns \c true if the QTemporaryDir is in auto remove
   mode. Auto-remove mode will automatically delete the directory from
   disk upon destruction. This makes it very easy to create your
   QTemporaryDir object on the stack, fill it with files, do something with
   the files, and finally on function return it will automatically clean up
   after itself.

   Auto-remove is on by default.

   \sa setAutoRemove(), remove()
*/
bool QTemporaryDir::autoRemove() const
{
    return d_ptr->autoRemove;
}

/*!
    Sets the QTemporaryDir into auto-remove mode if \a b is true.

    Auto-remove is on by default.

    \sa autoRemove(), remove()
*/
void QTemporaryDir::setAutoRemove(bool b)
{
    d_ptr->autoRemove = b;
}

/*!
    Removes the temporary directory, including all its contents.

    Returns \c true if removing was successful.
*/
bool QTemporaryDir::remove()
{
    if (!d_ptr->success)
        return false;
    Q_ASSERT(!path().isEmpty());
    Q_ASSERT(path() != QLatin1String("."));

    const bool result = QDir(path()).removeRecursively();
    if (!result) {
        qWarning() << "QTemporaryDir: Unable to remove"
                   << QDir::toNativeSeparators(path())
                   << "most likely due to the presence of read-only files.";
    }
    return result;
}

QT_END_NAMESPACE

#endif // QT_NO_TEMPORARYFILE
