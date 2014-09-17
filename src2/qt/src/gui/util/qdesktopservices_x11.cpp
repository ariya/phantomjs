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

#include "qdesktopservices.h"

#ifndef QT_NO_DESKTOPSERVICES

#include <qprocess.h>
#include <qurl.h>
#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <private/qt_x11_p.h>
#include <qcoreapplication.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

inline static bool launch(const QUrl &url, const QString &client)
{
#if !defined(QT_NO_PROCESS)
    return (QProcess::startDetached(client + QLatin1Char(' ') + QString::fromLatin1(url.toEncoded().constData())));
#else
    return (::system((client + QLatin1Char(' ') + QString::fromLatin1(url.toEncoded().constData())).toLocal8Bit().constData()) != -1);
#endif
}

static bool openDocument(const QUrl &url)
{
    if (!url.isValid())
        return false;

    if (launch(url, QLatin1String("xdg-open")))
        return true;

    // Use the X11->desktopEnvironment value if X11 is non-NULL,
    //  otherwise just attempt to launch command regardless of the desktop environment
    if ((!X11 || (X11 && X11->desktopEnvironment == DE_GNOME)) && launch(url, QLatin1String("gnome-open"))) {
        return true;
    } else {
        if ((!X11 || (X11 && X11->desktopEnvironment == DE_KDE)) && launch(url, QLatin1String("kfmclient exec")))
            return true;
    }

    if (launch(url, QLatin1String("firefox")))
        return true;
    if (launch(url, QLatin1String("mozilla")))
        return true;
    if (launch(url, QLatin1String("netscape")))
        return true;
    if (launch(url, QLatin1String("opera")))
        return true;

    return false;
}

static bool launchWebBrowser(const QUrl &url)
{
    if (!url.isValid())
        return false;
    if (url.scheme() == QLatin1String("mailto"))
        return openDocument(url);

    if (launch(url, QLatin1String("xdg-open")))
        return true;
    if (launch(url, QString::fromLocal8Bit(getenv("DEFAULT_BROWSER"))))
        return true;
    if (launch(url, QString::fromLocal8Bit(getenv("BROWSER"))))
        return true;

    // Use the X11->desktopEnvironment value if X11 is non-NULL,
    //  otherwise just attempt to launch command regardless of the desktop environment
    if ((!X11 || (X11 && X11->desktopEnvironment == DE_GNOME)) && launch(url, QLatin1String("gnome-open"))) {
        return true;
    } else {
        if ((!X11 || (X11 && X11->desktopEnvironment == DE_KDE)) && launch(url, QLatin1String("kfmclient openURL")))
            return true;
    }

    if (launch(url, QLatin1String("firefox")))
        return true;
    if (launch(url, QLatin1String("mozilla")))
        return true;
    if (launch(url, QLatin1String("netscape")))
        return true;
    if (launch(url, QLatin1String("opera")))
        return true;
    return false;
}



QString QDesktopServices::storageLocation(StandardLocation type)
{
    if (type == QDesktopServices::HomeLocation)
        return QDir::homePath();
    if (type == QDesktopServices::TempLocation)
        return QDir::tempPath();

    // http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html
    if (type == QDesktopServices::CacheLocation) {
        QString xdgCacheHome = QLatin1String(qgetenv("XDG_CACHE_HOME"));
        if (xdgCacheHome.isEmpty())
            xdgCacheHome = QDir::homePath() + QLatin1String("/.cache");
        xdgCacheHome += QLatin1Char('/') + QCoreApplication::organizationName()
                    + QLatin1Char('/') + QCoreApplication::applicationName();
        return xdgCacheHome;
    }

    if (type == QDesktopServices::DataLocation) {
        QString xdgDataHome = QLatin1String(qgetenv("XDG_DATA_HOME"));
        if (xdgDataHome.isEmpty())
            xdgDataHome = QDir::homePath() + QLatin1String("/.local/share");
        xdgDataHome += QLatin1String("/data/")
                    + QCoreApplication::organizationName() + QLatin1Char('/')
                    + QCoreApplication::applicationName();
        return xdgDataHome;
    }

    // http://www.freedesktop.org/wiki/Software/xdg-user-dirs
    QString xdgConfigHome = QLatin1String(qgetenv("XDG_CONFIG_HOME"));
    if (xdgConfigHome.isEmpty())
        xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
    QFile file(xdgConfigHome + QLatin1String("/user-dirs.dirs"));
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QHash<QString, QString> lines;
        QTextStream stream(&file);
        // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
        QRegExp exp(QLatin1String("^XDG_(.*)_DIR=(.*)$"));
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (exp.indexIn(line) != -1) {
                QStringList lst = exp.capturedTexts();
                QString key = lst.at(1);
                QString value = lst.at(2);
                if (value.length() > 2
                    && value.startsWith(QLatin1Char('\"'))
                    && value.endsWith(QLatin1Char('\"')))
                    value = value.mid(1, value.length() - 2);
                // Store the key and value: "DESKTOP", "$HOME/Desktop"
                lines[key] = value;
            }
        }

        QString key;
        switch (type) {
        case DesktopLocation: key = QLatin1String("DESKTOP"); break;
        case DocumentsLocation: key = QLatin1String("DOCUMENTS"); break;
        case PicturesLocation: key = QLatin1String("PICTURES"); break;
        case MusicLocation: key = QLatin1String("MUSIC"); break;
        case MoviesLocation: key = QLatin1String("VIDEOS"); break;
        default: break;
        }
        if (!key.isEmpty() && lines.contains(key)) {
            QString value = lines[key];
            // value can start with $HOME
            if (value.startsWith(QLatin1String("$HOME")))
                value = QDir::homePath() + value.mid(5);
            return value;
        }
    }

    QDir emptyDir;
    QString path;
    switch (type) {
    case DesktopLocation:
        path = QDir::homePath() + QLatin1String("/Desktop");
        break;
    case DocumentsLocation:
        path = QDir::homePath() + QLatin1String("/Documents");
       break;
    case PicturesLocation:
        path = QDir::homePath() + QLatin1String("/Pictures");
        break;

    case FontsLocation:
        path = QDir::homePath() + QLatin1String("/.fonts");
        break;

    case MusicLocation:
        path = QDir::homePath() + QLatin1String("/Music");
        break;

    case MoviesLocation:
        path = QDir::homePath() + QLatin1String("/Videos");
        break;

    case ApplicationsLocation:
    default:
        break;
    }

    return path;
}

QString QDesktopServices::displayName(StandardLocation type)
{
    Q_UNUSED(type);
    return QString();
}

QT_END_NAMESPACE

#endif // QT_NO_DESKTOPSERVICES
