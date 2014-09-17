/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

//    A SentinelLinkedList is a linked list with dummy head and tail sentinels,
//    which allow for branch-less insertion and removal, and removal without a
//    pointer to the list.
//    
//    Requires: Node is a concrete class with:
//        Node(SentinelTag);
//        void setPrev(Node*);
//        Node* prev();
//        void setNext(Node*);
//        Node* next();

#ifndef SentinelLinkedList_h
#define SentinelLinkedList_h

namespace WTF {

enum SentinelTag { Sentinel };

template <typename Node> class SentinelLinkedList {
public:
    typedef Node* iterator;

    SentinelLinkedList();

    void push(Node*);
    static void remove(Node*);

    iterator begin();
    iterator end();

private:
    Node m_headSentinel;
    Node m_tailSentinel;
};

template <typename Node> inline SentinelLinkedList<Node>::SentinelLinkedList()
    : m_headSentinel(Sentinel)
    , m_tailSentinel(Sentinel)
{
    m_headSentinel.setNext(&m_tailSentinel);
    m_headSentinel.setPrev(0);

    m_tailSentinel.setPrev(&m_headSentinel);
    m_tailSentinel.setNext(0);
}

template <typename Node> inline typename SentinelLinkedList<Node>::iterator SentinelLinkedList<Node>::begin()
{
    return m_headSentinel.next();
}

template <typename Node> inline typename SentinelLinkedList<Node>::iterator SentinelLinkedList<Node>::end()
{
    return &m_tailSentinel;
}

template <typename Node> inline void SentinelLinkedList<Node>::push(Node* node)
{
    ASSERT(node);
    Node* prev = &m_headSentinel;
    Node* next = m_headSentinel.next();

    node->setPrev(prev);
    node->setNext(next);

    prev->setNext(node);
    next->setPrev(node);
}

template <typename Node> inline void SentinelLinkedList<Node>::remove(Node* node)
{
    Node* prev = node->prev();
    Node* next = node->next();

    prev->setNext(next);
    next->setPrev(prev);
}

}

using WTF::SentinelLinkedList;

#endif

