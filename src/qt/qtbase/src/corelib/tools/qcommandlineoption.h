/****************************************************************************
**
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
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

#ifndef QCOMMANDLINEOPTION_H
#define QCOMMANDLINEOPTION_H

#include <QtCore/qstringlist.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QCommandLineOptionPrivate;

class Q_CORE_EXPORT QCommandLineOption
{
public:
    explicit QCommandLineOption(const QString &name);
    explicit QCommandLineOption(const QStringList &names);
    /*implicit*/ QCommandLineOption(const QString &name, const QString &description,
                                const QString &valueName = QString(),
                                const QString &defaultValue = QString());
    /*implicit*/ QCommandLineOption(const QStringList &names, const QString &description,
                                const QString &valueName = QString(),
                                const QString &defaultValue = QString());
    QCommandLineOption(const QCommandLineOption &other);

    ~QCommandLineOption();

    QCommandLineOption &operator=(const QCommandLineOption &other);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QCommandLineOption &operator=(QCommandLineOption &&other)
    { qSwap(d, other.d); return *this; }
#endif

    inline void swap(QCommandLineOption &other)
    { qSwap(d, other.d); }

    QStringList names() const;

    void setValueName(const QString &name);
    QString valueName() const;

    void setDescription(const QString &description);
    QString description() const;

    void setDefaultValue(const QString &defaultValue);
    void setDefaultValues(const QStringList &defaultValues);
    QStringList defaultValues() const;

private:
    QSharedDataPointer<QCommandLineOptionPrivate> d;
};

Q_DECLARE_SHARED(QCommandLineOption)

QT_END_NAMESPACE

#endif // QCOMMANDLINEOPTION_H
