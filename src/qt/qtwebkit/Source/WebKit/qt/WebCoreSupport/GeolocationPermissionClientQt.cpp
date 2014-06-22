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
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "qwebkitglobal.h"

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

void GeolocationPermissionClientQt::requestGeolocationPermissionForFrame(QWebFrameAdapter* webFrame, Geolocation* listener)
{
    m_pendingPermissionRequests.insert(webFrame, listener);

    QWebPageAdapter* page = QWebPageAdapter::kit(webFrame->frame->page());
    page->geolocationPermissionRequested(webFrame);
}


void GeolocationPermissionClientQt::cancelGeolocationPermissionRequestForFrame(QWebFrameAdapter* webFrame, Geolocation* listener)
{
    m_pendingPermissionRequests.remove(webFrame);

    QWebPageAdapter* page = QWebPageAdapter::kit(webFrame->frame->page());
    page->geolocationPermissionRequestCancelled(webFrame);
}

void GeolocationPermissionClientQt::setPermission(QWebFrameAdapter* webFrame, bool granted)
{  
    if (!m_pendingPermissionRequests.contains(webFrame)) 
        return;

    Geolocation* listener = m_pendingPermissionRequests.value(webFrame);
    listener->setIsAllowed(granted);

    m_pendingPermissionRequests.remove(webFrame);
}

#endif // ENABLE(GEOLOCATION)
}
