/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (c) 2000 Daniel Molkentin (molkentin@kde.org)
 *  Copyright (c) 2000 Stefan Schimanski (schimmi@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
 *  Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "NavigatorMediaStream.h"

#if ENABLE(MEDIA_STREAM)

#include "Dictionary.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "Navigator.h"
#include "NavigatorUserMediaErrorCallback.h"
#include "NavigatorUserMediaSuccessCallback.h"
#include "Page.h"
#include "UserMediaController.h"
#include "UserMediaRequest.h"

namespace WebCore {

NavigatorMediaStream::NavigatorMediaStream()
{
}

NavigatorMediaStream::~NavigatorMediaStream()
{
}

void NavigatorMediaStream::webkitGetUserMedia(Navigator* navigator, const Dictionary& options, PassRefPtr<NavigatorUserMediaSuccessCallback> successCallback, PassRefPtr<NavigatorUserMediaErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (!successCallback)
        return;

    UserMediaController* userMedia = UserMediaController::from(navigator->frame() ? navigator->frame()->page() : 0);
    if (!userMedia) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    RefPtr<UserMediaRequest> request = UserMediaRequest::create(navigator->frame()->document(), userMedia, options, successCallback, errorCallback, ec);
    if (!request) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    request->start();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
