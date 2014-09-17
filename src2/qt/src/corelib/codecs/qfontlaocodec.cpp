/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qfontlaocodec_p.h"
#include "qlist.h"

#ifndef QT_NO_CODECS
#ifndef QT_NO_BIG_CODECS

QT_BEGIN_NAMESPACE

static unsigned char const unicode_to_mulelao[256] =
    {
        // U+0E80
        0x00, 0xa1, 0xa2, 0x00, 0xa4, 0x00, 0x00, 0xa7,
        0xa8, 0x00, 0xaa, 0x00, 0x00, 0xad, 0x00, 0x00,
        // U+0E90
        0x00, 0x00, 0x00, 0x00, 0xb4, 0xb5, 0xb6, 0xb7,
        0x00, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
        // U+0EA0
        0x00, 0xc1, 0xc2, 0xc3, 0x00, 0xc5, 0x00, 0xc7,
        0x00, 0x00, 0xca, 0xcb, 0x00, 0xcd, 0xce, 0xcf,
        // U+0EB0
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0x00, 0xdb, 0xdc, 0xdd, 0x00, 0x00,
        // U+0EC0
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0x00, 0xe6, 0x00,
        0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0x00, 0x00,
        // U+0ED0
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0x00, 0x00, 0xfc, 0xfd, 0x00, 0x00,
        // U+0EE0
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // U+0EF0
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };


QFontLaoCodec::~QFontLaoCodec()
{
}

QByteArray QFontLaoCodec::name() const
{
    return "mulelao-1";
}

int QFontLaoCodec::mibEnum() const
{
    return -4242;
}

QString QFontLaoCodec::convertToUnicode(const char *, int, ConverterState *) const
{
    return QString();
}

QByteArray QFontLaoCodec::convertFromUnicode(const QChar *uc, int len, ConverterState *) const
{
    QByteArray rstring(len, Qt::Uninitialized);
    uchar *rdata = (uchar *) rstring.data();
    const QChar *sdata = uc;
    int i = 0;
    for (; i < len; ++i, ++sdata, ++rdata) {
        if (sdata->unicode() < 0x80) {
            *rdata = (uchar) sdata->unicode();
        } else if (sdata->unicode() >= 0x0e80 && sdata->unicode() <= 0x0eff) {
            uchar lao = unicode_to_mulelao[sdata->unicode() - 0x0e80];
            if (lao)
                *rdata = lao;
            else
                *rdata = 0;
        } else {
            *rdata = 0;
        }
    }
    return rstring;
}

QT_END_NAMESPACE

#endif // QT_NO_BIG_CODECS
#endif // QT_NO_CODECS
