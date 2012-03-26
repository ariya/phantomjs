/*
 * Copyright (C) 2006 Apple Computer, Inc.
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
#include "LocalCurrentGraphicsContext.h"

#include "GraphicsContext.h"
#include <AppKit/NSGraphicsContext.h>

namespace WebCore {

LocalCurrentGraphicsContext::LocalCurrentGraphicsContext(GraphicsContext* graphicsContext)
{
    m_savedGraphicsContext = graphicsContext;
    graphicsContext->save();
    
    if (graphicsContext->platformContext() == [[NSGraphicsContext currentContext] graphicsPort]) {
        m_savedNSGraphicsContext = 0;
        return;
    }
    
    m_savedNSGraphicsContext = [[NSGraphicsContext currentContext] retain];
    NSGraphicsContext* newContext = [NSGraphicsContext graphicsContextWithGraphicsPort:graphicsContext->platformContext() flipped:YES];
    [NSGraphicsContext setCurrentContext:newContext];
}

LocalCurrentGraphicsContext::~LocalCurrentGraphicsContext()
{
    m_savedGraphicsContext->restore();

    if (m_savedNSGraphicsContext) {
        [NSGraphicsContext setCurrentContext:m_savedNSGraphicsContext];
        [m_savedNSGraphicsContext release];
    }
}

}
