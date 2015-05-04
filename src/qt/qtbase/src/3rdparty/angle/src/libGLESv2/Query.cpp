//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Query.cpp: Implements the gl::Query class

#include "libGLESv2/Query.h"
#include "libGLESv2/renderer/QueryImpl.h"

namespace gl
{
Query::Query(rx::QueryImpl *impl, GLuint id)
    : RefCountObject(id),
      mQuery(impl)
{
}

Query::~Query()
{
    SafeDelete(mQuery);
}

Error Query::begin()
{
    return mQuery->begin();
}

Error Query::end()
{
    return mQuery->end();
}

Error Query::getResult(GLuint *params)
{
    return mQuery->getResult(params);
}

Error Query::isResultAvailable(GLuint *available)
{
    return mQuery->isResultAvailable(available);
}

GLenum Query::getType() const
{
    return mQuery->getType();
}

}
