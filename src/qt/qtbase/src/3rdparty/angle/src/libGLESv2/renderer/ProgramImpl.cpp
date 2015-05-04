//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramD3D.cpp: Defines the rx::ProgramD3D class which implements rx::ProgramImpl.

#include "libGLESv2/renderer/ProgramImpl.h"

#include "common/utilities.h"
#include "libGLESv2/main.h"

namespace rx
{

namespace
{

unsigned int ParseAndStripArrayIndex(std::string* name)
{
    unsigned int subscript = GL_INVALID_INDEX;

    // Strip any trailing array operator and retrieve the subscript
    size_t open = name->find_last_of('[');
    size_t close = name->find_last_of(']');
    if (open != std::string::npos && close == name->length() - 1)
    {
        subscript = atoi(name->substr(open + 1).c_str());
        name->erase(open);
    }

    return subscript;
}

}

ProgramImpl::~ProgramImpl()
{
    // Ensure that reset was called by the inherited class during destruction
    ASSERT(mUniformIndex.size() == 0);
}

gl::LinkedUniform *ProgramImpl::getUniformByLocation(GLint location) const
{
    ASSERT(location >= 0 && static_cast<size_t>(location) < mUniformIndex.size());
    return mUniforms[mUniformIndex[location].index];
}

gl::LinkedUniform *ProgramImpl::getUniformByName(const std::string &name) const
{
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        if (mUniforms[uniformIndex]->name == name)
        {
            return mUniforms[uniformIndex];
        }
    }

    return NULL;
}

gl::UniformBlock *ProgramImpl::getUniformBlockByIndex(GLuint blockIndex) const
{
    ASSERT(blockIndex < mUniformBlocks.size());
    return mUniformBlocks[blockIndex];
}

GLint ProgramImpl::getUniformLocation(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    unsigned int numUniforms = mUniformIndex.size();
    for (unsigned int location = 0; location < numUniforms; location++)
    {
        if (mUniformIndex[location].name == name)
        {
            const int index = mUniformIndex[location].index;
            const bool isArray = mUniforms[index]->isArray();

            if ((isArray && mUniformIndex[location].element == subscript) ||
                (subscript == GL_INVALID_INDEX))
            {
                return location;
            }
        }
    }

    return -1;
}

GLuint ProgramImpl::getUniformIndex(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    // The app is not allowed to specify array indices other than 0 for arrays of basic types
    if (subscript != 0 && subscript != GL_INVALID_INDEX)
    {
        return GL_INVALID_INDEX;
    }

    unsigned int numUniforms = mUniforms.size();
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        if (mUniforms[index]->name == name)
        {
            if (mUniforms[index]->isArray() || subscript == GL_INVALID_INDEX)
            {
                return index;
            }
        }
    }

    return GL_INVALID_INDEX;
}

GLuint ProgramImpl::getUniformBlockIndex(std::string name) const
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    unsigned int numUniformBlocks = mUniformBlocks.size();
    for (unsigned int blockIndex = 0; blockIndex < numUniformBlocks; blockIndex++)
    {
        const gl::UniformBlock &uniformBlock = *mUniformBlocks[blockIndex];
        if (uniformBlock.name == name)
        {
            const bool arrayElementZero = (subscript == GL_INVALID_INDEX && uniformBlock.elementIndex == 0);
            if (subscript == uniformBlock.elementIndex || arrayElementZero)
            {
                return blockIndex;
            }
        }
    }

    return GL_INVALID_INDEX;
}

void ProgramImpl::reset()
{
    SafeDeleteContainer(mUniforms);
    mUniformIndex.clear();
    SafeDeleteContainer(mUniformBlocks);
    mTransformFeedbackLinkedVaryings.clear();
}

}
