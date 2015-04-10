/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qfileiconprovider.h"
#include "qfileiconprovider_p.h"

#include <qapplication.h>
#include <qdir.h>
#include <qpixmapcache.h>
#include <private/qfunctions_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformtheme.h>

#if defined(Q_OS_WIN)
#  include <qt_windows.h>
#  ifndef Q_OS_WINRT
#    include <commctrl.h>
#    include <objbase.h>
#  endif
#endif

#if defined(Q_OS_UNIX) && !defined(QT_NO_STYLE_GTK)
#  include <private/qgtkstyle_p_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
  \class QFileIconProvider

  \inmodule QtWidgets

  \brief The QFileIconProvider class provides file icons for the QDirModel and the QFileSystemModel classes.
*/

/*!
  \enum QFileIconProvider::IconType
  \value Computer
  \value Desktop
  \value Trashcan
  \value Network
  \value Drive
  \value Folder
  \value File
*/


/*!
    \enum QFileIconProvider::Option
    \since 5.2

    \value DontUseCustomDirectoryIcons Always use the default directory icon.
    Some platforms allow the user to set a different icon. Custom icon lookup
    cause a big performance impact over network or removable drives.
*/

QFileIconProviderPrivate::QFileIconProviderPrivate() :
    homePath(QDir::home().absolutePath())
{
}

QIcon QFileIconProviderPrivate::getIcon(QStyle::StandardPixmap name) const
{
    switch (name) {
    case QStyle::SP_FileIcon:
        if (file.isNull())
            file = QApplication::style()->standardIcon(name);
        return file;
    case QStyle::SP_FileLinkIcon:
        if (fileLink.isNull())
            fileLink = QApplication::style()->standardIcon(name);
        return fileLink;
    case QStyle::SP_DirIcon:
        if (directory.isNull())
            directory = QApplication::style()->standardIcon(name);
        return directory;
    case QStyle::SP_DirLinkIcon:
        if (directoryLink.isNull())
            directoryLink = QApplication::style()->standardIcon(name);
        return directoryLink;
    case QStyle::SP_DriveHDIcon:
        if (harddisk.isNull())
            harddisk = QApplication::style()->standardIcon(name);
        return harddisk;
    case QStyle::SP_DriveFDIcon:
        if (floppy.isNull())
            floppy = QApplication::style()->standardIcon(name);
        return floppy;
    case QStyle::SP_DriveCDIcon:
        if (cdrom.isNull())
            cdrom = QApplication::style()->standardIcon(name);
        return cdrom;
    case QStyle::SP_DriveNetIcon:
        if (network.isNull())
            network = QApplication::style()->standardIcon(name);
        return network;
    case QStyle::SP_ComputerIcon:
        if (computer.isNull())
            computer = QApplication::style()->standardIcon(name);
        return computer;
    case QStyle::SP_DesktopIcon:
        if (desktop.isNull())
            desktop = QApplication::style()->standardIcon(name);
        return desktop;
    case QStyle::SP_TrashIcon:
        if (trashcan.isNull())
            trashcan = QApplication::style()->standardIcon(name);
        return trashcan;
    case QStyle::SP_DirHomeIcon:
        if (home.isNull())
            home = QApplication::style()->standardIcon(name);
        return home;
    default:
        return QIcon();
    }
    return QIcon();
}

/*!
  Constructs a file icon provider.
*/

QFileIconProvider::QFileIconProvider()
    : d_ptr(new QFileIconProviderPrivate)
{
}

/*!
  Destroys the file icon provider.

*/

QFileIconProvider::~QFileIconProvider()
{
}

/*!
    \since 5.2
    Sets \a options that affect the icon provider.
    \sa options()
*/

void QFileIconProvider::setOptions(QFileIconProvider::Options options)
{
    Q_D(QFileIconProvider);
    d->options = options;
}

/*!
    \since 5.2
    Returns all the options that affect the icon provider.
    By default, all options are disabled.
    \sa setOptions()
*/

QFileIconProvider::Options QFileIconProvider::options() const
{
    Q_D(const QFileIconProvider);
    return d->options;
}

/*!
  Returns an icon set for the given \a type.
*/

QIcon QFileIconProvider::icon(IconType type) const
{
    Q_D(const QFileIconProvider);
    switch (type) {
    case Computer:
        return d->getIcon(QStyle::SP_ComputerIcon);
    case Desktop:
        return d->getIcon(QStyle::SP_DesktopIcon);
    case Trashcan:
        return d->getIcon(QStyle::SP_TrashIcon);
    case Network:
        return d->getIcon(QStyle::SP_DriveNetIcon);
    case Drive:
        return d->getIcon(QStyle::SP_DriveHDIcon);
    case Folder:
        return d->getIcon(QStyle::SP_DirIcon);
    case File:
        return d->getIcon(QStyle::SP_FileIcon);
    default:
        break;
    };
    return QIcon();
}

static bool isCacheable(const QFileInfo &fi)
{
    if (!fi.isFile())
        return false;

#ifdef Q_OS_WIN
    // On windows it's faster to just look at the file extensions. QTBUG-13182
    const QString fileExtension = fi.suffix();
    // Will return false for .exe, .lnk and .ico extensions
    return fileExtension.compare(QLatin1String("exe"), Qt::CaseInsensitive) &&
           fileExtension.compare(QLatin1String("lnk"), Qt::CaseInsensitive) &&
           fileExtension.compare(QLatin1String("ico"), Qt::CaseInsensitive);
#else
    return !fi.isExecutable() && !fi.isSymLink();
#endif
}

QIcon QFileIconProviderPrivate::getIcon(const QFileInfo &fi) const
{
    QIcon retIcon;
    const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
    if (!theme)
        return retIcon;

    QList<int> sizes = theme->themeHint(QPlatformTheme::IconPixmapSizes).value<QList<int> >();
    if (sizes.isEmpty())
        return retIcon;

    const QString keyBase = QLatin1String("qt_.") + fi.suffix().toUpper();

    bool cacheable = isCacheable(fi);
    if (cacheable) {
        QPixmap pixmap;
        QPixmapCache::find(keyBase + QString::number(sizes.at(0)), pixmap);
        if (!pixmap.isNull()) {
            bool iconIsComplete = true;
            retIcon.addPixmap(pixmap);
            for (int i = 1; i < sizes.count(); i++)
                if (QPixmapCache::find(keyBase + QString::number(sizes.at(i)), pixmap)) {
                    retIcon.addPixmap(pixmap);
                } else {
                    iconIsComplete = false;
                    break;
                }
            if (iconIsComplete)
                return retIcon;
        }
    }

    QPlatformTheme::IconOptions iconOptions;
    if (options & QFileIconProvider::DontUseCustomDirectoryIcons)
        iconOptions |= QPlatformTheme::DontUseCustomDirectoryIcons;

    Q_FOREACH (int size, sizes) {
        QPixmap pixmap = theme->fileIconPixmap(fi, QSizeF(size, size), iconOptions);
        if (!pixmap.isNull()) {
            retIcon.addPixmap(pixmap);
            if (cacheable)
                QPixmapCache::insert(keyBase + QString::number(size), pixmap);
        }
    }

    return retIcon;
}


/*!
  Returns an icon for the file described by \a info.
*/

QIcon QFileIconProvider::icon(const QFileInfo &info) const
{
    Q_D(const QFileIconProvider);

#if defined(Q_OS_UNIX) && !defined(QT_NO_STYLE_GTK)
    const QByteArray desktopEnvironment = QGuiApplicationPrivate::platformIntegration()->services()->desktopEnvironment();
    if (desktopEnvironment != QByteArrayLiteral("KDE")) {
        QIcon gtkIcon = QGtkStylePrivate::getFilesystemIcon(info);
        if (!gtkIcon.isNull())
            return gtkIcon;
    }
#endif

    QIcon retIcon = d->getIcon(info);
    if (!retIcon.isNull())
        return retIcon;

    if (info.isRoot())
#if defined (Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    {
        UINT type = GetDriveType((wchar_t *)info.absoluteFilePath().utf16());

        switch (type) {
        case DRIVE_REMOVABLE:
            return d->getIcon(QStyle::SP_DriveFDIcon);
        case DRIVE_FIXED:
            return d->getIcon(QStyle::SP_DriveHDIcon);
        case DRIVE_REMOTE:
            return d->getIcon(QStyle::SP_DriveNetIcon);
        case DRIVE_CDROM:
            return d->getIcon(QStyle::SP_DriveCDIcon);
        case DRIVE_RAMDISK:
        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
        default:
            return d->getIcon(QStyle::SP_DriveHDIcon);
        }
    }
#else
    return d->getIcon(QStyle::SP_DriveHDIcon);
#endif

    if (info.isFile()) {
        if (info.isSymLink())
            return d->getIcon(QStyle::SP_FileLinkIcon);
        else
            return d->getIcon(QStyle::SP_FileIcon);
    }
  if (info.isDir()) {
    if (info.isSymLink()) {
      return d->getIcon(QStyle::SP_DirLinkIcon);
    } else {
      if (info.absoluteFilePath() == d->homePath) {
        return d->getIcon(QStyle::SP_DirHomeIcon);
      } else {
        return d->getIcon(QStyle::SP_DirIcon);
      }
    }
  }
  return QIcon();
}

/*!
  Returns the type of the file described by \a info.
*/

QString QFileIconProvider::type(const QFileInfo &info) const
{
    if (info.isRoot())
        return QApplication::translate("QFileDialog", "Drive");
    if (info.isFile()) {
        if (!info.suffix().isEmpty())
            return info.suffix() + QLatin1Char(' ') + QApplication::translate("QFileDialog", "File");
        return QApplication::translate("QFileDialog", "File");
    }

    if (info.isDir())
#ifdef Q_OS_WIN
        return QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer");
#else
        return QApplication::translate("QFileDialog", "Folder", "All other platforms");
#endif
    // Windows   - "File Folder"
    // OS X      - "Folder"
    // Konqueror - "Folder"
    // Nautilus  - "folder"

    if (info.isSymLink())
#ifdef Q_OS_MAC
        return QApplication::translate("QFileDialog", "Alias", "Mac OS X Finder");
#else
        return QApplication::translate("QFileDialog", "Shortcut", "All other platforms");
#endif
    // OS X      - "Alias"
    // Windows   - "Shortcut"
    // Konqueror - "Folder" or "TXT File" i.e. what it is pointing to
    // Nautilus  - "link to folder" or "link to object file", same as Konqueror

    return QApplication::translate("QFileDialog", "Unknown");
}

QT_END_NAMESPACE
