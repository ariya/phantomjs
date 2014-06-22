//
// Copyright (c) 2010-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBGLESV2_UNIFORM_H_
#define LIBGLESV2_UNIFORM_H_

#include <string>
#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/debug.h"

namespace gl
{

// Helper struct representing a single shader uniform
struct Uniform
{
    Uniform(GLenum type, GLenum precision, const std::string &name, unsigned int arraySize);

    ~Uniform();

    bool isArray() const;
    unsigned int elementCount() const;

    const GLenum type;
    const GLenum precision;
    const std::string name;
    const unsigned int arraySize;

    unsigned char *data;
    bool dirty;

    int psRegisterIndex;
    int vsRegisterIndex;
    unsigned int registerCount;
};

typedef std::vector<Uniform*> UniformArray;

}

#endif   // LIBGLESV2_UNIFORM_H_
