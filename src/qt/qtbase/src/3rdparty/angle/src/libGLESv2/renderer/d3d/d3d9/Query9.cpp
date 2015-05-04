//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Query9.cpp: Defines the rx::Query9 class which implements rx::QueryImpl.

#include "libGLESv2/renderer/d3d/d3d9/Query9.h"
#include "libGLESv2/renderer/d3d/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/main.h"

#include <GLES2/gl2ext.h>

namespace rx
{
Query9::Query9(Renderer9 *renderer, GLenum type)
    : QueryImpl(type),
      mResult(GL_FALSE),
      mQueryFinished(false),
      mRenderer(renderer),
      mQuery(NULL)
{
}

Query9::~Query9()
{
    SafeRelease(mQuery);
}

gl::Error Query9::begin()
{
    if (mQuery == NULL)
    {
        HRESULT result = mRenderer->getDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, &mQuery);
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Internal query creation failed, result: 0x%X.", result);
        }
    }

    HRESULT result = mQuery->Issue(D3DISSUE_BEGIN);
    ASSERT(SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to begin internal query, result: 0x%X.", result);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Query9::end()
{
    ASSERT(mQuery);

    HRESULT result = mQuery->Issue(D3DISSUE_END);
    ASSERT(SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to end internal query, result: 0x%X.", result);
    }

    mQueryFinished = false;
    mResult = GL_FALSE;

    return gl::Error(GL_NO_ERROR);
}

gl::Error Query9::getResult(GLuint *params)
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

gl::Error Query9::isResultAvailable(GLuint *available)
{
    gl::Error error = testQuery();
    if (error.isError())
    {
        return error;
    }

    *available = (mQueryFinished ? GL_TRUE : GL_FALSE);

    return gl::Error(GL_NO_ERROR);
}

gl::Error Query9::testQuery()
{
    if (!mQueryFinished)
    {
        ASSERT(mQuery);

        DWORD numPixels = 0;

        HRESULT hres = mQuery->GetData(&numPixels, sizeof(DWORD), D3DGETDATA_FLUSH);
        if (hres == S_OK)
        {
            mQueryFinished = true;

            switch (getType())
            {
              case GL_ANY_SAMPLES_PASSED_EXT:
              case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
                mResult = (numPixels > 0) ? GL_TRUE : GL_FALSE;
                break;

              default:
                UNREACHABLE();
                break;
            }
        }
        else if (d3d9::isDeviceLostError(hres))
        {
            mRenderer->notifyDeviceLost();
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to test get query result, device is lost.");
        }
        else if (mRenderer->testDeviceLost(true))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to test get query result, device is lost.");
        }
    }

    return gl::Error(GL_NO_ERROR);
}

}
