//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderD3D.h: Defines the rx::ShaderD3D class which implements rx::ShaderImpl.

#ifndef LIBGLESV2_RENDERER_SHADERD3D_H_
#define LIBGLESV2_RENDERER_SHADERD3D_H_

#include "libGLESv2/renderer/ShaderImpl.h"
#include "libGLESv2/renderer/Workarounds.h"
#include "libGLESv2/Shader.h"

#include <map>

namespace rx
{
class DynamicHLSL;
class RendererD3D;

class ShaderD3D : public ShaderImpl
{
    friend class DynamicHLSL;

  public:
    ShaderD3D(const gl::Data &data, GLenum type, RendererD3D *renderer);
    virtual ~ShaderD3D();

    static ShaderD3D *makeShaderD3D(ShaderImpl *impl);
    static const ShaderD3D *makeShaderD3D(const ShaderImpl *impl);

    // ShaderImpl implementation
    virtual const std::string &getInfoLog() const { return mInfoLog; }
    virtual const std::string &getTranslatedSource() const { return mHlsl; }
    virtual std::string getDebugInfo() const;

    // D3D-specific methods
    virtual void uncompile();
    void resetVaryingsRegisterAssignment();
    unsigned int getUniformRegister(const std::string &uniformName) const;
    unsigned int getInterfaceBlockRegister(const std::string &blockName) const;
    int getSemanticIndex(const std::string &attributeName) const;
    void appendDebugInfo(const std::string &info) { mDebugInfo += info; }

    D3DWorkaroundType getD3DWorkarounds() const;
    int getShaderVersion() const { return mShaderVersion; }
    bool usesDepthRange() const { return mUsesDepthRange; }
    bool usesPointSize() const { return mUsesPointSize; }

    static void releaseCompiler();
    static ShShaderOutput getCompilerOutputType(GLenum shader);

    virtual bool compile(const gl::Data &data, const std::string &source);

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderD3D);

    void compileToHLSL(const gl::Data &data, void *compiler, const std::string &source);
    void parseVaryings(void *compiler);

    void initializeCompiler(const gl::Data &data);
    void parseAttributes(void *compiler);
    void *getCompiler();

    static bool compareVarying(const gl::PackedVarying &x, const gl::PackedVarying &y);

    static void *mFragmentCompiler;
    static void *mVertexCompiler;

    GLenum mType;
    RendererD3D *mRenderer;

    int mShaderVersion;

    bool mUsesMultipleRenderTargets;
    bool mUsesFragColor;
    bool mUsesFragData;
    bool mUsesFragCoord;
    bool mUsesFrontFacing;
    bool mUsesPointSize;
    bool mUsesPointCoord;
    bool mUsesDepthRange;
    bool mUsesFragDepth;
    bool mUsesDiscardRewriting;
    bool mUsesNestedBreak;

    std::string mHlsl;
    std::string mInfoLog;
    std::string mDebugInfo;
    std::map<std::string, unsigned int> mUniformRegisterMap;
    std::map<std::string, unsigned int> mInterfaceBlockRegisterMap;
};

}

#endif // LIBGLESV2_RENDERER_SHADERD3D_H_
