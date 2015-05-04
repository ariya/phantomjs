//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Query11.cpp: Defines the rx::Query11 class which implements rx::QueryImpl.

#include "libGLESv2/renderer/d3d/d3d11/Query11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/main.h"
#include "common/utilities.h"

#include <GLES2/gl2ext.h>

namespace rx
{

Query11::Query11(Renderer11 *renderer, GLenum type)
    : QueryImpl(type),
      mResult(0),
      mQueryFinished(false),
      mRenderer(renderer),
      mQuery(NULL)
{
}

Query11::~Query11()
{
    SafeRelease(mQuery);
}

gl::Error Query11::begin()
{
    if (mQuery == NULL)
    {
        D3D11_QUERY_DESC queryDesc;
        queryDesc.Query = gl_d3d11::ConvertQueryType(getType());
        queryDesc.MiscFlags = 0;

        HRESULT result = mRenderer->getDevice()->CreateQuery(&queryDesc, &mQuery);
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Internal query creation failed, result: 0x%X.", result);
        }
    }

    mRenderer->getDeviceContext()->Begin(mQuery);
    return gl::Error(GL_NO_ERROR);
}

gl::Error Query11::end()
{
    ASSERT(mQuery);
    mRenderer->getDeviceContext()->End(mQuery);

    mQueryFinished = false;
    mResult = GL_FALSE;

    return gl::Error(GL_NO_ERROR);
}

gl::Error Query11::getResult(GLuint *params)
{
    while (!mQueryFinished)
    {
        gl::Error error = testQuery();
        if (error.isError())
        {
            return error;
        }

        if (!mQueryFinished)
        {
            Sleep(0);
        }
    }

    ASSERT(mQueryFinished);
    *params = mResult;

    return gl::Error(GL_NO_ERROR);
}

gl::Error Query11::isResultAvailable(GLuint *available)
{
    gl::Error error = testQuery();
    if (error.isError())
    {
        return error;
    }

    *available = (mQueryFinished ? GL_TRUE : GL_FALSE);

    return gl::Error(GL_NO_ERROR);
}

gl::Error Query11::testQuery()
{
    if (!mQueryFinished)
    {
        ASSERT(mQuery);

        ID3D11DeviceContext *context = mRenderer->getDeviceContext();
        switch (getType())
        {
          case GL_ANY_SAMPLES_PASSED_EXT:
          case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
            {
                UINT64 numPixels = 0;
                HRESULT result = context->GetData(mQuery, &numPixels, sizeof(numPixels), 0);
                if (FAILED(result))
                {
                    return gl::Error(GL_OUT_OF_MEMORY, "Failed to get the data of an internal query, result: 0x%X.", result);
                }

                if (result == S_OK)
                {
                    mQueryFinished = true;
                    mResult = (numPixels > 0) ? GL_TRUE : GL_FALSE;
                }
            }
            break;

          case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
            {
                D3D11_QUERY_DATA_SO_STATISTICS soStats = { 0 };
                HRESULT result = context->GetData(mQuery, &soStats, sizeof(soStats), 0);
                if (FAILED(result))
                {
                    return gl::Error(GL_OUT_OF_MEMORY, "Failed to get the data of an internal query, result: 0x%X.", result);
                }

                if (result == S_OK)
                {
                    mQueryFinished = true;
                    mResult = static_cast<GLuint>(soStats.NumPrimitivesWritten);
                }
            }
            break;

        default:
            UNREACHABLE();
            break;
        }

        if (!mQueryFinished && mRenderer->testDeviceLost(true))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to test get query result, device is lost.");
        }
    }

    return gl::Error(GL_NO_ERROR);
}

}
