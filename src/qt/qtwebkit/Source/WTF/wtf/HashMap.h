/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
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

#include <wtf/HashTable.h>

namespace WTF {

    template<typename KeyTraits, typename MappedTraits> struct HashMapValueTraits;

    template<typename T> struct ReferenceTypeMaker {
        typedef T& ReferenceType;
    };
    template<typename T> struct ReferenceTypeMaker<T&> {
        typedef T& ReferenceType;
    };

    template<typename T> struct KeyValuePairKeyExtractor {
        static const typename T::KeyType& extract(const T& p) { return p.key; }
    };

    template<typename KeyArg, typename MappedArg, typename HashArg = typename DefaultHash<KeyArg>::Hash,
        typename KeyTraitsArg = HashTraits<KeyArg>, typename MappedTraitsArg = HashTraits<MappedArg> >
    class HashMap {
        WTF_MAKE_FAST_ALLOCATED;
    private:
        typedef KeyTraitsArg KeyTraits;
        typedef MappedTraitsArg MappedTraits;
        typedef HashMapValueTraits<KeyTraits, MappedTraits> ValueTraits;

    public:
        typedef typename KeyTraits::TraitType KeyType;
        typedef typename MappedTraits::TraitType MappedType;
        typedef typename ValueTraits::TraitType ValueType;

    private:
        typedef typename MappedTraits::PassInType MappedPassInType;
        typedef typename MappedTraits::PassOutType MappedPassOutType;
        typedef typename MappedTraits::PeekType MappedPeekType;

        typedef typename ReferenceTypeMaker<MappedPassInType>::ReferenceType MappedPassInReferenceType;

        typedef HashArg HashFunctions;

        typedef HashTable<KeyType, ValueType, KeyValuePairKeyExtractor<ValueType>,
            HashFunctions, ValueTraits, KeyTraits> HashTableType;

        class HashMapKeysProxy;
        class HashMapValuesProxy;

    public:
        typedef HashTableIteratorAdapter<HashTableType, ValueType> iterator;
        typedef HashTableConstIteratorAdapter<HashTableType, ValueType> const_iterator;
        typedef typename HashTableType::AddResult AddResult;

    public:
        void swap(HashMap&);

        int size() const;
        int capacity() const;
        bool isEmpty() const;

        // iterators iterate over pairs of keys and values
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;

        HashMapKeysProxy& keys() { return static_cast<HashMapKeysProxy&>(*this); }
        const HashMapKeysProxy& keys() const { return static_cast<const HashMapKeysProxy&>(*this); }

        HashMapValuesProxy& values() { return static_cast<HashMapValuesProxy&>(*this); }
        const HashMapValuesProxy& values() const { return static_cast<const HashMapValuesProxy&>(*this); }

        iterator find(const KeyType&);
        const_iterator find(const KeyType&) const;
        bool contains(const KeyType&) const;
        MappedPeekType get(const KeyType&) const;

        // replaces value but not key if key is already present
        // return value is a pair of the iterator to the key location, 
        // and a boolean that's true if a new value was actually added
        AddResult set(const KeyType&, MappedPassInType);

        // does nothing if key is already present
        // return value is a pair of the iterator to the key location, 
        // and a boolean that's true if a new value was actually added
        AddResult add(const KeyType&, MappedPassInType);

        void remove(const KeyType&);
        void remove(iterator);
        void clear();

        MappedPassOutType take(const KeyType&); // efficient combination of get with remove

        // An alternate version of find() that finds the object by hashing and comparing
        // with some other type, to avoid the cost of type conversion. HashTranslator
        // must have the following function members:
        //   static unsigned hash(const T&);
        //   static bool equal(const ValueType&, const T&);
        template<typename HashTranslator, typename T> iterator find(const T&);
        template<typename HashTranslator, typename T> const_iterator find(const T&) const;
        template<typename HashTranslator, typename T> bool contains(const T&) const;

        // An alternate version of add() that finds the object by hashing and comparing
        // with some other type, to avoid the cost of type conversion if the object is already
        // in the table. HashTranslator must have the following function members:
        //   static unsigned hash(const T&);
        //   static bool equal(const ValueType&, const T&);
        //   static translate(ValueType&, const T&, unsigned hashCode);
        template<typename HashTranslator, typename T> AddResult add(const T&, MappedPassInType);

        void checkConsistency() const;

        static bool isValidKey(const KeyType&);

    private:
        AddResult inlineAdd(const KeyType&, MappedPassInReferenceType);

        HashTableType m_impl;
    };

    template<typename KeyArg, typename MappedArg, typename HashArg, typename KeyTraitsArg, typename MappedTraitsArg>
    class HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg>::HashMapKeysProxy : 
        private HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg> {
        public:
            typedef HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg> HashMapType;
            typedef typename HashMapType::iterator::Keys iterator;
            typedef typename HashMapType::const_iterator::Keys const_iterator;

            iterator begin()
            {
                return HashMapType::begin().keys();
            }

            iterator end()
            {
                return HashMapType::end().keys();
            }

            const_iterator begin() const
            {
                return HashMapType::begin().keys();
            }

            const_iterator end() const
            {
                return HashMapType::end().keys();
            }

        private:
            friend class HashMap;

            // These are intentionally not implemented.
            HashMapKeysProxy();
            HashMapKeysProxy(const HashMapKeysProxy&);
            HashMapKeysProxy& operator=(const HashMapKeysProxy&);
            ~HashMapKeysProxy();
    };

    template<typename KeyArg, typename MappedArg, typename HashArg,  typename KeyTraitsArg, typename MappedTraitsArg>
    class HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg>::HashMapValuesProxy : 
        private HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg> {
        public:
            typedef HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg> HashMapType;
            typedef typename HashMapType::iterator::Values iterator;
            typedef typename HashMapType::const_iterator::Values const_iterator;

            iterator begin()
            {
                return HashMapType::begin().values();
            }

            iterator end()
            {
                return HashMapType::end().values();
            }

            const_iterator begin() const
            {
                return HashMapType::begin().values();
            }

            const_iterator end() const
            {
                return HashMapType::end().values();
            }

        private:
            friend class HashMap;

            // These are intentionally not implemented.
            HashMapValuesProxy();
            HashMapValuesProxy(const HashMapValuesProxy&);
            HashMapValuesProxy& operator=(const HashMapValuesProxy&);
            ~HashMapValuesProxy();
    };

    template<typename KeyTraits, typename MappedTraits> struct HashMapValueTraits : KeyValuePairHashTraits<KeyTraits, MappedTraits> {
        static const bool hasIsEmptyValueFunction = true;
        static bool isEmptyValue(const typename KeyValuePairHashTraits<KeyTraits, MappedTraits>::TraitType& value)
        {
            return isHashTraitsEmptyValue<KeyTraits>(value.key);
        }
    };

    template<typename ValueTraits, typename HashFunctions>
    struct HashMapTranslator {
        template<typename T> static unsigned hash(const T& key) { return HashFunctions::hash(key); }
        template<typename T, typename U> static bool equal(const T& a, const U& b) { return HashFunctions::equal(a, b); }
        template<typename T, typename U, typename V> static void translate(T& location, const U& key, const V& mapped)
        {
            location.key = key;
            ValueTraits::ValueTraits::store(mapped, location.value);
        }
    };

    template<typename ValueTraits, typename Translator>
    struct HashMapTranslatorAdapter {
        template<typename T> static unsigned hash(const T& key) { return Translator::hash(key); }
        template<typename T, typename U> static bool equal(const T& a, const U& b) { return Translator::equal(a, b); }
        template<typename T, typename U, typename V> static void translate(T& location, const U& key, const V& mapped, unsigned hashCode)
        {
            Translator::translate(location.key, key, hashCode);
            ValueTraits::ValueTraits::store(mapped, location.value);
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
    template<typename HashTranslator, typename TYPE>
    inline typename HashMap<T, U, V, W, X>::iterator
    HashMap<T, U, V, W, X>::find(const TYPE& value)
    {
        return m_impl.template find<HashMapTranslatorAdapter<ValueTraits, HashTranslator> >(value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    template<typename HashTranslator, typename TYPE>
    inline typename HashMap<T, U, V, W, X>::const_iterator 
    HashMap<T, U, V, W, X>::find(const TYPE& value) const
    {
        return m_impl.template find<HashMapTranslatorAdapter<ValueTraits, HashTranslator> >(value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    template<typename HashTranslator, typename TYPE>
    inline bool
    HashMap<T, U, V, W, X>::contains(const TYPE& value) const
    {
        return m_impl.template contains<HashMapTranslatorAdapter<ValueTraits, HashTranslator> >(value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    typename HashMap<T, U, V, W, X>::AddResult
    HashMap<T, U, V, W, X>::inlineAdd(const KeyType& key, MappedPassInReferenceType mapped) 
    {
        return m_impl.template add<HashMapTranslator<ValueTraits, HashFunctions> >(key, mapped);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    typename HashMap<T, U, V, W, X>::AddResult
    HashMap<T, U, V, W, X>::set(const KeyType& key, MappedPassInType mapped) 
    {
        AddResult result = inlineAdd(key, mapped);
        if (!result.isNewEntry) {
            // The inlineAdd call above found an existing hash table entry; we need to set the mapped value.
            MappedTraits::store(mapped, result.iterator->value);
        }
        return result;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    template<typename HashTranslator, typename TYPE>
    typename HashMap<T, U, V, W, X>::AddResult
    HashMap<T, U, V, W, X>::add(const TYPE& key, MappedPassInType value)
    {
        return m_impl.template addPassingHashCode<HashMapTranslatorAdapter<ValueTraits, HashTranslator> >(key, value);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    typename HashMap<T, U, V, W, X>::AddResult
    HashMap<T, U, V, W, X>::add(const KeyType& key, MappedPassInType mapped)
    {
        return inlineAdd(key, mapped);
    }

    template<typename T, typename U, typename V, typename W, typename MappedTraits>
    typename HashMap<T, U, V, W, MappedTraits>::MappedPeekType
    HashMap<T, U, V, W, MappedTraits>::get(const KeyType& key) const
    {
        ValueType* entry = const_cast<HashTableType&>(m_impl).lookup(key);
        if (!entry)
            return MappedTraits::peek(MappedTraits::emptyValue());
        return MappedTraits::peek(entry->value);
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
    typename HashMap<T, U, V, W, MappedTraits>::MappedPassOutType
    HashMap<T, U, V, W, MappedTraits>::take(const KeyType& key)
    {
        iterator it = find(key);
        if (it == end())
            return MappedTraits::passOut(MappedTraits::emptyValue());
        MappedPassOutType result = MappedTraits::passOut(it->value);
        remove(it);
        return result;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void HashMap<T, U, V, W, X>::checkConsistency() const
    {
        m_impl.checkTableConsistency();
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline bool HashMap<T, U, V, W, X>::isValidKey(const KeyType& key)
    {
        if (KeyTraits::isDeletedValue(key))
            return false;

        if (HashFunctions::safeToCompareToEmptyOrDeleted) {
            if (key == KeyTraits::emptyValue())
                return false;
        } else {
            if (isHashTraitsEmptyValue<KeyTraits>(key))
                return false;
        }

        return true;
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
            const_iterator bPos = b.find(it->key);
            if (bPos == notFound || it->value != bPos->value)
                return false;
        }

        return true;
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline bool operator!=(const HashMap<T, U, V, W, X>& a, const HashMap<T, U, V, W, X>& b)
    {
        return !(a == b);
    }

    template<typename T, typename U, typename V, typename W, typename X>
    inline void deleteAllValues(const HashMap<T, U, V, W, X>& collection)
    {
        typedef typename HashMap<T, U, V, W, X>::const_iterator iterator;
        iterator end = collection.end();
        for (iterator it = collection.begin(); it != end; ++it)
            delete it->value;
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

#include <wtf/RefPtrHashMap.h>

#endif /* WTF_HashMap_h */
