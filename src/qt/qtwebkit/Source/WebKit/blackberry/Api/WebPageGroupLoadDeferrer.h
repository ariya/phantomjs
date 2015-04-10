/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef WebPageGroupLoadDeferrer_h
#define WebPageGroupLoadDeferrer_h

#include "BlackBerryGlobal.h"
#include "BlackBerryPlatformMisc.h"

namespace WebCore {
class PageGroupLoadDeferrer;
}

namespace BlackBerry {
namespace WebKit {

class WebPage;

// WebPageGroupLoadDeferrer is supposed to be used in the same way as WebCore::PageGroupLoadDeferrer.
// Declare a WebPageGroupLoadDeferrer object in the scope where the page group should defer loading and DOM timers.
class BLACKBERRY_EXPORT WebPageGroupLoadDeferrer {
public:
    explicit WebPageGroupLoadDeferrer(WebPage*);
    ~WebPageGroupLoadDeferrer();
private:
    WebCore::PageGroupLoadDeferrer* m_pageGroupLoadDeferrer;
    DISABLE_COPY(WebPageGroupLoadDeferrer)
};

} // namespace WebKit
} // namespace BlackBerry

#endif // WebPageGroupLoadDeferrer_h
