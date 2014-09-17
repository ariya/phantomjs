/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef WebExceptionHandler_h
#define WebExceptionHandler_h

typedef int WebDOMExceptionCode;
typedef void (*WebExceptionHandler)(WebDOMExceptionCode);

// Used from the outside to register a callback that gets fired whenever an exception is raised
void webInstallExceptionHandler(WebExceptionHandler);

// Never used by the bindings, only indirectly by webDOMRaiseError
void webRaiseDOMException(WebDOMExceptionCode);

// Used from the bindings
inline void webDOMRaiseError(WebDOMExceptionCode ec) 
{
    if (ec)
        webRaiseDOMException(ec);
}

#endif
