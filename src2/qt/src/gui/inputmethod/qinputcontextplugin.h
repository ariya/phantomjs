/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

/****************************************************************************
**
** Definition of QInputContextPlugin class
**
** Copyright (C) 2003-2004 immodule for Qt Project.  All rights reserved.
**
** This file is written to contribute to Nokia Corporation and/or its subsidiary(-ies) under their own
** license. You may use this file under your Qt license. Following
** description is copied from their original file headers. Contact
** immodule-qt@freedesktop.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef QINPUTCONTEXTPLUGIN_H
#define QINPUTCONTEXTPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_IM)

class QInputContext;
class QInputContextPluginPrivate;

struct Q_GUI_EXPORT QInputContextFactoryInterface : public QFactoryInterface
{
    virtual QInputContext *create( const QString &key ) = 0;
    virtual QStringList languages( const QString &key ) = 0;
    virtual QString displayName( const QString &key ) = 0;
    virtual QString description( const QString &key ) = 0;
};

#define QInputContextFactoryInterface_iid "com.trolltech.Qt.QInputContextFactoryInterface"
Q_DECLARE_INTERFACE(QInputContextFactoryInterface, QInputContextFactoryInterface_iid)

class Q_GUI_EXPORT QInputContextPlugin : public QObject, public QInputContextFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QInputContextFactoryInterface:QFactoryInterface)
public:
    explicit QInputContextPlugin(QObject *parent = 0);
    ~QInputContextPlugin();

    virtual QStringList keys() const = 0;
    virtual QInputContext *create( const QString &key ) = 0;
    virtual QStringList languages( const QString &key ) = 0;
    virtual QString displayName( const QString &key ) = 0;
    virtual QString description( const QString &key ) = 0;
};

#endif // QT_NO_IM

QT_END_NAMESPACE

QT_END_HEADER

#endif // QINPUTCONTEXTPLUGIN_H
