/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "LayoutRepainter.h"

#include "RenderObject.h"

namespace WebCore {

LayoutRepainter::LayoutRepainter(RenderObject& object, bool checkForRepaint)
    : m_object(object)
    , m_repaintContainer(0)
    , m_checkForRepaint(checkForRepaint)
{
    if (m_checkForRepaint) {
        m_repaintContainer = m_object.containerForRepaint();
        m_oldBounds = m_object.clippedOverflowRectForRepaint(m_repaintContainer);
        m_oldOutlineBox = m_object.outlineBoundsForRepaint(m_repaintContainer);
    }
}

bool LayoutRepainter::repaintAfterLayout()
{
    return m_checkForRepaint ? m_object.repaintAfterLayoutIfNeeded(m_repaintContainer, m_oldBounds, m_oldOutlineBox) : false;
}

} // namespace WebCore
