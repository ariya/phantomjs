//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Create strings that declare built-in definitions, add built-ins that
// cannot be expressed in the files, and establish mappings between 
// built-in functions and operators.
//

#include "compiler/Initialize.h"

#include "compiler/intermediate.h"

void InsertBuiltInFunctions(ShShaderType type, ShShaderSpec spec, const ShBuiltInResources &resources, TSymbolTable &symbolTable)
{
    TType *float1 = new TType(EbtFloat, EbpUndefined, EvqGlobal, 1);
    TType *float2 = new TType(EbtFloat, EbpUndefined, EvqGlobal, 2);
    TType *float3 = new TType(EbtFloat, EbpUndefined, EvqGlobal, 3);
    TType *float4 = new TType(EbtFloat, EbpUndefined, EvqGlobal, 4);

    TType *int2 = new TType(EbtInt, EbpUndefined, EvqGlobal, 2);
    TType *int3 = new TType(EbtInt, EbpUndefined, EvqGlobal, 3);
    TType *int4 = new TType(EbtInt, EbpUndefined, EvqGlobal, 4);

    //
    // Angle and Trigonometric Functions.
    //
    symbolTable.insertBuiltIn(float1, "radians", float1);
    symbolTable.insertBuiltIn(float2, "radians", float2);
    symbolTable.insertBuiltIn(float3, "radians", float3);
    symbolTable.insertBuiltIn(float4, "radians", float4);

    symbolTable.insertBuiltIn(float1, "degrees", float1);
    symbolTable.insertBuiltIn(float2, "degrees", float2);
    symbolTable.insertBuiltIn(float3, "degrees", float3);
    symbolTable.insertBuiltIn(float4, "degrees", float4);

    symbolTable.insertBuiltIn(float1, "sin", float1);
    symbolTable.insertBuiltIn(float2, "sin", float2);
    symbolTable.insertBuiltIn(float3, "sin", float3);
    symbolTable.insertBuiltIn(float4, "sin", float4);

    symbolTable.insertBuiltIn(float1, "cos", float1);
    symbolTable.insertBuiltIn(float2, "cos", float2);
    symbolTable.insertBuiltIn(float3, "cos", float3);
    symbolTable.insertBuiltIn(float4, "cos", float4);

    symbolTable.insertBuiltIn(float1, "tan", float1);
    symbolTable.insertBuiltIn(float2, "tan", float2);
    symbolTable.insertBuiltIn(float3, "tan", float3);
    symbolTable.insertBuiltIn(float4, "tan", float4);

    symbolTable.insertBuiltIn(float1, "asin", float1);
    symbolTable.insertBuiltIn(float2, "asin", float2);
    symbolTable.insertBuiltIn(float3, "asin", float3);
    symbolTable.insertBuiltIn(float4, "asin", float4);

    symbolTable.insertBuiltIn(float1, "acos", float1);
    symbolTable.insertBuiltIn(float2, "acos", float2);
    symbolTable.insertBuiltIn(float3, "acos", float3);
    symbolTable.insertBuiltIn(float4, "acos", float4);

    symbolTable.insertBuiltIn(float1, "atan", float1, float1);
    symbolTable.insertBuiltIn(float2, "atan", float2, float2);
    symbolTable.insertBuiltIn(float3, "atan", float3, float3);
    symbolTable.insertBuiltIn(float4, "atan", float4, float4);

    symbolTable.insertBuiltIn(float1, "atan", float1);
    symbolTable.insertBuiltIn(float2, "atan", float2);
    symbolTable.insertBuiltIn(float3, "atan", float3);
    symbolTable.insertBuiltIn(float4, "atan", float4);

    //
    // Exponential Functions.
    //
    symbolTable.insertBuiltIn(float1, "pow", float1, float1);
    symbolTable.insertBuiltIn(float2, "pow", float2, float2);
    symbolTable.insertBuiltIn(float3, "pow", float3, float3);
    symbolTable.insertBuiltIn(float4, "pow", float4, float4);

    symbolTable.insertBuiltIn(float1, "exp", float1);
    symbolTable.insertBuiltIn(float2, "exp", float2);
    symbolTable.insertBuiltIn(float3, "exp", float3);
    symbolTable.insertBuiltIn(float4, "exp", float4);

    symbolTable.insertBuiltIn(float1, "log", float1);
    symbolTable.insertBuiltIn(float2, "log", float2);
    symbolTable.insertBuiltIn(float3, "log", float3);
    symbolTable.insertBuiltIn(float4, "log", float4);

    symbolTable.insertBuiltIn(float1, "exp2", float1);
    symbolTable.insertBuiltIn(float2, "exp2", float2);
    symbolTable.insertBuiltIn(float3, "exp2", float3);
    symbolTable.insertBuiltIn(float4, "exp2", float4);

    symbolTable.insertBuiltIn(float1, "log2", float1);
    symbolTable.insertBuiltIn(float2, "log2", float2);
    symbolTable.insertBuiltIn(float3, "log2", float3);
    symbolTable.insertBuiltIn(float4, "log2", float4);

    symbolTable.insertBuiltIn(float1, "sqrt", float1);
    symbolTable.insertBuiltIn(float2, "sqrt", float2);
    symbolTable.insertBuiltIn(float3, "sqrt", float3);
    symbolTable.insertBuiltIn(float4, "sqrt", float4);

    symbolTable.insertBuiltIn(float1, "inversesqrt", float1);
    symbolTable.insertBuiltIn(float2, "inversesqrt", float2);
    symbolTable.insertBuiltIn(float3, "inversesqrt", float3);
    symbolTable.insertBuiltIn(float4, "inversesqrt", float4);

    //
    // Common Functions.
    //
    symbolTable.insertBuiltIn(float1, "abs", float1);
    symbolTable.insertBuiltIn(float2, "abs", float2);
    symbolTable.insertBuiltIn(float3, "abs", float3);
    symbolTable.insertBuiltIn(float4, "abs", float4);

    symbolTable.insertBuiltIn(float1, "sign", float1);
    symbolTable.insertBuiltIn(float2, "sign", float2);
    symbolTable.insertBuiltIn(float3, "sign", float3);
    symbolTable.insertBuiltIn(float4, "sign", float4);

    symbolTable.insertBuiltIn(float1, "floor", float1);
    symbolTable.insertBuiltIn(float2, "floor", float2);
    symbolTable.insertBuiltIn(float3, "floor", float3);
    symbolTable.insertBuiltIn(float4, "floor", float4);

    symbolTable.insertBuiltIn(float1, "ceil", float1);
    symbolTable.insertBuiltIn(float2, "ceil", float2);
    symbolTable.insertBuiltIn(float3, "ceil", float3);
    symbolTable.insertBuiltIn(float4, "ceil", float4);

    symbolTable.insertBuiltIn(float1, "fract", float1);
    symbolTable.insertBuiltIn(float2, "fract", float2);
    symbolTable.insertBuiltIn(float3, "fract", float3);
    symbolTable.insertBuiltIn(float4, "fract", float4);

    symbolTable.insertBuiltIn(float1, "mod", float1, float1);
    symbolTable.insertBuiltIn(float2, "mod", float2, float1);
    symbolTable.insertBuiltIn(float3, "mod", float3, float1);
    symbolTable.insertBuiltIn(float4, "mod", float4, float1);
    symbolTable.insertBuiltIn(float2, "mod", float2, float2);
    symbolTable.insertBuiltIn(float3, "mod", float3, float3);
    symbolTable.insertBuiltIn(float4, "mod", float4, float4);

    symbolTable.insertBuiltIn(float1, "min", float1, float1);
    symbolTable.insertBuiltIn(float2, "min", float2, float1);
    symbolTable.insertBuiltIn(float3, "min", float3, float1);
    symbolTable.insertBuiltIn(float4, "min", float4, float1);
    symbolTable.insertBuiltIn(float2, "min", float2, float2);
    symbolTable.insertBuiltIn(float3, "min", float3, float3);
    symbolTable.insertBuiltIn(float4, "min", float4, float4);

    symbolTable.insertBuiltIn(float1, "max", float1, float1);
    symbolTable.insertBuiltIn(float2, "max", float2, float1);
    symbolTable.insertBuiltIn(float3, "max", float3, float1);
    symbolTable.insertBuiltIn(float4, "max", float4, float1);
    symbolTable.insertBuiltIn(float2, "max", float2, float2);
    symbolTable.insertBuiltIn(float3, "max", float3, float3);
    symbolTable.insertBuiltIn(float4, "max", float4, float4);

    symbolTable.insertBuiltIn(float1, "clamp", float1, float1, float1);
    symbolTable.insertBuiltIn(float2, "clamp", float2, float1, float1);
    symbolTable.insertBuiltIn(float3, "clamp", float3, float1, float1);
    symbolTable.insertBuiltIn(float4, "clamp", float4, float1, float1);
    symbolTable.insertBuiltIn(float2, "clamp", float2, float2, float2);
    symbolTable.insertBuiltIn(float3, "clamp", float3, float3, float3);
    symbolTable.insertBuiltIn(float4, "clamp", float4, float4, float4);

    symbolTable.insertBuiltIn(float1, "mix", float1, float1, float1);
    symbolTable.insertBuiltIn(float2, "mix", float2, float2, float1);
    symbolTable.insertBuiltIn(float3, "mix", float3, float3, float1);
    symbolTable.insertBuiltIn(float4, "mix", float4, float4, float1);
    symbolTable.insertBuiltIn(float2, "mix", float2, float2, float2);
    symbolTable.insertBuiltIn(float3, "mix", float3, float3, float3);
    symbolTable.insertBuiltIn(float4, "mix", float4, float4, float4);

    symbolTable.insertBuiltIn(float1, "step", float1, float1);
    symbolTable.insertBuiltIn(float2, "step", float2, float2);
    symbolTable.insertBuiltIn(float3, "step", float3, float3);
    symbolTable.insertBuiltIn(float4, "step", float4, float4);
    symbolTable.insertBuiltIn(float2, "step", float1, float2);
    symbolTable.insertBuiltIn(float3, "step", float1, float3);
    symbolTable.insertBuiltIn(float4, "step", float1, float4);

    symbolTable.insertBuiltIn(float1, "smoothstep", float1, float1, float1);
    symbolTable.insertBuiltIn(float2, "smoothstep", float2, float2, float2);
    symbolTable.insertBuiltIn(float3, "smoothstep", float3, float3, float3);
    symbolTable.insertBuiltIn(float4, "smoothstep", float4, float4, float4);
    symbolTable.insertBuiltIn(float2, "smoothstep", float1, float1, float2);
    symbolTable.insertBuiltIn(float3, "smoothstep", float1, float1, float3);
    symbolTable.insertBuiltIn(float4, "smoothstep", float1, float1, float4);

    //
    // Geometric Functions.
    //
    symbolTable.insertBuiltIn(float1, "length", float1);
    symbolTable.insertBuiltIn(float1, "length", float2);
    symbolTable.insertBuiltIn(float1, "length", float3);
    symbolTable.insertBuiltIn(float1, "length", float4);

    symbolTable.insertBuiltIn(float1, "distance", float1, float1);
    symbolTable.insertBuiltIn(float1, "distance", float2, float2);
    symbolTable.insertBuiltIn(float1, "distance", float3, float3);
    symbolTable.insertBuiltIn(float1, "distance", float4, float4);

    symbolTable.insertBuiltIn(float1, "dot", float1, float1);
    symbolTable.insertBuiltIn(float1, "dot", float2, float2);
    symbolTable.insertBuiltIn(float1, "dot", float3, float3);
    symbolTable.insertBuiltIn(float1, "dot", float4, float4);

    symbolTable.insertBuiltIn(float3, "cross", float3, float3);
    symbolTable.insertBuiltIn(float1, "normalize", float1);
    symbolTable.insertBuiltIn(float2, "normalize", float2);
    symbolTable.insertBuiltIn(float3, "normalize", float3);
    symbolTable.insertBuiltIn(float4, "normalize", float4);

    symbolTable.insertBuiltIn(float1, "faceforward", float1, float1, float1);
    symbolTable.insertBuiltIn(float2, "faceforward", float2, float2, float2);
    symbolTable.insertBuiltIn(float3, "faceforward", float3, float3, float3);
    symbolTable.insertBuiltIn(float4, "faceforward", float4, float4, float4);

    symbolTable.insertBuiltIn(float1, "reflect", float1, float1);
    symbolTable.insertBuiltIn(float2, "reflect", float2, float2);
    symbolTable.insertBuiltIn(float3, "reflect", float3, float3);
    symbolTable.insertBuiltIn(float4, "reflect", float4, float4);

    symbolTable.insertBuiltIn(float1, "refract", float1, float1, float1);
    symbolTable.insertBuiltIn(float2, "refract", float2, float2, float1);
    symbolTable.insertBuiltIn(float3, "refract", float3, float3, float1);
    symbolTable.insertBuiltIn(float4, "refract", float4, float4, float1);

    TType *mat2 = new TType(EbtFloat, EbpUndefined, EvqGlobal, 2, true);
    TType *mat3 = new TType(EbtFloat, EbpUndefined, EvqGlobal, 3, true);
    TType *mat4 = new TType(EbtFloat, EbpUndefined, EvqGlobal, 4, true);

    //
    // Matrix Functions.
    //
    symbolTable.insertBuiltIn(mat2, "matrixCompMult", mat2, mat2);
    symbolTable.insertBuiltIn(mat3, "matrixCompMult", mat3, mat3);
    symbolTable.insertBuiltIn(mat4, "matrixCompMult", mat4, mat4);

    TType *bool1 = new TType(EbtBool, EbpUndefined, EvqGlobal, 1);
    TType *bool2 = new TType(EbtBool, EbpUndefined, EvqGlobal, 2);
    TType *bool3 = new TType(EbtBool, EbpUndefined, EvqGlobal, 3);
    TType *bool4 = new TType(EbtBool, EbpUndefined, EvqGlobal, 4);

    //
    // Vector relational functions.
    //
    symbolTable.insertBuiltIn(bool2, "lessThan", float2, float2);
    symbolTable.insertBuiltIn(bool3, "lessThan", float3, float3);
    symbolTable.insertBuiltIn(bool4, "lessThan", float4, float4);

    symbolTable.insertBuiltIn(bool2, "lessThan", int2, int2);
    symbolTable.insertBuiltIn(bool3, "lessThan", int3, int3);
    symbolTable.insertBuiltIn(bool4, "lessThan", int4, int4);

    symbolTable.insertBuiltIn(bool2, "lessThanEqual", float2, float2);
    symbolTable.insertBuiltIn(bool3, "lessThanEqual", float3, float3);
    symbolTable.insertBuiltIn(bool4, "lessThanEqual", float4, float4);

    symbolTable.insertBuiltIn(bool2, "lessThanEqual", int2, int2);
    symbolTable.insertBuiltIn(bool3, "lessThanEqual", int3, int3);
    symbolTable.insertBuiltIn(bool4, "lessThanEqual", int4, int4);

    symbolTable.insertBuiltIn(bool2, "greaterThan", float2, float2);
    symbolTable.insertBuiltIn(bool3, "greaterThan", float3, float3);
    symbolTable.insertBuiltIn(bool4, "greaterThan", float4, float4);

    symbolTable.insertBuiltIn(bool2, "greaterThan", int2, int2);
    symbolTable.insertBuiltIn(bool3, "greaterThan", int3, int3);
    symbolTable.insertBuiltIn(bool4, "greaterThan", int4, int4);

    symbolTable.insertBuiltIn(bool2, "greaterThanEqual", float2, float2);
    symbolTable.insertBuiltIn(bool3, "greaterThanEqual", float3, float3);
    symbolTable.insertBuiltIn(bool4, "greaterThanEqual", float4, float4);

    symbolTable.insertBuiltIn(bool2, "greaterThanEqual", int2, int2);
    symbolTable.insertBuiltIn(bool3, "greaterThanEqual", int3, int3);
    symbolTable.insertBuiltIn(bool4, "greaterThanEqual", int4, int4);

    symbolTable.insertBuiltIn(bool2, "equal", float2, float2);
    symbolTable.insertBuiltIn(bool3, "equal", float3, float3);
    symbolTable.insertBuiltIn(bool4, "equal", float4, float4);

    symbolTable.insertBuiltIn(bool2, "equal", int2, int2);
    symbolTable.insertBuiltIn(bool3, "equal", int3, int3);
    symbolTable.insertBuiltIn(bool4, "equal", int4, int4);

    symbolTable.insertBuiltIn(bool2, "equal", bool2, bool2);
    symbolTable.insertBuiltIn(bool3, "equal", bool3, bool3);
    symbolTable.insertBuiltIn(bool4, "equal", bool4, bool4);

    symbolTable.insertBuiltIn(bool2, "notEqual", float2, float2);
    symbolTable.insertBuiltIn(bool3, "notEqual", float3, float3);
    symbolTable.insertBuiltIn(bool4, "notEqual", float4, float4);

    symbolTable.insertBuiltIn(bool2, "notEqual", int2, int2);
    symbolTable.insertBuiltIn(bool3, "notEqual", int3, int3);
    symbolTable.insertBuiltIn(bool4, "notEqual", int4, int4);

    symbolTable.insertBuiltIn(bool2, "notEqual", bool2, bool2);
    symbolTable.insertBuiltIn(bool3, "notEqual", bool3, bool3);
    symbolTable.insertBuiltIn(bool4, "notEqual", bool4, bool4);

    symbolTable.insertBuiltIn(bool1, "any", bool2);
    symbolTable.insertBuiltIn(bool1, "any", bool3);
    symbolTable.insertBuiltIn(bool1, "any", bool4);

    symbolTable.insertBuiltIn(bool1, "all", bool2);
    symbolTable.insertBuiltIn(bool1, "all", bool3);
    symbolTable.insertBuiltIn(bool1, "all", bool4);

    symbolTable.insertBuiltIn(bool2, "not", bool2);
    symbolTable.insertBuiltIn(bool3, "not", bool3);
    symbolTable.insertBuiltIn(bool4, "not", bool4);

    TType *sampler2D = new TType(EbtSampler2D, EbpUndefined, EvqGlobal, 1);
    TType *samplerCube = new TType(EbtSamplerCube, EbpUndefined, EvqGlobal, 1);

    //
    // Texture Functions for GLSL ES 1.0
    //
    symbolTable.insertBuiltIn(float4, "texture2D", sampler2D, float2);
    symbolTable.insertBuiltIn(float4, "texture2DProj", sampler2D, float3);
    symbolTable.insertBuiltIn(float4, "texture2DProj", sampler2D, float4);
    symbolTable.insertBuiltIn(float4, "textureCube", samplerCube, float3);

    if (resources.OES_EGL_image_external)
    {
        TType *samplerExternalOES = new TType(EbtSamplerExternalOES, EbpUndefined, EvqGlobal, 1);

        symbolTable.insertBuiltIn(float4, "texture2D", samplerExternalOES, float2);
        symbolTable.insertBuiltIn(float4, "texture2DProj", samplerExternalOES, float3);
        symbolTable.insertBuiltIn(float4, "texture2DProj", samplerExternalOES, float4);
    }

    if (resources.ARB_texture_rectangle)
    {
        TType *sampler2DRect = new TType(EbtSampler2DRect, EbpUndefined, EvqGlobal, 1);

        symbolTable.insertBuiltIn(float4, "texture2DRect", sampler2DRect, float2);
        symbolTable.insertBuiltIn(float4, "texture2DRectProj", sampler2DRect, float3);
        symbolTable.insertBuiltIn(float4, "texture2DRectProj", sampler2DRect, float4);
    }

    if (type == SH_FRAGMENT_SHADER)
    {
        symbolTable.insertBuiltIn(float4, "texture2D", sampler2D, float2, float1);
        symbolTable.insertBuiltIn(float4, "texture2DProj", sampler2D, float3, float1);
        symbolTable.insertBuiltIn(float4, "texture2DProj", sampler2D, float4, float1);
        symbolTable.insertBuiltIn(float4, "textureCube", samplerCube, float3, float1);

        if (resources.OES_standard_derivatives)
        {
            symbolTable.insertBuiltIn(float1, "dFdx", float1);
            symbolTable.insertBuiltIn(float2, "dFdx", float2);
            symbolTable.insertBuiltIn(float3, "dFdx", float3);
            symbolTable.insertBuiltIn(float4, "dFdx", float4);
            
            symbolTable.insertBuiltIn(float1, "dFdy", float1);
            symbolTable.insertBuiltIn(float2, "dFdy", float2);
            symbolTable.insertBuiltIn(float3, "dFdy", float3);
            symbolTable.insertBuiltIn(float4, "dFdy", float4);

            symbolTable.insertBuiltIn(float1, "fwidth", float1);
            symbolTable.insertBuiltIn(float2, "fwidth", float2);
            symbolTable.insertBuiltIn(float3, "fwidth", float3);
            symbolTable.insertBuiltIn(float4, "fwidth", float4);
        }
    }

    if(type == SH_VERTEX_SHADER)
    {
        symbolTable.insertBuiltIn(float4, "texture2DLod", sampler2D, float2, float1);
        symbolTable.insertBuiltIn(float4, "texture2DProjLod", sampler2D, float3, float1);
        symbolTable.insertBuiltIn(float4, "texture2DProjLod", sampler2D, float4, float1);
        symbolTable.insertBuiltIn(float4, "textureCubeLod", samplerCube, float3, float1);
    }

    //
    // Depth range in window coordinates
    //
    TFieldList *fields = NewPoolTFieldList();
    TField *near = new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1), NewPoolTString("near"));
    TField *far = new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1), NewPoolTString("far"));
    TField *diff = new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1), NewPoolTString("diff"));
    fields->push_back(near);
    fields->push_back(far);
    fields->push_back(diff);
    TStructure *depthRangeStruct = new TStructure(NewPoolTString("gl_DepthRangeParameters"), fields);
    TVariable *depthRangeParameters = new TVariable(&depthRangeStruct->name(), depthRangeStruct, true);
    symbolTable.insert(*depthRangeParameters);
    TVariable *depthRange = new TVariable(NewPoolTString("gl_DepthRange"), TType(depthRangeStruct));
    depthRange->setQualifier(EvqUniform);
    symbolTable.insert(*depthRange);

    //
    // Implementation dependent built-in constants.
    //
    symbolTable.insertConstInt("gl_MaxVertexAttribs", resources.MaxVertexAttribs);
    symbolTable.insertConstInt("gl_MaxVertexUniformVectors", resources.MaxVertexUniformVectors);
    symbolTable.insertConstInt("gl_MaxVaryingVectors", resources.MaxVaryingVectors);
    symbolTable.insertConstInt("gl_MaxVertexTextureImageUnits", resources.MaxVertexTextureImageUnits);
    symbolTable.insertConstInt("gl_MaxCombinedTextureImageUnits", resources.MaxCombinedTextureImageUnits);
    symbolTable.insertConstInt("gl_MaxTextureImageUnits", resources.MaxTextureImageUnits);
    symbolTable.insertConstInt("gl_MaxFragmentUniformVectors", resources.MaxFragmentUniformVectors);

    if (spec != SH_CSS_SHADERS_SPEC)
    {
        symbolTable.insertConstInt("gl_MaxDrawBuffers", resources.MaxDrawBuffers);
    }
}

void IdentifyBuiltIns(ShShaderType type, ShShaderSpec spec,
                      const ShBuiltInResources &resources,
                      TSymbolTable &symbolTable)
{
    //
    // First, insert some special built-in variables that are not in 
    // the built-in header files.
    //
    switch(type) {
    case SH_FRAGMENT_SHADER:
        symbolTable.insert(*new TVariable(NewPoolTString("gl_FragCoord"),                       TType(EbtFloat, EbpMedium, EvqFragCoord,   4)));
        symbolTable.insert(*new TVariable(NewPoolTString("gl_FrontFacing"),                     TType(EbtBool,  EbpUndefined, EvqFrontFacing, 1)));
        symbolTable.insert(*new TVariable(NewPoolTString("gl_PointCoord"),                      TType(EbtFloat, EbpMedium, EvqPointCoord,  2)));

        //
        // In CSS Shaders, gl_FragColor, gl_FragData, and gl_MaxDrawBuffers are not available.
        // Instead, css_MixColor and css_ColorMatrix are available.
        //
        if (spec != SH_CSS_SHADERS_SPEC) {
            symbolTable.insert(*new TVariable(NewPoolTString("gl_FragColor"),                   TType(EbtFloat, EbpMedium, EvqFragColor,   4)));
            symbolTable.insert(*new TVariable(NewPoolTString("gl_FragData[gl_MaxDrawBuffers]"), TType(EbtFloat, EbpMedium, EvqFragData,    4)));
            if (resources.EXT_frag_depth) {
                symbolTable.insert(*new TVariable(NewPoolTString("gl_FragDepthEXT"),            TType(EbtFloat, resources.FragmentPrecisionHigh ? EbpHigh : EbpMedium, EvqFragDepth, 1)));
                symbolTable.relateToExtension("gl_FragDepthEXT", "GL_EXT_frag_depth");
            }
        } else {
            symbolTable.insert(*new TVariable(NewPoolTString("css_MixColor"),                   TType(EbtFloat, EbpMedium, EvqGlobal,      4)));
            symbolTable.insert(*new TVariable(NewPoolTString("css_ColorMatrix"),                TType(EbtFloat, EbpMedium, EvqGlobal,      4, true)));
        }

        break;

    case SH_VERTEX_SHADER:
        symbolTable.insert(*new TVariable(NewPoolTString("gl_Position"),    TType(EbtFloat, EbpHigh, EvqPosition,    4)));
        symbolTable.insert(*new TVariable(NewPoolTString("gl_PointSize"),   TType(EbtFloat, EbpMedium, EvqPointSize,   1)));
        break;

    default: assert(false && "Language not supported");
    }

    //
    // Next, identify which built-ins from the already loaded headers have
    // a mapping to an operator.  Those that are not identified as such are
    // expected to be resolved through a library of functions, versus as
    // operations.
    //
    symbolTable.relateToOperator("matrixCompMult",   EOpMul);

    symbolTable.relateToOperator("equal",            EOpVectorEqual);
    symbolTable.relateToOperator("notEqual",         EOpVectorNotEqual);
    symbolTable.relateToOperator("lessThan",         EOpLessThan);
    symbolTable.relateToOperator("greaterThan",      EOpGreaterThan);
    symbolTable.relateToOperator("lessThanEqual",    EOpLessThanEqual);
    symbolTable.relateToOperator("greaterThanEqual", EOpGreaterThanEqual);
    
    symbolTable.relateToOperator("radians",      EOpRadians);
    symbolTable.relateToOperator("degrees",      EOpDegrees);
    symbolTable.relateToOperator("sin",          EOpSin);
    symbolTable.relateToOperator("cos",          EOpCos);
    symbolTable.relateToOperator("tan",          EOpTan);
    symbolTable.relateToOperator("asin",         EOpAsin);
    symbolTable.relateToOperator("acos",         EOpAcos);
    symbolTable.relateToOperator("atan",         EOpAtan);

    symbolTable.relateToOperator("pow",          EOpPow);
    symbolTable.relateToOperator("exp2",         EOpExp2);
    symbolTable.relateToOperator("log",          EOpLog);
    symbolTable.relateToOperator("exp",          EOpExp);
    symbolTable.relateToOperator("log2",         EOpLog2);
    symbolTable.relateToOperator("sqrt",         EOpSqrt);
    symbolTable.relateToOperator("inversesqrt",  EOpInverseSqrt);

    symbolTable.relateToOperator("abs",          EOpAbs);
    symbolTable.relateToOperator("sign",         EOpSign);
    symbolTable.relateToOperator("floor",        EOpFloor);
    symbolTable.relateToOperator("ceil",         EOpCeil);
    symbolTable.relateToOperator("fract",        EOpFract);
    symbolTable.relateToOperator("mod",          EOpMod);
    symbolTable.relateToOperator("min",          EOpMin);
    symbolTable.relateToOperator("max",          EOpMax);
    symbolTable.relateToOperator("clamp",        EOpClamp);
    symbolTable.relateToOperator("mix",          EOpMix);
    symbolTable.relateToOperator("step",         EOpStep);
    symbolTable.relateToOperator("smoothstep",   EOpSmoothStep);

    symbolTable.relateToOperator("length",       EOpLength);
    symbolTable.relateToOperator("distance",     EOpDistance);
    symbolTable.relateToOperator("dot",          EOpDot);
    symbolTable.relateToOperator("cross",        EOpCross);
    symbolTable.relateToOperator("normalize",    EOpNormalize);
    symbolTable.relateToOperator("faceforward",  EOpFaceForward);
    symbolTable.relateToOperator("reflect",      EOpReflect);
    symbolTable.relateToOperator("refract",      EOpRefract);
    
    symbolTable.relateToOperator("any",          EOpAny);
    symbolTable.relateToOperator("all",          EOpAll);
    symbolTable.relateToOperator("not",          EOpVectorLogicalNot);

    // Map language-specific operators.
    switch(type) {
    case SH_VERTEX_SHADER:
        break;
    case SH_FRAGMENT_SHADER:
        if (resources.OES_standard_derivatives) {
            symbolTable.relateToOperator("dFdx",   EOpDFdx);
            symbolTable.relateToOperator("dFdy",   EOpDFdy);
            symbolTable.relateToOperator("fwidth", EOpFwidth);

            symbolTable.relateToExtension("dFdx", "GL_OES_standard_derivatives");
            symbolTable.relateToExtension("dFdy", "GL_OES_standard_derivatives");
            symbolTable.relateToExtension("fwidth", "GL_OES_standard_derivatives");
        }
        break;
    default: break;
    }

    // Finally add resource-specific variables.
    switch(type) {
    case SH_FRAGMENT_SHADER:
        if (spec != SH_CSS_SHADERS_SPEC) {
            // Set up gl_FragData.  The array size.
            TType fragData(EbtFloat, EbpMedium, EvqFragData, 4, false, true);
            fragData.setArraySize(resources.MaxDrawBuffers);
            symbolTable.insert(*new TVariable(NewPoolTString("gl_FragData"),    fragData));
        }
        break;
    default: break;
    }
}

void InitExtensionBehavior(const ShBuiltInResources& resources,
                           TExtensionBehavior& extBehavior)
{
    if (resources.OES_standard_derivatives)
        extBehavior["GL_OES_standard_derivatives"] = EBhUndefined;
    if (resources.OES_EGL_image_external)
        extBehavior["GL_OES_EGL_image_external"] = EBhUndefined;
    if (resources.ARB_texture_rectangle)
        extBehavior["GL_ARB_texture_rectangle"] = EBhUndefined;
    if (resources.EXT_draw_buffers)
        extBehavior["GL_EXT_draw_buffers"] = EBhUndefined;
    if (resources.EXT_frag_depth)
        extBehavior["GL_EXT_frag_depth"] = EBhUndefined;
}
