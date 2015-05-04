//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Query.h: Defines the gl::Query class

#ifndef LIBGLESV2_QUERY_H_
#define LIBGLESV2_QUERY_H_

#include "libGLESv2/Error.h"
#include "common/angleutils.h"
#include "common/RefCountObject.h"

#include "angle_gl.h"

namespace rx
{
class QueryImpl;
}

namespace gl
{

class Query : public RefCountObject
{
  public:
    Query(rx::QueryImpl *impl, GLuint id);
    virtual ~Query();

    Error begin();
    Error end();

    Error getResult(GLuint *params);
    Error isResultAvailable(GLuint *available);

    GLenum getType() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Query);

    rx::QueryImpl *mQuery;
};

}

#endif   // LIBGLESV2_QUERY_H_
