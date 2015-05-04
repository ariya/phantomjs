//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// blocklayout.h:
//   Methods and classes related to uniform layout and packing in GLSL and HLSL.
//

#ifndef COMMON_BLOCKLAYOUT_H_
#define COMMON_BLOCKLAYOUT_H_

#include <cstddef>
#include <vector>

#include "angle_gl.h"
#include <GLSLANG/ShaderLang.h>

namespace sh
{
struct ShaderVariable;
struct InterfaceBlockField;
struct Uniform;
struct Varying;
struct InterfaceBlock;

struct BlockMemberInfo
{
    BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
        : offset(offset),
          arrayStride(arrayStride),
          matrixStride(matrixStride),
          isRowMajorMatrix(isRowMajorMatrix)
    {}

    static BlockMemberInfo getDefaultBlockInfo()
    {
        return BlockMemberInfo(-1, -1, -1, false);
    }

    int offset;
    int arrayStride;
    int matrixStride;
    bool isRowMajorMatrix;
};

class BlockLayoutEncoder
{
  public:
    BlockLayoutEncoder();

    BlockMemberInfo encodeType(GLenum type, unsigned int arraySize, bool isRowMajorMatrix);

    size_t getBlockSize() const { return mCurrentOffset * BytesPerComponent; }
    size_t getCurrentRegister() const { return mCurrentOffset / ComponentsPerRegister; }
    size_t getCurrentElement() const { return mCurrentOffset % ComponentsPerRegister; }

    virtual void enterAggregateType() = 0;
    virtual void exitAggregateType() = 0;

    static const size_t BytesPerComponent = 4u;
    static const unsigned int ComponentsPerRegister = 4u;

  protected:
    size_t mCurrentOffset;

    void nextRegister();

    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut) = 0;
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride) = 0;
};

// Block layout according to the std140 block layout
// See "Standard Uniform Block Layout" in Section 2.11.6 of the OpenGL ES 3.0 specification

class Std140BlockEncoder : public BlockLayoutEncoder
{
  public:
    Std140BlockEncoder();

    virtual void enterAggregateType();
    virtual void exitAggregateType();

  protected:
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride);
};

// Block layout packed according to the D3D9 or default D3D10+ register packing rules
// See http://msdn.microsoft.com/en-us/library/windows/desktop/bb509632(v=vs.85).aspx
// The strategy should be ENCODE_LOOSE for D3D9 constant blocks, and ENCODE_PACKED
// for everything else (D3D10+ constant blocks and all attributes/varyings).

class HLSLBlockEncoder : public BlockLayoutEncoder
{
  public:
    enum HLSLBlockEncoderStrategy
    {
        ENCODE_PACKED,
        ENCODE_LOOSE
    };

    HLSLBlockEncoder(HLSLBlockEncoderStrategy strategy);

    virtual void enterAggregateType();
    virtual void exitAggregateType();
    void skipRegisters(unsigned int numRegisters);

    bool isPacked() const { return mEncoderStrategy == ENCODE_PACKED; }

    static HLSLBlockEncoderStrategy GetStrategyFor(ShShaderOutput outputType);

  protected:
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride);

    HLSLBlockEncoderStrategy mEncoderStrategy;
};

// This method returns the number of used registers for a ShaderVariable. It is dependent on the HLSLBlockEncoder
// class to count the number of used registers in a struct (which are individually packed according to the same rules).
unsigned int HLSLVariableRegisterCount(const Varying &variable);
unsigned int HLSLVariableRegisterCount(const Uniform &variable, ShShaderOutput outputType);

}

#endif // COMMON_BLOCKLAYOUT_H_
