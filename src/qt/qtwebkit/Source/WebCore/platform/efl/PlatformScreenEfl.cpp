/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformScreen.h"

#include "FloatRect.h"
#include "NotImplemented.h"
#include "Widget.h"

#include <Ecore_Evas.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

namespace WebCore {

int screenDepth(Widget*)
{
#ifdef HAVE_ECORE_X
    return ecore_x_default_depth_get(ecore_x_display_get(), ecore_x_default_screen_get());
#else
    return 24;
#endif
}

int screenDepthPerComponent(Widget* widget)
{
    if (!widget)
        return 8;

    int depth = screenDepth(widget);

    switch (depth) {
    // Special treat 0 as an error, and return 8 bit per component.
    case 0:
    case 24:
    case 32:
        return 8;
    case 8:
        return 2;
    default:
        return depth / 3;
    }
}

bool screenIsMonochrome(Widget* widget)
{
    return screenDepth(widget) < 2;
}

FloatRect screenRect(Widget* widget)
{
#ifdef HAVE_ECORE_X
    UNUSED_PARAM(widget);
    // Fallback to realistic values if the EcoreX call fails
    // and we cannot accurately detect the screen size.
    int width = 800;
    int height = 600;
    ecore_x_screen_size_get(ecore_x_default_screen_get(), &width, &height);
    return FloatRect(0, 0, width, height);
#else
    if (!widget || !widget->evas())
        return FloatRect();

    int x, y, w, h;
    ecore_evas_screen_geometry_get(ecore_evas_ecore_evas_get(widget->evas()), &x, &y, &w, &h);
    return FloatRect(x, y, w, h);
#endif
}

FloatRect screenAvailableRect(Widget* widget)
{
    notImplemented();
    return screenRect(widget);
}

void screenColorProfile(ColorProfile&)
{
    notImplemented();
}

}
