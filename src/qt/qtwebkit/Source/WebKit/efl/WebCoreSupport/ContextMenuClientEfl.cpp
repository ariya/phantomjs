/*
 * Copyright (C) 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2009 Igalia S.L.
 * Copyright (C) 2010 ProFUSION embedded systems
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ContextMenuClientEfl.h"

#if ENABLE(CONTEXT_MENUS)

#include "ContextMenu.h"
#include "HitTestResult.h"
#include "KURL.h"
#include "NotImplemented.h"
#include <wtf/RefCounted.h>

namespace WebCore {

void ContextMenuClientEfl::contextMenuDestroyed()
{
    delete this;
}

PassOwnPtr<ContextMenu> ContextMenuClientEfl::customizeMenu(PassOwnPtr<ContextMenu> menu)
{
    // We ignore this client callback and do context menu customization when we are told to show the menu.
    return menu;
}

void ContextMenuClientEfl::contextMenuItemSelected(ContextMenuItem*, const ContextMenu*)
{
    notImplemented();
}

void ContextMenuClientEfl::downloadURL(const KURL&)
{
    notImplemented();
}

void ContextMenuClientEfl::lookUpInDictionary(Frame*)
{
    notImplemented();
}

void ContextMenuClientEfl::speak(const String&)
{
    notImplemented();
}

bool ContextMenuClientEfl::isSpeaking()
{
    notImplemented();
    return false;
}

void ContextMenuClientEfl::stopSpeaking()
{
    notImplemented();
}

void ContextMenuClientEfl::searchWithGoogle(const Frame*)
{
    notImplemented();
}

}

#endif // ENABLE(CONTEXT_MENUS)
