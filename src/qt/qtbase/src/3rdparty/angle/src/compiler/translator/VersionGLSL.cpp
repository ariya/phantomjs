//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/VersionGLSL.h"

static const int GLSL_VERSION_110 = 110;
static const int GLSL_VERSION_120 = 120;

// We need to scan for the following:
// 1. "invariant" keyword: This can occur in both - vertex and fragment shaders
//    but only at the global scope.
// 2. "gl_PointCoord" built-in variable: This can only occur in fragment shader
//    but inside any scope.
// 3. Call to a matrix constructor with another matrix as argument.
//    (These constructors were reserved in GLSL version 1.10.)
// 4. Arrays as "out" function parameters.
//    GLSL spec section 6.1.1: "When calling a function, expressions that do
//    not evaluate to l-values cannot be passed to parameters declared as
//    out or inout."
//    GLSL 1.1 section 5.8: "Other binary or unary expressions,
//    non-dereferenced arrays, function names, swizzles with repeated fields,
//    and constants cannot be l-values."
//    GLSL 1.2 relaxed the restriction on arrays, section 5.8: "Variables that
//    are built-in types, entire structures or arrays... are all l-values."
//
// TODO(alokp): The following two cases of invariant decalaration get lost
// during parsing - they do not get carried over to the intermediate tree.
// Handle these cases:
// 1. When a pragma is used to force all output variables to be invariant:
//    - #pragma STDGL invariant(all)
// 2. When a previously decalared or built-in variable is marked invariant:
//    - invariant gl_Position;
//    - varying vec3 color; invariant color;
//
TVersionGLSL::TVersionGLSL(ShShaderType type)
    : mShaderType(type),
      mVersion(GLSL_VERSION_110)
{
}

void TVersionGLSL::visitSymbol(TIntermSymbol* node)
{
    if (node->getSymbol() == "gl_PointCoord")
        updateVersion(GLSL_VERSION_120);
}

void TVersionGLSL::visitConstantUnion(TIntermConstantUnion*)
{
}

bool TVersionGLSL::visitBinary(Visit, TIntermBinary*)
{
    return true;
}

bool TVersionGLSL::visitUnary(Visit, TIntermUnary*)
{
    return true;
}

bool TVersionGLSL::visitSelection(Visit, TIntermSelection*)
{
    return true;
}

bool TVersionGLSL::visitAggregate(Visit, TIntermAggregate* node)
{
    bool visitChildren = true;

    switch (node->getOp()) {
      case EOpSequence:
        // We need to visit sequence children to get to global or inner scope.
        visitChildren = true;
        break;
      case EOpDeclaration: {
        const TIntermSequence& sequence = node->getSequence();
        TQualifier qualifier = sequence.front()->getAsTyped()->getQualifier();
        if ((qualifier == EvqInvariantVaryingIn) ||
            (qualifier == EvqInvariantVaryingOut)) {
            updateVersion(GLSL_VERSION_120);
        }
        break;
      }
      case EOpParameters: {
        const TIntermSequence& params = node->getSequence();
        for (TIntermSequence::const_iterator iter = params.begin();
             iter != params.end(); ++iter)
        {
            const TIntermTyped* param = (*iter)->getAsTyped();
            if (param->isArray())
            {
                TQualifier qualifier = param->getQualifier();
                if ((qualifier == EvqOut) || (qualifier ==  EvqInOut))
                {
                    updateVersion(GLSL_VERSION_120);
                    break;
                }
            }
        }
        // Fully processed. No need to visit children.
        visitChildren = false;
        break;
      }
      case EOpConstructMat2:
      case EOpConstructMat3:
      case EOpConstructMat4: {
        const TIntermSequence& sequence = node->getSequence();
        if (sequence.size() == 1) {
          TIntermTyped* typed = sequence.front()->getAsTyped();
          if (typed && typed->isMatrix()) {
            updateVersion(GLSL_VERSION_120);
          }
        }
        break;
      }

      default: break;
    }

    return visitChildren;
}

bool TVersionGLSL::visitLoop(Visit, TIntermLoop*)
{
    return true;
}

bool TVersionGLSL::visitBranch(Visit, TIntermBranch*)
{
    return true;
}

void TVersionGLSL::updateVersion(int version)
{
    mVersion = std::max(version, mVersion);
}

