/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This program is free software; you can redistribute it and/or
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

#ifndef QtFileDownloader_h
#define QtFileDownloader_h

#include <QNetworkReply>
#include <QNetworkRequest>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

QT_BEGIN_NAMESPACE
class QFile;
class QNetworkAccessManager;
class QNetworkRequest;
QT_END_NAMESPACE

namespace WebCore {
class ResourceError;
}

namespace WebKit {
class Download;

class QtFileDownloader : public QObject {
    Q_OBJECT
public:
    QtFileDownloader(Download*, PassOwnPtr<QNetworkReply>);
    virtual ~QtFileDownloader();
    void cancel();
    void init();
    void startTransfer(const QString& destination);

    enum DownloadError {
        DownloadErrorAborted = 0,
        DownloadErrorCannotWriteToFile,
        DownloadErrorCannotOpenFile,
        DownloadErrorDestinationAlreadyExists,
        DownloadErrorCancelled,
        DownloadErrorCannotDetermineFilename,
        DownloadErrorNetworkFailure
    };

private Q_SLOTS:
    void onReadyRead();
    void onFinished();
    void onError(QNetworkReply::NetworkError);

private:
    void abortDownloadWritingAndEmitError(QtFileDownloader::DownloadError);
    QString determineFilename();
    void handleDownloadResponse();

    Download* m_download;
    OwnPtr<QNetworkReply> m_reply;
    OwnPtr<QFile> m_destinationFile;
    QNetworkReply::NetworkError m_error;
    bool m_headersRead;
};

} // namespace WebKit

#endif
