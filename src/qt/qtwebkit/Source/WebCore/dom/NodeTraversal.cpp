/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "NodeTraversal.h"

#include "ContainerNode.h"
#include "PseudoElement.h"

namespace WebCore {

namespace NodeTraversal {

Node* previousIncludingPseudo(const Node* current, const Node* stayWithin)
{
    Node* previous;
    if (current == stayWithin)
        return 0;
    if ((previous = current->pseudoAwarePreviousSibling())) {
        while (previous->pseudoAwareLastChild())
            previous = previous->pseudoAwareLastChild();
        return previous;
    }
    return current->parentNode();
}

Node* nextIncludingPseudo(const Node* current, const Node* stayWithin)
{
    Node* next;
    if ((next = current->pseudoAwareFirstChild()))
        return next;
    if (current == stayWithin)
        return 0;
    if ((next = current->pseudoAwareNextSibling()))
        return next;
    for (current = current->parentNode(); current; current = current->parentNode()) {
        if (current == stayWithin)
            return 0;
        if ((next = current->pseudoAwareNextSibling()))
            return next;
    }
    return 0;
}

Node* nextIncludingPseudoSkippingChildren(const Node* current, const Node* stayWithin)
{
    Node* next;
    if (current == stayWithin)
        return 0;
    if ((next = current->pseudoAwareNextSibling()))
        return next;
    for (current = current->parentNode(); current; current = current->parentNode()) {
        if (current == stayWithin)
            return 0;
        if ((next = current->pseudoAwareNextSibling()))
            return next;
    }
    return 0;
}

Node* nextAncestorSibling(const Node* current)
{
    ASSERT(!current->nextSibling());
    for (current = current->parentNode(); current; current = current->parentNode()) {
        if (current->nextSibling())
            return current->nextSibling();
    }
    return 0;
}

Node* nextAncestorSibling(const Node* current, const Node* stayWithin)
{
    ASSERT(!current->nextSibling());
    ASSERT(current != stayWithin);
    for (current = current->parentNode(); current; current = current->parentNode()) {
        if (current == stayWithin)
            return 0;
        if (current->nextSibling())
            return current->nextSibling();
    }
    return 0;
}

Node* previous(const Node* current, const Node* stayWithin)
{
    if (current == stayWithin)
        return 0;
    if (current->previousSibling()) {
        Node* previous = current->previousSibling();
        while (previous->lastChild())
            previous = previous->lastChild();
        return previous;
    }
    return current->parentNode();
}

Node* previousSkippingChildren(const Node* current, const Node* stayWithin)
{
    if (current == stayWithin)
        return 0;
    if (current->previousSibling())
        return current->previousSibling();
    for (current = current->parentNode(); current; current = current->parentNode()) {
        if (current == stayWithin)
            return 0;
        if (current->previousSibling())
            return current->previousSibling();
    }
    return 0;
}

Node* nextPostOrder(const Node* current, const Node* stayWithin)
{
    if (current == stayWithin)
        return 0;
    if (!current->nextSibling())
        return current->parentNode();
    Node* next = current->nextSibling();
    while (next->firstChild())
        next = next->firstChild();
    return next;
}

static Node* previousAncestorSiblingPostOrder(const Node* current, const Node* stayWithin)
{
    ASSERT(!current->previousSibling());
    for (current = current->parentNode(); current; current = current->parentNode()) {
        if (current == stayWithin)
            return 0;
        if (current->previousSibling())
            return current->previousSibling();
    }
    return 0;
}

Node* previousPostOrder(const Node* current, const Node* stayWithin)
{
    if (current->lastChild())
        return current->lastChild();
    if (current == stayWithin)
        return 0;
    if (current->previousSibling())
        return current->previousSibling();
    return previousAncestorSiblingPostOrder(current, stayWithin);
}

Node* previousSkippingChildrenPostOrder(const Node* current, const Node* stayWithin)
{
    if (current == stayWithin)
        return 0;
    if (current->previousSibling())
        return current->previousSibling();
    return previousAncestorSiblingPostOrder(current, stayWithin);
}

}
}
