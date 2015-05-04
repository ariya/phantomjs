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

#ifndef QFUTURESYNCHRONIZER_H
#define QFUTURESYNCHRONIZER_H

#include <QtCore/qfuture.h>

#ifndef QT_NO_QFUTURE

QT_BEGIN_NAMESPACE


template <typename T>
class QFutureSynchronizer
{
    Q_DISABLE_COPY(QFutureSynchronizer)

public:
    QFutureSynchronizer() : m_cancelOnWait(false) { }
    explicit QFutureSynchronizer(const QFuture<T> &future)
        : m_cancelOnWait(false)
    { addFuture(future); }
    ~QFutureSynchronizer()  { waitForFinished(); }

    void setFuture(const QFuture<T> &future)
    {
        waitForFinished();
        m_futures.clear();
        addFuture(future);
    }

    void addFuture(const QFuture<T> &future)
    {
        m_futures.append(future);
    }

    void waitForFinished()
    {
        if (m_cancelOnWait) {
            for (int i = 0; i < m_futures.count(); ++i) {
                 m_futures[i].cancel();
            }
        }

        for (int i = 0; i < m_futures.count(); ++i) {
             m_futures[i].waitForFinished();
         }
    }

    void clearFutures()
    {
        m_futures.clear();
    }

    QList<QFuture<T> > futures() const
    {
        return m_futures;
    }

    void setCancelOnWait(bool enabled)
    {
        m_cancelOnWait = enabled;
    }

    bool cancelOnWait() const
    {
        return m_cancelOnWait;
    }

protected:
    QList<QFuture<T> > m_futures;
    bool m_cancelOnWait;
};

QT_END_NAMESPACE
#endif // QT_NO_QFUTURE

#endif // QFUTURESYNCHRONIZER_H
