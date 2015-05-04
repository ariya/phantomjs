//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderImpl.h: Defines the abstract rx::ShaderImpl class.

#ifndef LIBGLESV2_RENDERER_SHADERIMPL_H_
#define LIBGLESV2_RENDERER_SHADERIMPL_H_

#include <vector>

#include "common/angleutils.h"
#include "libGLESv2/Shader.h"

namespace rx
{

class ShaderImpl
{
  public:
    ShaderImpl() { }
    virtual ~ShaderImpl() { }

    virtual bool compile(const gl::Data &data, const std::string &source) = 0;
    virtual const std::string &getInfoLog() const = 0;
    virtual const std::string &getTranslatedSource() const = 0;
    virtual std::string getDebugInfo() const = 0;

    const std::vector<gl::PackedVarying> &getVaryings() const { return mVaryings; }
    const std::vector<sh::Uniform> &getUniforms() const { return mUniforms; }
    const std::vector<sh::InterfaceBlock> &getInterfaceBlocks() const  { return mInterfaceBlocks; }
    const std::vector<sh::Attribute> &getActiveAttributes() const { return mActiveAttributes; }
    const std::vector<sh::Attribute> &getActiveOutputVariables() const { return mActiveOutputVariables; }

    std::vector<gl::PackedVarying> &getVaryings() { return mVaryings; }
    std::vector<sh::Uniform> &getUniforms() { return mUniforms; }
    std::vector<sh::InterfaceBlock> &getInterfaceBlocks() { return mInterfaceBlocks; }
    std::vector<sh::Attribute> &getActiveAttributes() { return mActiveAttributes; }
    std::vector<sh::Attribute> &getActiveOutputVariables() { return mActiveOutputVariables; }

  protected:
    DISALLOW_COPY_AND_ASSIGN(ShaderImpl);

    std::vector<gl::PackedVarying> mVaryings;
    std::vector<sh::Uniform> mUniforms;
    std::vector<sh::InterfaceBlock> mInterfaceBlocks;
    std::vector<sh::Attribute> mActiveAttributes;
    std::vector<sh::Attribute> mActiveOutputVariables;
};

}

#endif // LIBGLESV2_RENDERER_SHADERIMPL_H_
