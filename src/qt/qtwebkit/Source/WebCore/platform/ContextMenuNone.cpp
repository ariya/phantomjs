/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CONTEXT_MENUS) && USE(CROSS_PLATFORM_CONTEXT_MENUS)

#include "ContextMenu.h"

#include "NotImplemented.h"

namespace WebCore {

ContextMenu::ContextMenu(PlatformContextMenu menu)
{
    getContextMenuItems(menu, m_items);
}

void ContextMenu::getContextMenuItems(PlatformContextMenu, Vector<ContextMenuItem>&)
{
    notImplemented();
}

PlatformContextMenu ContextMenu::createPlatformContextMenuFromItems(const Vector<ContextMenuItem>&)
{
    notImplemented();
    return 0;
}

PlatformContextMenu ContextMenu::platformContextMenu() const
{
    return createPlatformContextMenuFromItems(m_items);
}

}
#endif // ENABLE(CONTEXT_MENUS) && USE(CROSS_PLATFORM_CONTEXT_MENUS)
