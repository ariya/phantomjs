/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2011, Benjamin Poulain <ikipou@gmail.com>
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

#ifndef WTF_ListHashSet_h
#define WTF_ListHashSet_h

#include "Assertions.h"
#include "HashSet.h"
#include "OwnPtr.h"
#include "PassOwnPtr.h"
#include "StdLibExtras.h"

namespace WTF {

    // ListHashSet: Just like HashSet, this class provides a Set
    // interface - a collection of unique objects with O(1) insertion,
    // removal and test for containership. However, it also has an
    // order - iterating it will always give back values in the order
    // in which they are added.

    // In theory it would be possible to add prepend, insertAfter
    // and an append that moves the element to the end even if already present,
    // but unclear yet if these are needed.

    template<typename Value, size_t inlineCapacity, typename HashFunctions> class ListHashSet;

    template<typename T> struct IdentityExtractor;

    template<typename Value, size_t inlineCapacity, typename HashFunctions>
    void deleteAllValues(const ListHashSet<Value, inlineCapacity, HashFunctions>&);

    template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetIterator;
    template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetConstIterator;

    template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNode;
    template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNodeAllocator;
    template<typename ValueArg, size_t inlineCapacity, typename HashArg> struct ListHashSetNodeHashFunctions;

    template<typename ValueArg, size_t inlineCapacity = 256, typename HashArg = typename DefaultHash<ValueArg>::Hash> class ListHashSet {
        WTF_MAKE_FAST_ALLOCATED;
    private:
        typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
        typedef ListHashSetNodeAllocator<ValueArg, inlineCapacity> NodeAllocator;

        typedef HashTraits<Node*> NodeTraits;
        typedef ListHashSetNodeHashFunctions<ValueArg, inlineCapacity, HashArg> NodeHash;

        typedef HashTable<Node*, Node*, IdentityExtractor<Node*>, NodeHash, NodeTraits, NodeTraits> ImplType;
        typedef HashTableIterator<Node*, Node*, IdentityExtractor<Node*>, NodeHash, NodeTraits, NodeTraits> ImplTypeIterator;
        typedef HashTableConstIterator<Node*, Node*, IdentityExtractor<Node*>, NodeHash, NodeTraits, NodeTraits> ImplTypeConstIterator;

        typedef HashArg HashFunctions;

    public:
        typedef ValueArg ValueType;
        typedef ListHashSetIterator<ValueType, inlineCapacity, HashArg> iterator;
        typedef ListHashSetConstIterator<ValueType, inlineCapacity, HashArg> const_iterator;

        friend class ListHashSetConstIterator<ValueType, inlineCapacity, HashArg>;

        ListHashSet();
        ListHashSet(const ListHashSet&);
        ListHashSet& operator=(const ListHashSet&);
        ~ListHashSet();

        void swap(ListHashSet&);

        int size() const;
        int capacity() const;
        bool isEmpty() const;

        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;

        ValueType& first();
        const ValueType& first() const;

        ValueType& last();
        const ValueType& last() const;
        void removeLast();

        iterator find(const ValueType&);
        const_iterator find(const ValueType&) const;
        bool contains(const ValueType&) const;

        // An alternate version of find() that finds the object by hashing and comparing
        // with some other type, to avoid the cost of type conversion.
        // The HashTranslator interface is defined in HashSet.
        template<typename T, typename HashTranslator> iterator find(const T&);
        template<typename T, typename HashTranslator> const_iterator find(const T&) const;
        template<typename T, typename HashTranslator> bool contains(const T&) const;

        // the return value is a pair of an iterator to the new value's location, 
        // and a bool that is true if an new entry was added
        pair<iterator, bool> add(const ValueType&);

        pair<iterator, bool> insertBefore(const ValueType& beforeValue, const ValueType& newValue);
        pair<iterator, bool> insertBefore(iterator it, const ValueType&);

        void remove(const ValueType&);
        void remove(iterator);
        void clear();

    private:
        void unlinkAndDelete(Node*);
        void appendNode(Node*);
        void insertNodeBefore(Node* beforeNode, Node* newNode);
        void deleteAllNodes();
        iterator makeIterator(Node*);
        const_iterator makeConstIterator(Node*) const;

        friend void deleteAllValues<>(const ListHashSet&);

        ImplType m_impl;
        Node* m_head;
        Node* m_tail;
        OwnPtr<NodeAllocator> m_allocator;
    };

    template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNodeAllocator {
        typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
        typedef ListHashSetNodeAllocator<ValueArg, inlineCapacity> NodeAllocator;

        ListHashSetNodeAllocator() 
            : m_freeList(pool())
            , m_isDoneWithInitialFreeList(false)
        { 
            memset(m_pool.pool, 0, sizeof(m_pool.pool));
        }

        Node* allocate()
        { 
            Node* result = m_freeList;

            if (!result)
                return static_cast<Node*>(fastMalloc(sizeof(Node)));

            ASSERT(!result->m_isAllocated);

            Node* next = result->m_next;
            ASSERT(!next || !next->m_isAllocated);
            if (!next && !m_isDoneWithInitialFreeList) {
                next = result + 1;
                if (next == pastPool()) {
                    m_isDoneWithInitialFreeList = true;
                    next = 0;
                } else {
                    ASSERT(inPool(next));
                    ASSERT(!next->m_isAllocated);
                }
            }
            m_freeList = next;

            return result;
        }

        void deallocate(Node* node) 
        {
            if (inPool(node)) {
#ifndef NDEBUG
                node->m_isAllocated = false;
#endif
                node->m_next = m_freeList;
                m_freeList = node;
                return;
            }

            fastFree(node);
        }

    private:
        Node* pool() { return reinterpret_cast_ptr<Node*>(m_pool.pool); }
        Node* pastPool() { return pool() + m_poolSize; }

        bool inPool(Node* node)
        {
            return node >= pool() && node < pastPool();
        }

        Node* m_freeList;
        bool m_isDoneWithInitialFreeList;
        static const size_t m_poolSize = inlineCapacity;
        union {
            char pool[sizeof(Node) * m_poolSize];
            double forAlignment;
        } m_pool;
    };

    template<typename ValueArg, size_t inlineCapacity> struct ListHashSetNode {
        typedef ListHashSetNodeAllocator<ValueArg, inlineCapacity> NodeAllocator;

        ListHashSetNode(ValueArg value)
            : m_value(value)
            , m_prev(0)
            , m_next(0)
#ifndef NDEBUG
            , m_isAllocated(true)
#endif
        {
        }

        void* operator new(size_t, NodeAllocator* allocator)
        {
            return allocator->allocate();
        }
        void destroy(NodeAllocator* allocator)
        {
            this->~ListHashSetNode();
            allocator->deallocate(this);
        }

        ValueArg m_value;
        ListHashSetNode* m_prev;
        ListHashSetNode* m_next;

#ifndef NDEBUG
        bool m_isAllocated;
#endif
    };

    template<typename ValueArg, size_t inlineCapacity, typename HashArg> struct ListHashSetNodeHashFunctions {
        typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
        
        static unsigned hash(Node* const& key) { return HashArg::hash(key->m_value); }
        static bool equal(Node* const& a, Node* const& b) { return HashArg::equal(a->m_value, b->m_value); }
        static const bool safeToCompareToEmptyOrDeleted = false;
    };

    template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetIterator {
    private:
        typedef ListHashSet<ValueArg, inlineCapacity, HashArg> ListHashSetType;
        typedef ListHashSetIterator<ValueArg, inlineCapacity, HashArg> iterator;
        typedef ListHashSetConstIterator<ValueArg, inlineCapacity, HashArg> const_iterator;
        typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
        typedef ValueArg ValueType;
        typedef ValueType& ReferenceType;
        typedef ValueType* PointerType;

        friend class ListHashSet<ValueArg, inlineCapacity, HashArg>;

        ListHashSetIterator(const ListHashSetType* set, Node* position) : m_iterator(set, position) { }

    public:
        ListHashSetIterator() { }

        // default copy, assignment and destructor are OK

        PointerType get() const { return const_cast<PointerType>(m_iterator.get()); }
        ReferenceType operator*() const { return *get(); }
        PointerType operator->() const { return get(); }

        iterator& operator++() { ++m_iterator; return *this; }

        // postfix ++ intentionally omitted

        iterator& operator--() { --m_iterator; return *this; }

        // postfix -- intentionally omitted

        // Comparison.
        bool operator==(const iterator& other) const { return m_iterator == other.m_iterator; }
        bool operator!=(const iterator& other) const { return m_iterator != other.m_iterator; }

        operator const_iterator() const { return m_iterator; }

    private:
        Node* node() { return m_iterator.node(); }

        const_iterator m_iterator;
    };

    template<typename ValueArg, size_t inlineCapacity, typename HashArg> class ListHashSetConstIterator {
    private:
        typedef ListHashSet<ValueArg, inlineCapacity, HashArg> ListHashSetType;
        typedef ListHashSetIterator<ValueArg, inlineCapacity, HashArg> iterator;
        typedef ListHashSetConstIterator<ValueArg, inlineCapacity, HashArg> const_iterator;
        typedef ListHashSetNode<ValueArg, inlineCapacity> Node;
        typedef ValueArg ValueType;
        typedef const ValueType& ReferenceType;
        typedef const ValueType* PointerType;

        friend class ListHashSet<ValueArg, inlineCapacity, HashArg>;
        friend class ListHashSetIterator<ValueArg, inlineCapacity, HashArg>;

        ListHashSetConstIterator(const ListHashSetType* set, Node* position)
            : m_set(set)
            , m_position(position)
        {
        }

    public:
        ListHashSetConstIterator()
        {
        }

        PointerType get() const
        {
            return &m_position->m_value;
        }
        ReferenceType operator*() const { return *get(); }
        PointerType operator->() const { return get(); }

        const_iterator& operator++()
        {
            ASSERT(m_position != 0);
            m_position = m_position->m_next;
            return *this;
        }

        // postfix ++ intentionally omitted

        const_iterator& operator--()
        {
            ASSERT(m_position != m_set->m_head);
            if (!m_position)
                m_position = m_set->m_tail;
            else
                m_position = m_position->m_prev;
            return *this;
        }

        // postfix -- intentionally omitted

        // Comparison.
        bool operator==(const const_iterator& other) const
        {
            return m_position == other.m_position;
        }
        bool operator!=(const const_iterator& other) const
        {
            return m_position != other.m_position;
        }

    private:
        Node* node() { return m_position; }

        const ListHashSetType* m_set;
        Node* m_position;
    };


    template<typename ValueType, size_t inlineCapacity, typename HashFunctions>
    struct ListHashSetTranslator {
    private:
        typedef ListHashSetNode<ValueType, inlineCapacity> Node;
        typedef ListHashSetNodeAllocator<ValueType, inlineCapacity> NodeAllocator;
    public:
        static unsigned hash(const ValueType& key) { return HashFunctions::hash(key); }
        static bool equal(Node* const& a, const ValueType& b) { return HashFunctions::equal(a->m_value, b); }
        static void translate(Node*& location, const ValueType& key, NodeAllocator* allocator)
        {
            location = new (allocator) Node(key);
        }
    };

    template<typename T, size_t inlineCapacity, typename U>
    inline ListHashSet<T, inlineCapacity, U>::ListHashSet()
        : m_head(0)
        , m_tail(0)
        , m_allocator(adoptPtr(new NodeAllocator))
    {
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline ListHashSet<T, inlineCapacity, U>::ListHashSet(const ListHashSet& other)
        : m_head(0)
        , m_tail(0)
        , m_allocator(adoptPtr(new NodeAllocator))
    {
        const_iterator end = other.end();
        for (const_iterator it = other.begin(); it != end; ++it)
            add(*it);
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline ListHashSet<T, inlineCapacity, U>& ListHashSet<T, inlineCapacity, U>::operator=(const ListHashSet& other)
    {
        ListHashSet tmp(other);
        swap(tmp);
        return *this;
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline void ListHashSet<T, inlineCapacity, U>::swap(ListHashSet& other)
    {
        m_impl.swap(other.m_impl);
        std::swap(m_head, other.m_head);
        std::swap(m_tail, other.m_tail);
        m_allocator.swap(other.m_allocator);
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline ListHashSet<T, inlineCapacity, U>::~ListHashSet()
    {
        deleteAllNodes();
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline int ListHashSet<T, inlineCapacity, U>::size() const
    {
        return m_impl.size(); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline int ListHashSet<T, inlineCapacity, U>::capacity() const
    {
        return m_impl.capacity(); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline bool ListHashSet<T, inlineCapacity, U>::isEmpty() const
    {
        return m_impl.isEmpty(); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline typename ListHashSet<T, inlineCapacity, U>::iterator ListHashSet<T, inlineCapacity, U>::begin()
    {
        return makeIterator(m_head); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline typename ListHashSet<T, inlineCapacity, U>::iterator ListHashSet<T, inlineCapacity, U>::end()
    {
        return makeIterator(0);
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline typename ListHashSet<T, inlineCapacity, U>::const_iterator ListHashSet<T, inlineCapacity, U>::begin() const
    {
        return makeConstIterator(m_head); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline typename ListHashSet<T, inlineCapacity, U>::const_iterator ListHashSet<T, inlineCapacity, U>::end() const
    {
        return makeConstIterator(0); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline T& ListHashSet<T, inlineCapacity, U>::first()
    {
        ASSERT(!isEmpty());
        return m_head->m_value;
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline const T& ListHashSet<T, inlineCapacity, U>::first() const
    {
        ASSERT(!isEmpty());
        return m_head->m_value;
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline T& ListHashSet<T, inlineCapacity, U>::last()
    {
        ASSERT(!isEmpty());
        return m_tail->m_value;
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline const T& ListHashSet<T, inlineCapacity, U>::last() const
    {
        ASSERT(!isEmpty());
        return m_tail->m_value;
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline void ListHashSet<T, inlineCapacity, U>::removeLast()
    {
        ASSERT(!isEmpty());
        m_impl.remove(m_tail);
        unlinkAndDelete(m_tail);
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline typename ListHashSet<T, inlineCapacity, U>::iterator ListHashSet<T, inlineCapacity, U>::find(const ValueType& value)
    {
        typedef ListHashSetTranslator<ValueType, inlineCapacity, HashFunctions> Translator;
        ImplTypeIterator it = m_impl.template find<ValueType, Translator>(value);
        if (it == m_impl.end())
            return end();
        return makeIterator(*it); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline typename ListHashSet<T, inlineCapacity, U>::const_iterator ListHashSet<T, inlineCapacity, U>::find(const ValueType& value) const
    {
        typedef ListHashSetTranslator<ValueType, inlineCapacity, HashFunctions> Translator;
        ImplTypeConstIterator it = m_impl.template find<ValueType, Translator>(value);
        if (it == m_impl.end())
            return end();
        return makeConstIterator(*it);
    }

    template<typename ValueType, size_t inlineCapacity, typename T, typename Translator>
    struct ListHashSetTranslatorAdapter {
    private:
        typedef ListHashSetNode<ValueType, inlineCapacity> Node;
    public:
        static unsigned hash(const T& key) { return Translator::hash(key); }
        static bool equal(Node* const& a, const T& b) { return Translator::equal(a->m_value, b); }
    };

    template<typename ValueType, size_t inlineCapacity, typename U>
    template<typename T, typename HashTranslator>
    inline typename ListHashSet<ValueType, inlineCapacity, U>::iterator ListHashSet<ValueType, inlineCapacity, U>::find(const T& value)
    {
        typedef ListHashSetTranslatorAdapter<ValueType, inlineCapacity, T, HashTranslator> Adapter;
        ImplTypeConstIterator it = m_impl.template find<T, Adapter>(value);
        if (it == m_impl.end())
            return end();
        return makeIterator(*it);
    }

    template<typename ValueType, size_t inlineCapacity, typename U>
    template<typename T, typename HashTranslator>
    inline typename ListHashSet<ValueType, inlineCapacity, U>::const_iterator ListHashSet<ValueType, inlineCapacity, U>::find(const T& value) const
    {
        typedef ListHashSetTranslatorAdapter<ValueType, inlineCapacity, T, HashTranslator> Adapter;
        ImplTypeConstIterator it = m_impl.template find<T, Adapter>(value);
        if (it == m_impl.end())
            return end();
        return makeConstIterator(*it);
    }

    template<typename ValueType, size_t inlineCapacity, typename U>
    template<typename T, typename HashTranslator>
    inline bool ListHashSet<ValueType, inlineCapacity, U>::contains(const T& value) const
    {
        typedef ListHashSetTranslatorAdapter<ValueType, inlineCapacity, T, HashTranslator> Adapter;
        return m_impl.template contains<T, Adapter>(value);
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline bool ListHashSet<T, inlineCapacity, U>::contains(const ValueType& value) const
    {
        typedef ListHashSetTranslator<ValueType, inlineCapacity, HashFunctions> Translator;
        return m_impl.template contains<ValueType, Translator>(value);
    }

    template<typename T, size_t inlineCapacity, typename U>
    pair<typename ListHashSet<T, inlineCapacity, U>::iterator, bool> ListHashSet<T, inlineCapacity, U>::add(const ValueType &value)
    {
        typedef ListHashSetTranslator<ValueType, inlineCapacity, HashFunctions> Translator;
        pair<typename ImplType::iterator, bool> result = m_impl.template add<ValueType, NodeAllocator*, Translator>(value, m_allocator.get());
        if (result.second)
            appendNode(*result.first);
        return std::make_pair(makeIterator(*result.first), result.second);
    }

    template<typename T, size_t inlineCapacity, typename U>
    pair<typename ListHashSet<T, inlineCapacity, U>::iterator, bool> ListHashSet<T, inlineCapacity, U>::insertBefore(iterator it, const ValueType& newValue)
    {
        typedef ListHashSetTranslator<ValueType, inlineCapacity, HashFunctions> Translator;
        pair<typename ImplType::iterator, bool> result = m_impl.template add<ValueType, NodeAllocator*, Translator>(newValue, m_allocator.get());
        if (result.second)
            insertNodeBefore(it.node(), *result.first);
        return std::make_pair(makeIterator(*result.first), result.second);

    }

    template<typename T, size_t inlineCapacity, typename U>
    pair<typename ListHashSet<T, inlineCapacity, U>::iterator, bool> ListHashSet<T, inlineCapacity, U>::insertBefore(const ValueType& beforeValue, const ValueType& newValue)
    {
        return insertBefore(find(beforeValue), newValue); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline void ListHashSet<T, inlineCapacity, U>::remove(iterator it)
    {
        if (it == end())
            return;
        m_impl.remove(it.node());
        unlinkAndDelete(it.node());
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline void ListHashSet<T, inlineCapacity, U>::remove(const ValueType& value)
    {
        remove(find(value));
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline void ListHashSet<T, inlineCapacity, U>::clear()
    {
        deleteAllNodes();
        m_impl.clear(); 
        m_head = 0;
        m_tail = 0;
    }

    template<typename T, size_t inlineCapacity, typename U>
    void ListHashSet<T, inlineCapacity, U>::unlinkAndDelete(Node* node)
    {
        if (!node->m_prev) {
            ASSERT(node == m_head);
            m_head = node->m_next;
        } else {
            ASSERT(node != m_head);
            node->m_prev->m_next = node->m_next;
        }

        if (!node->m_next) {
            ASSERT(node == m_tail);
            m_tail = node->m_prev;
        } else {
            ASSERT(node != m_tail);
            node->m_next->m_prev = node->m_prev;
        }

        node->destroy(m_allocator.get());
    }

    template<typename T, size_t inlineCapacity, typename U>
    void ListHashSet<T, inlineCapacity, U>::appendNode(Node* node)
    {
        node->m_prev = m_tail;
        node->m_next = 0;

        if (m_tail) {
            ASSERT(m_head);
            m_tail->m_next = node;
        } else {
            ASSERT(!m_head);
            m_head = node;
        }

        m_tail = node;
    }

    template<typename T, size_t inlineCapacity, typename U>
    void ListHashSet<T, inlineCapacity, U>::insertNodeBefore(Node* beforeNode, Node* newNode)
    {
        if (!beforeNode)
            return appendNode(newNode);
        
        newNode->m_next = beforeNode;
        newNode->m_prev = beforeNode->m_prev;
        if (beforeNode->m_prev)
            beforeNode->m_prev->m_next = newNode;
        beforeNode->m_prev = newNode;

        if (!newNode->m_prev)
            m_head = newNode;
    }

    template<typename T, size_t inlineCapacity, typename U>
    void ListHashSet<T, inlineCapacity, U>::deleteAllNodes()
    {
        if (!m_head)
            return;

        for (Node* node = m_head, *next = m_head->m_next; node; node = next, next = node ? node->m_next : 0)
            node->destroy(m_allocator.get());
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline ListHashSetIterator<T, inlineCapacity, U> ListHashSet<T, inlineCapacity, U>::makeIterator(Node* position) 
    {
        return ListHashSetIterator<T, inlineCapacity, U>(this, position); 
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline ListHashSetConstIterator<T, inlineCapacity, U> ListHashSet<T, inlineCapacity, U>::makeConstIterator(Node* position) const
    { 
        return ListHashSetConstIterator<T, inlineCapacity, U>(this, position); 
    }

    template<bool, typename ValueType, typename HashTableType>
    void deleteAllValues(HashTableType& collection)
    {
        typedef typename HashTableType::const_iterator iterator;
        iterator end = collection.end();
        for (iterator it = collection.begin(); it != end; ++it)
            delete (*it)->m_value;
    }

    template<typename T, size_t inlineCapacity, typename U>
    inline void deleteAllValues(const ListHashSet<T, inlineCapacity, U>& collection)
    {
        deleteAllValues<true, typename ListHashSet<T, inlineCapacity, U>::ValueType>(collection.m_impl);
    }

} // namespace WTF

using WTF::ListHashSet;

#endif /* WTF_ListHashSet_h */
