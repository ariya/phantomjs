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

#include "config.h"
#include "QtWebError.h"

#include <QtCore/QUrl>
#include <WKSharedAPICast.h>
#include <WKString.h>
#include <WKStringQt.h>
#include <WKType.h>
#include <WKURL.h>
#include <WKURLQt.h>

namespace WebKit {

QtWebError::QtWebError(WKErrorRef errorRef)
    : error(errorRef)
{
}

QtWebError::Type QtWebError::type() const
{
    WKRetainPtr<WKStringRef> errorDomainPtr = adoptWK(WKErrorCopyDomain(error.get()));
    WTF::String errorDomain = toWTFString(errorDomainPtr.get());

    if (errorDomain == "QtNetwork")
        return QtWebError::NetworkError;
    if (errorDomain == "HTTP")
        return QtWebError::HttpError;
    if (errorDomain == "Download")
        return QtWebError::DownloadError;
    return QtWebError::InternalError;
}

int QtWebError::errorCode() const
{
    return WKErrorGetErrorCode(error.get());
}

QString QtWebError::url() const
{
    return adoptToQString(WKErrorCopyFailingURL(error.get()));
}

QString QtWebError::description() const
{
    return adoptToQString(WKErrorCopyLocalizedDescription(error.get()));
}

bool QtWebError::isCancellation() const
{
    return toImpl(error.get())->platformError().isCancellation();
}

} // namespace WebKit
