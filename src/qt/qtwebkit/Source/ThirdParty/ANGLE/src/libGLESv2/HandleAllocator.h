//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// HandleAllocator.h: Defines the gl::HandleAllocator class, which is used to
// allocate GL handles.

#ifndef LIBGLESV2_HANDLEALLOCATOR_H_
#define LIBGLESV2_HANDLEALLOCATOR_H_

#define GL_APICALL
#include <GLES2/gl2.h>

#include <vector>

#include "common/angleutils.h"

namespace gl
{

class HandleAllocator
{
  public:
    HandleAllocator();
    virtual ~HandleAllocator();

    void setBaseHandle(GLuint value);

    GLuint allocate();
    void release(GLuint handle);

  private:
    DISALLOW_COPY_AND_ASSIGN(HandleAllocator);

    GLuint mBaseValue;
    GLuint mNextValue;
    typedef std::vector<GLuint> HandleList;
    HandleList mFreeValues;
};

}

#endif   // LIBGLESV2_HANDLEALLOCATOR_H_
