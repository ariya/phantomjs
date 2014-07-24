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

#ifndef Arguments_h
#define Arguments_h

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include <wtf/TypeTraits.h>

namespace CoreIPC {
    
struct Arguments0 {
    typedef Arguments0 ValueType;

    void encode(ArgumentEncoder&) const 
    {
    }

    static bool decode(ArgumentDecoder&, Arguments0&)
    {
        return true;
    }
};

template<typename T1> struct Arguments1 {
    typedef Arguments1<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type> ValueType;

    Arguments1()
    {
    }

    Arguments1(T1 t1) 
        : argument1(t1)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        encoder << argument1;
    }

    static bool decode(ArgumentDecoder& decoder, Arguments1& result)
    {
        return decoder.decode(result.argument1);
    }
    
    T1 argument1;
};
    
template<typename T1, typename T2> struct Arguments2 : Arguments1<T1> {
    typedef Arguments2<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type> ValueType;

    Arguments2() 
    {
    }

    Arguments2(T1 t1, T2 t2) 
        : Arguments1<T1>(t1)
        , argument2(t2)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments1<T1>::encode(encoder);
        encoder << argument2;
    }

    static bool decode(ArgumentDecoder& decoder, Arguments2& result)
    {
        if (!Arguments1<T1>::decode(decoder, result))
            return false;
        
        return decoder.decode(result.argument2);
    }

    T2 argument2;
};

template<typename T1, typename T2, typename T3> struct Arguments3 : Arguments2<T1, T2> {
    typedef Arguments3<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T3>::Type>::Type> ValueType;

    Arguments3()
    {
    }

    Arguments3(T1 t1, T2 t2, T3 t3) 
        : Arguments2<T1, T2>(t1, t2)
        , argument3(t3)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments2<T1, T2>::encode(encoder);
        encoder << argument3;
    }

    static bool decode(ArgumentDecoder& decoder, Arguments3& result)
    {
        if (!Arguments2<T1, T2>::decode(decoder, result))
            return false;
        
        return decoder.decode(result.argument3);
    }

    T3 argument3;
};

template<typename T1, typename T2, typename T3, typename T4> struct Arguments4 : Arguments3<T1, T2, T3> {
    typedef Arguments4<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T3>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T4>::Type>::Type> ValueType;

    Arguments4()
    {
    }

    Arguments4(T1 t1, T2 t2, T3 t3, T4 t4)
        : Arguments3<T1, T2, T3>(t1, t2, t3)
        , argument4(t4)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments3<T1, T2, T3>::encode(encoder);
        encoder << argument4;
    }
    
    static bool decode(ArgumentDecoder& decoder, Arguments4& result)
    {
        if (!Arguments3<T1, T2, T3>::decode(decoder, result))
            return false;
        
        return decoder.decode(result.argument4);
    }

    T4 argument4;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5> struct Arguments5 : Arguments4<T1, T2, T3, T4> {
    typedef Arguments5<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T3>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T4>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T5>::Type>::Type> ValueType;

    Arguments5()
    {
    }

    Arguments5(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
        : Arguments4<T1, T2, T3, T4>(t1, t2, t3, t4)
        , argument5(t5)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments4<T1, T2, T3, T4>::encode(encoder);
        encoder << argument5;
    }
    
    static bool decode(ArgumentDecoder& decoder, Arguments5& result)
    {
        if (!Arguments4<T1, T2, T3, T4>::decode(decoder, result))
            return false;
        
        return decoder.decode(result.argument5);
    }

    T5 argument5;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> struct Arguments6 : Arguments5<T1, T2, T3, T4, T5> {
    typedef Arguments6<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T3>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T4>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T5>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T6>::Type>::Type> ValueType;

    Arguments6()
    {
    }

    Arguments6(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
        : Arguments5<T1, T2, T3, T4, T5>(t1, t2, t3, t4, t5)
        , argument6(t6)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments5<T1, T2, T3, T4, T5>::encode(encoder);
        encoder << argument6;
    }
    
    static bool decode(ArgumentDecoder& decoder, Arguments6& result)
    {
        if (!Arguments5<T1, T2, T3, T4, T5>::decode(decoder, result))
            return false;
        
        return decoder.decode(result.argument6);
    }

    T6 argument6;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> struct Arguments7 : Arguments6<T1, T2, T3, T4, T5, T6> {
    typedef Arguments7<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T3>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T4>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T5>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T6>::Type>::Type,
                       typename WTF::RemoveConst<typename WTF::RemoveReference<T7>::Type>::Type> ValueType;

    Arguments7()
    {
    }

    Arguments7(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
        : Arguments6<T1, T2, T3, T4, T5, T6>(t1, t2, t3, t4, t5, t6)
        , argument7(t7)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments6<T1, T2, T3, T4, T5, T6>::encode(encoder);
        encoder << argument7;
    }
    
    static bool decode(ArgumentDecoder& decoder, Arguments7& result)
    {
        if (!Arguments6<T1, T2, T3, T4, T5, T6>::decode(decoder, result))
            return false;
        
        return decoder.decode(result.argument7);
    }

    T7 argument7;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> struct Arguments8 : Arguments7<T1, T2, T3, T4, T5, T6, T7> {
    typedef Arguments8<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T3>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T4>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T5>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T6>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T7>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T8>::Type>::Type> ValueType;

    Arguments8() { }
    
    Arguments8(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
        : Arguments7<T1, T2, T3, T4, T5, T6, T7>(t1, t2, t3, t4, t5, t6, t7)
        , argument8(t8)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments7<T1, T2, T3, T4, T5, T6, T7>::encode(encoder);
        encoder << argument8;
    }

    static bool decode(ArgumentDecoder& decoder, Arguments8& result)
    {
        if (!Arguments7<T1, T2, T3, T4, T5, T6, T7>::decode(decoder, result))
            return false;

        return decoder.decode(result.argument8);
    }

    T8 argument8;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10> struct Arguments10 : Arguments8<T1, T2, T3, T4, T5, T6, T7, T8> {
    typedef Arguments10<typename WTF::RemoveConst<typename WTF::RemoveReference<T1>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T2>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T3>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T4>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T5>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T6>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T7>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T8>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T9>::Type>::Type,
    typename WTF::RemoveConst<typename WTF::RemoveReference<T10>::Type>::Type> ValueType;

    Arguments10() { }
    
    Arguments10(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10)
        : Arguments8<T1, T2, T3, T4, T5, T6, T7, T8>(t1, t2, t3, t4, t5, t6, t7, t8)
        , argument9(t9)
        , argument10(t10)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        Arguments8<T1, T2, T3, T4, T5, T6, T7, T8>::encode(encoder);
        encoder << argument9;
        encoder << argument10;
    }

    static bool decode(ArgumentDecoder& decoder, Arguments10& result)
    {
        if (!Arguments8<T1, T2, T3, T4, T5, T6, T7, T8>::decode(decoder, result))
            return false;

        decoder.decode(result.argument9);
        return decoder.decode(result.argument10);
    }

    T9 argument9;
    T10 argument10;
};

} // namespace CoreIPC

#endif // Arguments_h
