//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_VERSIONGLSL_H_
#define COMPILER_TRANSLATOR_VERSIONGLSL_H_

#include "compiler/translator/IntermNode.h"

#include "compiler/translator/Pragma.h"

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
// TODO: ES3 equivalent versions of GLSL
class TVersionGLSL : public TIntermTraverser
{
  public:
    TVersionGLSL(sh::GLenum type, const TPragma &pragma);

    // Returns 120 if the following is used the shader:
    // - "invariant",
    // - "gl_PointCoord",
    // - matrix/matrix constructors
    // - array "out" parameters
    // Else 110 is returned.
    int getVersion() { return mVersion; }

    virtual void visitSymbol(TIntermSymbol *);
    virtual bool visitAggregate(Visit, TIntermAggregate *);

  protected:
    void updateVersion(int version);

  private:
    int mVersion;
};

#endif  // COMPILER_TRANSLATOR_VERSIONGLSL_H_
