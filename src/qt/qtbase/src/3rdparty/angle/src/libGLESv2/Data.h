//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Data.h: Container class for all GL relevant state, caps and objects

#ifndef LIBGLESV2_DATA_H_
#define LIBGLESV2_DATA_H_

#include "libGLESv2/State.h"

namespace gl
{

struct Data
{
  public:
    Data(GLint clientVersion, const State &state, const Caps &caps,
         const TextureCapsMap &textureCaps, const Extensions &extensions,
         const ResourceManager *resourceManager);
    ~Data();

    Data(const Data &other);
    Data &operator=(const Data &other);

    GLint clientVersion;
    const State *state;
    const Caps *caps;
    const TextureCapsMap *textureCaps;
    const Extensions *extensions;
    const ResourceManager *resourceManager;
};

}

#endif // LIBGLESV2_DATA_H_
