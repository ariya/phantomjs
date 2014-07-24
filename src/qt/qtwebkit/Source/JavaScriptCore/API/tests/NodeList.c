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

#include "NodeList.h"

#include <stdlib.h>

extern NodeList* NodeList_new(Node* parentNode)
{
    Node_ref(parentNode);

    NodeList* nodeList = (NodeList*)malloc(sizeof(NodeList));
    nodeList->parentNode = parentNode;
    nodeList->refCount = 0;
    return nodeList;
}

extern unsigned NodeList_length(NodeList* nodeList)
{
    /* Linear count from tail -- good enough for our purposes here */
    unsigned i = 0;
    NodeLink* n = nodeList->parentNode->childNodesTail;
    while (n) {
        n = n->prev;
        ++i;
    }

    return i;
}

extern Node* NodeList_item(NodeList* nodeList, unsigned index)
{
    unsigned length = NodeList_length(nodeList);
    if (index >= length)
        return NULL;

    /* Linear search from tail -- good enough for our purposes here */
    NodeLink* n = nodeList->parentNode->childNodesTail;
    unsigned i = 0;
    unsigned count = length - 1 - index;
    while (i < count) {
        ++i;
        n = n->prev;
    }
    return n->node;
}

extern void NodeList_ref(NodeList* nodeList)
{
    ++nodeList->refCount;
}

extern void NodeList_deref(NodeList* nodeList)
{
    if (--nodeList->refCount == 0) {
        Node_deref(nodeList->parentNode);
        free(nodeList);
    }
}
