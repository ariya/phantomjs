//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Data.cpp: Container class for all GL relevant state, caps and objects

#include "libGLESv2/Data.h"
#include "libGLESv2/ResourceManager.h"

namespace gl
{

Data::Data(GLint clientVersionIn, const State &stateIn, const Caps &capsIn,
           const TextureCapsMap &textureCapsIn, const Extensions &extensionsIn,
           const ResourceManager *resourceManagerIn)
    : clientVersion(clientVersionIn),
      state(&stateIn),
      caps(&capsIn),
      textureCaps(&textureCapsIn),
      extensions(&extensionsIn),
      resourceManager(resourceManagerIn)
{}

Data::~Data()
{
}

Data::Data(const Data &other)
    : clientVersion(other.clientVersion),
      state(other.state),
      caps(other.caps),
      textureCaps(other.textureCaps),
      extensions(other.extensions),
      resourceManager(other.resourceManager)
{
}

Data &Data::operator=(const Data &other)
{
    clientVersion = other.clientVersion;
    state = other.state;
    caps = other.caps;
    textureCaps = other.textureCaps;
    extensions = other.extensions;
    resourceManager = other.resourceManager;
    return *this;
}

}
