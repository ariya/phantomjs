/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qnetworkreplyfileimpl_p.h"

#include "QtCore/qdatetime.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QDebug>

QT_BEGIN_NAMESPACE

QNetworkReplyFileImplPrivate::QNetworkReplyFileImplPrivate()
    : QNetworkReplyPrivate(), realFileSize(0)
{
}

QNetworkReplyFileImpl::~QNetworkReplyFileImpl()
{
}

QNetworkReplyFileImpl::QNetworkReplyFileImpl(QObject *parent, const QNetworkRequest &req, const QNetworkAccessManager::Operation op)
    : QNetworkReply(*new QNetworkReplyFileImplPrivate(), parent)
{
    setRequest(req);
    setUrl(req.url());
    setOperation(op);
    setFinished(true);
    QNetworkReply::open(QIODevice::ReadOnly);

    QNetworkReplyFileImplPrivate *d = (QNetworkReplyFileImplPrivate*) d_func();

    QUrl url = req.url();
    if (url.host() == QLatin1String("localhost"))
        url.setHost(QString());

#if !defined(Q_OS_WIN)
    // do not allow UNC paths on Unix
    if (!url.host().isEmpty()) {
        // we handle only local files
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Request for opening non-local file %1").arg(url.toString());
        setError(QNetworkReply::ProtocolInvalidOperationError, msg);
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
            Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ProtocolInvalidOperationError));
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
        return;
    }
#endif
    if (url.path().isEmpty())
        url.setPath(QLatin1String("/"));
    setUrl(url);


    QString fileName = url.toLocalFile();
    if (fileName.isEmpty()) {
        if (url.scheme() == QLatin1String("qrc")) {
            fileName = QLatin1Char(':') + url.path();
        } else {
#if defined(Q_OS_ANDROID)
            if (url.scheme() == QLatin1String("assets"))
                fileName = QLatin1String("assets:") + url.path();
            else
#endif
                fileName = url.toString(QUrl::RemoveAuthority | QUrl::RemoveFragment | QUrl::RemoveQuery);
        }
    }

    QFileInfo fi(fileName);
    if (fi.isDir()) {
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Cannot open %1: Path is a directory").arg(url.toString());
        setError(QNetworkReply::ContentOperationNotPermittedError, msg);
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
            Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentOperationNotPermittedError));
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
        return;
    }

    d->realFile.setFileName(fileName);
    bool opened = d->realFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    // could we open the file?
    if (!opened) {
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Error opening %1: %2")
                .arg(d->realFile.fileName(), d->realFile.errorString());

        if (d->realFile.exists()) {
            setError(QNetworkReply::ContentAccessDenied, msg);
            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentAccessDenied));
        } else {
            setError(QNetworkReply::ContentNotFoundError, msg);
            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentNotFoundError));
        }
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
        return;
    }

    setHeader(QNetworkRequest::LastModifiedHeader, fi.lastModified());
    d->realFileSize = fi.size();
    setHeader(QNetworkRequest::ContentLengthHeader, d->realFileSize);

    QMetaObject::invokeMethod(this, "metaDataChanged", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "downloadProgress", Qt::QueuedConnection,
        Q_ARG(qint64, d->realFileSize), Q_ARG(qint64, d->realFileSize));
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}
void QNetworkReplyFileImpl::close()
{
    Q_D(QNetworkReplyFileImpl);
    QNetworkReply::close();
    d->realFile.close();
}

void QNetworkReplyFileImpl::abort()
{
    Q_D(QNetworkReplyFileImpl);
    QNetworkReply::close();
    d->realFile.close();
}

qint64 QNetworkReplyFileImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyFileImpl);
    if (!d->realFile.isOpen())
        return QNetworkReply::bytesAvailable();
    return QNetworkReply::bytesAvailable() + d->realFile.bytesAvailable();
}

bool QNetworkReplyFileImpl::isSequential () const
{
    return true;
}

qint64 QNetworkReplyFileImpl::size() const
{
    Q_D(const QNetworkReplyFileImpl);
    return d->realFileSize;
}

/*!
    \internal
*/
qint64 QNetworkReplyFileImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyFileImpl);
    if (!d->realFile.isOpen())
        return -1;
    qint64 ret = d->realFile.read(data, maxlen);
    if (bytesAvailable() == 0 && d->realFile.isOpen())
        d->realFile.close();
    if (ret == 0 && bytesAvailable() == 0)
        return -1;
    else
        return ret;
}


QT_END_NAMESPACE

#include "moc_qnetworkreplyfileimpl_p.cpp"

