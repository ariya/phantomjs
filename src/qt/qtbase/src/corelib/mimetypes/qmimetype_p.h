/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMIMETYPE_P_H
#define QMIMETYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmimetype.h"

#include <QtCore/qhash.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QMimeTypePrivate : public QSharedData
{
public:
    typedef QHash<QString, QString> LocaleHash;

    QMimeTypePrivate();
    explicit QMimeTypePrivate(const QMimeType &other);

    void clear();

    void addGlobPattern(const QString &pattern);

    QString name;
    LocaleHash localeComments;
    QString genericIconName;
    QString iconName;
    QStringList globPatterns;
    bool loaded;
};

QT_END_NAMESPACE

#define QMIMETYPE_BUILDER \
    QT_BEGIN_NAMESPACE \
    static QMimeType buildQMimeType ( \
                         const QString &name, \
                         const QString &genericIconName, \
                         const QString &iconName, \
                         const QStringList &globPatterns \
                     ) \
    { \
        QMimeTypePrivate qMimeTypeData; \
        qMimeTypeData.name = name; \
        qMimeTypeData.genericIconName = genericIconName; \
        qMimeTypeData.iconName = iconName; \
        qMimeTypeData.globPatterns = globPatterns; \
        return QMimeType(qMimeTypeData); \
    } \
    QT_END_NAMESPACE

#ifdef Q_COMPILER_RVALUE_REFS
#define QMIMETYPE_BUILDER_FROM_RVALUE_REFS \
    QT_BEGIN_NAMESPACE \
    static QMimeType buildQMimeType ( \
                         QString &&name, \
                         QString &&genericIconName, \
                         QString &&iconName, \
                         QStringList &&globPatterns \
                     ) \
    { \
        QMimeTypePrivate qMimeTypeData; \
        qMimeTypeData.name = std::move(name); \
        qMimeTypeData.genericIconName = std::move(genericIconName); \
        qMimeTypeData.iconName = std::move(iconName); \
        qMimeTypeData.globPatterns = std::move(globPatterns); \
        return QMimeType(qMimeTypeData); \
    } \
    QT_END_NAMESPACE
#endif

#endif   // QMIMETYPE_P_H
