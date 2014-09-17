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

#include "qguiplatformplugin_p.h"
#include <qdebug.h>
#include <qfile.h>
#include <qdir.h>
#include <qsettings.h>
#include "private/qfactoryloader_p.h"
#include "qstylefactory.h"
#include "qapplication.h"
#include "qplatformdefs.h"
#include "qicon.h"

#ifdef Q_WS_WINCE
#include "qguifunctions_wince.h"
extern bool qt_wince_is_smartphone(); //qguifunctions_wince.cpp
extern bool qt_wince_is_mobile();     //qguifunctions_wince.cpp
extern bool qt_wince_is_pocket_pc();  //qguifunctions_wince.cpp
#endif


#if defined(Q_WS_X11)
#include <private/qkde_p.h>
#include <private/qgtkstyle_p.h>
#include <private/qt_x11_p.h>
#endif


QT_BEGIN_NAMESPACE


/*! \internal
    Return (an construct if necesseray) the Gui Platform plugin.

    The plugin key to be loaded is inside the QT_PLATFORM_PLUGIN environment variable.
    If it is not set, it will be the DESKTOP_SESSION on X11.

    If no plugin can be loaded, the default one is returned.
 */
QGuiPlatformPlugin *qt_guiPlatformPlugin()
{
    static QGuiPlatformPlugin *plugin;
    if (!plugin)
    {
#ifndef QT_NO_LIBRARY

        QString key = QString::fromLocal8Bit(qgetenv("QT_PLATFORM_PLUGIN"));
#ifdef Q_WS_X11
        if (key.isEmpty()) {
            switch(X11->desktopEnvironment) {
            case DE_KDE:
                key = QString::fromLatin1("kde");
                break;
            default:
                key = QString::fromLocal8Bit(qgetenv("DESKTOP_SESSION"));
                break;
            }
        }
#endif

        if (!key.isEmpty() && QApplication::desktopSettingsAware()) {
            QFactoryLoader loader(QGuiPlatformPluginInterface_iid, QLatin1String("/gui_platform"));
            plugin = qobject_cast<QGuiPlatformPlugin *>(loader.instance(key));
        }
#endif // QT_NO_LIBRARY

        if(!plugin) {
            static QGuiPlatformPlugin def;
            plugin = &def;
        }
    }
    return plugin;
}


/* \class QPlatformPlugin
    QGuiPlatformPlugin can be used to integrate Qt applications in a platform built on top of Qt.
    The application developer should not know or use the plugin, it is only used by Qt internaly.

    But full platform that are built on top of Qt may provide a plugin so 3rd party Qt application
    running in the platform are integrated.
 */

/*
    The constructor can be used to install hooks in Qt
 */
QGuiPlatformPlugin::QGuiPlatformPlugin(QObject *parent) : QObject(parent) {}
QGuiPlatformPlugin::~QGuiPlatformPlugin() {}


/* return the string key to be used by default the application */
QString QGuiPlatformPlugin::styleName()
{
#if defined(Q_WS_WIN) && defined(Q_WS_WINCE)
    if (qt_wince_is_smartphone() || qt_wince_is_pocket_pc())
        return QLatin1String("WindowsMobile");
    else
        return QLatin1String("WindowsCE");
#elif defined(Q_WS_WIN)
    if ((QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA
        && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
        return QLatin1String("WindowsVista");
    else if ((QSysInfo::WindowsVersion >= QSysInfo::WV_XP
        && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
        return QLatin1String("WindowsXP");
    else
        return QLatin1String("Windows");                // default styles for Windows
#elif defined(Q_WS_X11) && defined(Q_OS_SOLARIS)
    return QLatin1String("CDE");                        // default style for X11 on Solaris
#elif defined(Q_WS_S60)
    return QLatin1String("S60");                        // default style for Symbian with S60
#elif defined(Q_OS_SYMBIAN)
    return QLatin1String("Windows");                    // default style for Symbian without S60
#elif defined(Q_WS_X11) && defined(Q_OS_IRIX)
    return QLatin1String("SGI");                        // default style for X11 on IRIX
#elif defined(Q_WS_QWS) || defined(Q_WS_QPA)
    return QLatin1String("Plastique");                  // default style for X11 and small devices
#elif defined(Q_WS_MAC)
    return QLatin1String("Macintosh");              // default style for all Mac's
#elif defined(Q_WS_X11)
    QString stylename;
    switch(X11->desktopEnvironment) {
    case DE_KDE:
        stylename = QKde::kdeStyle();
        break;
    case DE_GNOME: {
        QStringList availableStyles = QStyleFactory::keys();
        // Set QGtkStyle for GNOME if available
        QString gtkStyleKey = QString::fromLatin1("GTK+");
        if (availableStyles.contains(gtkStyleKey)) {
            stylename = gtkStyleKey;
            break;
        }
        if (X11->use_xrender)
            stylename = QLatin1String("cleanlooks");
        else
            stylename = QLatin1String("windows");
        break;
    }
    case DE_CDE:
        stylename = QLatin1String("cde");
        break;
    default:
        // Don't do anything
        break;
    }
    return stylename;
#endif
}

/* return an additional default palette  (only work on X11) */
QPalette QGuiPlatformPlugin::palette()
{
#ifdef Q_WS_X11
    if (QApplication::desktopSettingsAware() && X11->desktopEnvironment == DE_KDE)
        return QKde::kdePalette();
#endif

    return QPalette();
}

/* the default icon theme name for QIcon::fromTheme. */
QString QGuiPlatformPlugin::systemIconThemeName()
{
    QString result;
#ifdef Q_WS_X11
    if (X11->desktopEnvironment == DE_GNOME) {
        result = QString::fromLatin1("gnome");
#ifndef QT_NO_STYLE_GTK
        result = QGtkStylePrivate::getGConfString(QLatin1String("/desktop/gnome/interface/icon_theme"), result);
#endif
    } else if (X11->desktopEnvironment == DE_KDE) {
        result =  X11->desktopVersion >= 4 ? QString::fromLatin1("oxygen") : QString::fromLatin1("crystalsvg");
        QSettings settings(QKde::kdeHome() + QLatin1String("/share/config/kdeglobals"), QSettings::IniFormat);
        settings.beginGroup(QLatin1String("Icons"));
        result = settings.value(QLatin1String("Theme"), result).toString();
    }
#endif
    return result;
}


QStringList QGuiPlatformPlugin::iconThemeSearchPaths()
{
    QStringList paths;
#if defined(Q_WS_X11)
    QString xdgDirString = QFile::decodeName(getenv("XDG_DATA_DIRS"));
    if (xdgDirString.isEmpty())
        xdgDirString = QLatin1String("/usr/local/share/:/usr/share/");

    QStringList xdgDirs = xdgDirString.split(QLatin1Char(':'));

    for (int i = 0 ; i < xdgDirs.size() ; ++i) {
        QDir dir(xdgDirs[i]);
        if (dir.exists())
            paths.append(dir.path() + QLatin1String("/icons"));
    }
    if (X11->desktopEnvironment == DE_KDE) {
        paths << QLatin1Char(':') + QKde::kdeHome() + QLatin1String("/share/icons");
        QStringList kdeDirs = QFile::decodeName(getenv("KDEDIRS")).split(QLatin1Char(':'));
        for (int i = 0 ; i< kdeDirs.count() ; ++i) {
            QDir dir(QLatin1Char(':') + kdeDirs.at(i) + QLatin1String("/share/icons"));
            if (dir.exists())
                paths.append(dir.path());
        }
    }

    // Add home directory first in search path
    QDir homeDir(QDir::homePath() + QLatin1String("/.icons"));
    if (homeDir.exists())
        paths.prepend(homeDir.path());
#endif

#if defined(Q_WS_WIN)
    paths.append(qApp->applicationDirPath() + QLatin1String("/icons"));
#elif defined(Q_WS_MAC)
    paths.append(qApp->applicationDirPath() + QLatin1String("/../Resources/icons"));
#endif
    return paths;
}

/* backend for QFileIconProvider,  null icon means default */
QIcon QGuiPlatformPlugin::fileSystemIcon(const QFileInfo &)
{
    return QIcon();
}

/* Like QStyle::styleHint */
int QGuiPlatformPlugin::platformHint(PlatformHint hint)
{
    int ret = 0;
    switch(hint)
    {
        case PH_ToolButtonStyle:
            ret = Qt::ToolButtonIconOnly;
#ifdef Q_WS_X11
            if (X11->desktopEnvironment == DE_KDE && X11->desktopVersion >= 4
                && QApplication::desktopSettingsAware()) {
                ret = QKde::kdeToolButtonStyle();
            }
#endif
            break;
        case PH_ToolBarIconSize:
#ifdef Q_WS_X11
            if (X11->desktopEnvironment == DE_KDE && X11->desktopVersion >= 4
                && QApplication::desktopSettingsAware()) {
                ret = QKde::kdeToolBarIconSize();
            }
#endif
            //by default keep ret = 0 so QCommonStyle will use the style default
            break;
        default:
            break;
    }
    return ret;
}


QT_END_NAMESPACE
