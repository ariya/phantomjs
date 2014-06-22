/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef SimpleArgumentCoder_h
#define SimpleArgumentCoder_h

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include <utility>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/TypeTraits.h>
#include <wtf/Vector.h>

namespace CoreIPC {

// An argument coder works on POD types
template<typename T> struct SimpleArgumentCoder {
    static void encode(ArgumentEncoder& encoder, const T& t)
    {
        encoder.encodeFixedLengthData(reinterpret_cast<const uint8_t*>(&t), sizeof(T), __alignof(T));
    }

    static bool decode(ArgumentDecoder& decoder, T& t)
    {
        return decoder.decodeFixedLengthData(reinterpret_cast<uint8_t*>(&t), sizeof(T), __alignof(T));
    }
};

template<typename T, typename U> struct ArgumentCoder<std::pair<T, U> > {
    static void encode(ArgumentEncoder& encoder, const std::pair<T, U>& pair)
    {
        encoder << pair.first << pair.second;
    }

    static bool decode(ArgumentDecoder& decoder, std::pair<T, U>& pair)
    {
        T first;
        if (!decoder.decode(first))
            return false;

        U second;
        if (!decoder.decode(second))
            return false;

        pair.first = first;
        pair.second = second;
        return true;
    }
};

template<typename KeyType, typename ValueType> struct ArgumentCoder<WTF::KeyValuePair<KeyType, ValueType> > {
    static void encode(ArgumentEncoder& encoder, const WTF::KeyValuePair<KeyType, ValueType>& pair)
    {
        encoder << pair.key << pair.value;
    }

    static bool decode(ArgumentDecoder& decoder, WTF::KeyValuePair<KeyType, ValueType>& pair)
    {
        KeyType key;
        if (!decoder.decode(key))
            return false;

        ValueType value;
        if (!decoder.decode(value))
            return false;

        pair.key = key;
        pair.value = value;
        return true;
    }
};

template<bool fixedSizeElements, typename T> struct VectorArgumentCoder;

template<typename T> struct VectorArgumentCoder<false, T> {
    static void encode(ArgumentEncoder& encoder, const Vector<T>& vector)
    {
        encoder << static_cast<uint64_t>(vector.size());
        for (size_t i = 0; i < vector.size(); ++i)
            encoder << vector[i];
    }

    static bool decode(ArgumentDecoder& decoder, Vector<T>& vector)
    {
        uint64_t size;
        if (!decoder.decode(size))
            return false;

        Vector<T> tmp;
        for (size_t i = 0; i < size; ++i) {
            T element;
            if (!decoder.decode(element))
                return false;
            
            tmp.append(element);
        }

        tmp.shrinkToFit();
        vector.swap(tmp);
        return true;
    }
};

template<typename T> struct VectorArgumentCoder<true, T> {
    static void encode(ArgumentEncoder& encoder, const Vector<T>& vector)
    {
        encoder << static_cast<uint64_t>(vector.size());
        encoder.encodeFixedLengthData(reinterpret_cast<const uint8_t*>(vector.data()), vector.size() * sizeof(T), __alignof(T));
    }
    
    static bool decode(ArgumentDecoder& decoder, Vector<T>& vector)
    {
        uint64_t size;
        if (!decoder.decode(size))
            return false;

        // Since we know the total size of the elements, we can allocate the vector in
        // one fell swoop. Before allocating we must however make sure that the decoder buffer
        // is big enough.
        if (!decoder.bufferIsLargeEnoughToContain<T>(size)) {
            decoder.markInvalid();
            return false;
        }

        Vector<T> temp;
        temp.resize(size);

        decoder.decodeFixedLengthData(reinterpret_cast<uint8_t*>(temp.data()), size * sizeof(T), __alignof(T));

        vector.swap(temp);
        return true;
    }
};

template<typename T> struct ArgumentCoder<Vector<T> > : VectorArgumentCoder<WTF::IsArithmetic<T>::value, T> { };

template<typename KeyArg, typename MappedArg, typename HashArg, typename KeyTraitsArg, typename MappedTraitsArg> struct ArgumentCoder<HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg> > {
    typedef HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg> HashMapType;

    static void encode(ArgumentEncoder& encoder, const HashMapType& hashMap)
    {
        encoder << static_cast<uint64_t>(hashMap.size());
        for (typename HashMapType::const_iterator it = hashMap.begin(), end = hashMap.end(); it != end; ++it)
            encoder << *it;
    }

    static bool decode(ArgumentDecoder& decoder, HashMapType& hashMap)
    {
        uint64_t hashMapSize;
        if (!decoder.decode(hashMapSize))
            return false;

        HashMapType tempHashMap;
        for (uint64_t i = 0; i < hashMapSize; ++i) {
            KeyArg key;
            MappedArg value;
            if (!decoder.decode(key))
                return false;
            if (!decoder.decode(value))
                return false;

            if (!tempHashMap.add(key, value).isNewEntry) {
                // The hash map already has the specified key, bail.
                decoder.markInvalid();
                return false;
            }
        }

        hashMap.swap(tempHashMap);
        return true;
    }
};

template<> struct ArgumentCoder<AtomicString> {
    static void encode(ArgumentEncoder&, const AtomicString&);
    static bool decode(ArgumentDecoder&, AtomicString&);
};

template<> struct ArgumentCoder<CString> {
    static void encode(ArgumentEncoder&, const CString&);
    static bool decode(ArgumentDecoder&, CString&);
};

template<> struct ArgumentCoder<String> {
    static void encode(ArgumentEncoder&, const String&);
    static bool decode(ArgumentDecoder&, String&);
};

} // namespace CoreIPC

#endif // ArgumentCoders_h
