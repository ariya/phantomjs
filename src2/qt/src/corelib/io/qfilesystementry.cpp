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

#include "qfilesystementry_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/private/qfsfileengine_p.h>
#ifdef Q_OS_WIN
#include <QtCore/qstringbuilder.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN
static bool isUncRoot(const QString &server)
{
    QString localPath = QDir::toNativeSeparators(server);
    if (!localPath.startsWith(QLatin1String("\\\\")))
        return false;

    int idx = localPath.indexOf(QLatin1Char('\\'), 2);
    if (idx == -1 || idx + 1 == localPath.length())
        return true;

    localPath = localPath.right(localPath.length() - idx - 1).trimmed();
    return localPath.isEmpty();
}

static inline QString fixIfRelativeUncPath(const QString &path)
{
    QString currentPath = QDir::currentPath();
    if (currentPath.startsWith(QLatin1String("//")))
        return currentPath % QChar(QLatin1Char('/')) % path;
    return path;
}
#endif

QFileSystemEntry::QFileSystemEntry()
    : m_lastSeparator(0),
    m_firstDotInFileName(0),
    m_lastDotInFileName(0)
{
}

/*!
   \internal
   Use this constructor when the path is supplied by user code, as it may contain a mix
   of '/' and the native separator.
 */
QFileSystemEntry::QFileSystemEntry(const QString &filePath)
    : m_filePath(QDir::fromNativeSeparators(filePath)),
    m_lastSeparator(-2),
    m_firstDotInFileName(-2),
    m_lastDotInFileName(0)
{
}

/*!
   \internal
   Use this constructor when the path is guaranteed to be in internal format, i.e. all
   directory separators are '/' and not the native separator.
 */
QFileSystemEntry::QFileSystemEntry(const QString &filePath, FromInternalPath /* dummy */)
    : m_filePath(filePath),
    m_lastSeparator(-2),
    m_firstDotInFileName(-2),
    m_lastDotInFileName(0)
{
}

/*!
   \internal
   Use this constructor when the path comes from a native API
 */
QFileSystemEntry::QFileSystemEntry(const NativePath &nativeFilePath, FromNativePath /* dummy */)
    : m_nativeFilePath(nativeFilePath),
    m_lastSeparator(-2),
    m_firstDotInFileName(-2),
    m_lastDotInFileName(0)
{
}

QFileSystemEntry::QFileSystemEntry(const QString &filePath, const NativePath &nativeFilePath)
    : m_filePath(QDir::fromNativeSeparators(filePath)),
    m_nativeFilePath(nativeFilePath),
    m_lastSeparator(-2),
    m_firstDotInFileName(-2),
    m_lastDotInFileName(0)
{
}

QString QFileSystemEntry::filePath() const
{
    resolveFilePath();
    return m_filePath;
}

QFileSystemEntry::NativePath QFileSystemEntry::nativeFilePath() const
{
    resolveNativeFilePath();
    return m_nativeFilePath;
}

void QFileSystemEntry::resolveFilePath() const
{
    if (m_filePath.isEmpty() && !m_nativeFilePath.isEmpty()) {
#if defined(QFILESYSTEMENTRY_NATIVE_PATH_IS_UTF16)
        m_filePath = QDir::fromNativeSeparators(m_nativeFilePath);
#ifdef Q_OS_WIN
        if (m_filePath.startsWith(QLatin1String("//?/UNC/")))
            m_filePath = m_filePath.remove(2,6);
        if (m_filePath.startsWith(QLatin1String("//?/")))
            m_filePath = m_filePath.remove(0,4);
#endif
#else
        m_filePath = QDir::fromNativeSeparators(QFile::decodeName(m_nativeFilePath));
#endif
    }
}

void QFileSystemEntry::resolveNativeFilePath() const
{
    if (!m_filePath.isEmpty() && m_nativeFilePath.isEmpty()) {
#ifdef Q_OS_WIN
        QString filePath = m_filePath;
        if (isRelative())
            filePath = fixIfRelativeUncPath(m_filePath);
        m_nativeFilePath = QFSFileEnginePrivate::longFileName(QDir::toNativeSeparators(filePath));
#elif defined(QFILESYSTEMENTRY_NATIVE_PATH_IS_UTF16)
        m_nativeFilePath = QDir::toNativeSeparators(m_filePath);
#else
        m_nativeFilePath = QFile::encodeName(QDir::toNativeSeparators(m_filePath));
#endif
    }
}

QString QFileSystemEntry::fileName() const
{
    findLastSeparator();
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == QLatin1Char(':'))
        return m_filePath.mid(2);
#endif
    return m_filePath.mid(m_lastSeparator + 1);
}

QString QFileSystemEntry::path() const
{
    findLastSeparator();
    if (m_lastSeparator == -1) {
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
        if (m_filePath.length() >= 2 && m_filePath.at(1) == QLatin1Char(':'))
            return m_filePath.left(2);
#endif
        return QString(QLatin1Char('.'));
    }
    if (m_lastSeparator == 0)
        return QString(QLatin1Char('/'));
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    if (m_lastSeparator == 2 && m_filePath.at(1) == QLatin1Char(':'))
        return m_filePath.left(m_lastSeparator + 1);
#endif
    return m_filePath.left(m_lastSeparator);
}

QString QFileSystemEntry::baseName() const
{
    findFileNameSeparators();
    int length = -1;
    if (m_firstDotInFileName >= 0) {
        length = m_firstDotInFileName;
        if (m_lastSeparator != -1) // avoid off by one
            length--;
    }
#if defined(Q_OS_WIN)
    if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == QLatin1Char(':'))
        return m_filePath.mid(2, length - 2);
#endif
    return m_filePath.mid(m_lastSeparator + 1, length);
}

QString QFileSystemEntry::completeBaseName() const
{
    findFileNameSeparators();
    int length = -1;
    if (m_firstDotInFileName >= 0) {
        length = m_firstDotInFileName + m_lastDotInFileName;
        if (m_lastSeparator != -1) // avoid off by one
            length--;
    }
#if defined(Q_OS_WIN)
    if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == QLatin1Char(':'))
        return m_filePath.mid(2, length - 2);
#endif
    return m_filePath.mid(m_lastSeparator + 1, length);
}

QString QFileSystemEntry::suffix() const
{
    findFileNameSeparators();

    if (m_lastDotInFileName == -1)
        return QString();

    return m_filePath.mid(qMax((qint16)0, m_lastSeparator) + m_firstDotInFileName + m_lastDotInFileName + 1);
}

QString QFileSystemEntry::completeSuffix() const
{
    findFileNameSeparators();
    if (m_firstDotInFileName == -1)
        return QString();

    return m_filePath.mid(qMax((qint16)0, m_lastSeparator) + m_firstDotInFileName + 1);
}

#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
bool QFileSystemEntry::isRelative() const
{
    resolveFilePath();
    return (m_filePath.isEmpty() || (!m_filePath.isEmpty() && (m_filePath[0].unicode() != '/')
        && (!(m_filePath.length() >= 2 && m_filePath[1].unicode() == ':'))));
}

bool QFileSystemEntry::isAbsolute() const
{
    resolveFilePath();
    return (!m_filePath.isEmpty() && ((m_filePath.length() >= 3
                                       && (m_filePath[0].isLetter() && m_filePath[1].unicode() == ':' && m_filePath[2].unicode() == '/'))
#ifdef Q_OS_WIN
                                      || (m_filePath.length() >= 2 && (m_filePath.at(0) == QLatin1Char('/') && m_filePath.at(1) == QLatin1Char('/')))
#endif
                                      ));
}
#else
bool QFileSystemEntry::isRelative() const
{
    return !isAbsolute();
}

bool QFileSystemEntry::isAbsolute() const
{
    resolveFilePath();
    return (!m_filePath.isEmpty() && (m_filePath[0].unicode() == '/'));
}
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
bool QFileSystemEntry::isDriveRoot() const
{
    resolveFilePath();
    return (m_filePath.length() == 3
           && m_filePath.at(0).isLetter() && m_filePath.at(1) == QLatin1Char(':')
           && m_filePath.at(2) == QLatin1Char('/'));
}
#endif

bool QFileSystemEntry::isRoot() const
{
    resolveFilePath();
    if (m_filePath == QLatin1String("/")
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
            || isDriveRoot()
#if defined(Q_OS_WIN)
            || isUncRoot(m_filePath)
#endif
#endif
            )
        return true;

    return false;
}

bool QFileSystemEntry::isEmpty() const
{
    resolveNativeFilePath();
    return m_nativeFilePath.isEmpty();
}

// private methods

void QFileSystemEntry::findLastSeparator() const
{
    if (m_lastSeparator == -2) {
        resolveFilePath();
        m_lastSeparator = -1;
        for (int i = m_filePath.size() - 1; i >= 0; --i) {
            if (m_filePath[i].unicode() == '/') {
                m_lastSeparator = i;
                break;
            }
        }
    }
}

void QFileSystemEntry::findFileNameSeparators() const
{
    if (m_firstDotInFileName == -2) {
        resolveFilePath();
        int firstDotInFileName = -1;
        int lastDotInFileName = -1;
        int lastSeparator = m_lastSeparator;

        int stop;
        if (lastSeparator < 0) {
            lastSeparator = -1;
            stop = 0;
        } else {
            stop = lastSeparator;
        }

        int i = m_filePath.size() - 1;
        for (; i >= stop; --i) {
            if (m_filePath[i].unicode() == '.') {
                firstDotInFileName = lastDotInFileName = i;
                break;
            } else if (m_filePath[i].unicode() == '/') {
                lastSeparator = i;
                break;
            }
        }

        if (lastSeparator != i) {
            for (--i; i >= stop; --i) {
                if (m_filePath[i].unicode() == '.')
                    firstDotInFileName = i;
                else if (m_filePath[i].unicode() == '/') {
                    lastSeparator = i;
                    break;
                }
            }
        }
        m_lastSeparator = lastSeparator;
        m_firstDotInFileName = firstDotInFileName == -1 ? -1 : firstDotInFileName - qMax(0, lastSeparator);
        if (lastDotInFileName == -1)
            m_lastDotInFileName = -1;
        else if (firstDotInFileName == lastDotInFileName)
            m_lastDotInFileName = 0;
        else
            m_lastDotInFileName = lastDotInFileName - firstDotInFileName;
    }
}

bool QFileSystemEntry::isClean() const
{
    resolveFilePath();
    int dots = 0;
    bool dotok = true; // checking for ".." or "." starts to relative paths
    bool slashok = true;
    for (QString::const_iterator iter = m_filePath.constBegin(); iter != m_filePath.constEnd(); iter++) {
        if (*iter == QLatin1Char('/')) {
            if (dots == 1 || dots == 2)
                return false; // path contains "./" or "../"
            if (!slashok)
                return false; // path contains "//"
            dots = 0;
            dotok = true;
            slashok = false;
        } else if (dotok) {
            slashok = true;
            if (*iter == QLatin1Char('.')) {
                dots++;
                if (dots > 2)
                    dotok = false;
            } else {
                //path element contains a character other than '.', it's clean
                dots = 0;
                dotok = false;
            }
        }
    }
    return (dots != 1 && dots != 2); // clean if path doesn't end in . or ..
}

QT_END_NAMESPACE
