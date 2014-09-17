/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebExceptionHandler.h"

// To avoid a global static symbol, we declare it in a function
static inline WebExceptionHandler& globalExceptionHandler()
{
    static WebExceptionHandler s_globalExceptionHandler;
    return s_globalExceptionHandler;
}

void webInstallExceptionHandler(WebExceptionHandler handler)
{
    ASSERT(handler);
    globalExceptionHandler() = handler;
}

void webRaiseDOMException(WebDOMExceptionCode ec)
{
    ASSERT(ec);
    if (WebExceptionHandler& handler = globalExceptionHandler())
        (*handler)(ec);
}
