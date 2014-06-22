/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#import "config.h"
#import "Chrome.h"

#import "BlockExceptions.h"
#import "ChromeClient.h"

namespace WebCore {


void Chrome::focusNSView(NSView* view)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    // Handle the WK2 case where there is no view passed in.
    if (!view) {
        client()->makeFirstResponder();
        return;
    }
    
    NSResponder *firstResponder = client()->firstResponder();
    if (firstResponder == view)
        return;

    if (![view window] || ![view superview] || ![view acceptsFirstResponder])
        return;

    client()->makeFirstResponder(view);

    // Setting focus can actually cause a style change which might
    // remove the view from its superview while it's being made
    // first responder. This confuses AppKit so we must restore
    // the old first responder.
    if (![view superview])
        client()->makeFirstResponder(firstResponder);

    END_BLOCK_OBJC_EXCEPTIONS;
}

} // namespace WebCore
