//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BinaryStream.h: Provides binary serialization of simple types.

#ifndef LIBGLESV2_BINARYSTREAM_H_
#define LIBGLESV2_BINARYSTREAM_H_

#include "common/angleutils.h"

namespace gl
{

class BinaryInputStream
{
  public:
    BinaryInputStream(const void *data, size_t length)
    {
        mError = false;
        mOffset = 0;
        mData = static_cast<const char*>(data);
        mLength = length;
    }

    template <typename T>
    void read(T *v, size_t num)
    {
        union
        {
            T dummy;  // Compilation error for non-trivial types
        } dummy;
        (void) dummy;

        if (mError)
        {
            return;
        }

        size_t length = num * sizeof(T);

        if (mOffset + length > mLength)
        {
            mError = true;
            return;
        }

        memcpy(v, mData + mOffset, length);
        mOffset += length;
    }

    template <typename T>
    void read(T * v)
    {
        read(v, 1);
    }

    void read(std::string *v)
    {
        size_t length;
        read(&length);

        if (mError)
        {
            return;
        }

        if (mOffset + length > mLength)
        {
            mError = true;
            return;
        }

        v->assign(mData + mOffset, length);
        mOffset += length;
    }

    void skip(size_t length)
    {
        if (mOffset + length > mLength)
        {
            mError = true;
            return;
        }

        mOffset += length;
    }

    size_t offset() const
    {
        return mOffset;
    }

    bool error() const
    {
        return mError;
    }

    bool endOfStream() const
    {
        return mOffset == mLength;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(BinaryInputStream);
    bool mError;
    size_t mOffset;
    const char *mData;
    size_t mLength;
};

class BinaryOutputStream
{
  public:
    BinaryOutputStream()
    {
    }

    template <typename T>
    void write(const T *v, size_t num)
    {
        union
        {
            T dummy;  // Compilation error for non-trivial types
        } dummy;
        (void) dummy;

        const char *asBytes = reinterpret_cast<const char*>(v);
        mData.insert(mData.end(), asBytes, asBytes + num * sizeof(T));
    }

    template <typename T>
    void write(const T &v)
    {
        write(&v, 1);
    }

    void write(const std::string &v)
    {
        size_t length = v.length();
        write(length);

        write(v.c_str(), length);
    }

    size_t length() const
    {
        return mData.size();
    }

    const void* data() const
    {
        return mData.size() ? &mData[0] : NULL;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(BinaryOutputStream);
    std::vector<char> mData;
};
}

#endif  // LIBGLESV2_BINARYSTREAM_H_
