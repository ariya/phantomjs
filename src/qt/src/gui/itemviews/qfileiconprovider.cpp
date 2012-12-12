/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QT_NO_FILEICONPROVIDER
#include <qstyle.h>
#include <qapplication.h>
#include <qdir.h>
#include <qpixmapcache.h>
#if defined(Q_WS_WIN)
#  define _WIN32_IE 0x0500
#  include <qt_windows.h>
#  include <commctrl.h>
#  include <objbase.h>
#elif defined(Q_WS_MAC)
#  include <private/qt_cocoa_helpers_mac_p.h>
#endif

#include <private/qfunctions_p.h>
#include <private/qguiplatformplugin_p.h>

#if defined(Q_WS_X11) && !defined(QT_NO_STYLE_GTK)
#  include <private/qgtkstyle_p.h>
#  include <private/qt_x11_p.h>
#endif

#ifndef SHGFI_ADDOVERLAYS
#  define SHGFI_ADDOVERLAYS 0x000000020
#  define SHGFI_OVERLAYINDEX 0x000000040
#endif

QT_BEGIN_NAMESPACE

/*!
  \class QFileIconProvider

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

class QFileIconProviderPrivate
{
    Q_DECLARE_PUBLIC(QFileIconProvider)

public:
    QFileIconProviderPrivate();
    QIcon getIcon(QStyle::StandardPixmap name) const;
#ifdef Q_WS_WIN
    QIcon getWinIcon(const QFileInfo &fi) const;
#elif defined(Q_WS_MAC)
    QIcon getMacIcon(const QFileInfo &fi) const;
#endif
    QFileIconProvider *q_ptr;
    const QString homePath;

private:
    mutable QIcon file;
    mutable QIcon fileLink;
    mutable QIcon directory;
    mutable QIcon directoryLink;
    mutable QIcon harddisk;
    mutable QIcon floppy;
    mutable QIcon cdrom;
    mutable QIcon ram;
    mutable QIcon network;
    mutable QIcon computer;
    mutable QIcon desktop;
    mutable QIcon trashcan;
    mutable QIcon generic;
    mutable QIcon home;
};

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

#ifdef Q_WS_WIN
QIcon QFileIconProviderPrivate::getWinIcon(const QFileInfo &fileInfo) const
{
    QIcon retIcon;
    const QString fileExtension = QLatin1Char('.') + fileInfo.suffix().toUpper();

    QString key;
    if (fileInfo.isFile() && !fileInfo.isExecutable() && !fileInfo.isSymLink() && fileExtension != QLatin1String(".ICO"))
        key = QLatin1String("qt_") + fileExtension;

    QPixmap pixmap;
    if (!key.isEmpty()) {
        QPixmapCache::find(key, pixmap);
    }

    if (!pixmap.isNull()) {
        retIcon.addPixmap(pixmap);
        if (QPixmapCache::find(key + QLatin1Char('l'), pixmap))
            retIcon.addPixmap(pixmap);
        return retIcon;
    }

    /* We don't use the variable, but by storing it statically, we
     * ensure CoInitialize is only called once. */
    static HRESULT comInit = CoInitialize(NULL);
    Q_UNUSED(comInit);

    SHFILEINFO info;
    unsigned long val = 0;

    //Get the small icon
#ifndef Q_OS_WINCE
    val = SHGetFileInfo((const wchar_t *)QDir::toNativeSeparators(fileInfo.filePath()).utf16(), 0, &info,
                        sizeof(SHFILEINFO), SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_ADDOVERLAYS|SHGFI_OVERLAYINDEX);
#else
    val = SHGetFileInfo((const wchar_t *)QDir::toNativeSeparators(fileInfo.filePath()).utf16(), 0, &info,
                        sizeof(SHFILEINFO), SHGFI_SMALLICON|SHGFI_SYSICONINDEX);
#endif

    // Even if GetFileInfo returns a valid result, hIcon can be empty in some cases
    if (val && info.hIcon) {
        if (fileInfo.isDir() && !fileInfo.isRoot()) {
            //using the unique icon index provided by windows save us from duplicate keys
            key = QString::fromLatin1("qt_dir_%1").arg(info.iIcon);
            QPixmapCache::find(key, pixmap);
            if (!pixmap.isNull()) {
                retIcon.addPixmap(pixmap);
                if (QPixmapCache::find(key + QLatin1Char('l'), pixmap))
                    retIcon.addPixmap(pixmap);
                DestroyIcon(info.hIcon);
                return retIcon;
            }
        }
        if (pixmap.isNull()) {
#ifndef Q_OS_WINCE
            pixmap = QPixmap::fromWinHICON(info.hIcon);
#else
            pixmap = QPixmap::fromWinHICON(ImageList_GetIcon((HIMAGELIST) val, info.iIcon, ILD_NORMAL));
#endif
            if (!pixmap.isNull()) {
                retIcon.addPixmap(pixmap);
                if (!key.isEmpty())
                    QPixmapCache::insert(key, pixmap);
            }
            else {
              qWarning("QFileIconProviderPrivate::getWinIcon() no small icon found");
            }
        }
        DestroyIcon(info.hIcon);
    }

    //Get the big icon
#ifndef Q_OS_WINCE
    val = SHGetFileInfo((const wchar_t *)QDir::toNativeSeparators(fileInfo.filePath()).utf16(), 0, &info,
                        sizeof(SHFILEINFO), SHGFI_ICON|SHGFI_LARGEICON|SHGFI_SYSICONINDEX|SHGFI_ADDOVERLAYS|SHGFI_OVERLAYINDEX);
#else
    val = SHGetFileInfo((const wchar_t *)QDir::toNativeSeparators(fileInfo.filePath()).utf16(), 0, &info,
                        sizeof(SHFILEINFO), SHGFI_LARGEICON|SHGFI_SYSICONINDEX);
#endif
    if (val && info.hIcon) {
        if (fileInfo.isDir() && !fileInfo.isRoot()) {
            //using the unique icon index provided by windows save us from duplicate keys
            key = QString::fromLatin1("qt_dir_%1").arg(info.iIcon);
        }
#ifndef Q_OS_WINCE
        pixmap = QPixmap::fromWinHICON(info.hIcon);
#else
        pixmap = QPixmap::fromWinHICON(ImageList_GetIcon((HIMAGELIST) val, info.iIcon, ILD_NORMAL));
#endif
        if (!pixmap.isNull()) {
            retIcon.addPixmap(pixmap);
            if (!key.isEmpty())
                QPixmapCache::insert(key + QLatin1Char('l'), pixmap);
        }
        else {
            qWarning("QFileIconProviderPrivate::getWinIcon() no large icon found");
        }
        DestroyIcon(info.hIcon);
    }
    return retIcon;
}

#elif defined(Q_WS_MAC)
QIcon QFileIconProviderPrivate::getMacIcon(const QFileInfo &fi) const
{
    QIcon retIcon;
    QString fileExtension = fi.suffix().toUpper();
    fileExtension.prepend(QLatin1String("."));

    const QString keyBase = QLatin1String("qt_") + fileExtension;

    QPixmap pixmap;
    if (fi.isFile() && !fi.isExecutable() && !fi.isSymLink()) {
        QPixmapCache::find(keyBase + QLatin1String("16"), pixmap);
    }

    if (!pixmap.isNull()) {
        retIcon.addPixmap(pixmap);
        if (QPixmapCache::find(keyBase + QLatin1String("32"), pixmap)) {
            retIcon.addPixmap(pixmap);
            if (QPixmapCache::find(keyBase + QLatin1String("64"), pixmap)) {
                retIcon.addPixmap(pixmap);
                if (QPixmapCache::find(keyBase + QLatin1String("128"), pixmap)) {
                    retIcon.addPixmap(pixmap);
                    return retIcon;
                }
            }
        }
    }


    FSRef macRef;
    OSStatus status = FSPathMakeRef(reinterpret_cast<const UInt8*>(fi.canonicalFilePath().toUtf8().constData()),
                                    &macRef, 0);
    if (status != noErr)
        return retIcon;
    FSCatalogInfo info;
    HFSUniStr255 macName;
    status = FSGetCatalogInfo(&macRef, kIconServicesCatalogInfoMask, &info, &macName, 0, 0);
    if (status != noErr)
        return retIcon;
    IconRef iconRef;
    SInt16 iconLabel;
    status = GetIconRefFromFileInfo(&macRef, macName.length, macName.unicode,
                                    kIconServicesCatalogInfoMask, &info, kIconServicesNormalUsageFlag,
                                    &iconRef, &iconLabel);
    if (status != noErr)
        return retIcon;
    qt_mac_constructQIconFromIconRef(iconRef, 0, &retIcon);
    ReleaseIconRef(iconRef);

    if (fi.isFile() && !fi.isExecutable() && !fi.isSymLink()) {
        pixmap = retIcon.pixmap(16);
        QPixmapCache::insert(keyBase + QLatin1String("16"), pixmap);
        pixmap = retIcon.pixmap(32);
        QPixmapCache::insert(keyBase + QLatin1String("32"), pixmap);
        pixmap = retIcon.pixmap(64);
        QPixmapCache::insert(keyBase + QLatin1String("64"), pixmap);
        pixmap = retIcon.pixmap(128);
        QPixmapCache::insert(keyBase + QLatin1String("128"), pixmap);
    }

    return retIcon;
}
#endif


/*!
  Returns an icon for the file described by \a info.
*/

QIcon QFileIconProvider::icon(const QFileInfo &info) const
{
    Q_D(const QFileIconProvider);

    QIcon platformIcon = qt_guiPlatformPlugin()->fileSystemIcon(info);
    if (!platformIcon.isNull())
        return platformIcon;

#if defined(Q_WS_X11) && !defined(QT_NO_STYLE_GTK)
    if (X11->desktopEnvironment == DE_GNOME) {
        QIcon gtkIcon = QGtkStylePrivate::getFilesystemIcon(info);
        if (!gtkIcon.isNull())
            return gtkIcon;
    }
#endif

#ifdef Q_WS_MAC
    QIcon retIcon = d->getMacIcon(info);
    if (!retIcon.isNull())
        return retIcon;
#elif defined Q_WS_WIN
    QIcon icon = d->getWinIcon(info);
    if (!icon.isNull())
        return icon;
#endif
    if (info.isRoot())
#if defined (Q_WS_WIN) && !defined(Q_WS_WINCE)
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
#ifdef Q_WS_WIN
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

#endif
