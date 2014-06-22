#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Query11.cpp: Defines the rx::Query11 class which implements rx::QueryImpl.

#include "libGLESv2/renderer/d3d11/Query11.h"
#include "libGLESv2/renderer/d3d11/Renderer11.h"
#include "libGLESv2/main.h"

namespace rx
{

Query11::Query11(rx::Renderer11 *renderer, GLenum type) : QueryImpl(type)
{
    mRenderer = renderer;
    mQuery = NULL;
}

Query11::~Query11()
{
    if (mQuery)
    {
        mQuery->Release();
        mQuery = NULL;
    }
}

void Query11::begin()
{
    if (mQuery == NULL)
    {
        D3D11_QUERY_DESC queryDesc;
        queryDesc.Query = D3D11_QUERY_OCCLUSION;
        queryDesc.MiscFlags = 0;

        if (FAILED(mRenderer->getDevice()->CreateQuery(&queryDesc, &mQuery)))
        {
            return gl::error(GL_OUT_OF_MEMORY);
        }
    }

    mRenderer->getDeviceContext()->Begin(mQuery);
}

void Query11::end()
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    mRenderer->getDeviceContext()->End(mQuery);

    mStatus = GL_FALSE;
    mResult = GL_FALSE;
}

GLuint Query11::getResult()
{
    if (mQuery != NULL)
    {
        while (!testQuery())
        {
            Sleep(0);
            // explicitly check for device loss, some drivers seem to return S_FALSE
            // if the device is lost
            if (mRenderer->testDeviceLost(true))
            {
                return gl::error(GL_OUT_OF_MEMORY, 0);
            }
        }
    }

    return mResult;
}

GLboolean Query11::isResultAvailable()
{
    if (mQuery != NULL)
    {
        testQuery();
    }

    return mStatus;
}

GLboolean Query11::testQuery()
{
    if (mQuery != NULL && mStatus != GL_TRUE)
    {
        UINT64 numPixels = 0;
        HRESULT result = mRenderer->getDeviceContext()->GetData(mQuery, &numPixels, sizeof(UINT64), 0);
        if (result == S_OK)
        {
            mStatus = GL_TRUE;

            switch (getType())
            {
              case GL_ANY_SAMPLES_PASSED_EXT:
              case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
                mResult = (numPixels > 0) ? GL_TRUE : GL_FALSE;
                break;
              default:
                UNREACHABLE();
            }
        }
        else if (mRenderer->testDeviceLost(true))
        {
            return gl::error(GL_OUT_OF_MEMORY, GL_TRUE);
        }

        return mStatus;
    }

    return GL_TRUE; // prevent blocking when query is null
}

}
