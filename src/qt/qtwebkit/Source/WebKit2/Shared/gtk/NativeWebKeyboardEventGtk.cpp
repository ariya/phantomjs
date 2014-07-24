/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011, 2012 Igalia S.L
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NativeWebKeyboardEvent.h"

#include "WebEventFactory.h"
#include <gdk/gdk.h>

using namespace WebCore;

namespace WebKit {

NativeWebKeyboardEvent::NativeWebKeyboardEvent(GdkEvent* event, const WebCore::CompositionResults& compositionResults, GtkInputMethodFilter::EventFakedForComposition faked)
    : WebKeyboardEvent(WebEventFactory::createWebKeyboardEvent(event, compositionResults))
    , m_nativeEvent(gdk_event_copy(event))
    , m_compositionResults(compositionResults)
    , m_fakeEventForComposition(faked == GtkInputMethodFilter::EventFaked)
{
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(const NativeWebKeyboardEvent& event)
    : WebKeyboardEvent(WebEventFactory::createWebKeyboardEvent(event.nativeEvent(), event.compositionResults()))
    , m_nativeEvent(gdk_event_copy(event.nativeEvent()))
    , m_compositionResults(event.compositionResults())
    , m_fakeEventForComposition(event.isFakeEventForComposition())
{
}

} // namespace WebKit
