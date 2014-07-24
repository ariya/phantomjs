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

#include "config.h"
#include "WebViewportArguments.h"

#include "ViewportArguments.h"

namespace BlackBerry {
namespace WebKit {

WebViewportArguments::WebViewportArguments()
    :d(new WebCore::ViewportArguments(WebCore::ViewportArguments::ViewportMeta))
{
}

WebViewportArguments::~WebViewportArguments()
{
    delete d;
    d = 0;
}

float WebViewportArguments::zoom() const
{
    return d->zoom;
}

void WebViewportArguments::setZoom(float zoom)
{
    d->zoom = zoom;
}

float WebViewportArguments::minZoom() const
{
    return d->minZoom;
}

void WebViewportArguments::setMinZoom(float zoom)
{
    d->minZoom = zoom;
}

float WebViewportArguments::maxZoom() const
{
    return d->maxZoom;
}

void WebViewportArguments::setMaxZoom(float zoom)
{
    d->maxZoom = zoom;
}

float WebViewportArguments::width() const
{
    return d->width;
}

void WebViewportArguments::setWidth(float width)
{
    d->width = width;
}

float WebViewportArguments::height() const
{
    return d->height;
}

void WebViewportArguments::setHeight(float height)
{
    d->height = height;
}

float WebViewportArguments::devicePixelRatio() const
{
    return 0;
}

void WebViewportArguments::setDevicePixelRatio(float)
{
}

float WebViewportArguments::userZoom() const
{
    return d->userZoom;
}

void WebViewportArguments::setUserZoom(float zoom)
{
    d->userZoom = zoom;
}

bool WebViewportArguments::operator==(const WebViewportArguments& other)
{
    return *d == *(other.d);
}

bool WebViewportArguments::operator!=(const WebViewportArguments& other)
{
    return *d != *(other.d);
}

} // namespace WebKit
} // namespace BlackBerry
