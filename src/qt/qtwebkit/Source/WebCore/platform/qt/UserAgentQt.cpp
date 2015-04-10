/*
    Copyright (C) 2008, 2009, 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.
    Copyright (C) 2007 Apple Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "config.h"

#include "UserAgentQt.h"

#include <QGuiApplication>

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#if OS(WINDOWS)
#include <SystemInfo.h>
#endif // OS(WINDOWS)

namespace WebCore {

/*!
    This function is called when a user agent for HTTP requests is needed.

    This implementation returns the following value:

    "Mozilla/5.0 (%Platform%%Security%%Subplatform%) AppleWebKit/%WebKitVersion% (KHTML, like Gecko) %AppVersion Safari/%WebKitVersion%"

    In this string the following values are replaced the first time the function is called:
    \list
    \li %Platform% expands to the windowing system followed by "; " if it is not Windows (e.g. "X11; ").
    \li %Security% expands to "N; " if SSL is disabled.
    \li %Subplatform% expands to the operating system version (e.g. "Windows NT 6.1" or "Intel Mac OS X 10.5").
    \li %WebKitVersion% is the version of WebKit the application was compiled against.
    \endlist

    The following value is replaced each time the funciton is called
    \list
    \li %AppVersion% expands to QCoreApplication::applicationName()/QCoreApplication::applicationVersion() if they're set; otherwise defaulting to Qt and the current Qt version.
    \endlist
*/
String UserAgentQt::standardUserAgent(const String &applicationNameForUserAgent, unsigned int webkitMajorVersion, unsigned int webkitMinorVersion)
{
    static QString ua;

    if (ua.isNull()) {

        ua = QLatin1String("Mozilla/5.0 (%1%2%3) AppleWebKit/%4 (KHTML, like Gecko) %99 Safari/%5");

        // Platform.
        ua = ua.arg(QLatin1String(
#if OS(MAC_OS_X)
            "Macintosh; "
#elif OS(WINDOWS)
            ""
#else
            (QGuiApplication::platformName() == QLatin1String("xcb")) ? "X11; " : "Unknown; "
#endif
        ));


        // Security strength.
        QString securityStrength;
#if defined(QT_NO_OPENSSL)
        securityStrength = QLatin1String("N; ");
#endif
        ua = ua.arg(securityStrength);

        // Operating system.
        ua = ua.arg(QLatin1String(
#if OS(AIX)
            "AIX"
#elif OS(WINDOWS)
            windowsVersionForUAString().latin1().data()
#elif OS(MAC_OS_X)
#if CPU(X86) || CPU(X86_64)
            "Intel Mac OS X"
#else
            "PPC Mac OS X"
#endif

#elif OS(FREEBSD)
            "FreeBSD"
#elif OS(HURD)
            "GNU Hurd"
#elif OS(LINUX)

#if CPU(X86_64)
            "Linux x86_64"
#elif CPU(X86)
            "Linux i686"
#else
            "Linux"
#endif

#elif OS(NETBSD)
            "NetBSD"
#elif OS(OPENBSD)
            "OpenBSD"
#elif OS(QNX)
            "QNX"
#elif OS(SOLARIS)
            "Sun Solaris"
#elif OS(UNIX) // FIXME Looks like all unix variants above are the only cases where OS_UNIX is set.
            "UNIX BSD/SYSV system"
#else
            "Unknown"
#endif
        ));

        // WebKit version.
        QString version = QString::number(webkitMajorVersion) + QLatin1Char('.') + QString::number(webkitMinorVersion);
        ua = ua.arg(version, version);
    }

    QString appName;
    if (applicationNameForUserAgent.isEmpty())
        appName = QCoreApplication::applicationName();
    else
        appName = applicationNameForUserAgent;

    if (!appName.isEmpty()) {
        QString appVer = QCoreApplication::applicationVersion();
        if (!appVer.isEmpty())
            appName.append(QLatin1Char('/') + appVer);
    } else {
        // Qt version.
        appName = QLatin1String("Qt/") + QLatin1String(qVersion());
    }

    return ua.arg(appName);
}

}
