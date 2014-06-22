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

#include "config.h"
#include "WebPageGroupLoadDeferrer.h"

#include "PageGroupLoadDeferrer.h"
#include "WebPage.h"
#include "WebPage_p.h"

namespace BlackBerry {
namespace WebKit {

WebPageGroupLoadDeferrer::WebPageGroupLoadDeferrer(WebPage* webPage)
{
    WebCore::TimerBase::fireTimersInNestedEventLoop();
    m_pageGroupLoadDeferrer = new WebCore::PageGroupLoadDeferrer(webPage->d->m_page, true /* defer the page itself */);
}

WebPageGroupLoadDeferrer::~WebPageGroupLoadDeferrer()
{
    delete m_pageGroupLoadDeferrer;
}

} // namespace WebKit
} // namespace BlackBerry
