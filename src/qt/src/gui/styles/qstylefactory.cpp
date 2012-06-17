/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstylefactory.h"
#include "qstyleplugin.h"
#include "private/qfactoryloader_p.h"
#include "qmutex.h"

#include "qapplication.h"
#include "qwindowsstyle.h"
#include "qmotifstyle.h"
#include "qcdestyle.h"
#ifndef QT_NO_STYLE_PLASTIQUE
#include "qplastiquestyle.h"
#endif
#ifndef QT_NO_STYLE_CLEANLOOKS
#include "qcleanlooksstyle.h"
#endif
#ifndef QT_NO_STYLE_GTK
#include "qgtkstyle.h"
#endif
#ifndef QT_NO_STYLE_WINDOWSXP
#include "qwindowsxpstyle.h"
#endif
#ifndef QT_NO_STYLE_WINDOWSVISTA
#include "qwindowsvistastyle.h"
#endif
#ifndef QT_NO_STYLE_WINDOWSCE
#include "qwindowscestyle.h"
#endif
#ifndef QT_NO_STYLE_WINDOWSMOBILE
#include "qwindowsmobilestyle.h"
#endif
#ifndef QT_NO_STYLE_S60
#include "qs60style.h"
#endif

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_STYLE_MAC) && defined(Q_WS_MAC)
QT_BEGIN_INCLUDE_NAMESPACE
#  include "qmacstyle_mac.h"
QT_END_INCLUDE_NAMESPACE
#endif

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QStyleFactoryInterface_iid, QLatin1String("/styles"), Qt::CaseInsensitive))
#endif

/*!
    \class QStyleFactory
    \brief The QStyleFactory class creates QStyle objects.

    \ingroup appearance

    The QStyle class is an abstract base class that encapsulates the
    look and feel of a GUI. QStyleFactory creates a QStyle object
    using the create() function and a key identifying the style. The
    styles are either built-in or dynamically loaded from a style
    plugin (see QStylePlugin).

    The valid keys can be retrieved using the keys()
    function. Typically they include "windows", "motif", "cde",
    "plastique" and "cleanlooks".  Depending on the platform,
    "windowsxp", "windowsvista" and "macintosh" may be available.
    Note that keys are case insensitive.

    \sa QStyle
*/

/*!
    Creates and returns a QStyle object that matches the given \a key, or
    returns 0 if no matching style is found.

    Both built-in styles and styles from style plugins are queried for a
    matching style.

    \note The keys used are case insensitive.

    \sa keys()
*/
QStyle *QStyleFactory::create(const QString& key)
{
    QStyle *ret = 0;
    QString style = key.toLower();
#ifndef QT_NO_STYLE_WINDOWS
    if (style == QLatin1String("windows"))
        ret = new QWindowsStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSCE
    if (style == QLatin1String("windowsce"))
        ret = new QWindowsCEStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSMOBILE
    if (style == QLatin1String("windowsmobile"))
        ret = new QWindowsMobileStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSXP
    if (style == QLatin1String("windowsxp"))
        ret = new QWindowsXPStyle;
    else
#endif
#ifndef QT_NO_STYLE_WINDOWSVISTA
    if (style == QLatin1String("windowsvista"))
        ret = new QWindowsVistaStyle;
    else
#endif
#ifndef QT_NO_STYLE_MOTIF
    if (style == QLatin1String("motif"))
        ret = new QMotifStyle;
    else
#endif
#ifndef QT_NO_STYLE_CDE
    if (style == QLatin1String("cde"))
        ret = new QCDEStyle;
    else
#endif
#ifndef QT_NO_STYLE_S60
    if (style == QLatin1String("s60"))
        ret = new QS60Style;
    else
#endif
#ifndef QT_NO_STYLE_PLASTIQUE
    if (style == QLatin1String("plastique"))
        ret = new QPlastiqueStyle;
    else
#endif
#ifndef QT_NO_STYLE_CLEANLOOKS
    if (style == QLatin1String("cleanlooks"))
        ret = new QCleanlooksStyle;
    else
#endif
#ifndef QT_NO_STYLE_GTK
    if (style == QLatin1String("gtk") || style == QLatin1String("gtk+"))
        ret = new QGtkStyle;
    else
#endif
#ifndef QT_NO_STYLE_MAC
    if (style.startsWith(QLatin1String("macintosh"))) {
        ret = new QMacStyle;
#  ifdef Q_WS_MAC
        if (style == QLatin1String("macintosh"))
            style += QLatin1String(" (aqua)");
#  endif
    } else
#endif
    { } // Keep these here - they make the #ifdefery above work
#if !defined(QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
    if(!ret) {
        if (QStyleFactoryInterface *factory = qobject_cast<QStyleFactoryInterface*>(loader()->instance(style)))
            ret = factory->create(style);
    }
#endif
    if(ret)
        ret->setObjectName(style);
    return ret;
}

/*!
    Returns the list of valid keys, i.e. the keys this factory can
    create styles for.

    \sa create()
*/
QStringList QStyleFactory::keys()
{
#if !defined(QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
    QStringList list = loader()->keys();
#else
    QStringList list;
#endif
#ifndef QT_NO_STYLE_WINDOWS
    if (!list.contains(QLatin1String("Windows")))
        list << QLatin1String("Windows");
#endif
#ifndef QT_NO_STYLE_WINDOWSCE
    if (!list.contains(QLatin1String("WindowsCE")))
        list << QLatin1String("WindowsCE");
#endif
#ifndef QT_NO_STYLE_WINDOWSMOBILE
    if (!list.contains(QLatin1String("WindowsMobile")))
        list << QLatin1String("WindowsMobile");
#endif
#ifndef QT_NO_STYLE_WINDOWSXP
    if (!list.contains(QLatin1String("WindowsXP")) &&
        (QSysInfo::WindowsVersion >= QSysInfo::WV_XP && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
        list << QLatin1String("WindowsXP");
#endif
#ifndef QT_NO_STYLE_WINDOWSVISTA
    if (!list.contains(QLatin1String("WindowsVista")) &&
        (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
        list << QLatin1String("WindowsVista");
#endif
#ifndef QT_NO_STYLE_MOTIF
    if (!list.contains(QLatin1String("Motif")))
        list << QLatin1String("Motif");
#endif
#ifndef QT_NO_STYLE_CDE
    if (!list.contains(QLatin1String("CDE")))
        list << QLatin1String("CDE");
#endif
#ifndef QT_NO_STYLE_S60
    if (!list.contains(QLatin1String("S60")))
        list << QLatin1String("S60");
#endif
#ifndef QT_NO_STYLE_PLASTIQUE
    if (!list.contains(QLatin1String("Plastique")))
        list << QLatin1String("Plastique");
#endif
#ifndef QT_NO_STYLE_GTK
    if (!list.contains(QLatin1String("GTK+")))
        list << QLatin1String("GTK+");
#endif
#ifndef QT_NO_STYLE_CLEANLOOKS
    if (!list.contains(QLatin1String("Cleanlooks")))
        list << QLatin1String("Cleanlooks");
#endif
#ifndef QT_NO_STYLE_MAC
    QString mstyle = QLatin1String("Macintosh");
# ifdef Q_WS_MAC
    mstyle += QLatin1String(" (aqua)");
# endif
    if (!list.contains(mstyle))
        list << mstyle;
#endif
    return list;
}

QT_END_NAMESPACE
