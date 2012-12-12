/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <qtextcodecplugin.h>
#include <qtextcodec.h>
#include <qstringlist.h>

#ifndef QT_NO_TEXTCODECPLUGIN

#include "qbig5codec.h"

QT_BEGIN_NAMESPACE

class TWTextCodecs : public QTextCodecPlugin
{
public:
    TWTextCodecs() {}

    QList<QByteArray> names() const;
    QList<QByteArray> aliases() const;
    QList<int> mibEnums() const;

    QTextCodec *createForMib(int);
    QTextCodec *createForName(const QByteArray &);
};

QList<QByteArray> TWTextCodecs::names() const
{
    QList<QByteArray> list;
    list += QBig5Codec::_name();
    list += QBig5hkscsCodec::_name();
#ifdef Q_WS_X11
    list += QFontBig5Codec::_name();
    list += QFontBig5hkscsCodec::_name();
#endif
    return list;
}

QList<QByteArray> TWTextCodecs::aliases() const
{
    QList<QByteArray> list;
    list += QBig5Codec::_aliases();
    list += QBig5hkscsCodec::_aliases();
#ifdef Q_WS_X11
    list += QFontBig5Codec::_aliases();
    list += QFontBig5hkscsCodec::_aliases();
#endif
    return list;
}

QList<int> TWTextCodecs::mibEnums() const
{
    QList<int> list;
    list += QBig5Codec::_mibEnum();
    list += QBig5hkscsCodec::_mibEnum();
#ifdef Q_WS_X11
    list += QFontBig5Codec::_mibEnum();
    list += QFontBig5hkscsCodec::_mibEnum();
#endif
    return list;
}

QTextCodec *TWTextCodecs::createForMib(int mib)
{
    if (mib == QBig5Codec::_mibEnum())
        return new QBig5Codec;
    if (mib == QBig5hkscsCodec::_mibEnum())
        return new QBig5hkscsCodec;
#ifdef Q_WS_X11
    if (mib == QFontBig5hkscsCodec::_mibEnum())
        return new QFontBig5hkscsCodec;
    if (mib == QFontBig5Codec::_mibEnum())
        return new QFontBig5Codec;
#endif
    return 0;
}


QTextCodec *TWTextCodecs::createForName(const QByteArray &name)
{
    if (name == QBig5Codec::_name() || QBig5Codec::_aliases().contains(name))
        return new QBig5Codec;
    if (name == QBig5hkscsCodec::_name() || QBig5hkscsCodec::_aliases().contains(name))
        return new QBig5hkscsCodec;
#ifdef Q_WS_X11
    if (name == QFontBig5hkscsCodec::_name() || QFontBig5hkscsCodec::_aliases().contains(name))
        return new QFontBig5hkscsCodec;
    if (name == QFontBig5Codec::_name() || QFontBig5Codec::_aliases().contains(name))
        return new QFontBig5Codec;
#endif
    return 0;
}


Q_EXPORT_STATIC_PLUGIN(TWTextCodecs);
Q_EXPORT_PLUGIN2(qtwcodecs, TWTextCodecs);

#endif // QT_NO_TEXTCODECPLUGIN

QT_END_NAMESPACE
