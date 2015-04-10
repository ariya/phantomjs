/*
 * Copyright (C) 2011 Andreas Kling <kling@webkit.org>
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

#ifndef QtWebError_h
#define QtWebError_h

#include "qwebdownloaditem_p.h"
#include <QtNetwork/QNetworkReply>
#include <WKError.h>
#include <WKRetainPtr.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace WebKit {

class QtWebError {
public:
    enum Type {
        NoError,
        InternalError,
        NetworkError,
        HttpError,
        DownloadError
    };

    Type type() const;
    QString url() const;
    int errorCode() const;
    QString description() const;
    bool isCancellation() const;

    int errorCodeAsHttpStatusCode() const { return errorCode(); }
    QNetworkReply::NetworkError errorCodeAsNetworkError() const { return static_cast<QNetworkReply::NetworkError>(errorCode()); }
    QWebDownloadItem::DownloadError errorCodeAsDownloadError() const { return static_cast<QWebDownloadItem::DownloadError>(errorCode()); }

    QtWebError(const QtWebError&);

    QtWebError(WKErrorRef);

private:
    WKRetainPtr<WKErrorRef> error;
};

} // namespace WebKit

#endif /* QtWebError_h */
