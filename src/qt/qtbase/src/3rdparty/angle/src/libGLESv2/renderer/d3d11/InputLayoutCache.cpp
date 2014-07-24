#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InputLayoutCache.cpp: Defines InputLayoutCache, a class that builds and caches
// D3D11 input layouts.

#include "libGLESv2/renderer/d3d11/InputLayoutCache.h"
#include "libGLESv2/renderer/d3d11/VertexBuffer11.h"
#include "libGLESv2/renderer/d3d11/BufferStorage11.h"
#include "libGLESv2/renderer/d3d11/ShaderExecutable11.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/VertexDataManager.h"

#include "third_party/murmurhash/MurmurHash3.h"

namespace rx
{

const unsigned int InputLayoutCache::kMaxInputLayouts = 1024;

InputLayoutCache::InputLayoutCache() : mInputLayoutMap(kMaxInputLayouts, hashInputLayout, compareInputLayouts)
{
    mCounter = 0;
    mDevice = NULL;
    mDeviceContext = NULL;
    mCurrentIL = NULL;
    for (unsigned int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mCurrentBuffers[i] = -1;
        mCurrentVertexStrides[i] = -1;
        mCurrentVertexOffsets[i] = -1;
    }
}

InputLayoutCache::~InputLayoutCache()
{
    clear();
}

void InputLayoutCache::initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
    clear();
    mDevice = device;
    mDeviceContext = context;
}

void InputLayoutCache::clear()
{
    for (InputLayoutMap::iterator i = mInputLayoutMap.begin(); i != mInputLayoutMap.end(); i++)
    {
        i->second.inputLayout->Release();
    }
    mInputLayoutMap.clear();
    markDirty();
}

void InputLayoutCache::markDirty()
{
    mCurrentIL = NULL;
    for (unsigned int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mCurrentBuffers[i] = -1;
        mCurrentVertexStrides[i] = -1;
        mCurrentVertexOffsets[i] = -1;
    }
}

GLenum InputLayoutCache::applyVertexBuffers(TranslatedAttribute attributes[gl::MAX_VERTEX_ATTRIBS],
                                            gl::ProgramBinary *programBinary)
{
    int sortedSemanticIndices[gl::MAX_VERTEX_ATTRIBS];
    programBinary->sortAttributesByLayout(attributes, sortedSemanticIndices);

    if (!mDevice || !mDeviceContext)
    {
        ERR("InputLayoutCache is not initialized.");
        return GL_INVALID_OPERATION;
    }

    InputLayoutKey ilKey = { 0 };

    ID3D11Buffer *vertexBuffers[gl::MAX_VERTEX_ATTRIBS] = { NULL };
    unsigned int vertexBufferSerials[gl::MAX_VERTEX_ATTRIBS] = { 0 };
    UINT vertexStrides[gl::MAX_VERTEX_ATTRIBS] = { 0 };
    UINT vertexOffsets[gl::MAX_VERTEX_ATTRIBS] = { 0 };

    static const char* semanticName = "TEXCOORD";

    for (unsigned int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (attributes[i].active)
        {
            VertexBuffer11 *vertexBuffer = VertexBuffer11::makeVertexBuffer11(attributes[i].vertexBuffer);
            BufferStorage11 *bufferStorage = attributes[i].storage ? BufferStorage11::makeBufferStorage11(attributes[i].storage) : NULL;

            D3D11_INPUT_CLASSIFICATION inputClass = attributes[i].divisor > 0 ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;

            // Record the type of the associated vertex shader vector in our key
            // This will prevent mismatched vertex shaders from using the same input layout
            GLint attributeSize;
            programBinary->getActiveAttribute(sortedSemanticIndices[i], 0, NULL, &attributeSize, &ilKey.elements[ilKey.elementCount].glslElementType, NULL);

            ilKey.elements[ilKey.elementCount].desc.SemanticName = semanticName;
            ilKey.elements[ilKey.elementCount].desc.SemanticIndex = i;
            ilKey.elements[ilKey.elementCount].desc.Format = attributes[i].attribute->mArrayEnabled ? vertexBuffer->getDXGIFormat(*attributes[i].attribute) : DXGI_FORMAT_R32G32B32A32_FLOAT;
            ilKey.elements[ilKey.elementCount].desc.InputSlot = i;
            ilKey.elements[ilKey.elementCount].desc.AlignedByteOffset = 0;
            ilKey.elements[ilKey.elementCount].desc.InputSlotClass = inputClass;
            ilKey.elements[ilKey.elementCount].desc.InstanceDataStepRate = attributes[i].divisor;
            ilKey.elementCount++;

            vertexBuffers[i] = bufferStorage ? bufferStorage->getBuffer(BUFFER_USAGE_VERTEX) : vertexBuffer->getBuffer();
            vertexBufferSerials[i] = bufferStorage ? bufferStorage->getSerial() : vertexBuffer->getSerial();
            vertexStrides[i] = attributes[i].stride;
            vertexOffsets[i] = attributes[i].offset;
        }
    }

    ID3D11InputLayout *inputLayout = NULL;

    InputLayoutMap::iterator i = mInputLayoutMap.find(ilKey);
    if (i != mInputLayoutMap.end())
    {
        inputLayout = i->second.inputLayout;
        i->second.lastUsedTime = mCounter++;
    }
    else
    {
        ShaderExecutable11 *shader = ShaderExecutable11::makeShaderExecutable11(programBinary->getVertexExecutable());

        D3D11_INPUT_ELEMENT_DESC descs[gl::MAX_VERTEX_ATTRIBS];
        for (unsigned int j = 0; j < ilKey.elementCount; ++j)
        {
            descs[j] = ilKey.elements[j].desc;
        }

        HRESULT result = mDevice->CreateInputLayout(descs, ilKey.elementCount, shader->getFunction(), shader->getLength(), &inputLayout);
        if (FAILED(result))
        {
            ERR("Failed to crate input layout, result: 0x%08x", result);
            return GL_INVALID_OPERATION;
        }

        if (mInputLayoutMap.size() >= kMaxInputLayouts)
        {
            TRACE("Overflowed the limit of %u input layouts, removing the least recently used "
                  "to make room.", kMaxInputLayouts);

            InputLayoutMap::iterator leastRecentlyUsed = mInputLayoutMap.begin();
            for (InputLayoutMap::iterator i = mInputLayoutMap.begin(); i != mInputLayoutMap.end(); i++)
            {
                if (i->second.lastUsedTime < leastRecentlyUsed->second.lastUsedTime)
                {
                    leastRecentlyUsed = i;
                }
            }
            leastRecentlyUsed->second.inputLayout->Release();
            mInputLayoutMap.erase(leastRecentlyUsed);
        }

        InputLayoutCounterPair inputCounterPair;
        inputCounterPair.inputLayout = inputLayout;
        inputCounterPair.lastUsedTime = mCounter++;

        mInputLayoutMap.insert(std::make_pair(ilKey, inputCounterPair));
    }

    if (inputLayout != mCurrentIL)
    {
        mDeviceContext->IASetInputLayout(inputLayout);
        mCurrentIL = inputLayout;
    }

    for (unsigned int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (vertexBufferSerials[i] != mCurrentBuffers[i] || vertexStrides[i] != mCurrentVertexStrides[i] ||
            vertexOffsets[i] != mCurrentVertexOffsets[i])
        {
            mDeviceContext->IASetVertexBuffers(i, 1, &vertexBuffers[i], &vertexStrides[i], &vertexOffsets[i]);
            mCurrentBuffers[i] = vertexBufferSerials[i];
            mCurrentVertexStrides[i] = vertexStrides[i];
            mCurrentVertexOffsets[i] = vertexOffsets[i];
        }
    }

    return GL_NO_ERROR;
}

std::size_t InputLayoutCache::hashInputLayout(const InputLayoutKey &inputLayout)
{
    static const unsigned int seed = 0xDEADBEEF;

    std::size_t hash = 0;
    MurmurHash3_x86_32(inputLayout.begin(), inputLayout.end() - inputLayout.begin(), seed, &hash);
    return hash;
}

bool InputLayoutCache::compareInputLayouts(const InputLayoutKey &a, const InputLayoutKey &b)
{
    if (a.elementCount != b.elementCount)
    {
        return false;
    }

    return std::equal(a.begin(), a.end(), b.begin());
}

}
