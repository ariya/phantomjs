//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// vertexconversion.h: A library of vertex conversion classes that can be used to build
// the FormatConverter objects used by the buffer conversion system.

#ifndef LIBGLESV2_VERTEXCONVERSION_H_
#define LIBGLESV2_VERTEXCONVERSION_H_

namespace rx
{

// Conversion types:
// static const bool identity: true if this is an identity transform, false otherwise
// static U convert(T): convert a single element from the input type to the output type
// typedef ... OutputType: the type produced by this conversion

template <class T>
struct Identity
{
    static const bool identity = true;

    typedef T OutputType;

    static T convert(T x)
    {
        return x;
    }
};

template <class FromT, class ToT>
struct Cast
{
    static const bool identity = false;

    typedef ToT OutputType;

    static ToT convert(FromT x)
    {
        return static_cast<ToT>(x);
    }
};

template <class T>
struct Cast<T, T>
{
    static const bool identity = true;

    typedef T OutputType;

    static T convert(T x)
    {
        return static_cast<T>(x);
    }
};

template <class T>
struct Normalize
{
    static const bool identity = false;

    typedef float OutputType;

    static float convert(T x)
    {
        typedef std::numeric_limits<T> NL;
        float f = static_cast<float>(x);

        if (NL::is_signed)
        {
            // const float => VC2008 computes it at compile time
            // static const float => VC2008 computes it the first time we get here, stores it to memory with static guard and all that.
            const float divisor = 1.0f/(2*static_cast<float>(NL::max())+1);
            return (2*f+1)*divisor;
        }
        else
        {
            return f/NL::max();
        }
    }
};

template <class FromType, std::size_t ScaleBits>
struct FixedToFloat
{
    static const bool identity = false;

    typedef float OutputType;

    static float convert(FromType x)
    {
        const float divisor = 1.0f / static_cast<float>(static_cast<FromType>(1) << ScaleBits);
        return static_cast<float>(x) * divisor;
    }
};

// Widen types:
// static const unsigned int initialWidth: number of components before conversion
// static const unsigned int finalWidth: number of components after conversion

// Float is supported at any size.
template <std::size_t N>
struct NoWiden
{
    static const std::size_t initialWidth = N;
    static const std::size_t finalWidth = N;
};

// SHORT, norm-SHORT, norm-UNSIGNED_SHORT are supported but only with 2 or 4 components
template <std::size_t N>
struct WidenToEven
{
    static const std::size_t initialWidth = N;
    static const std::size_t finalWidth = N+(N&1);
};

template <std::size_t N>
struct WidenToFour
{
    static const std::size_t initialWidth = N;
    static const std::size_t finalWidth = 4;
};

// Most types have 0 and 1 that are just that.
template <class T>
struct SimpleDefaultValues
{
    static T zero() { return static_cast<T>(0); }
    static T one() { return static_cast<T>(1); }
};

// But normalised types only store [0,1] or [-1,1] so 1.0 is represented by the max value.
template <class T>
struct NormalizedDefaultValues
{
    static T zero() { return static_cast<T>(0); }
    static T one() { return std::numeric_limits<T>::max(); }
};

// Converter:
// static const bool identity: true if this is an identity transform (with no widening)
// static const std::size_t finalSize: number of bytes per output vertex
// static void convertArray(const void *in, std::size_t stride, std::size_t n, void *out): convert an array of vertices. Input may be strided, but output will be unstrided.

template <class InT, class WidenRule, class Converter, class DefaultValueRule = SimpleDefaultValues<InT> >
struct VertexDataConverter
{
    typedef typename Converter::OutputType OutputType;
    typedef InT InputType;

    static const bool identity = (WidenRule::initialWidth == WidenRule::finalWidth) && Converter::identity;
    static const std::size_t finalSize = WidenRule::finalWidth * sizeof(OutputType);

    static void convertArray(const InputType *in, std::size_t stride, std::size_t n, OutputType *out)
    {
        for (std::size_t i = 0; i < n; i++)
        {
            const InputType *ein = pointerAddBytes(in, i * stride);

            copyComponent(out, ein, 0, static_cast<OutputType>(DefaultValueRule::zero()));
            copyComponent(out, ein, 1, static_cast<OutputType>(DefaultValueRule::zero()));
            copyComponent(out, ein, 2, static_cast<OutputType>(DefaultValueRule::zero()));
            copyComponent(out, ein, 3, static_cast<OutputType>(DefaultValueRule::one()));

            out += WidenRule::finalWidth;
        }
    }

    static void convertArray(const void *in, std::size_t stride, std::size_t n, void *out)
    {
        return convertArray(static_cast<const InputType*>(in), stride, n, static_cast<OutputType*>(out));
    }

  private:
    // Advance the given pointer by a number of bytes (not pointed-to elements).
    template <class T>
    static T *pointerAddBytes(T *basePtr, std::size_t numBytes)
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(basePtr) + numBytes);
    }

    static void copyComponent(OutputType *out, const InputType *in, std::size_t elementindex, OutputType defaultvalue)
    {
        if (WidenRule::finalWidth > elementindex)
        {
            if (WidenRule::initialWidth > elementindex)
            {
                out[elementindex] = Converter::convert(in[elementindex]);
            }
            else
            {
                out[elementindex] = defaultvalue;
            }
        }
    }
};

}

#endif   // LIBGLESV2_VERTEXCONVERSION_H_
