#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexDeclarationCache.cpp: Implements a helper class to construct and cache vertex declarations.

#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/d3d9/VertexBuffer9.h"
#include "libGLESv2/renderer/d3d9/VertexDeclarationCache.h"

namespace rx
{

VertexDeclarationCache::VertexDeclarationCache() : mMaxLru(0)
{
    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        mVertexDeclCache[i].vertexDeclaration = NULL;
        mVertexDeclCache[i].lruCount = 0;
    }

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mAppliedVBs[i].serial = 0;
    }

    mLastSetVDecl = NULL;
    mInstancingEnabled = true;
}

VertexDeclarationCache::~VertexDeclarationCache()
{
    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        if (mVertexDeclCache[i].vertexDeclaration)
        {
            mVertexDeclCache[i].vertexDeclaration->Release();
        }
    }
}

GLenum VertexDeclarationCache::applyDeclaration(IDirect3DDevice9 *device, TranslatedAttribute attributes[], gl::ProgramBinary *programBinary, GLsizei instances, GLsizei *repeatDraw)
{
    *repeatDraw = 1;

    int indexedAttribute = gl::MAX_VERTEX_ATTRIBS;
    int instancedAttribute = gl::MAX_VERTEX_ATTRIBS;

    if (instances > 0)
    {
        // Find an indexed attribute to be mapped to D3D stream 0
        for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
        {
            if (attributes[i].active)
            {
                if (indexedAttribute == gl::MAX_VERTEX_ATTRIBS && attributes[i].divisor == 0)
                {
                    indexedAttribute = i;
                }
                else if (instancedAttribute == gl::MAX_VERTEX_ATTRIBS && attributes[i].divisor != 0)
                {
                    instancedAttribute = i;
                }
                if (indexedAttribute != gl::MAX_VERTEX_ATTRIBS && instancedAttribute != gl::MAX_VERTEX_ATTRIBS)
                    break;   // Found both an indexed and instanced attribute
            }
        }

        if (indexedAttribute == gl::MAX_VERTEX_ATTRIBS)
        {
            return GL_INVALID_OPERATION;
        }
    }

    D3DVERTEXELEMENT9 elements[gl::MAX_VERTEX_ATTRIBS + 1];
    D3DVERTEXELEMENT9 *element = &elements[0];

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (attributes[i].active)
        {
            // Directly binding the storage buffer is not supported for d3d9
            ASSERT(attributes[i].storage == NULL);

            int stream = i;

            if (instances > 0)
            {
                // Due to a bug on ATI cards we can't enable instancing when none of the attributes are instanced.
                if (instancedAttribute == gl::MAX_VERTEX_ATTRIBS)
                {
                    *repeatDraw = instances;
                }
                else
                {
                    if (i == indexedAttribute)
                    {
                        stream = 0;
                    }
                    else if (i == 0)
                    {
                        stream = indexedAttribute;
                    }

                    UINT frequency = 1;
                    
                    if (attributes[i].divisor == 0)
                    {
                        frequency = D3DSTREAMSOURCE_INDEXEDDATA | instances;
                    }
                    else
                    {
                        frequency = D3DSTREAMSOURCE_INSTANCEDATA | attributes[i].divisor;
                    }
                    
                    device->SetStreamSourceFreq(stream, frequency);
                    mInstancingEnabled = true;
                }
            }

            VertexBuffer9 *vertexBuffer = VertexBuffer9::makeVertexBuffer9(attributes[i].vertexBuffer);

            if (mAppliedVBs[stream].serial != attributes[i].serial ||
                mAppliedVBs[stream].stride != attributes[i].stride ||
                mAppliedVBs[stream].offset != attributes[i].offset)
            {
                device->SetStreamSource(stream, vertexBuffer->getBuffer(), attributes[i].offset, attributes[i].stride);
                mAppliedVBs[stream].serial = attributes[i].serial;
                mAppliedVBs[stream].stride = attributes[i].stride;
                mAppliedVBs[stream].offset = attributes[i].offset;
            }

            element->Stream = stream;
            element->Offset = 0;
            element->Type = attributes[i].attribute->mArrayEnabled ? vertexBuffer->getDeclType(*attributes[i].attribute) : D3DDECLTYPE_FLOAT4;
            element->Method = D3DDECLMETHOD_DEFAULT;
            element->Usage = D3DDECLUSAGE_TEXCOORD;
            element->UsageIndex = programBinary->getSemanticIndex(i);
            element++;
        }
    }

    if (instances == 0 || instancedAttribute == gl::MAX_VERTEX_ATTRIBS)
    {
        if (mInstancingEnabled)
        {
            for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
            {
                device->SetStreamSourceFreq(i, 1);
            }

            mInstancingEnabled = false;
        }
    }

    static const D3DVERTEXELEMENT9 end = D3DDECL_END();
    *(element++) = end;

    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        VertexDeclCacheEntry *entry = &mVertexDeclCache[i];
        if (memcmp(entry->cachedElements, elements, (element - elements) * sizeof(D3DVERTEXELEMENT9)) == 0 && entry->vertexDeclaration)
        {
            entry->lruCount = ++mMaxLru;
            if(entry->vertexDeclaration != mLastSetVDecl)
            {
                device->SetVertexDeclaration(entry->vertexDeclaration);
                mLastSetVDecl = entry->vertexDeclaration;
            }

            return GL_NO_ERROR;
        }
    }

    VertexDeclCacheEntry *lastCache = mVertexDeclCache;

    for (int i = 0; i < NUM_VERTEX_DECL_CACHE_ENTRIES; i++)
    {
        if (mVertexDeclCache[i].lruCount < lastCache->lruCount)
        {
            lastCache = &mVertexDeclCache[i];
        }
    }

    if (lastCache->vertexDeclaration != NULL)
    {
        lastCache->vertexDeclaration->Release();
        lastCache->vertexDeclaration = NULL;
        // mLastSetVDecl is set to the replacement, so we don't have to worry
        // about it.
    }

    memcpy(lastCache->cachedElements, elements, (element - elements) * sizeof(D3DVERTEXELEMENT9));
    device->CreateVertexDeclaration(elements, &lastCache->vertexDeclaration);
    device->SetVertexDeclaration(lastCache->vertexDeclaration);
    mLastSetVDecl = lastCache->vertexDeclaration;
    lastCache->lruCount = ++mMaxLru;

    return GL_NO_ERROR;
}

void VertexDeclarationCache::markStateDirty()
{
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mAppliedVBs[i].serial = 0;
    }

    mLastSetVDecl = NULL;
    mInstancingEnabled = true;   // Forces it to be disabled when not used
}

}
