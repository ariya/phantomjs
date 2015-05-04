//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ResourceManager.cpp: Implements the gl::ResourceManager class, which tracks and 
// retrieves objects which may be shared by multiple Contexts.

#include "libGLESv2/ResourceManager.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Sampler.h"
#include "libGLESv2/Fence.h"
#include "libGLESv2/renderer/Renderer.h"

namespace gl
{
ResourceManager::ResourceManager(rx::Renderer *renderer)
    : mRenderer(renderer),
      mRefCount(1)
{
}

ResourceManager::~ResourceManager()
{
    while (!mBufferMap.empty())
    {
        deleteBuffer(mBufferMap.begin()->first);
    }

    while (!mProgramMap.empty())
    {
        deleteProgram(mProgramMap.begin()->first);
    }

    while (!mShaderMap.empty())
    {
        deleteShader(mShaderMap.begin()->first);
    }

    while (!mRenderbufferMap.empty())
    {
        deleteRenderbuffer(mRenderbufferMap.begin()->first);
    }

    while (!mTextureMap.empty())
    {
        deleteTexture(mTextureMap.begin()->first);
    }

    while (!mSamplerMap.empty())
    {
        deleteSampler(mSamplerMap.begin()->first);
    }

    while (!mFenceSyncMap.empty())
    {
        deleteFenceSync(mFenceSyncMap.begin()->first);
    }
}

void ResourceManager::addRef()
{
    mRefCount++;
}

void ResourceManager::release()
{
    if (--mRefCount == 0)
    {
        delete this;
    }
}

// Returns an unused buffer name
GLuint ResourceManager::createBuffer()
{
    GLuint handle = mBufferHandleAllocator.allocate();

    mBufferMap[handle] = NULL;

    return handle;
}

// Returns an unused shader/program name
GLuint ResourceManager::createShader(const gl::Data &data, GLenum type)
{
    GLuint handle = mProgramShaderHandleAllocator.allocate();

    if (type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER)
    {
        mShaderMap[handle] = new Shader(this, mRenderer->createShader(data, type), type, handle);
    }
    else UNREACHABLE();

    return handle;
}

// Returns an unused program/shader name
GLuint ResourceManager::createProgram()
{
    GLuint handle = mProgramShaderHandleAllocator.allocate();

    mProgramMap[handle] = new Program(mRenderer, this, handle);

    return handle;
}

// Returns an unused texture name
GLuint ResourceManager::createTexture()
{
    GLuint handle = mTextureHandleAllocator.allocate();

    mTextureMap[handle] = NULL;

    return handle;
}

// Returns an unused renderbuffer name
GLuint ResourceManager::createRenderbuffer()
{
    GLuint handle = mRenderbufferHandleAllocator.allocate();

    mRenderbufferMap[handle] = NULL;

    return handle;
}

// Returns an unused sampler name
GLuint ResourceManager::createSampler()
{
    GLuint handle = mSamplerHandleAllocator.allocate();

    mSamplerMap[handle] = NULL;

    return handle;
}

// Returns the next unused fence name, and allocates the fence
GLuint ResourceManager::createFenceSync()
{
    GLuint handle = mFenceSyncHandleAllocator.allocate();

    FenceSync *fenceSync = new FenceSync(mRenderer->createFenceSync(), handle);
    fenceSync->addRef();
    mFenceSyncMap[handle] = fenceSync;

    return handle;
}

void ResourceManager::deleteBuffer(GLuint buffer)
{
    BufferMap::iterator bufferObject = mBufferMap.find(buffer);

    if (bufferObject != mBufferMap.end())
    {
        mBufferHandleAllocator.release(bufferObject->first);
        if (bufferObject->second) bufferObject->second->release();
        mBufferMap.erase(bufferObject);
    }
}

void ResourceManager::deleteShader(GLuint shader)
{
    ShaderMap::iterator shaderObject = mShaderMap.find(shader);

    if (shaderObject != mShaderMap.end())
    {
        if (shaderObject->second->getRefCount() == 0)
        {
            mProgramShaderHandleAllocator.release(shaderObject->first);
            delete shaderObject->second;
            mShaderMap.erase(shaderObject);
        }
        else
        {
            shaderObject->second->flagForDeletion();
        }
    }
}

void ResourceManager::deleteProgram(GLuint program)
{
    ProgramMap::iterator programObject = mProgramMap.find(program);

    if (programObject != mProgramMap.end())
    {
        if (programObject->second->getRefCount() == 0)
        {
            mProgramShaderHandleAllocator.release(programObject->first);
            delete programObject->second;
            mProgramMap.erase(programObject);
        }
        else
        { 
            programObject->second->flagForDeletion();
        }
    }
}

void ResourceManager::deleteTexture(GLuint texture)
{
    TextureMap::iterator textureObject = mTextureMap.find(texture);

    if (textureObject != mTextureMap.end())
    {
        mTextureHandleAllocator.release(textureObject->first);
        if (textureObject->second) textureObject->second->release();
        mTextureMap.erase(textureObject);
    }
}

void ResourceManager::deleteRenderbuffer(GLuint renderbuffer)
{
    RenderbufferMap::iterator renderbufferObject = mRenderbufferMap.find(renderbuffer);

    if (renderbufferObject != mRenderbufferMap.end())
    {
        mRenderbufferHandleAllocator.release(renderbufferObject->first);
        if (renderbufferObject->second) renderbufferObject->second->release();
        mRenderbufferMap.erase(renderbufferObject);
    }
}

void ResourceManager::deleteSampler(GLuint sampler)
{
    auto samplerObject = mSamplerMap.find(sampler);

    if (samplerObject != mSamplerMap.end())
    {
        mSamplerHandleAllocator.release(samplerObject->first);
        if (samplerObject->second) samplerObject->second->release();
        mSamplerMap.erase(samplerObject);
    }
}

void ResourceManager::deleteFenceSync(GLuint fenceSync)
{
    auto fenceObjectIt = mFenceSyncMap.find(fenceSync);

    if (fenceObjectIt != mFenceSyncMap.end())
    {
        mFenceSyncHandleAllocator.release(fenceObjectIt->first);
        if (fenceObjectIt->second) fenceObjectIt->second->release();
        mFenceSyncMap.erase(fenceObjectIt);
    }
}

Buffer *ResourceManager::getBuffer(unsigned int handle)
{
    BufferMap::iterator buffer = mBufferMap.find(handle);

    if (buffer == mBufferMap.end())
    {
        return NULL;
    }
    else
    {
        return buffer->second;
    }
}

Shader *ResourceManager::getShader(unsigned int handle)
{
    ShaderMap::iterator shader = mShaderMap.find(handle);

    if (shader == mShaderMap.end())
    {
        return NULL;
    }
    else
    {
        return shader->second;
    }
}

Texture *ResourceManager::getTexture(unsigned int handle)
{
    if (handle == 0) return NULL;

    TextureMap::iterator texture = mTextureMap.find(handle);

    if (texture == mTextureMap.end())
    {
        return NULL;
    }
    else
    {
        return texture->second;
    }
}

Program *ResourceManager::getProgram(unsigned int handle) const
{
    ProgramMap::const_iterator program = mProgramMap.find(handle);

    if (program == mProgramMap.end())
    {
        return NULL;
    }
    else
    {
        return program->second;
    }
}

Renderbuffer *ResourceManager::getRenderbuffer(unsigned int handle)
{
    RenderbufferMap::iterator renderbuffer = mRenderbufferMap.find(handle);

    if (renderbuffer == mRenderbufferMap.end())
    {
        return NULL;
    }
    else
    {
        return renderbuffer->second;
    }
}

Sampler *ResourceManager::getSampler(unsigned int handle)
{
    auto sampler = mSamplerMap.find(handle);

    if (sampler == mSamplerMap.end())
    {
        return NULL;
    }
    else
    {
        return sampler->second;
    }
}

FenceSync *ResourceManager::getFenceSync(unsigned int handle)
{
    auto fenceObjectIt = mFenceSyncMap.find(handle);

    if (fenceObjectIt == mFenceSyncMap.end())
    {
        return NULL;
    }
    else
    {
        return fenceObjectIt->second;
    }
}

void ResourceManager::setRenderbuffer(GLuint handle, Renderbuffer *buffer)
{
    mRenderbufferMap[handle] = buffer;
}

void ResourceManager::checkBufferAllocation(unsigned int buffer)
{
    if (buffer != 0 && !getBuffer(buffer))
    {
        Buffer *bufferObject = new Buffer(mRenderer->createBuffer(), buffer);
        mBufferMap[buffer] = bufferObject;
        bufferObject->addRef();
    }
}

void ResourceManager::checkTextureAllocation(GLuint texture, GLenum type)
{
    if (!getTexture(texture) && texture != 0)
    {
        Texture *textureObject;

        if (type == GL_TEXTURE_2D)
        {
            textureObject = new Texture2D(mRenderer->createTexture(GL_TEXTURE_2D), texture);
        }
        else if (type == GL_TEXTURE_CUBE_MAP)
        {
            textureObject = new TextureCubeMap(mRenderer->createTexture(GL_TEXTURE_CUBE_MAP), texture);
        }
        else if (type == GL_TEXTURE_3D)
        {
            textureObject = new Texture3D(mRenderer->createTexture(GL_TEXTURE_3D), texture);
        }
        else if (type == GL_TEXTURE_2D_ARRAY)
        {
            textureObject = new Texture2DArray(mRenderer->createTexture(GL_TEXTURE_2D_ARRAY), texture);
        }
        else
        {
            UNREACHABLE();
            return;
        }

        mTextureMap[texture] = textureObject;
        textureObject->addRef();
    }
}

void ResourceManager::checkRenderbufferAllocation(GLuint renderbuffer)
{
    if (renderbuffer != 0 && !getRenderbuffer(renderbuffer))
    {
        Renderbuffer *renderbufferObject = new Renderbuffer(mRenderer->createRenderbuffer(), renderbuffer);
        mRenderbufferMap[renderbuffer] = renderbufferObject;
        renderbufferObject->addRef();
    }
}

void ResourceManager::checkSamplerAllocation(GLuint sampler)
{
    if (sampler != 0 && !getSampler(sampler))
    {
        Sampler *samplerObject = new Sampler(sampler);
        mSamplerMap[sampler] = samplerObject;
        samplerObject->addRef();
    }
}

bool ResourceManager::isSampler(GLuint sampler)
{
    return mSamplerMap.find(sampler) != mSamplerMap.end();
}

}
