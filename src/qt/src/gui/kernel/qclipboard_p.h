/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QCLIPBOARD_P_H
#define QCLIPBOARD_P_H

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

#include "private/qobject_p.h"
#include "QtGui/qmime.h"
#include "QtGui/qclipboard.h"

QT_BEGIN_NAMESPACE

class QClipboardPrivate;

class QMimeDataWrapper : public QMimeSource
{
public:
    QMimeDataWrapper() {}

    const char* format(int n) const;
    QByteArray encodedData(const char*) const;

    mutable QList<QByteArray> formats;
    const QMimeData *data;
};

class QMimeSourceWrapper : public QMimeData
{
public:
    QMimeSourceWrapper(QClipboardPrivate *priv, QClipboard::Mode m);
    ~QMimeSourceWrapper();

    bool hasFormat(const QString &mimetype) const;
    QStringList formats() const;

protected:
    QVariant retrieveData(const QString &mimetype, QVariant::Type) const;
private:
    QClipboardPrivate *d;
    QClipboard::Mode mode;
    QMimeSource *source;
};


class QClipboardPrivate : public QObjectPrivate
{
public:
    QClipboardPrivate() : QObjectPrivate() {
        for (int i = 0; i <= QClipboard::LastMode; ++i) {
            compat_data[i] = 0;
            wrapper[i] = new QMimeDataWrapper();
        }
    }
    ~QClipboardPrivate() {
        for (int i = 0; i <= QClipboard::LastMode; ++i) {
            delete wrapper[i];
            delete compat_data[i];
        }
    }

    mutable QMimeDataWrapper *wrapper[QClipboard::LastMode + 1];
    mutable QMimeSource *compat_data[QClipboard::LastMode + 1];
};

inline QMimeSourceWrapper::QMimeSourceWrapper(QClipboardPrivate *priv, QClipboard::Mode m)
    : QMimeData()
{
    d = priv;
    mode = m;
    source = d->compat_data[mode];
}

inline QMimeSourceWrapper::~QMimeSourceWrapper()
{
    if (d->compat_data[mode] == source)
        d->compat_data[mode] = 0;
    delete source;
}

QT_END_NAMESPACE

#endif // QCLIPBOARD_P_H
