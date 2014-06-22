/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 */

#include "config.h"
#if USE(COORDINATED_GRAPHICS)

#include "WKView.h"

#include "WKAPICast.h"
#include "WebView.h"

using namespace WebCore;
using namespace WebKit;

WKViewRef WKViewCreate(WKContextRef contextRef, WKPageGroupRef pageGroupRef)
{
    RefPtr<WebView> webView = WebView::create(toImpl(contextRef), toImpl(pageGroupRef));
    return toAPI(webView.release().leakRef());
}

void WKViewInitialize(WKViewRef viewRef)
{
    toImpl(viewRef)->initialize();
}

WKSize WKViewGetSize(WKViewRef viewRef)
{
    return toAPI(toImpl(viewRef)->size());
}

void WKViewSetSize(WKViewRef viewRef, WKSize size)
{
    toImpl(viewRef)->setSize(toIntSize(size));
}

void WKViewSetViewClient(WKViewRef viewRef, const WKViewClient* client)
{
    toImpl(viewRef)->initializeClient(client);
}

bool WKViewIsFocused(WKViewRef viewRef)
{
    return toImpl(viewRef)->isFocused();
}

void WKViewSetIsFocused(WKViewRef viewRef, bool isFocused)
{
    toImpl(viewRef)->setFocused(isFocused);
}

bool WKViewIsVisible(WKViewRef viewRef)
{
    return toImpl(viewRef)->isVisible();
}

void WKViewSetIsVisible(WKViewRef viewRef, bool isVisible)
{
    toImpl(viewRef)->setVisible(isVisible);
}

float WKViewGetContentScaleFactor(WKViewRef viewRef)
{
    return toImpl(viewRef)->contentScaleFactor();
}

void WKViewSetContentScaleFactor(WKViewRef viewRef, float scale)
{
    toImpl(viewRef)->setContentScaleFactor(scale);
}

WKPoint WKViewGetContentPosition(WKViewRef viewRef)
{
    const WebCore::FloatPoint& result = toImpl(viewRef)->contentPosition();
    return WKPointMake(result.x(), result.y());
}

void WKViewSetContentPosition(WKViewRef viewRef, WKPoint position)
{
    toImpl(viewRef)->setContentPosition(WebCore::FloatPoint(position.x, position.y));
}

void WKViewSetUserViewportTranslation(WKViewRef viewRef, double tx, double ty)
{
    toImpl(viewRef)->setUserViewportTranslation(tx, ty);
}

WKPoint WKViewUserViewportToContents(WKViewRef viewRef, WKPoint point)
{
    const WebCore::IntPoint& result = toImpl(viewRef)->userViewportToContents(toIntPoint(point));
    return WKPointMake(result.x(), result.y());
}

WKPoint WKViewUserViewportToScene(WKViewRef viewRef, WKPoint point)
{
    WebCore::IntPoint result = toImpl(viewRef)->userViewportToScene(toIntPoint(point));
    return WKPointMake(result.x(), result.y());
}

WKPoint WKViewContentsToUserViewport(WKViewRef viewRef, WKPoint point)
{
    WebCore::IntPoint result = toImpl(viewRef)->contentsToUserViewport(toIntPoint(point));
    return WKPointMake(result.x(), result.y());
}

void WKViewPaintToCurrentGLContext(WKViewRef viewRef)
{
    toImpl(viewRef)->paintToCurrentGLContext();
}

WKPageRef WKViewGetPage(WKViewRef viewRef)
{
    return toImpl(viewRef)->pageRef();
}

void WKViewSetDrawsBackground(WKViewRef viewRef, bool flag)
{
    toImpl(viewRef)->setDrawsBackground(flag);
}

bool WKViewGetDrawsBackground(WKViewRef viewRef)
{
    return toImpl(viewRef)->drawsBackground();
}

void WKViewSetDrawsTransparentBackground(WKViewRef viewRef, bool flag)
{
    toImpl(viewRef)->setDrawsTransparentBackground(flag);
}

bool WKViewGetDrawsTransparentBackground(WKViewRef viewRef)
{
    return toImpl(viewRef)->drawsTransparentBackground();
}

void WKViewSuspendActiveDOMObjectsAndAnimations(WKViewRef viewRef)
{
    toImpl(viewRef)->suspendActiveDOMObjectsAndAnimations();
}

void WKViewResumeActiveDOMObjectsAndAnimations(WKViewRef viewRef)
{
    toImpl(viewRef)->resumeActiveDOMObjectsAndAnimations();
}

void WKViewSetShowsAsSource(WKViewRef viewRef, bool flag)
{
    toImpl(viewRef)->setShowsAsSource(flag);
}

bool WKViewGetShowsAsSource(WKViewRef viewRef)
{
    return toImpl(viewRef)->showsAsSource();
}

bool WKViewExitFullScreen(WKViewRef viewRef)
{
#if ENABLE(FULLSCREEN_API)
    return toImpl(viewRef)->exitFullScreen();
#else
    UNUSED_PARAM(viewRef);
    return false;
#endif
}

void WKViewSetOpacity(WKViewRef view, double opacity)
{
    toImpl(view)->setOpacity(opacity);
}

double WKViewOpacity(WKViewRef view)
{
    return toImpl(view)->opacity();
}

void WKViewFindZoomableAreaForRect(WKViewRef viewRef, WKRect wkRect)
{
    IntRect rect = toIntRect(wkRect);
    toImpl(viewRef)->findZoomableAreaForPoint(rect.center(), rect.size());
}

#endif // USE(COORDINATED_GRAPHICS)
