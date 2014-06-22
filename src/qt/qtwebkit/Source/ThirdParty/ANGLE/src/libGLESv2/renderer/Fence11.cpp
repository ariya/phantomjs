#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence11.cpp: Defines the rx::Fence11 class which implements rx::FenceImpl.

#include "libGLESv2/renderer/Fence11.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/Renderer11.h"

namespace rx
{

Fence11::Fence11(rx::Renderer11 *renderer)
{
    mRenderer = renderer;
    mQuery = NULL;
}

Fence11::~Fence11()
{
    if (mQuery)
    {
        mQuery->Release();
        mQuery = NULL;
    }
}

GLboolean Fence11::isFence()
{
    // GL_NV_fence spec:
    // A name returned by GenFencesNV, but not yet set via SetFenceNV, is not the name of an existing fence.
    return mQuery != NULL;
}

void Fence11::setFence(GLenum condition)
{
    if (!mQuery)
    {
        D3D11_QUERY_DESC queryDesc;
        queryDesc.Query = D3D11_QUERY_EVENT;
        queryDesc.MiscFlags = 0;

        if (FAILED(mRenderer->getDevice()->CreateQuery(&queryDesc, &mQuery)))
        {
            return gl::error(GL_OUT_OF_MEMORY);
        }
    }

    mRenderer->getDeviceContext()->End(mQuery);

    setCondition(condition);
    setStatus(GL_FALSE);
}

GLboolean Fence11::testFence()
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION, GL_TRUE);
    }

    HRESULT result = mRenderer->getDeviceContext()->GetData(mQuery, NULL, 0, 0);

    if (mRenderer->isDeviceLost())
    {
       return gl::error(GL_OUT_OF_MEMORY, GL_TRUE);
    }

    ASSERT(result == S_OK || result == S_FALSE);
    setStatus(result == S_OK);
    return getStatus();
}

void Fence11::finishFence()
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

void Fence11::getFenceiv(GLenum pname, GLint *params)
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

            HRESULT result = mRenderer->getDeviceContext()->GetData(mQuery, NULL, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH);

            if (mRenderer->isDeviceLost())
            {
                params[0] = GL_TRUE;
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
