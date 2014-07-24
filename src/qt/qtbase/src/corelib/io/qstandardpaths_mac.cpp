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

#include "qstandardpaths.h"
#include <qdir.h>
#include <private/qcore_mac_p.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>

QT_BEGIN_NAMESPACE

/*
    Translates a QStandardPaths::StandardLocation into the mac equivalent.
*/
OSType translateLocation(QStandardPaths::StandardLocation type)
{
    switch (type) {
    case QStandardPaths::ConfigLocation:
    case QStandardPaths::GenericConfigLocation:
        return kPreferencesFolderType;
    case QStandardPaths::DesktopLocation:
        return kDesktopFolderType;
    case QStandardPaths::DownloadLocation: // needs NSSearchPathForDirectoriesInDomains with NSDownloadsDirectory
                                           // which needs an objective-C *.mm file...
    case QStandardPaths::DocumentsLocation:
        return kDocumentsFolderType;
    case QStandardPaths::FontsLocation:
        // There are at least two different font directories on the mac: /Library/Fonts and ~/Library/Fonts.
        // To select a specific one we have to specify a different first parameter when calling FSFindFolder.
        return kFontsFolderType;
    case QStandardPaths::ApplicationsLocation:
        return kApplicationsFolderType;
    case QStandardPaths::MusicLocation:
        return kMusicDocumentsFolderType;
    case QStandardPaths::MoviesLocation:
        return kMovieDocumentsFolderType;
    case QStandardPaths::PicturesLocation:
        return kPictureDocumentsFolderType;
    case QStandardPaths::TempLocation:
        return kTemporaryFolderType;
    case QStandardPaths::GenericDataLocation:
    case QStandardPaths::RuntimeLocation:
    case QStandardPaths::DataLocation:
        return kApplicationSupportFolderType;
    case QStandardPaths::GenericCacheLocation:
    case QStandardPaths::CacheLocation:
        return kCachedDataFolderType;
    default:
        return kDesktopFolderType;
    }
}

/*
    Constructs a full unicode path from a FSRef.
*/
static QString getFullPath(const FSRef &ref)
{
    QByteArray ba(2048, 0);
    if (FSRefMakePath(&ref, reinterpret_cast<UInt8 *>(ba.data()), ba.size()) == noErr)
        return QString::fromUtf8(ba.constData()).normalized(QString::NormalizationForm_C);
    return QString();
}

static void appendOrganizationAndApp(QString &path)
{
#ifndef QT_BOOTSTRAPPED
    const QString org = QCoreApplication::organizationName();
    if (!org.isEmpty())
        path += QLatin1Char('/') + org;
    const QString appName = QCoreApplication::applicationName();
    if (!appName.isEmpty())
        path += QLatin1Char('/') + appName;
#else
    Q_UNUSED(path);
#endif
}

static QString macLocation(QStandardPaths::StandardLocation type, short domain)
{
    // http://developer.apple.com/documentation/Carbon/Reference/Folder_Manager/Reference/reference.html
    FSRef ref;
    OSErr err = FSFindFolder(domain, translateLocation(type), false, &ref);
    if (err)
       return QString();

   QString path = getFullPath(ref);

    if (type == QStandardPaths::DataLocation || type == QStandardPaths::CacheLocation)
        appendOrganizationAndApp(path);
    return path;
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    if (isTestModeEnabled()) {
        const QString qttestDir = QDir::homePath() + QLatin1String("/.qttest");
        QString path;
        switch (type) {
        case GenericDataLocation:
        case DataLocation:
            path = qttestDir + QLatin1String("/Application Support");
            if (type == DataLocation)
                appendOrganizationAndApp(path);
            return path;
        case GenericCacheLocation:
        case CacheLocation:
            path = qttestDir + QLatin1String("/Cache");
            if (type == CacheLocation)
                appendOrganizationAndApp(path);
            return path;
        case GenericConfigLocation:
        case ConfigLocation:
            return qttestDir + QLatin1String("/Preferences");
        default:
            break;
        }
    }

    switch (type) {
    case HomeLocation:
        return QDir::homePath();
    case TempLocation:
        return QDir::tempPath();
    case GenericDataLocation:
    case DataLocation:
    case GenericCacheLocation:
    case CacheLocation:
    case RuntimeLocation:
        return macLocation(type, kUserDomain);
    default:
        return macLocation(type, kOnAppropriateDisk);
    }
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;

    if (type == GenericDataLocation || type == DataLocation || type == GenericCacheLocation || type == CacheLocation) {
        const QString path = macLocation(type, kOnAppropriateDisk);
        if (!path.isEmpty())
            dirs.append(path);
    }

    if (type == DataLocation) {
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (mainBundle) {
            CFURLRef bundleUrl = CFBundleCopyBundleURL(mainBundle);
            CFStringRef cfBundlePath = CFURLCopyPath(bundleUrl);
            QString bundlePath = QCFString::toQString(cfBundlePath);
            CFRelease(cfBundlePath);
            CFRelease(bundleUrl);

            CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(mainBundle);
            CFStringRef cfResourcesPath = CFURLCopyPath(bundleUrl);
            QString resourcesPath = QCFString::toQString(cfResourcesPath);
            CFRelease(cfResourcesPath);
            CFRelease(resourcesUrl);

            // Handle bundled vs unbundled executables. CFBundleGetMainBundle() returns
            // a valid bundle in both cases. CFBundleCopyResourcesDirectoryURL() returns
            // an absolute path for unbundled executables.
            if (resourcesPath.startsWith(QLatin1Char('/')))
                dirs.append(resourcesPath);
            else
                dirs.append(bundlePath + resourcesPath);
        }
    }
    const QString localDir = writableLocation(type);
    dirs.prepend(localDir);
    return dirs;
}

#ifndef QT_BOOTSTRAPPED
QString QStandardPaths::displayName(StandardLocation type)
{
    if (QStandardPaths::HomeLocation == type)
        return QCoreApplication::translate("QStandardPaths", "Home");

    FSRef ref;
    OSErr err = FSFindFolder(kOnAppropriateDisk, translateLocation(type), false, &ref);
    if (err)
        return QString();

    QCFString displayName;
    err = LSCopyDisplayNameForRef(&ref, &displayName);
    if (err)
        return QString();

    return static_cast<QString>(displayName);
}
#endif

QT_END_NAMESPACE
