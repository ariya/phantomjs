/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebPopupMenu.h"

#import "PlatformPopupMenuData.h"
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/PopupMenuClient.h>

using namespace WebCore;

namespace WebKit {

void WebPopupMenu::setUpPlatformData(const IntRect&, PlatformPopupMenuData& data)
{
#if USE(APPKIT)
    // FIXME: font will be nil here for custom fonts, we should fix that.
    NSFont *font = m_popupClient->menuStyle().font().primaryFont()->getNSFont();
    if (!font)
        return;
    
    CFDictionaryRef fontDescriptorAttributes = (CFDictionaryRef)[[font fontDescriptor] fontAttributes];
    if (!fontDescriptorAttributes)
        return;
    
    data.fontInfo.fontAttributeDictionary = fontDescriptorAttributes;
    data.shouldPopOver = m_popupClient->shouldPopOver();
#else
    UNUSED_PARAM(data);
#endif
}

} // namespace WebKit
