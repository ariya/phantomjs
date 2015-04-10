//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_VERSIONGLSL_H_
#define COMPILER_VERSIONGLSL_H_

#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/intermediate.h"

// Traverses the intermediate tree to return the minimum GLSL version
// required to legally access all built-in features used in the shader.
// GLSL 1.1 which is mandated by OpenGL 2.0 provides:
//   - #version and #extension to declare version and extensions.
//   - built-in functions refract, exp, and log.
//   - updated step() to compare x < edge instead of x <= edge.
// GLSL 1.2 which is mandated by OpenGL 2.1 provides:
//   - many changes to reduce differences when compared to the ES specification.
//   - invariant keyword and its support.
//   - c++ style name hiding rules.
//   - built-in variable gl_PointCoord for fragment shaders.
//   - matrix constructors taking matrix as argument.
//   - array as "out" function parameters
//
class TVersionGLSL : public TIntermTraverser {
public:
    TVersionGLSL(ShShaderType type);

    // Returns 120 if the following is used the shader:
    // - "invariant",
    // - "gl_PointCoord",
    // - matrix/matrix constructors
    // - array "out" parameters
    // Else 110 is returned.
    int getVersion() { return mVersion; }

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitSelection(Visit, TIntermSelection*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);
    virtual bool visitBranch(Visit, TIntermBranch*);

protected:
    void updateVersion(int version);

private:
    ShShaderType mShaderType;
    int mVersion;
};

#endif  // COMPILER_VERSIONGLSL_H_
