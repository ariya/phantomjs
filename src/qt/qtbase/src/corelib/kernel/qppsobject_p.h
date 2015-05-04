/****************************************************************************
 **
 ** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QPPSOBJECT_P_H
#define QPPSOBJECT_P_H

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

#include "qppsattribute_p.h"

#include <QMap>
#include <QObject>
#include <QVariantMap>

QT_BEGIN_NAMESPACE

class QPpsObjectPrivate;

class Q_CORE_EXPORT QPpsObject : public QObject
{
    Q_OBJECT

public:
    enum OpenMode {
        Publish = 1,
        Subscribe = 2,
        PublishSubscribe = Publish | Subscribe,
        Create = 4,
        DeleteContents = 8
    };
    Q_DECLARE_FLAGS(OpenModes, OpenMode)

    explicit QPpsObject(const QString &path, QObject *parent = 0);
    virtual ~QPpsObject();

    int error() const;
    QString errorString() const;

    bool isReadyReadEnabled() const;
    bool isBlocking() const;
    bool setBlocking(bool enable);
    bool isOpen() const;

    bool open(QPpsObject::OpenModes mode = QPpsObject::PublishSubscribe);
    bool close();
    bool remove();

    QByteArray read(bool *ok = 0);
    bool write(const QByteArray &byteArray);

    int writeMessage(const QString &msg, const QVariantMap &dat);
    int writeMessage(const QString &msg, const QString &id, const QVariantMap &dat);

    static QVariantMap decode(const QByteArray &rawData, bool *ok = 0);
    static QPpsAttributeMap decodeWithFlags(const QByteArray &rawData, bool *ok = 0);
    static QPpsAttributeMap decodeWithFlags(const QByteArray &rawData,
                                            QPpsAttribute *objectAttribute, bool *ok = 0);

    static QByteArray encode(const QVariantMap &ppsData, bool *ok = 0);
    static QByteArray encodeMessage(const QString &msg, const QVariantMap &dat, bool *ok = 0);
    static QByteArray encodeMessage(const QString &msg, const QString &id, const QVariantMap &dat,
                                    bool *ok = 0);

    static int sendMessage(const QString &path, const QString &message);
    static int sendMessage(const QString &path, const QVariantMap &message);
    static int sendMessage(const QString &path, const QString &msg, const QVariantMap &dat);
    static int sendMessage(const QString &path, const QByteArray &ppsData);

public Q_SLOTS:
    void setReadyReadEnabled(bool enable);

Q_SIGNALS:
    void readyRead();

private:
    QScopedPointer<QPpsObjectPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QPpsObject)
    Q_DISABLE_COPY(QPpsObject)
};

QT_END_NAMESPACE

#endif // QPPSOBJECT_P_H
