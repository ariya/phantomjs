/*
    Copyright (C) 2009 Robert Hogan <robert@roberthogan.net>

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
#include <qwebkitversion.h>
#include <WebKitVersion.h>

/*!
    \relates QWebPage
    \since 4.6
    Returns the version number of WebKit at run-time as a string (for
    example, "531.3").

    This version is commonly used in WebKit based browsers as part
    of the user agent string. Web servers and JavaScript might use
    it to identify the presence of certain WebKit engine features
    and behaviour.

    The evolution of this version is bound to the releases of Apple's
    Safari browser. For a version specific to the QtWebKit library,
    see QTWEBKIT_VERSION

    \sa QWebPage::userAgentForUrl()
*/
QString qWebKitVersion()
{
    return QString::fromLatin1("%1.%2").arg(WEBKIT_MAJOR_VERSION).arg(WEBKIT_MINOR_VERSION);
}

/*!
    \relates QWebPage
    \since 4.6
    Returns the 'major' version number of WebKit at run-time as an integer
    (for example, 531). This is the version of WebKit the application
    was compiled against.

    \sa qWebKitVersion()
*/
int qWebKitMajorVersion()
{
    return WEBKIT_MAJOR_VERSION;
}

/*!
    \relates QWebPage
    \since 4.6
    Returns the 'minor' version number of WebKit at run-time as an integer
    (for example, 3). This is the version of WebKit the application
    was compiled against.

    \sa qWebKitVersion()
*/
int qWebKitMinorVersion()
{
    return WEBKIT_MINOR_VERSION;
}

/*!
    \macro QTWEBKIT_VERSION
    \relates QWebPage

    This macro expands a numeric value of the form 0xMMNNPP (MM =
    major, NN = minor, PP = patch) that specifies QtWebKit's version
    number. For example, if you compile your application against QtWebKit
    2.1.2, the QTWEBKIT_VERSION macro will expand to 0x020102.

    You can use QTWEBKIT_VERSION to use the latest QtWebKit API where
    available.

    \sa QT_VERSION
*/

/*!
    \macro QTWEBKIT_VERSION_STR
    \relates QWebPage

    This macro expands to a string that specifies QtWebKit's version number
    (for example, "2.1.2"). This is the version against which the
    application is compiled.

    \sa QTWEBKIT_VERSION
*/

/*!
    \macro QTWEBKIT_VERSION_CHECK
    \relates QWebPage

    Turns the major, minor and patch numbers of a version into an
    integer, 0xMMNNPP (MM = major, NN = minor, PP = patch). This can
    be compared with another similarly processed version id, for example
    in a preprocessor statement:

    \code
    #if QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 1, 0)
    // code to use API new in QtWebKit 2.1.0
    #endif
    \endcode
*/
