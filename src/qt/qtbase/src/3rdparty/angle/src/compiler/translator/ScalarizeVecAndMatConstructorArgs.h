//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_SCALARIZE_VEC_AND_MAT_CONSTRUCTOR_ARGS_H_
#define COMPILER_TRANSLATOR_SCALARIZE_VEC_AND_MAT_CONSTRUCTOR_ARGS_H_

#include "compiler/translator/IntermNode.h"

class ScalarizeVecAndMatConstructorArgs : public TIntermTraverser
{
  public:
    ScalarizeVecAndMatConstructorArgs(sh::GLenum shaderType,
                                      bool fragmentPrecisionHigh)
        : mTempVarCount(0),
          mShaderType(shaderType),
          mFragmentPrecisionHigh(fragmentPrecisionHigh) {}

  protected:
    virtual bool visitAggregate(Visit visit, TIntermAggregate *node);

  private:
    void scalarizeArgs(TIntermAggregate *aggregate,
                       bool scalarizeVector, bool scalarizeMatrix);

    // If we have the following code:
    //   mat4 m(0);
    //   vec4 v(1, m);
    // We will rewrite to:
    //   mat4 m(0);
    //   mat4 _webgl_tmp_mat_0 = m;
    //   vec4 v(1, _webgl_tmp_mat_0[0][0], _webgl_tmp_mat_0[0][1], _webgl_tmp_mat_0[0][2]);
    // This function is to create nodes for "mat4 _webgl_tmp_mat_0 = m;" and insert it to
    // the code sequence.
    // Return the temporary variable name.
    TString createTempVariable(TIntermTyped *original);

    std::vector<TIntermSequence> mSequenceStack;
    int mTempVarCount;

    sh::GLenum mShaderType;
    bool mFragmentPrecisionHigh;
};

#endif  // COMPILER_TRANSLATOR_SCALARIZE_VEC_AND_MAT_CONSTRUCTOR_ARGS_H_
