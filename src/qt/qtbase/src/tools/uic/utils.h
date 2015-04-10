/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef UTILS_H
#define UTILS_H

#include "ui4.h"
#include <qstring.h>
#include <qlist.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

inline bool toBool(const QString &str)
{ return str.toLower() == QLatin1String("true"); }

inline QString toString(const DomString *str)
{ return str ? str->text() : QString(); }

enum StringFlags {
    Utf8String = 0x1,
    MultiLineString = 0x2
};

inline QString fixString(const QString &str, const QString &indent,
                         unsigned *stringFlags = 0)
{
    QString cursegment;
    QStringList result;
    const QByteArray utf8 = str.toUtf8();
    const int utf8Length = utf8.length();

    unsigned flags = 0;

    for (int i = 0; i < utf8Length; ++i) {
        const uchar cbyte = utf8.at(i);
        if (cbyte >= 0x80) {
            cursegment += QLatin1Char('\\');
            cursegment += QString::number(cbyte, 8);
            flags |= Utf8String;
        } else {
            switch(cbyte) {
            case '\\':
                cursegment += QLatin1String("\\\\"); break;
            case '\"':
                cursegment += QLatin1String("\\\""); break;
            case '\r':
                break;
            case '\n':
                flags |= MultiLineString;
                cursegment += QLatin1String("\\n\"\n\""); break;
            default:
                cursegment += QLatin1Char(cbyte);
            }
        }

        if (cursegment.length() > 1024) {
            result << cursegment;
            cursegment.clear();
        }
    }

    if (!cursegment.isEmpty())
        result << cursegment;


    QString joinstr = QLatin1String("\"\n");
    joinstr += indent;
    joinstr += indent;
    joinstr += QLatin1Char('"');

    QString rc(QLatin1Char('"'));
    rc += result.join(joinstr);
    rc += QLatin1Char('"');

    if (result.size() > 1)
        flags |= MultiLineString;

    if (stringFlags)
        *stringFlags = flags;

    return rc;
}

inline QString writeString(const QString &s, const QString &indent)
{
    unsigned flags = 0;
    const QString ret = fixString(s, indent, &flags);
    if (flags & Utf8String)
        return QLatin1String("QString::fromUtf8(") + ret + QLatin1Char(')');
    // MSVC cannot concat L"foo" "bar" (C2308: concatenating mismatched strings),
    // use QLatin1String instead (all platforms to avoid cross-compiling issues).
    if (flags & MultiLineString)
        return QLatin1String("QLatin1String(") + ret + QLatin1Char(')');
    return QLatin1String("QStringLiteral(") + ret + QLatin1Char(')');
}

inline QHash<QString, DomProperty *> propertyMap(const QList<DomProperty *> &properties)
{
    QHash<QString, DomProperty *> map;

    for (int i=0; i<properties.size(); ++i) {
        DomProperty *p = properties.at(i);
        map.insert(p->attributeName(), p);
    }

    return map;
}

inline QStringList unique(const QStringList &lst)
{
    QHash<QString, bool> h;
    for (int i=0; i<lst.size(); ++i)
        h.insert(lst.at(i), true);
    return h.keys();
}

QT_END_NAMESPACE

#endif // UTILS_H
