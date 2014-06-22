/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/


#ifndef qwebdownloaditem_p_p_h
#define qwebdownloaditem_p_p_h

#include "qwebdownloaditem_p.h"
#include <QUrl>

namespace WebKit {
class DownloadProxy;
}

class QWebDownloadItemPrivate : public QObject {
    Q_OBJECT
public:
    QWebDownloadItemPrivate(QWebDownloadItem*);

    void didReceiveResponse(QWebDownloadItem* download) { emit receivedResponse(download); }

    QWebDownloadItem* q;

    WebKit::DownloadProxy* downloadProxy;

    QUrl sourceUrl;
    QString suggestedFilename;
    QString destinationPath;
    QString mimeType;
    quint64 expectedContentLength;
    quint64 totalBytesReceived;

Q_SIGNALS:
    void receivedResponse(QWebDownloadItem*);
};

#endif // qwebdownloaditem_p_p_h
