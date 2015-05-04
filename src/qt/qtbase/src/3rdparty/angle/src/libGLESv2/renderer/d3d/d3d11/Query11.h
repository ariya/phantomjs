//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Query11.h: Defines the rx::Query11 class which implements rx::QueryImpl.

#ifndef LIBGLESV2_RENDERER_QUERY11_H_
#define LIBGLESV2_RENDERER_QUERY11_H_

#include "libGLESv2/renderer/QueryImpl.h"

namespace rx
{
class Renderer11;

class Query11 : public QueryImpl
{
  public:
    Query11(Renderer11 *renderer, GLenum type);
    virtual ~Query11();

    virtual gl::Error begin();
    virtual gl::Error end();
    virtual gl::Error getResult(GLuint *params);
    virtual gl::Error isResultAvailable(GLuint *available);

  private:
    DISALLOW_COPY_AND_ASSIGN(Query11);

    gl::Error testQuery();

    GLuint mResult;

    bool mQueryFinished;

    Renderer11 *mRenderer;
    ID3D11Query *mQuery;
};

}

#endif // LIBGLESV2_RENDERER_QUERY11_H_
