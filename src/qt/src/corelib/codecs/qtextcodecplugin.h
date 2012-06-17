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

#ifndef QTEXTCODECPLUGIN_H
#define QTEXTCODECPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>
#include <QtCore/qlist.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_TEXTCODECPLUGIN

class QTextCodec;

struct Q_CORE_EXPORT QTextCodecFactoryInterface : public QFactoryInterface
{
    virtual QTextCodec *create(const QString &key) = 0;
};

#define QTextCodecFactoryInterface_iid "com.trolltech.Qt.QTextCodecFactoryInterface"

Q_DECLARE_INTERFACE(QTextCodecFactoryInterface, QTextCodecFactoryInterface_iid)


class Q_CORE_EXPORT QTextCodecPlugin : public QObject, public QTextCodecFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextCodecFactoryInterface:QFactoryInterface)
public:
    explicit QTextCodecPlugin(QObject *parent = 0);
    ~QTextCodecPlugin();

    virtual QList<QByteArray> names() const = 0;
    virtual QList<QByteArray> aliases() const = 0;
    virtual QTextCodec *createForName(const QByteArray &name) = 0;

    virtual QList<int> mibEnums() const = 0;
    virtual QTextCodec *createForMib(int mib) = 0;

private:
    QStringList keys() const;
    QTextCodec *create(const QString &name);
};

#endif // QT_NO_TEXTCODECPLUGIN

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTEXTCODECPLUGIN_H
