/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QBEARERPLUGIN_P_H
#define QBEARERPLUGIN_P_H

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

#include "qbearerengine_p.h"

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

struct Q_NETWORK_EXPORT QBearerEngineFactoryInterface : public QFactoryInterface
{
    virtual QBearerEngine *create(const QString &key) const = 0;
};

#define QBearerEngineFactoryInterface_iid "com.trolltech.Qt.QBearerEngineFactoryInterface"
Q_DECLARE_INTERFACE(QBearerEngineFactoryInterface, QBearerEngineFactoryInterface_iid)

class Q_NETWORK_EXPORT QBearerEnginePlugin : public QObject, public QBearerEngineFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QBearerEngineFactoryInterface:QFactoryInterface)

public:
    explicit QBearerEnginePlugin(QObject *parent = 0);
    virtual ~QBearerEnginePlugin();

    virtual QStringList keys() const = 0;
    virtual QBearerEngine *create(const QString &key) const = 0;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_BEARERMANAGEMENT

#endif // QBEARERPLUGIN_P_H
