//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InputLayoutCache.h: Defines InputLayoutCache, a class that builds and caches
// D3D11 input layouts.

#ifndef LIBGLESV2_RENDERER_INPUTLAYOUTCACHE_H_
#define LIBGLESV2_RENDERER_INPUTLAYOUTCACHE_H_

#include "libGLESv2/Constants.h"
#include "common/angleutils.h"

namespace gl
{
class ProgramBinary;
}

namespace rx
{
struct TranslatedAttribute;

class InputLayoutCache
{
  public:
    InputLayoutCache();
    virtual ~InputLayoutCache();

    void initialize(ID3D11Device *device, ID3D11DeviceContext *context);
    void clear();

    GLenum applyVertexBuffers(TranslatedAttribute attributes[gl::MAX_VERTEX_ATTRIBS],
                              gl::ProgramBinary *programBinary);

  private:
    DISALLOW_COPY_AND_ASSIGN(InputLayoutCache);

    struct InputLayoutKey
    {
        unsigned int elementCount;
        D3D11_INPUT_ELEMENT_DESC elements[gl::MAX_VERTEX_ATTRIBS];
        GLenum glslElementType[gl::MAX_VERTEX_ATTRIBS];
    };

    struct InputLayoutCounterPair
    {
        ID3D11InputLayout *inputLayout;
        unsigned long long lastUsedTime;
    };

    static std::size_t hashInputLayout(const InputLayoutKey &inputLayout);
    static bool compareInputLayouts(const InputLayoutKey &a, const InputLayoutKey &b);

    typedef std::size_t (*InputLayoutHashFunction)(const InputLayoutKey &);
    typedef bool (*InputLayoutEqualityFunction)(const InputLayoutKey &, const InputLayoutKey &);
    typedef std::unordered_map<InputLayoutKey,
                               InputLayoutCounterPair,
                               InputLayoutHashFunction,
                               InputLayoutEqualityFunction> InputLayoutMap;
    InputLayoutMap mInputLayoutMap;

    static const unsigned int kMaxInputLayouts;

    unsigned long long mCounter;

    ID3D11Device *mDevice;
    ID3D11DeviceContext *mDeviceContext;
};

}

#endif // LIBGLESV2_RENDERER_INPUTLAYOUTCACHE_H_
