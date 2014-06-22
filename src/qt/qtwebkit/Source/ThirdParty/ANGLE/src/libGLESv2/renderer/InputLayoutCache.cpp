#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InputLayoutCache.cpp: Defines InputLayoutCache, a class that builds and caches
// D3D11 input layouts.

#include "libGLESv2/renderer/InputLayoutCache.h"
#include "libGLESv2/renderer/VertexBuffer11.h"
#include "libGLESv2/renderer/BufferStorage11.h"
#include "libGLESv2/renderer/ShaderExecutable11.h"
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
            programBinary->getActiveAttribute(ilKey.elementCount, 0, NULL, &attributeSize, &ilKey.glslElementType[ilKey.elementCount], NULL);

            ilKey.elements[ilKey.elementCount].SemanticName = semanticName;
            ilKey.elements[ilKey.elementCount].SemanticIndex = sortedSemanticIndices[i];
            ilKey.elements[ilKey.elementCount].Format = attributes[i].attribute->mArrayEnabled ? vertexBuffer->getDXGIFormat(*attributes[i].attribute) : DXGI_FORMAT_R32G32B32A32_FLOAT;
            ilKey.elements[ilKey.elementCount].InputSlot = i;
            ilKey.elements[ilKey.elementCount].AlignedByteOffset = 0;
            ilKey.elements[ilKey.elementCount].InputSlotClass = inputClass;
            ilKey.elements[ilKey.elementCount].InstanceDataStepRate = attributes[i].divisor;
            ilKey.elementCount++;

            vertexBuffers[i] = bufferStorage ? bufferStorage->getBuffer() : vertexBuffer->getBuffer();
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

        HRESULT result = mDevice->CreateInputLayout(ilKey.elements, ilKey.elementCount, shader->getFunction(), shader->getLength(), &inputLayout);
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

    mDeviceContext->IASetInputLayout(inputLayout);
    mDeviceContext->IASetVertexBuffers(0, gl::MAX_VERTEX_ATTRIBS, vertexBuffers, vertexStrides, vertexOffsets);

    return GL_NO_ERROR;
}

std::size_t InputLayoutCache::hashInputLayout(const InputLayoutKey &inputLayout)
{
    static const unsigned int seed = 0xDEADBEEF;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&inputLayout, sizeof(InputLayoutKey), seed, &hash);
    return hash;
}

bool InputLayoutCache::compareInputLayouts(const InputLayoutKey &a, const InputLayoutKey &b)
{
    return memcmp(&a, &b, sizeof(InputLayoutKey)) == 0;
}

}
