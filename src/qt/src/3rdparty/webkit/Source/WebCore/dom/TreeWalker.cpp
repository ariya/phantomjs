/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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
#include "TreeWalker.h"

#include "ExceptionCode.h"
#include "ContainerNode.h"
#include "NodeFilter.h"
#include "ScriptState.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

TreeWalker::TreeWalker(PassRefPtr<Node> rootNode, unsigned whatToShow, PassRefPtr<NodeFilter> filter, bool expandEntityReferences)
    : Traversal(rootNode, whatToShow, filter, expandEntityReferences)
    , m_current(root())
{
}

void TreeWalker::setCurrentNode(PassRefPtr<Node> node, ExceptionCode& ec)
{
    if (!node) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    m_current = node;
}

inline Node* TreeWalker::setCurrent(PassRefPtr<Node> node)
{
    m_current = node;
    return m_current.get();
}

Node* TreeWalker::parentNode(ScriptState* state)
{
    RefPtr<Node> node = m_current;
    while (node != root()) {
        node = node->parentNode();
        if (!node)
            return 0;
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        if (acceptNodeResult == NodeFilter::FILTER_ACCEPT)
            return setCurrent(node.release());
    }
    return 0;
}

Node* TreeWalker::firstChild(ScriptState* state)
{
    for (RefPtr<Node> node = m_current->firstChild(); node; ) {
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        switch (acceptNodeResult) {
            case NodeFilter::FILTER_ACCEPT:
                m_current = node.release();
                return m_current.get();
            case NodeFilter::FILTER_SKIP:
                if (node->firstChild()) {
                    node = node->firstChild();
                    continue;
                }
                break;
            case NodeFilter::FILTER_REJECT:
                break;
        }
        do {
            if (node->nextSibling()) {
                node = node->nextSibling();
                break;
            }
            ContainerNode* parent = node->parentNode();
            if (!parent || parent == root() || parent == m_current)
                return 0;
            node = parent;
        } while (node);
    }
    return 0;
}

Node* TreeWalker::lastChild(ScriptState* state)
{
    for (RefPtr<Node> node = m_current->lastChild(); node; ) {
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        switch (acceptNodeResult) {
            case NodeFilter::FILTER_ACCEPT:
                m_current = node.release();
                return m_current.get();
            case NodeFilter::FILTER_SKIP:
                if (node->lastChild()) {
                    node = node->lastChild();
                    continue;
                }
                break;
            case NodeFilter::FILTER_REJECT:
                break;
        }
        do {
            if (node->previousSibling()) {
                node = node->previousSibling();
                break;
            }
            ContainerNode* parent = node->parentNode();
            if (!parent || parent == root() || parent == m_current)
                return 0;
            node = parent;
        } while (node);
    }
    return 0;
}

Node* TreeWalker::previousSibling(ScriptState* state)
{
    RefPtr<Node> node = m_current;
    if (node == root())
        return 0;
    while (1) {
        for (RefPtr<Node> sibling = node->previousSibling(); sibling; ) {
            short acceptNodeResult = acceptNode(state, sibling.get());
            if (state && state->hadException())
                return 0;
            switch (acceptNodeResult) {
                case NodeFilter::FILTER_ACCEPT:
                    m_current = sibling.release();
                    return m_current.get();
                case NodeFilter::FILTER_SKIP:
                    if (sibling->lastChild()) {
                        sibling = sibling->lastChild();
                        node = sibling;
                        continue;
                    }
                    break;
                case NodeFilter::FILTER_REJECT:
                    break;
            }
            sibling = sibling->previousSibling();
        }
        node = node->parentNode();
        if (!node || node == root())
            return 0;
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        if (acceptNodeResult == NodeFilter::FILTER_ACCEPT)
            return 0;
    }
}

Node* TreeWalker::nextSibling(ScriptState* state)
{
    RefPtr<Node> node = m_current;
    if (node == root())
        return 0;
    while (1) {
        for (RefPtr<Node> sibling = node->nextSibling(); sibling; ) {
            short acceptNodeResult = acceptNode(state, sibling.get());
            if (state && state->hadException())
                return 0;
            switch (acceptNodeResult) {
                case NodeFilter::FILTER_ACCEPT:
                    m_current = sibling.release();
                    return m_current.get();
                case NodeFilter::FILTER_SKIP:
                    if (sibling->firstChild()) {
                        sibling = sibling->firstChild();
                        node = sibling;
                        continue;
                    }
                    break;
                case NodeFilter::FILTER_REJECT:
                    break;
            }
            sibling = sibling->nextSibling();
        }
        node = node->parentNode();
        if (!node || node == root())
            return 0;
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        if (acceptNodeResult == NodeFilter::FILTER_ACCEPT)
            return 0;
    }
}

Node* TreeWalker::previousNode(ScriptState* state)
{
    RefPtr<Node> node = m_current;
    while (node != root()) {
        while (Node* previousSibling = node->previousSibling()) {
            node = previousSibling;
            short acceptNodeResult = acceptNode(state, node.get());
            if (state && state->hadException())
                return 0;
            if (acceptNodeResult == NodeFilter::FILTER_REJECT)
                continue;
            while (Node* lastChild = node->lastChild()) {
                node = lastChild;
                acceptNodeResult = acceptNode(state, node.get());
                if (state && state->hadException())
                    return 0;
                if (acceptNodeResult == NodeFilter::FILTER_REJECT)
                    break;
            }
            if (acceptNodeResult == NodeFilter::FILTER_ACCEPT) {
                m_current = node.release();
                return m_current.get();
            }
        }
        if (node == root())
            return 0;
        ContainerNode* parent = node->parentNode();
        if (!parent)
            return 0;
        node = parent;
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        if (acceptNodeResult == NodeFilter::FILTER_ACCEPT)
            return setCurrent(node.release());
    }
    return 0;
}

Node* TreeWalker::nextNode(ScriptState* state)
{
    RefPtr<Node> node = m_current;
Children:
    while (Node* firstChild = node->firstChild()) {
        node = firstChild;
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        if (acceptNodeResult == NodeFilter::FILTER_ACCEPT)
            return setCurrent(node.release());
        if (acceptNodeResult == NodeFilter::FILTER_REJECT)
            break;
    }
    while (Node* nextSibling = node->traverseNextSibling(root())) {
        node = nextSibling;
        short acceptNodeResult = acceptNode(state, node.get());
        if (state && state->hadException())
            return 0;
        if (acceptNodeResult == NodeFilter::FILTER_ACCEPT)
            return setCurrent(node.release());
        if (acceptNodeResult == NodeFilter::FILTER_SKIP)
            goto Children;
    }
    return 0;
}

} // namespace WebCore
