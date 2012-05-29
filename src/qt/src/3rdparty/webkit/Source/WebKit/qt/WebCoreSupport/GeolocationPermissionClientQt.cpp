/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GeolocationPermissionClientQt.h"

#include "Geolocation.h"

#include "qwebframe.h"
#include "qwebkitglobal.h"
#include "qwebpage.h"

namespace WebCore {

#if ENABLE(GEOLOCATION)

static GeolocationPermissionClientQt* s_geolocationPermission;

GeolocationPermissionClientQt* GeolocationPermissionClientQt::geolocationPermissionClient()
{
    if (s_geolocationPermission)
        return s_geolocationPermission;

    s_geolocationPermission = new GeolocationPermissionClientQt();
    return s_geolocationPermission;
}

GeolocationPermissionClientQt::GeolocationPermissionClientQt()
{
}

GeolocationPermissionClientQt::~GeolocationPermissionClientQt()
{
}

void GeolocationPermissionClientQt::requestGeolocationPermissionForFrame(QWebFrame* webFrame, Geolocation* listener)
{
    m_pendingPermissionRequests.insert(webFrame, listener);

    QWebPage* page = webFrame->page();
    emit page->featurePermissionRequested(webFrame, QWebPage::Geolocation);
}


void GeolocationPermissionClientQt::cancelGeolocationPermissionRequestForFrame(QWebFrame* webFrame, Geolocation* listener)
{
    m_pendingPermissionRequests.remove(webFrame);

    QWebPage* page = webFrame->page();
    emit page->featurePermissionRequestCanceled(webFrame, QWebPage::Geolocation);
}

void GeolocationPermissionClientQt::setPermission(QWebFrame* webFrame, QWebPage::PermissionPolicy permission)
{  
    if (!m_pendingPermissionRequests.contains(webFrame)) 
        return;

    Geolocation* listener = m_pendingPermissionRequests.value(webFrame);

    if (permission == QWebPage::PermissionGrantedByUser)
        listener->setIsAllowed(true);
    else if (permission == QWebPage::PermissionDeniedByUser)
        listener->setIsAllowed(false);
    else
        return;

    m_pendingPermissionRequests.remove(webFrame);
}

#endif // ENABLE(GEOLOCATION)
}
