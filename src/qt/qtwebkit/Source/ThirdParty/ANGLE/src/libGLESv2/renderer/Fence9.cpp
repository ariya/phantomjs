#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence9.cpp: Defines the rx::Fence9 class.

#include "libGLESv2/renderer/Fence9.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/renderer9_utils.h"
#include "libGLESv2/renderer/Renderer9.h"

namespace rx
{

Fence9::Fence9(rx::Renderer9 *renderer)
{
    mRenderer = renderer;
    mQuery = NULL;
}

Fence9::~Fence9()
{
    if (mQuery)
    {
        mRenderer->freeEventQuery(mQuery);
        mQuery = NULL;
    }
}

GLboolean Fence9::isFence()
{
    // GL_NV_fence spec:
    // A name returned by GenFencesNV, but not yet set via SetFenceNV, is not the name of an existing fence.
    return mQuery != NULL;
}

void Fence9::setFence(GLenum condition)
{
    if (!mQuery)
    {
        mQuery = mRenderer->allocateEventQuery();
        if (!mQuery)
        {
            return gl::error(GL_OUT_OF_MEMORY);
        }
    }

    HRESULT result = mQuery->Issue(D3DISSUE_END);
    ASSERT(SUCCEEDED(result));

    setCondition(condition);
    setStatus(GL_FALSE);
}

GLboolean Fence9::testFence()
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION, GL_TRUE);
    }

    HRESULT result = mQuery->GetData(NULL, 0, D3DGETDATA_FLUSH);

    if (d3d9::isDeviceLostError(result))
    {
        mRenderer->notifyDeviceLost();
        return gl::error(GL_OUT_OF_MEMORY, GL_TRUE);
    }

    ASSERT(result == S_OK || result == S_FALSE);
    setStatus(result == S_OK);
    return getStatus();
}

void Fence9::finishFence()
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    while (!testFence())
    {
        Sleep(0);
    }
}

void Fence9::getFenceiv(GLenum pname, GLint *params)
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    switch (pname)
    {
        case GL_FENCE_STATUS_NV:
        {
            // GL_NV_fence spec:
            // Once the status of a fence has been finished (via FinishFenceNV) or tested and the returned status is TRUE (via either TestFenceNV
            // or GetFenceivNV querying the FENCE_STATUS_NV), the status remains TRUE until the next SetFenceNV of the fence.
            if (getStatus())
            {
                params[0] = GL_TRUE;
                return;
            }
            
            HRESULT result = mQuery->GetData(NULL, 0, 0);

            if (d3d9::isDeviceLostError(result))
            {
                params[0] = GL_TRUE;
                mRenderer->notifyDeviceLost();
                return gl::error(GL_OUT_OF_MEMORY);
            }

            ASSERT(result == S_OK || result == S_FALSE);
            setStatus(result == S_OK);
            params[0] = getStatus();

            break;
        }
        case GL_FENCE_CONDITION_NV:
            params[0] = getCondition();
            break;
        default:
            return gl::error(GL_INVALID_ENUM);
            break;
    }
}

}
