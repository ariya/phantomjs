/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2007 Staikos Computing Services Inc. <info@staikos.net>
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
#include "ContextMenu.h"

#include <Document.h>
#include <Frame.h>
#include <FrameView.h>
#include <QAction>
#include <wtf/Assertions.h>

namespace WebCore {

ContextMenu::ContextMenu()
{
}

ContextMenu::~ContextMenu()
{
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
    m_items.append(item);
}

unsigned ContextMenu::itemCount() const
{
    return m_items.count();
}

void ContextMenu::insertItem(unsigned position, ContextMenuItem& item)
{
    m_items.insert(position, item);
}

void ContextMenu::setPlatformDescription(PlatformMenuDescription)
{
    // doesn't make sense
}

PlatformMenuDescription ContextMenu::platformDescription() const
{
    return &m_items;
}

PlatformMenuDescription ContextMenu::releasePlatformDescription()
{
    return PlatformMenuDescription();
}

Vector<ContextMenuItem> contextMenuItemVector(const QList<ContextMenuItem>* items)
{
    int itemCount = items->size();
    Vector<ContextMenuItem> menuItemVector(itemCount);
    for (int i = 0; i < itemCount; ++i)
        menuItemVector.append(items->at(i));
    return menuItemVector;
}

PlatformMenuDescription platformMenuDescription(Vector<ContextMenuItem>& menuItemVector)
{
    // FIXME - Implement    
    return 0;
}

}
// vim: ts=4 sw=4 et
