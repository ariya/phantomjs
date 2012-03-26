/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef WTF_HashMap_h
#define WTF_HashMap_h

#include "HashTable.h"

namespace WTF {

    template<typename PairType> struct PairFirstExtractor;

    template<typename KeyArg, typename MappedArg, typename HashArg = typename DefaultHash<KeyArg>::Hash,
        typename KeyTraitsArg = HashTraits<KeyArg>, typename MappedTraitsArg = HashTraits<MappedArg> >
    class HashMap {
        WTF_MAKE_FAST_ALLOCATED;
    private:
        typedef KeyTraitsArg KeyTraits;
        typedef MappedTraitsArg MappedTraits;
        typedef PairHashTraits<KeyTraits, MappedTraits> ValueTraits;

    public:
        typedef typename KeyTraits::TraitType KeyType;
        typedef typename MappedTraits::TraitType MappedType;
        typedef typename ValueTraits::TraitType ValueType;

    private:
        typedef HashArg HashFunctions;

        typedef HashTable<KeyType, ValueType, PairFirstExtractor<ValueType>,
            HashFunctions, ValueTraits, KeyTraits> HashTableType;

    public:
        typedef HashTableIteratorAdapter<HashTableType, ValueType> iterator;
        typedef HashTableConstIteratorAdapter<HashTableType, ValueType> const_iterator;

        void swap(HashMap&);

        int size() const;
        int capacity() const;
        bool isEmpty() const;

        // iterators iterate over pairs of keys and values
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;

        iterator find(const KeyType&);
        const_iterator find(const KeyType&) const;
        bool contains(const KeyType&) const;
        MappedType get(const KeyType&) const;

        // replaces value but not key if key is already present
        // return value is a pair of the iterator to the key location, 
        // and a boolean that's true if a new value was actually added
        pair<iterator, bool> set(const KeyType&, const MappedType&); 

        // does nothing if key is already present
        // return value is a pair of the iterator to the key location, 
        // and a boolean that's true if a new value was actually added
        pair<iterator, bool> add(const KeyType&, const MappedType&); 

        void remove(const KeyType&);
        void remove(iterator);
        void clear();

        MappedType take(const KeyType&); // efficient combination of get with remove

        // An alternate version of find() that finds the object by hashing and comparing
        // with some other type, to avoid the cost of type conversion. HashTranslator
        // must have the following function members:
        //   static unsigned hash(const T&);
        //   static bool equal(const ValueType&, const T&);
        template<typename T, typename HashTranslator> iterator find(const T&);
        template<typename T, typename HashTranslator> const_iterator find(const T&) const;
        template<typename T, typename HashTranslator> bool contains(const T&) const;

        // An alternate version of add() that finds the object by hashing and comparing
        // with some other type, to avoid the cost of type conversion if the object is already
        // in the table. HashTranslator must have the following function members:
        //   static unsigned hash(const T&);
        //   static bool equal(const ValueType&, const T&);
        //   static translate(ValueType&, const T&, unsigned hashCode);
        template<typename T, typename HashTranslator> pair<iterator, bool> add(const T&, const MappedType&);

        void checkConsistency() const;

    private:
        pair<iterator, bool> inlineAdd(const KeyType&, const MappedType&);

        HashTableType m_impl;
    };

    template<typename PairType> struct PairFirstExtractor {
        static const typename PairType::first_type& extract(const PairType& p) { return p.first; }
    };

    template<typename ValueType, typename ValueTraits, typename HashFunctions>
    struct HashMapTranslator {
        typedef typename ValueType::first_type KeyType;
        typedef typename ValueType::second_type MappedType;

        static unsigned hash(const KeyType& key) { return HashFunctions::hash(key); }
        static bool equal(const KeyType& a, const KeyType& b) { return HashFunctions::equal(a, b); }
        static void translate(ValueType& location, const KeyType& key, const MappedType& mapped)
        {
            location.first = key;
            location.second = mapped;
        }
    };

    template<typename ValueType, typename ValueTraits, typename T, typename Translator>
    struct HashMapTranslatorAdapter {
        typedef typename ValueType::first_type KeyType;
        typedef typename ValueType::second_type MappedType;

        static unsigned hash(const T& key) { return Translator::hash(key); }
        static bool equal(const KeyType& a, const T& b) { return Translator::equal(a, b); }
        static void translate(ValueType& location, const T& key, const MappedType& mapped, unsigned hashCode)
        {
            Translator::translate(location.first, key, hashCode);
            location.second = mapped;
        }
    };

    template<typename T, typename U, typename V, typename W, typename X>
    inline void HashMap<T, U, V, W, X>::swap(HashMap& other)
    {
        m_impl.swap(other.m_impl); 
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline int HashMap<T, U, V, W, X>::size() const
    {
        return m_impl.size(); 
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline int HashMap<T, U, V, W, X>::capacity() const
    { 
        return m_impl.capacity(); 
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline bool HashMap<T, U, V, W, X>::isEmpty() const
    {
        return m_impl.isEmpty();
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline typename HashMap<T, U, V, W, X>::iterator HashMap<T, U, V, W, X>::begin()
    {
        return m_impl.begin();
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline typename HashMap<T, U, V, W, X>::iterator HashMap<T, U, V, W, X>::end()
    {
        return m_impl.end();
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline typename HashMap<T, U, V, W, X>::const_iterator HashMap<T, U, V, W, X>::begin() const
    {
        return m_impl.begin();
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline typename HashMap<T, U, V, W, X>::const_iterator HashMap<T, U, V, W, X>::end() const
    {
        return m_impl.end();
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline typename HashMap<T, U, V, W, X>::iterator HashMap<T, U, V, W, X>::find(const KeyType& key)
    {
        return m_impl.find(key);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline typename HashMap<T, U, V, W, X>::const_iterator HashMap<T, U, V, W, X>::find(const KeyType& key) const
    {
        return m_impl.find(key);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline bool HashMap<T, U, V, W, X>::contains(const KeyType& key) const
    {
        return m_impl.contains(key);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    template<typename TYPE, typename HashTranslator>
    inline typename HashMap<T, U, V, W, X>::iterator
    HashMap<T, U, V, W, X>::find(const TYPE& value)
    {
        typedef HashMapTranslatorAdapter<ValueType, ValueTraits, TYPE, HashTranslator> Adapter;
        return m_impl.template find<TYPE, Adapter>(value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    template<typename TYPE, typename HashTranslator>
    inline typename HashMap<T, U, V, W, X>::const_iterator 
    HashMap<T, U, V, W, X>::find(const TYPE& value) const
    {
        typedef HashMapTranslatorAdapter<ValueType, ValueTraits, TYPE, HashTranslator> Adapter;
        return m_impl.template find<TYPE, Adapter>(value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    template<typename TYPE, typename HashTranslator>
    inline bool
    HashMap<T, U, V, W, X>::contains(const TYPE& value) const
    {
        typedef HashMapTranslatorAdapter<ValueType, ValueTraits, TYPE, HashTranslator> Adapter;
        return m_impl.template contains<TYPE, Adapter>(value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline pair<typename HashMap<T, U, V, W, X>::iterator, bool>
    HashMap<T, U, V, W, X>::inlineAdd(const KeyType& key, const MappedType& mapped) 
    {
        typedef HashMapTranslator<ValueType, ValueTraits, HashFunctions> TranslatorType;
        return m_impl.template add<KeyType, MappedType, TranslatorType>(key, mapped);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    pair<typename HashMap<T, U, V, W, X>::iterator, bool>
    HashMap<T, U, V, W, X>::set(const KeyType& key, const MappedType& mapped) 
    {
        pair<iterator, bool> result = inlineAdd(key, mapped);
        if (!result.second) {
            // add call above didn't change anything, so set the mapped value
            result.first->second = mapped;
        }
        return result;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    template<typename TYPE, typename HashTranslator>
    pair<typename HashMap<T, U, V, W, X>::iterator, bool>
    HashMap<T, U, V, W, X>::add(const TYPE& key, const MappedType& value)
    {
        typedef HashMapTranslatorAdapter<ValueType, ValueTraits, TYPE, HashTranslator> Adapter;
        return m_impl.template addPassingHashCode<TYPE, MappedType, Adapter>(key, value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    pair<typename HashMap<T, U, V, W, X>::iterator, bool>
    HashMap<T, U, V, W, X>::add(const KeyType& key, const MappedType& mapped)
    {
        return inlineAdd(key, mapped);
    }

    template<typename T, typename U, typename V, typename W, typename MappedTraits>
    typename HashMap<T, U, V, W, MappedTraits>::MappedType
    HashMap<T, U, V, W, MappedTraits>::get(const KeyType& key) const
    {
        ValueType* entry = const_cast<HashTableType&>(m_impl).lookup(key);
        if (!entry)
            return MappedTraits::emptyValue();
        return entry->second;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void HashMap<T, U, V, W, X>::remove(iterator it)
    {
        if (it.m_impl == m_impl.end())
            return;
        m_impl.internalCheckTableConsistency();
        m_impl.removeWithoutEntryConsistencyCheck(it.m_impl);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void HashMap<T, U, V, W, X>::remove(const KeyType& key)
    {
        remove(find(key));
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void HashMap<T, U, V, W, X>::clear()
    {
        m_impl.clear();
    }

    template<typename T, typename U, typename V, typename W, typename MappedTraits>
    typename HashMap<T, U, V, W, MappedTraits>::MappedType
    HashMap<T, U, V, W, MappedTraits>::take(const KeyType& key)
    {
        // This can probably be made more efficient to avoid ref/deref churn.
        iterator it = find(key);
        if (it == end())
            return MappedTraits::emptyValue();
        typename HashMap<T, U, V, W, MappedTraits>::MappedType result = it->second;
        remove(it);
        return result;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void HashMap<T, U, V, W, X>::checkConsistency() const
    {
        m_impl.checkTableConsistency();
    }


    template<typename T, typename U, typename V, typename W, typename X>
    bool operator==(const HashMap<T, U, V, W, X>& a, const HashMap<T, U, V, W, X>& b)
    {
        if (a.size() != b.size())
            return false;

        typedef typename HashMap<T, U, V, W, X>::const_iterator const_iterator;

        const_iterator end = a.end();
        const_iterator notFound = b.end();
        for (const_iterator it = a.begin(); it != end; ++it) {
            const_iterator bPos = b.find(it->first);
            if (bPos == notFound || it->second != bPos->second)
                return false;
        }

        return true;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline bool operator!=(const HashMap<T, U, V, W, X>& a, const HashMap<T, U, V, W, X>& b)
    {
        return !(a == b);
    }

    template<typename MappedType, typename HashTableType>
    void deleteAllPairSeconds(HashTableType& collection)
    {
        typedef typename HashTableType::const_iterator iterator;
        iterator end = collection.end();
        for (iterator it = collection.begin(); it != end; ++it)
            delete it->second;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void deleteAllValues(const HashMap<T, U, V, W, X>& collection)
    {
        deleteAllPairSeconds<typename HashMap<T, U, V, W, X>::MappedType>(collection);
    }

    template<typename KeyType, typename HashTableType>
    void deleteAllPairFirsts(HashTableType& collection)
    {
        typedef typename HashTableType::const_iterator iterator;
        iterator end = collection.end();
        for (iterator it = collection.begin(); it != end; ++it)
            delete it->first;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void deleteAllKeys(const HashMap<T, U, V, W, X>& collection)
    {
        deleteAllPairFirsts<typename HashMap<T, U, V, W, X>::KeyType>(collection);
    }
    
    template<typename T, typename U, typename V, typename W, typename X, typename Y>
    inline void copyKeysToVector(const HashMap<T, U, V, W, X>& collection, Y& vector)
    {
        typedef typename HashMap<T, U, V, W, X>::const_iterator::Keys iterator;
        
        vector.resize(collection.size());
        
        iterator it = collection.begin().keys();
        iterator end = collection.end().keys();
        for (unsigned i = 0; it != end; ++it, ++i)
            vector[i] = *it;
    }  

    template<typename T, typename U, typename V, typename W, typename X, typename Y>
    inline void copyValuesToVector(const HashMap<T, U, V, W, X>& collection, Y& vector)
    {
        typedef typename HashMap<T, U, V, W, X>::const_iterator::Values iterator;
        
        vector.resize(collection.size());
        
        iterator it = collection.begin().values();
        iterator end = collection.end().values();
        for (unsigned i = 0; it != end; ++it, ++i)
            vector[i] = *it;
    }   

} // namespace WTF

using WTF::HashMap;

#include "RefPtrHashMap.h"

#endif /* WTF_HashMap_h */
