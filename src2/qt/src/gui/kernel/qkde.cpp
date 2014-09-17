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


#include "qkde_p.h"
#include <QtCore/QLibrary>
#include <QtCore/QDir>
#include <QtCore/qdebug.h>
#include <QtCore/QSettings>
#include "QtGui/qstylefactory.h"
#include "qt_x11_p.h"

#if defined(Q_WS_X11)

QT_BEGIN_NAMESPACE

/*! \internal
Gets the current KDE home path
like "/home/troll/.kde"
*/
QString QKde::kdeHome()
{
    static QString kdeHomePath;
    if (kdeHomePath.isEmpty()) {
        kdeHomePath = QString::fromLocal8Bit(qgetenv("KDEHOME"));
        if (kdeHomePath.isEmpty()) {
            QDir homeDir(QDir::homePath());
            QString kdeConfDir(QLatin1String("/.kde"));
            if (4 == X11->desktopVersion && homeDir.exists(QLatin1String(".kde4")))
            kdeConfDir = QLatin1String("/.kde4");
            kdeHomePath = QDir::homePath() + kdeConfDir;
        }
    }
    return kdeHomePath;
}

/*!\internal
  Reads the color from the config, and store it in the palette with the given color role if found
  */
static bool kdeColor(QPalette *pal, QPalette::ColorRole role, const QSettings &kdeSettings, const QString &kde4Key, const QString &kde3Key = QString())
{
    QVariant variant = kdeSettings.value(kde4Key);
    if (!variant.isValid())
        QVariant variant = kdeSettings.value(kde3Key);
    if (variant.isValid()) {
        QStringList values = variant.toStringList();
        if (values.size() == 3) {
            int r = values[0].toInt();
            int g = values[1].toInt();
            int b = values[2].toInt();
            pal->setBrush(role, QColor(r, g, b));
            return true;
        }
    }
    return false;
}


/*!\internal
    Returns the KDE palette
*/
QPalette QKde::kdePalette()
{
    const QSettings theKdeSettings(QKde::kdeHome() +
        QLatin1String("/share/config/kdeglobals"), QSettings::IniFormat);
    QPalette pal;

    // Setup KDE palette
    kdeColor(&pal, QPalette::Button, theKdeSettings, QLatin1String("Colors:Button/BackgroundNormal"), QLatin1String("buttonBackground"));
    kdeColor(&pal, QPalette::Window, theKdeSettings, QLatin1String("Colors:Window/BackgroundNormal"), QLatin1String("background"));
    kdeColor(&pal, QPalette::Text, theKdeSettings, QLatin1String("Colors:View/ForegroundNormal"), QLatin1String("foreground"));
    kdeColor(&pal, QPalette::WindowText, theKdeSettings, QLatin1String("Colors:Window/ForegroundNormal"), QLatin1String("windowForeground"));
    kdeColor(&pal, QPalette::Base, theKdeSettings, QLatin1String("Colors:View/BackgroundNormal"), QLatin1String("windowBackground"));
    kdeColor(&pal, QPalette::Highlight, theKdeSettings, QLatin1String("Colors:Selection/BackgroundNormal"), QLatin1String("selectBackground"));
    kdeColor(&pal, QPalette::HighlightedText, theKdeSettings, QLatin1String("Colors:Selection/ForegroundNormal"), QLatin1String("selectForeground"));
    kdeColor(&pal, QPalette::AlternateBase, theKdeSettings, QLatin1String("Colors:View/BackgroundAlternate"), QLatin1String("alternateBackground"));
    kdeColor(&pal, QPalette::ButtonText, theKdeSettings, QLatin1String("Colors:Button/ForegroundNormal"), QLatin1String("buttonForeground"));
    kdeColor(&pal, QPalette::Link, theKdeSettings, QLatin1String("Colors:View/ForegroundLink"), QLatin1String("linkColor"));
    kdeColor(&pal, QPalette::LinkVisited, theKdeSettings, QLatin1String("Colors:View/ForegroundVisited"), QLatin1String("visitedLinkColor"));
   //## TODO tooltip color

    return pal;
}

/*!\internal
    Returns the name of the QStyle to use.
    (read from the kde config if needed)
*/
QString QKde::kdeStyle()
{
    if (X11->desktopVersion >= 4) {
        QSettings kdeSettings(QKde::kdeHome() + QLatin1String("/share/config/kdeglobals"), QSettings::IniFormat);
        QString style = kdeSettings.value(QLatin1String("widgetStyle"), QLatin1String("Oxygen")).toString();

        QStringList availableStyles = QStyleFactory::keys();
        if(availableStyles.contains(style, Qt::CaseInsensitive))
            return style;
    }

    if (X11->use_xrender)
        return QLatin1String("plastique");
    else
        return QLatin1String("windows");
}


int QKde::kdeToolButtonStyle()
{
    QSettings settings(QKde::kdeHome() + QLatin1String("/share/config/kdeglobals"),
                        QSettings::IniFormat);
    settings.beginGroup(QLatin1String("Toolbar style"));
    QString toolbarStyle = settings.value(QLatin1String("ToolButtonStyle"), QLatin1String("TextBesideIcon")).toString();
    if (toolbarStyle == QLatin1String("TextBesideIcon"))
        return Qt::ToolButtonTextBesideIcon;
    else if (toolbarStyle == QLatin1String("TextOnly"))
        return Qt::ToolButtonTextOnly;
    else if (toolbarStyle == QLatin1String("TextUnderIcon"))
        return Qt::ToolButtonTextUnderIcon;

    return Qt::ToolButtonTextBesideIcon;
}

int QKde::kdeToolBarIconSize()
{
    static int iconSize = -1;
    if (iconSize == -1) {
        QSettings settings(QKde::kdeHome() + QLatin1String("/share/config/kdeglobals"),
                            QSettings::IniFormat);
                            settings.beginGroup(QLatin1String("ToolbarIcons"));
        iconSize = settings.value(QLatin1String("Size")).toInt();
    }
    return iconSize;
}

QT_END_NAMESPACE

#endif //Q_WS_X11

