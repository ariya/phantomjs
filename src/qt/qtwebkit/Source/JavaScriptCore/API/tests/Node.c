/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#include "Node.h"
#include <stddef.h>
#include <stdlib.h>

Node* Node_new(void)
{
    Node* node = (Node*)malloc(sizeof(Node));
    node->refCount = 0;
    node->nodeType = "Node";
    node->childNodesTail = NULL;
    
    return node;
}

void Node_appendChild(Node* node, Node* child)
{
    Node_ref(child);
    NodeLink* nodeLink = (NodeLink*)malloc(sizeof(NodeLink));
    nodeLink->node = child;
    nodeLink->prev = node->childNodesTail;
    node->childNodesTail = nodeLink;
}

void Node_removeChild(Node* node, Node* child)
{
    /* Linear search from tail -- good enough for our purposes here */
    NodeLink* current;
    NodeLink** currentHandle;
    for (currentHandle = &node->childNodesTail, current = *currentHandle; current; currentHandle = &current->prev, current = *currentHandle) {
        if (current->node == child) {
            Node_deref(current->node);
            *currentHandle = current->prev;
            free(current);
            break;
        }
    }
}

void Node_replaceChild(Node* node, Node* newChild, Node* oldChild)
{
    /* Linear search from tail -- good enough for our purposes here */
    NodeLink* current;
    for (current = node->childNodesTail; current; current = current->prev) {
        if (current->node == oldChild) {
            Node_deref(current->node);
            current->node = newChild;
        }
    }
}

void Node_ref(Node* node)
{
    ++node->refCount;
}

void Node_deref(Node* node)
{
    if (--node->refCount == 0)
        free(node);
}
