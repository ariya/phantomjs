/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
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

#include "qstorageinfo_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qvarlengtharray.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

static const int defaultBufferSize = MAX_PATH + 1;

void QStorageInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();

    if (rootPath.isEmpty())
        return;

    QString path = QDir::toNativeSeparators(rootPath);
    rootPath.clear();

    if (path.startsWith(QLatin1String("\\\\?\\")))
        path.remove(0, 4);
    if (path.length() < 2 || path.at(1) != QLatin1Char(':'))
        return;
    path[0] = path[0].toUpper();
    if (!(path.at(0).unicode() >= 'A' && path.at(0).unicode() <= 'Z'))
        return;
    if (!path.endsWith(QLatin1Char('\\')))
        path.append(QLatin1Char('\\'));

    // ### test if disk mounted to folder on other disk
    wchar_t buffer[defaultBufferSize];
    if (::GetVolumePathName(reinterpret_cast<const wchar_t *>(path.utf16()), buffer, defaultBufferSize))
        rootPath = QDir::fromNativeSeparators(QString::fromWCharArray(buffer));
}

static inline QByteArray getDevice(const QString &rootPath)
{
    const QString path = QDir::toNativeSeparators(rootPath);
    const UINT type = ::GetDriveType(reinterpret_cast<const wchar_t *>(path.utf16()));
    if (type == DRIVE_REMOTE) {
        QVarLengthArray<char, 256> buffer(256);
        DWORD bufferLength = buffer.size();
        DWORD result;
        UNIVERSAL_NAME_INFO *remoteNameInfo;
        do {
            buffer.resize(bufferLength);
            remoteNameInfo = reinterpret_cast<UNIVERSAL_NAME_INFO *>(buffer.data());
            result = ::WNetGetUniversalName(reinterpret_cast<const wchar_t *>(path.utf16()),
                                            UNIVERSAL_NAME_INFO_LEVEL,
                                            remoteNameInfo,
                                            &bufferLength);
        } while (result == ERROR_MORE_DATA);
        if (result == NO_ERROR)
            return QString::fromWCharArray(remoteNameInfo->lpUniversalName).toUtf8();
        return QByteArray();
    }

    wchar_t deviceBuffer[51];
    if (::GetVolumeNameForVolumeMountPoint(reinterpret_cast<const wchar_t *>(path.utf16()),
                                           deviceBuffer,
                                           sizeof(deviceBuffer) / sizeof(wchar_t))) {
        return QString::fromWCharArray(deviceBuffer).toLatin1();
    }
    return QByteArray();
}

void QStorageInfoPrivate::doStat()
{
    initRootPath();
    if (rootPath.isEmpty())
        return;

    retreiveVolumeInfo();
    device = getDevice(rootPath);
    retreiveDiskFreeSpace();
}

void QStorageInfoPrivate::retreiveVolumeInfo()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    wchar_t nameBuffer[defaultBufferSize];
    wchar_t fileSystemTypeBuffer[defaultBufferSize];
    DWORD fileSystemFlags = 0;
    const bool result = ::GetVolumeInformation(reinterpret_cast<const wchar_t *>(path.utf16()),
                                               nameBuffer,
                                               defaultBufferSize,
                                               Q_NULLPTR,
                                               Q_NULLPTR,
                                               &fileSystemFlags,
                                               fileSystemTypeBuffer,
                                               defaultBufferSize);
    if (!result) {
        ready = false;
        valid = ::GetLastError() == ERROR_NOT_READY;
    } else {
        ready = true;
        valid = true;

        fileSystemType = QString::fromWCharArray(fileSystemTypeBuffer).toLatin1();
        name = QString::fromWCharArray(nameBuffer);

        readOnly = (fileSystemFlags & FILE_READ_ONLY_VOLUME) != 0;
    }

    ::SetErrorMode(oldmode);
}

void QStorageInfoPrivate::retreiveDiskFreeSpace()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    ::GetDiskFreeSpaceEx(reinterpret_cast<const wchar_t *>(path.utf16()),
                         PULARGE_INTEGER(&bytesAvailable),
                         PULARGE_INTEGER(&bytesTotal),
                         PULARGE_INTEGER(&bytesFree));

    ::SetErrorMode(oldmode);
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    QList<QStorageInfo> volumes;

    QString driveName = QStringLiteral("A:/");
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    quint32 driveBits = quint32(::GetLogicalDrives()) & 0x3ffffff;
    ::SetErrorMode(oldmode);
    while (driveBits) {
        if (driveBits & 1) {
            QStorageInfo drive(driveName);
            if (!drive.rootPath().isEmpty()) // drive exists, but not mounted
                volumes.append(drive);
        }
        driveName[0] = driveName[0].unicode() + 1;
        driveBits = driveBits >> 1;
    }

    return volumes;
}

QStorageInfo QStorageInfoPrivate::root()
{
    return QStorageInfo(QDir::fromNativeSeparators(QFile::decodeName(qgetenv("SystemDrive"))));
}

QT_END_NAMESPACE
