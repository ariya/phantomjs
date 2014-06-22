//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_
#define COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/InfoSink.h"
#include "compiler/intermediate.h"

//
// This class decides which built-in functions need to be replaced with the
// emulated ones.
// It's only a workaround for OpenGL driver bugs, and isn't needed in general.
//
class BuiltInFunctionEmulator {
public:
    BuiltInFunctionEmulator(ShShaderType shaderType);
    // Records that a function is called by the shader and might needs to be
    // emulated.  If the function's group is not in mFunctionGroupFilter, this
    // becomes an no-op.
    // Returns true if the function call needs to be replaced with an emulated
    // one.
    bool SetFunctionCalled(TOperator op, const TType& param);
    bool SetFunctionCalled(
        TOperator op, const TType& param1, const TType& param2);

    // Output function emulation definition.  This should be before any other
    // shader source.
    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out, bool withPrecision) const;

    void MarkBuiltInFunctionsForEmulation(TIntermNode* root);

    void Cleanup();

    // "name(" becomes "webgl_name_emu(".
    static TString GetEmulatedFunctionName(const TString& name);

private:
    //
    // Built-in functions.
    //
    enum TBuiltInFunction {
        TFunctionCos1 = 0,  // float cos(float);
        TFunctionCos2,  // vec2 cos(vec2);
        TFunctionCos3,  // vec3 cos(vec3);
        TFunctionCos4,  // vec4 cos(vec4);

        TFunctionDistance1_1,  // float distance(float, float);
        TFunctionDistance2_2,  // vec2 distance(vec2, vec2);
        TFunctionDistance3_3,  // vec3 distance(vec3, vec3);
        TFunctionDistance4_4,  // vec4 distance(vec4, vec4);

        TFunctionDot1_1,  // float dot(float, float);
        TFunctionDot2_2,  // vec2 dot(vec2, vec2);
        TFunctionDot3_3,  // vec3 dot(vec3, vec3);
        TFunctionDot4_4,  // vec4 dot(vec4, vec4);

        TFunctionLength1,  // float length(float);
        TFunctionLength2,  // float length(vec2);
        TFunctionLength3,  // float length(vec3);
        TFunctionLength4,  // float length(vec4);

        TFunctionNormalize1,  // float normalize(float);
        TFunctionNormalize2,  // vec2 normalize(vec2);
        TFunctionNormalize3,  // vec3 normalize(vec3);
        TFunctionNormalize4,  // vec4 normalize(vec4);

        TFunctionReflect1_1,  // float reflect(float, float);
        TFunctionReflect2_2,  // vec2 reflect(vec2, vec2);
        TFunctionReflect3_3,  // vec3 reflect(vec3, vec3);
        TFunctionReflect4_4,  // vec4 reflect(vec4, vec4);

        TFunctionUnknown
    };

    TBuiltInFunction IdentifyFunction(TOperator op, const TType& param);
    TBuiltInFunction IdentifyFunction(
        TOperator op, const TType& param1, const TType& param2);

    bool SetFunctionCalled(TBuiltInFunction function);

    std::vector<TBuiltInFunction> mFunctions;

    const bool* mFunctionMask;  // a boolean flag for each function.
    const char** mFunctionSource;
};

#endif  // COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_
