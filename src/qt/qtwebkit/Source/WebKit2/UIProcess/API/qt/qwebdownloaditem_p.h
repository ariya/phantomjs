/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef qwebdownloaditem_p_h
#define qwebdownloaditem_p_h

#include "qwebkitglobal.h"
#include <QObject>
#include <QUrl>

class QWebDownloadItemPrivate;

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace WebKit {
class QtDownloadManager;
class QtWebError;
}

class QWEBKIT_EXPORT QWebDownloadItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint64 expectedContentLength READ expectedContentLength CONSTANT FINAL)
    Q_PROPERTY(QString destinationPath READ destinationPath WRITE setDestinationPath)
    Q_PROPERTY(QString suggestedFilename READ suggestedFilename CONSTANT FINAL)
    Q_PROPERTY(quint64 totalBytesReceived READ totalBytesReceived NOTIFY totalBytesReceivedChanged FINAL)
    Q_PROPERTY(QUrl url READ url CONSTANT FINAL)
    Q_ENUMS(DownloadError)
public:
    virtual ~QWebDownloadItem();

    enum DownloadError {
        Aborted = 0,
        CannotWriteToFile,
        CannotOpenFile,
        DestinationAlreadyExists,
        Cancelled,
        CannotDetermineFilename,
        NetworkFailure
    };

    QUrl url() const;
    QString destinationPath() const;
    QString suggestedFilename() const;
    QString mimeType() const;
    quint64 expectedContentLength() const;
    quint64 totalBytesReceived() const;
    void setDestinationPath(const QString& destination);

public Q_SLOTS:
    void start();
    void cancel();

Q_SIGNALS:
    void destinationFileCreated(const QString& destinationPath);
    void totalBytesReceivedChanged(quint64 bytesReceived);
    void succeeded();
    void failed(QWebDownloadItem::DownloadError error, const QUrl& url, const QString& description);

private:
    QWebDownloadItem(QObject* parent = 0);
    QWebDownloadItemPrivate* d;

    friend class WebKit::QtDownloadManager;
    friend class QQuickWebViewPrivate;
};

#endif // qwebdownloaditem_p_h
