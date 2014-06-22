/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Inc. All rights reserved.
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
#include "LabelsNodeList.h"

#include "Element.h"
#include "HTMLLabelElement.h"
#include "HTMLNames.h"
#include "NodeRareData.h"

namespace WebCore {

using namespace HTMLNames;

LabelsNodeList::LabelsNodeList(Node* forNode)
    : LiveNodeList(forNode, LabelsNodeListType, InvalidateOnForAttrChange, NodeListIsRootedAtDocument)
{
}

LabelsNodeList::~LabelsNodeList()
{
    ownerNode()->nodeLists()->removeCacheWithAtomicName(this, LabelsNodeListType, starAtom);
} 
    
bool LabelsNodeList::nodeMatches(Element* testNode) const
{
    return isHTMLLabelElement(testNode) && toHTMLLabelElement(testNode)->control() == ownerNode();
}

} // namespace WebCore
