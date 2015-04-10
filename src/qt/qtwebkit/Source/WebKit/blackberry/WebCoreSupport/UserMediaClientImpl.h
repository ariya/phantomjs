/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef UserMediaClientImpl_h
#define UserMediaClientImpl_h

#if ENABLE(MEDIA_STREAM)

#include "MediaStreamSource.h"
#include "UserMediaClient.h"

#include <wtf/PassRefPtr.h>

namespace BlackBerry {
namespace WebKit {
class WebPage;
}
}

namespace WebCore {

class UserMediaRequest;

class UserMediaClientImpl : public WebCore::UserMediaClient {
public:
    UserMediaClientImpl(BlackBerry::WebKit::WebPage*);
    ~UserMediaClientImpl();

    void pageDestroyed();
    void requestUserMedia(PassRefPtr<WebCore::UserMediaRequest>, const WebCore::MediaStreamSourceVector& audioSources, const WebCore::MediaStreamSourceVector& videoSources);
    void cancelUserMediaRequest(WebCore::UserMediaRequest*);

private:
    UserMediaClientImpl();

    BlackBerry::WebKit::WebPage* m_page;
};

}

#endif
#endif
