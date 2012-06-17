/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qtextcodecplugin.h"
#include "qstringlist.h"

#ifndef QT_NO_TEXTCODECPLUGIN

QT_BEGIN_NAMESPACE

/*!
    \class QTextCodecPlugin
    \brief The QTextCodecPlugin class provides an abstract base for custom QTextCodec plugins.
    \reentrant
    \ingroup plugins

    The text codec plugin is a simple plugin interface that makes it
    easy to create custom text codecs that can be loaded dynamically
    into applications.

    Writing a text codec plugin is achieved by subclassing this base
    class, reimplementing the pure virtual functions names(),
    aliases(), createForName(), mibEnums() and createForMib(), and
    exporting the class with the Q_EXPORT_PLUGIN2() macro. See \l{How
    to Create Qt Plugins} for details.

    See the \l{http://www.iana.org/assignments/character-sets}{IANA
    character-sets encoding file} for more information on mime
    names and mib enums.
*/

/*!
    \fn QStringList QTextCodecPlugin::names() const

    Returns the list of MIME names supported by this plugin.

    If a codec has several names, the extra names are returned by aliases().

    \sa createForName(), aliases()
*/

/*!
    \fn QList<QByteArray> QTextCodecPlugin::aliases() const

    Returns the list of aliases supported by this plugin.
*/

/*!
    \fn QTextCodec *QTextCodecPlugin::createForName(const QByteArray &name)

    Creates a QTextCodec object for the codec called \a name. The \a name
    must come from the list of encodings returned by names(). Encoding
    names are case sensitive.

    Example:

    \snippet doc/src/snippets/code/src_corelib_codecs_qtextcodecplugin.cpp 0

    \sa names()
*/


/*!
    \fn QList<int> QTextCodecPlugin::mibEnums() const

    Returns the list of mib enums supported by this plugin.

    \sa createForMib()
*/

/*!
    \fn QTextCodec *QTextCodecPlugin::createForMib(int mib);

    Creates a QTextCodec object for the mib enum \a mib.

    See \l{http://www.iana.org/assignments/character-sets}{the
    IANA character-sets encoding file} for more information.

    \sa mibEnums()
*/

/*!
    Constructs a text codec plugin with the given \a parent. This is
    invoked automatically by the Q_EXPORT_PLUGIN2() macro.
*/
QTextCodecPlugin::QTextCodecPlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the text codec plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QTextCodecPlugin::~QTextCodecPlugin()
{
}

QStringList QTextCodecPlugin::keys() const
{
    QStringList keys;
    QList<QByteArray> list = names();
    list += aliases();
    for (int i = 0; i < list.size(); ++i)
        keys += QString::fromLatin1(list.at(i));
    QList<int> mibs = mibEnums();
    for (int i = 0; i < mibs.count(); ++i)
        keys += QLatin1String("MIB: ") + QString::number(mibs.at(i));
    return keys;
}

QTextCodec *QTextCodecPlugin::create(const QString &name)
{
    if (name.startsWith(QLatin1String("MIB: ")))
        return createForMib(name.mid(4).toInt());
    return createForName(name.toLatin1());
}

QT_END_NAMESPACE

#endif // QT_NO_TEXTCODECPLUGIN
