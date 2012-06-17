/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef META_H
#define META_H

#include <qmap.h>
#include <qstringlist.h>
#include <qstring.h>

QT_BEGIN_NAMESPACE

class QMakeMetaInfo
{
    bool readLibtoolFile(const QString &f);
    bool readPkgCfgFile(const QString &f);
    QMap<QString, QStringList> vars;
    QString meta_type;
    static QMap<QString, QMap<QString, QStringList> > cache_vars;
    void clear();
public:
    QMakeMetaInfo();

    bool readLib(QString lib);
    static QString findLib(QString lib);
    static bool libExists(QString lib);
    QString type() const;

    bool isEmpty(const QString &v);
    QStringList &values(const QString &v);
    QString first(const QString &v);
    QMap<QString, QStringList> &variables();
};

inline bool QMakeMetaInfo::isEmpty(const QString &v)
{ return !vars.contains(v) || vars[v].isEmpty(); }

inline QString QMakeMetaInfo::type() const
{ return meta_type; }

inline QStringList &QMakeMetaInfo::values(const QString &v)
{ return vars[v]; }

inline QString QMakeMetaInfo::first(const QString &v)
{
#if defined(Q_CC_SUN) && (__SUNPRO_CC == 0x500) || defined(Q_CC_HP)
    // workaround for Sun WorkShop 5.0 bug fixed in Forte 6
    if (isEmpty(v))
        return QString("");
    else
        return vars[v].first();
#else
    return isEmpty(v) ? QString("") : vars[v].first();
#endif
}

inline QMap<QString, QStringList> &QMakeMetaInfo::variables()
{ return vars; }

inline bool QMakeMetaInfo::libExists(QString lib)
{ return !findLib(lib).isNull(); }

QT_END_NAMESPACE

#endif // META_H
