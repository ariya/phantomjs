/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#ifndef QCOLLATOR_P_H
#define QCOLLATOR_P_H

#include "qcollator.h"
#include <QVector>
#ifdef QT_USE_ICU
#include <unicode/ucol.h>
#elif defined(Q_OS_OSX)
#include <CoreServices/CoreServices.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef QT_USE_ICU
typedef UCollator *CollatorType;
typedef QByteArray CollatorKeyType;

#elif defined(Q_OS_OSX)
typedef QVector<UCCollationValue> CollatorKeyType;

struct CollatorType {
    CollatorType(int opt) : collator(NULL), options(opt) {}

    CollatorRef collator;
    UInt32 options;
};
#elif defined(Q_OS_WIN)
typedef QString CollatorKeyType;
typedef int CollatorType;
#else //posix
typedef QVector<wchar_t> CollatorKeyType;
typedef int CollatorType;
#endif

class Q_CORE_EXPORT QCollatorPrivate
{
public:
    QAtomicInt ref;
    QLocale locale;

    CollatorType collator;

    void clear() {
        cleanup();
        collator = 0;
    }

    void init();
    void cleanup();

    QCollatorPrivate()
        : ref(1), collator(0)
    { cleanup(); }

    ~QCollatorPrivate() { cleanup(); }

private:
    Q_DISABLE_COPY(QCollatorPrivate)
};

class Q_CORE_EXPORT QCollatorSortKeyPrivate : public QSharedData
{
    friend class QCollator;
public:
    QCollatorSortKeyPrivate(const CollatorKeyType &key)
        : QSharedData()
        , m_key(key)
    {
    }

    CollatorKeyType m_key;

private:
    Q_DISABLE_COPY(QCollatorSortKeyPrivate)
};


QT_END_NAMESPACE

#endif // QCOLLATOR_P_H
