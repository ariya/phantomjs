//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// copyimage.inl: Defines image copying functions

namespace rx
{

template <typename sourceType, typename colorDataType>
inline void ReadColor(const uint8_t *source, uint8_t *dest)
{
    sourceType::readColor(reinterpret_cast<gl::Color<colorDataType>*>(dest), reinterpret_cast<const sourceType*>(source));
}

template <typename destType, typename colorDataType>
inline void WriteColor(const uint8_t *source, uint8_t *dest)
{
    destType::writeColor(reinterpret_cast<destType*>(dest), reinterpret_cast<const gl::Color<colorDataType>*>(source));
}

template <typename sourceType, typename destType, typename colorDataType>
inline void CopyPixel(const uint8_t *source, uint8_t *dest)
{
    colorDataType temp;
    ReadColor<sourceType, colorDataType>(source, &temp);
    WriteColor<destType, colorDataType>(&temp, dest);
}

}
