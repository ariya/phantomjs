/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef PageViewportControllerClient_h
#define PageViewportControllerClient_h

#include <wtf/Noncopyable.h>

namespace WebCore {
class FloatPoint;
class IntSize;
}

namespace WebKit {

class PageViewportController;

class PageViewportControllerClient {
    WTF_MAKE_NONCOPYABLE(PageViewportControllerClient);
public:
    PageViewportControllerClient() { }
    virtual ~PageViewportControllerClient() { }

    virtual void setViewportPosition(const WebCore::FloatPoint& contentsPoint) = 0;
    virtual void setPageScaleFactor(float) = 0;

    virtual void didChangeContentsSize(const WebCore::IntSize&) = 0;
    virtual void didChangeVisibleContents() = 0;
    virtual void didChangeViewportAttributes() = 0;

    virtual void setController(PageViewportController*) = 0;
};

} // namespace WebKit

#endif // PageViewportControllerClient_h
