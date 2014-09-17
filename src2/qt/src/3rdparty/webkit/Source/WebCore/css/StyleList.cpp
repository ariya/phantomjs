/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 *               2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 *               2001-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008 Apple Inc. All rights reserved.
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
#include "StyleList.h"

#include <wtf/PassRefPtr.h>

namespace WebCore {

void StyleList::append(PassRefPtr<StyleBase> child)
{
    StyleBase* c = child.get();
    m_children.append(child);
    c->insertedIntoParent();
}

void StyleList::insert(unsigned position, PassRefPtr<StyleBase> child)
{
    StyleBase* c = child.get();
    if (position >= length())
        m_children.append(child);
    else
        m_children.insert(position, child);
    c->insertedIntoParent();
}

void StyleList::remove(unsigned position)
{
    if (position >= length())
        return;
    m_children.remove(position);
}

}
