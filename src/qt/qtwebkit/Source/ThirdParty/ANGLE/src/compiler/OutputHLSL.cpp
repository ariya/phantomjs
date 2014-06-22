//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/OutputHLSL.h"

#include "common/angleutils.h"
#include "compiler/debug.h"
#include "compiler/DetectDiscontinuity.h"
#include "compiler/InfoSink.h"
#include "compiler/SearchSymbol.h"
#include "compiler/UnfoldShortCircuit.h"

#include <algorithm>
#include <cfloat>
#include <stdio.h>

namespace sh
{
// Integer to TString conversion
TString str(int i)
{
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d", i);
    return buffer;
}

OutputHLSL::OutputHLSL(TParseContext &context, const ShBuiltInResources& resources, ShShaderOutput outputType)
    : TIntermTraverser(true, true, true), mContext(context), mOutputType(outputType)
{
    mUnfoldShortCircuit = new UnfoldShortCircuit(context, this);
    mInsideFunction = false;

    mUsesTexture2D = false;
    mUsesTexture2D_bias = false;
    mUsesTexture2DProj = false;
    mUsesTexture2DProj_bias = false;
    mUsesTexture2DProjLod = false;
    mUsesTexture2DLod = false;
    mUsesTextureCube = false;
    mUsesTextureCube_bias = false;
    mUsesTextureCubeLod = false;
    mUsesTexture2DLod0 = false;
    mUsesTexture2DLod0_bias = false;
    mUsesTexture2DProjLod0 = false;
    mUsesTexture2DProjLod0_bias = false;
    mUsesTextureCubeLod0 = false;
    mUsesTextureCubeLod0_bias = false;
    mUsesFragColor = false;
    mUsesFragData = false;
    mUsesDepthRange = false;
    mUsesFragCoord = false;
    mUsesPointCoord = false;
    mUsesFrontFacing = false;
    mUsesPointSize = false;
    mUsesFragDepth = false;
    mUsesXor = false;
    mUsesMod1 = false;
    mUsesMod2v = false;
    mUsesMod2f = false;
    mUsesMod3v = false;
    mUsesMod3f = false;
    mUsesMod4v = false;
    mUsesMod4f = false;
    mUsesFaceforward1 = false;
    mUsesFaceforward2 = false;
    mUsesFaceforward3 = false;
    mUsesFaceforward4 = false;
    mUsesEqualMat2 = false;
    mUsesEqualMat3 = false;
    mUsesEqualMat4 = false;
    mUsesEqualVec2 = false;
    mUsesEqualVec3 = false;
    mUsesEqualVec4 = false;
    mUsesEqualIVec2 = false;
    mUsesEqualIVec3 = false;
    mUsesEqualIVec4 = false;
    mUsesEqualBVec2 = false;
    mUsesEqualBVec3 = false;
    mUsesEqualBVec4 = false;
    mUsesAtan2_1 = false;
    mUsesAtan2_2 = false;
    mUsesAtan2_3 = false;
    mUsesAtan2_4 = false;

    mNumRenderTargets = resources.EXT_draw_buffers ? resources.MaxDrawBuffers : 1;

    mScopeDepth = 0;

    mUniqueIndex = 0;

    mContainsLoopDiscontinuity = false;
    mOutputLod0Function = false;
    mInsideDiscontinuousLoop = false;

    mExcessiveLoopIndex = NULL;

    if (mOutputType == SH_HLSL9_OUTPUT)
    {
        if (mContext.shaderType == SH_FRAGMENT_SHADER)
        {
            mUniformRegister = 3;   // Reserve registers for dx_DepthRange, dx_ViewCoords and dx_DepthFront
        }
        else
        {
            mUniformRegister = 2;   // Reserve registers for dx_DepthRange and dx_ViewAdjust
        }
    }
    else
    {
        mUniformRegister = 0;
    }

    mSamplerRegister = 0;
}

OutputHLSL::~OutputHLSL()
{
    delete mUnfoldShortCircuit;
}

void OutputHLSL::output()
{
    mContainsLoopDiscontinuity = mContext.shaderType == SH_FRAGMENT_SHADER && containsLoopDiscontinuity(mContext.treeRoot);

    mContext.treeRoot->traverse(this);   // Output the body first to determine what has to go in the header
    header();

    mContext.infoSink().obj << mHeader.c_str();
    mContext.infoSink().obj << mBody.c_str();
}

TInfoSinkBase &OutputHLSL::getBodyStream()
{
    return mBody;
}

const ActiveUniforms &OutputHLSL::getUniforms()
{
    return mActiveUniforms;
}

int OutputHLSL::vectorSize(const TType &type) const
{
    int elementSize = type.isMatrix() ? type.getNominalSize() : 1;
    int arraySize = type.isArray() ? type.getArraySize() : 1;

    return elementSize * arraySize;
}

void OutputHLSL::header()
{
    ShShaderType shaderType = mContext.shaderType;
    TInfoSinkBase &out = mHeader;

    for (StructDeclarations::iterator structDeclaration = mStructDeclarations.begin(); structDeclaration != mStructDeclarations.end(); structDeclaration++)
    {
        out << *structDeclaration;
    }

    for (Constructors::iterator constructor = mConstructors.begin(); constructor != mConstructors.end(); constructor++)
    {
        out << *constructor;
    }

    TString uniforms;
    TString varyings;
    TString attributes;

    for (ReferencedSymbols::const_iterator uniform = mReferencedUniforms.begin(); uniform != mReferencedUniforms.end(); uniform++)
    {
        const TType &type = uniform->second->getType();
        const TString &name = uniform->second->getSymbol();

        if (mOutputType == SH_HLSL11_OUTPUT && IsSampler(type.getBasicType()))   // Also declare the texture
        {
            int index = samplerRegister(mReferencedUniforms[name]);

            uniforms += "uniform SamplerState sampler_" + decorateUniform(name, type) + arrayString(type) + 
                        " : register(s" + str(index) + ");\n";

            uniforms += "uniform " + textureString(type) + " texture_" + decorateUniform(name, type) + arrayString(type) + 
                        " : register(t" + str(index) + ");\n";
        }
        else
        {
            uniforms += "uniform " + typeString(type) + " " + decorateUniform(name, type) + arrayString(type) + 
                        " : register(" + registerString(mReferencedUniforms[name]) + ");\n";
        }
    }

    for (ReferencedSymbols::const_iterator varying = mReferencedVaryings.begin(); varying != mReferencedVaryings.end(); varying++)
    {
        const TType &type = varying->second->getType();
        const TString &name = varying->second->getSymbol();

        // Program linking depends on this exact format
        varyings += "static " + typeString(type) + " " + decorate(name) + arrayString(type) + " = " + initializer(type) + ";\n";
    }

    for (ReferencedSymbols::const_iterator attribute = mReferencedAttributes.begin(); attribute != mReferencedAttributes.end(); attribute++)
    {
        const TType &type = attribute->second->getType();
        const TString &name = attribute->second->getSymbol();

        attributes += "static " + typeString(type) + " " + decorate(name) + arrayString(type) + " = " + initializer(type) + ";\n";
    }

    if (shaderType == SH_FRAGMENT_SHADER)
    {
        TExtensionBehavior::const_iterator iter = mContext.extensionBehavior().find("GL_EXT_draw_buffers");
        const bool usingMRTExtension = (iter != mContext.extensionBehavior().end() && (iter->second == EBhEnable || iter->second == EBhRequire));

        const unsigned int numColorValues = usingMRTExtension ? mNumRenderTargets : 1;

        out << "// Varyings\n";
        out <<  varyings;
        out << "\n"
               "static float4 gl_Color[" << numColorValues << "] =\n"
               "{\n";
        for (unsigned int i = 0; i < numColorValues; i++)
        {
            out << "    float4(0, 0, 0, 0)";
            if (i + 1 != numColorValues)
            {
                out << ",";
            }
            out << "\n";
        }
        out << "};\n";

        if (mUsesFragDepth)
        {
            out << "static float gl_Depth = 0.0;\n";
        }

        if (mUsesFragCoord)
        {
            out << "static float4 gl_FragCoord = float4(0, 0, 0, 0);\n";
        }

        if (mUsesPointCoord)
        {
            out << "static float2 gl_PointCoord = float2(0.5, 0.5);\n";
        }

        if (mUsesFrontFacing)
        {
            out << "static bool gl_FrontFacing = false;\n";
        }

        out << "\n";

        if (mUsesDepthRange)
        {
            out << "struct gl_DepthRangeParameters\n"
                   "{\n"
                   "    float near;\n"
                   "    float far;\n"
                   "    float diff;\n"
                   "};\n"
                   "\n";
        }

        if (mOutputType == SH_HLSL11_OUTPUT)
        {
            out << "cbuffer DriverConstants : register(b1)\n"
                   "{\n";

            if (mUsesDepthRange)
            {
                out << "    float3 dx_DepthRange : packoffset(c0);\n";
            }

            if (mUsesFragCoord)
            {
                out << "    float4 dx_ViewCoords : packoffset(c1);\n";
            }

            if (mUsesFragCoord || mUsesFrontFacing)
            {
                out << "    float3 dx_DepthFront : packoffset(c2);\n";
            }

            out << "};\n";
        }
        else
        {
            if (mUsesDepthRange)
            {
                out << "uniform float3 dx_DepthRange : register(c0);";
            }

            if (mUsesFragCoord)
            {
                out << "uniform float4 dx_ViewCoords : register(c1);\n";
            }

            if (mUsesFragCoord || mUsesFrontFacing)
            {
                out << "uniform float3 dx_DepthFront : register(c2);\n";
            }
        }

        out << "\n";

        if (mUsesDepthRange)
        {
            out << "static gl_DepthRangeParameters gl_DepthRange = {dx_DepthRange.x, dx_DepthRange.y, dx_DepthRange.z};\n"
                   "\n";
        }
        
        out <<  uniforms;
        out << "\n";

        if (mUsesTexture2D)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2D(sampler2D s, float2 t)\n"
                       "{\n"
                       "    return tex2D(s, t);\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2D(Texture2D t, SamplerState s, float2 uv)\n"
                       "{\n"
                       "    return t.Sample(s, uv);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2D_bias)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2D(sampler2D s, float2 t, float bias)\n"
                       "{\n"
                       "    return tex2Dbias(s, float4(t.x, t.y, 0, bias));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2D(Texture2D t, SamplerState s, float2 uv, float bias)\n"
                       "{\n"
                       "    return t.SampleBias(s, uv, bias);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DProj)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DProj(sampler2D s, float3 t)\n"
                       "{\n"
                       "    return tex2Dproj(s, float4(t.x, t.y, 0, t.z));\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProj(sampler2D s, float4 t)\n"
                       "{\n"
                       "    return tex2Dproj(s, t);\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DProj(Texture2D t, SamplerState s, float3 uvw)\n"
                       "{\n"
                       "    return t.Sample(s, float2(uvw.x / uvw.z, uvw.y / uvw.z));\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProj(Texture2D t, SamplerState s, float4 uvw)\n"
                       "{\n"
                       "    return t.Sample(s, float2(uvw.x / uvw.w, uvw.y / uvw.w));\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DProj_bias)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DProj(sampler2D s, float3 t, float bias)\n"
                       "{\n"
                       "    return tex2Dbias(s, float4(t.x / t.z, t.y / t.z, 0, bias));\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProj(sampler2D s, float4 t, float bias)\n"
                       "{\n"
                       "    return tex2Dbias(s, float4(t.x / t.w, t.y / t.w, 0, bias));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DProj(Texture2D t, SamplerState s, float3 uvw, float bias)\n"
                       "{\n"
                       "    return t.SampleBias(s, float2(uvw.x / uvw.z, uvw.y / uvw.z), bias);\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProj(Texture2D t, SamplerState s, float4 uvw, float bias)\n"
                       "{\n"
                       "    return t.SampleBias(s, float2(uvw.x / uvw.w, uvw.y / uvw.w), bias);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTextureCube)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_textureCube(samplerCUBE s, float3 t)\n"
                       "{\n"
                       "    return texCUBE(s, t);\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_textureCube(TextureCube t, SamplerState s, float3 uvw)\n"
                       "{\n"
                       "    return t.Sample(s, uvw);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTextureCube_bias)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_textureCube(samplerCUBE s, float3 t, float bias)\n"
                       "{\n"
                       "    return texCUBEbias(s, float4(t.x, t.y, t.z, bias));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_textureCube(TextureCube t, SamplerState s, float3 uvw, float bias)\n"
                       "{\n"
                       "    return t.SampleBias(s, uvw, bias);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        // These *Lod0 intrinsics are not available in GL fragment shaders.
        // They are used to sample using discontinuous texture coordinates.
        if (mUsesTexture2DLod0)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DLod0(sampler2D s, float2 t)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x, t.y, 0, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DLod0(Texture2D t, SamplerState s, float2 uv)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uv, 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DLod0_bias)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DLod0(sampler2D s, float2 t, float bias)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x, t.y, 0, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DLod0(Texture2D t, SamplerState s, float2 uv, float bias)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uv, 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DProjLod0)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DProjLod0(sampler2D s, float3 t)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.z, t.y / t.z, 0, 0));\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProjLod(sampler2D s, float4 t)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.w, t.y / t.w, 0, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DProjLod0(Texture2D t, SamplerState s, float3 uvw)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.z, uvw.y / uvw.z), 0);\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProjLod0(Texture2D t, SamplerState s, float4 uvw)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.w, uvw.y / uvw.w), 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DProjLod0_bias)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DProjLod0_bias(sampler2D s, float3 t, float bias)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.z, t.y / t.z, 0, 0));\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProjLod_bias(sampler2D s, float4 t, float bias)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.w, t.y / t.w, 0, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DProjLod_bias(Texture2D t, SamplerState s, float3 uvw, float bias)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.z, uvw.y / uvw.z), 0);\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProjLod_bias(Texture2D t, SamplerState s, float4 uvw, float bias)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.w, uvw.y / uvw.w), 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTextureCubeLod0)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_textureCubeLod0(samplerCUBE s, float3 t)\n"
                       "{\n"
                       "    return texCUBElod(s, float4(t.x, t.y, t.z, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_textureCubeLod0(TextureCube t, SamplerState s, float3 uvw)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uvw, 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTextureCubeLod0_bias)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_textureCubeLod0(samplerCUBE s, float3 t, float bias)\n"
                       "{\n"
                       "    return texCUBElod(s, float4(t.x, t.y, t.z, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_textureCubeLod0(TextureCube t, SamplerState s, float3 uvw, float bias)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uvw, 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (usingMRTExtension && mNumRenderTargets > 1)
        {
            out << "#define GL_USES_MRT\n";
        }

        if (mUsesFragColor)
        {
            out << "#define GL_USES_FRAG_COLOR\n";
        }

        if (mUsesFragData)
        {
            out << "#define GL_USES_FRAG_DATA\n";
        }
    }
    else   // Vertex shader
    {
        out << "// Attributes\n";
        out <<  attributes;
        out << "\n"
               "static float4 gl_Position = float4(0, 0, 0, 0);\n";
        
        if (mUsesPointSize)
        {
            out << "static float gl_PointSize = float(1);\n";
        }

        out << "\n"
               "// Varyings\n";
        out <<  varyings;
        out << "\n";

        if (mUsesDepthRange)
        {
            out << "struct gl_DepthRangeParameters\n"
                   "{\n"
                   "    float near;\n"
                   "    float far;\n"
                   "    float diff;\n"
                   "};\n"
                   "\n";
        }

        if (mOutputType == SH_HLSL11_OUTPUT)
        {
            if (mUsesDepthRange)
            {
                out << "cbuffer DriverConstants : register(b1)\n"
                       "{\n"
                       "    float3 dx_DepthRange : packoffset(c0);\n"
                       "};\n"
                       "\n";
            }
        }
        else
        {
            if (mUsesDepthRange)
            {
                out << "uniform float3 dx_DepthRange : register(c0);\n";
            }

            out << "uniform float4 dx_ViewAdjust : register(c1);\n"
                   "\n";
        }

        if (mUsesDepthRange)
        {
            out << "static gl_DepthRangeParameters gl_DepthRange = {dx_DepthRange.x, dx_DepthRange.y, dx_DepthRange.z};\n"
                   "\n";
        }

        out << uniforms;
        out << "\n";
        
        if (mUsesTexture2D)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2D(sampler2D s, float2 t)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x, t.y, 0, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2D(Texture2D t, SamplerState s, float2 uv)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uv, 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DLod)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DLod(sampler2D s, float2 t, float lod)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x, t.y, 0, lod));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DLod(Texture2D t, SamplerState s, float2 uv, float lod)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uv, lod);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DProj)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DProj(sampler2D s, float3 t)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.z, t.y / t.z, 0, 0));\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProj(sampler2D s, float4 t)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.w, t.y / t.w, 0, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DProj(Texture2D t, SamplerState s, float3 uvw)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.z, uvw.y / uvw.z), 0);\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProj(Texture2D t, SamplerState s, float4 uvw)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.w, uvw.y / uvw.w), 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTexture2DProjLod)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_texture2DProjLod(sampler2D s, float3 t, float lod)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.z, t.y / t.z, 0, lod));\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProjLod(sampler2D s, float4 t, float lod)\n"
                       "{\n"
                       "    return tex2Dlod(s, float4(t.x / t.w, t.y / t.w, 0, lod));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_texture2DProj(Texture2D t, SamplerState s, float3 uvw, float lod)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.z, uvw.y / uvw.z), lod);\n"
                       "}\n"
                       "\n"
                       "float4 gl_texture2DProj(Texture2D t, SamplerState s, float4 uvw)\n"
                       "{\n"
                       "    return t.SampleLevel(s, float2(uvw.x / uvw.w, uvw.y / uvw.w), lod);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTextureCube)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_textureCube(samplerCUBE s, float3 t)\n"
                       "{\n"
                       "    return texCUBElod(s, float4(t.x, t.y, t.z, 0));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_textureCube(TextureCube t, SamplerState s, float3 uvw)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uvw, 0);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }

        if (mUsesTextureCubeLod)
        {
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                out << "float4 gl_textureCubeLod(samplerCUBE s, float3 t, float lod)\n"
                       "{\n"
                       "    return texCUBElod(s, float4(t.x, t.y, t.z, lod));\n"
                       "}\n"
                       "\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                out << "float4 gl_textureCubeLod(TextureCube t, SamplerState s, float3 uvw, float lod)\n"
                       "{\n"
                       "    return t.SampleLevel(s, uvw, lod);\n"
                       "}\n"
                       "\n";
            }
            else UNREACHABLE();
        }
    }

    if (mUsesFragCoord)
    {
        out << "#define GL_USES_FRAG_COORD\n";
    }

    if (mUsesPointCoord)
    {
        out << "#define GL_USES_POINT_COORD\n";
    }

    if (mUsesFrontFacing)
    {
        out << "#define GL_USES_FRONT_FACING\n";
    }

    if (mUsesPointSize)
    {
        out << "#define GL_USES_POINT_SIZE\n";
    }

    if (mUsesFragDepth)
    {
        out << "#define GL_USES_FRAG_DEPTH\n";
    }

    if (mUsesDepthRange)
    {
        out << "#define GL_USES_DEPTH_RANGE\n";
    }

    if (mUsesXor)
    {
        out << "bool xor(bool p, bool q)\n"
               "{\n"
               "    return (p || q) && !(p && q);\n"
               "}\n"
               "\n";
    }

    if (mUsesMod1)
    {
        out << "float mod(float x, float y)\n"
               "{\n"
               "    return x - y * floor(x / y);\n"
               "}\n"
               "\n";
    }

    if (mUsesMod2v)
    {
        out << "float2 mod(float2 x, float2 y)\n"
               "{\n"
               "    return x - y * floor(x / y);\n"
               "}\n"
               "\n";
    }

    if (mUsesMod2f)
    {
        out << "float2 mod(float2 x, float y)\n"
               "{\n"
               "    return x - y * floor(x / y);\n"
               "}\n"
               "\n";
    }
    
    if (mUsesMod3v)
    {
        out << "float3 mod(float3 x, float3 y)\n"
               "{\n"
               "    return x - y * floor(x / y);\n"
               "}\n"
               "\n";
    }

    if (mUsesMod3f)
    {
        out << "float3 mod(float3 x, float y)\n"
               "{\n"
               "    return x - y * floor(x / y);\n"
               "}\n"
               "\n";
    }

    if (mUsesMod4v)
    {
        out << "float4 mod(float4 x, float4 y)\n"
               "{\n"
               "    return x - y * floor(x / y);\n"
               "}\n"
               "\n";
    }

    if (mUsesMod4f)
    {
        out << "float4 mod(float4 x, float y)\n"
               "{\n"
               "    return x - y * floor(x / y);\n"
               "}\n"
               "\n";
    }

    if (mUsesFaceforward1)
    {
        out << "float faceforward(float N, float I, float Nref)\n"
               "{\n"
               "    if(dot(Nref, I) >= 0)\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesFaceforward2)
    {
        out << "float2 faceforward(float2 N, float2 I, float2 Nref)\n"
               "{\n"
               "    if(dot(Nref, I) >= 0)\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesFaceforward3)
    {
        out << "float3 faceforward(float3 N, float3 I, float3 Nref)\n"
               "{\n"
               "    if(dot(Nref, I) >= 0)\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesFaceforward4)
    {
        out << "float4 faceforward(float4 N, float4 I, float4 Nref)\n"
               "{\n"
               "    if(dot(Nref, I) >= 0)\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesEqualMat2)
    {
        out << "bool equal(float2x2 m, float2x2 n)\n"
               "{\n"
               "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] &&\n"
               "           m[1][0] == n[1][0] && m[1][1] == n[1][1];\n"
               "}\n";
    }

    if (mUsesEqualMat3)
    {
        out << "bool equal(float3x3 m, float3x3 n)\n"
               "{\n"
               "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] && m[0][2] == n[0][2] &&\n"
               "           m[1][0] == n[1][0] && m[1][1] == n[1][1] && m[1][2] == n[1][2] &&\n"
               "           m[2][0] == n[2][0] && m[2][1] == n[2][1] && m[2][2] == n[2][2];\n"
               "}\n";
    }

    if (mUsesEqualMat4)
    {
        out << "bool equal(float4x4 m, float4x4 n)\n"
               "{\n"
               "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] && m[0][2] == n[0][2] && m[0][3] == n[0][3] &&\n"
               "           m[1][0] == n[1][0] && m[1][1] == n[1][1] && m[1][2] == n[1][2] && m[1][3] == n[1][3] &&\n"
               "           m[2][0] == n[2][0] && m[2][1] == n[2][1] && m[2][2] == n[2][2] && m[2][3] == n[2][3] &&\n"
               "           m[3][0] == n[3][0] && m[3][1] == n[3][1] && m[3][2] == n[3][2] && m[3][3] == n[3][3];\n"
               "}\n";
    }

    if (mUsesEqualVec2)
    {
        out << "bool equal(float2 v, float2 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y;\n"
               "}\n";
    }

    if (mUsesEqualVec3)
    {
        out << "bool equal(float3 v, float3 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z;\n"
               "}\n";
    }

    if (mUsesEqualVec4)
    {
        out << "bool equal(float4 v, float4 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z && v.w == u.w;\n"
               "}\n";
    }

    if (mUsesEqualIVec2)
    {
        out << "bool equal(int2 v, int2 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y;\n"
               "}\n";
    }

    if (mUsesEqualIVec3)
    {
        out << "bool equal(int3 v, int3 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z;\n"
               "}\n";
    }

    if (mUsesEqualIVec4)
    {
        out << "bool equal(int4 v, int4 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z && v.w == u.w;\n"
               "}\n";
    }

    if (mUsesEqualBVec2)
    {
        out << "bool equal(bool2 v, bool2 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y;\n"
               "}\n";
    }

    if (mUsesEqualBVec3)
    {
        out << "bool equal(bool3 v, bool3 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z;\n"
               "}\n";
    }

    if (mUsesEqualBVec4)
    {
        out << "bool equal(bool4 v, bool4 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z && v.w == u.w;\n"
               "}\n";
    }

    if (mUsesAtan2_1)
    {
        out << "float atanyx(float y, float x)\n"
               "{\n"
               "    if(x == 0 && y == 0) x = 1;\n"   // Avoid producing a NaN
               "    return atan2(y, x);\n"
               "}\n";
    }

    if (mUsesAtan2_2)
    {
        out << "float2 atanyx(float2 y, float2 x)\n"
               "{\n"
               "    if(x[0] == 0 && y[0] == 0) x[0] = 1;\n"
               "    if(x[1] == 0 && y[1] == 0) x[1] = 1;\n"
               "    return float2(atan2(y[0], x[0]), atan2(y[1], x[1]));\n"
               "}\n";
    }

    if (mUsesAtan2_3)
    {
        out << "float3 atanyx(float3 y, float3 x)\n"
               "{\n"
               "    if(x[0] == 0 && y[0] == 0) x[0] = 1;\n"
               "    if(x[1] == 0 && y[1] == 0) x[1] = 1;\n"
               "    if(x[2] == 0 && y[2] == 0) x[2] = 1;\n"
               "    return float3(atan2(y[0], x[0]), atan2(y[1], x[1]), atan2(y[2], x[2]));\n"
               "}\n";
    }

    if (mUsesAtan2_4)
    {
        out << "float4 atanyx(float4 y, float4 x)\n"
               "{\n"
               "    if(x[0] == 0 && y[0] == 0) x[0] = 1;\n"
               "    if(x[1] == 0 && y[1] == 0) x[1] = 1;\n"
               "    if(x[2] == 0 && y[2] == 0) x[2] = 1;\n"
               "    if(x[3] == 0 && y[3] == 0) x[3] = 1;\n"
               "    return float4(atan2(y[0], x[0]), atan2(y[1], x[1]), atan2(y[2], x[2]), atan2(y[3], x[3]));\n"
               "}\n";
    }
}

void OutputHLSL::visitSymbol(TIntermSymbol *node)
{
    TInfoSinkBase &out = mBody;

    TString name = node->getSymbol();

    if (name == "gl_FragColor")
    {
        out << "gl_Color[0]";
        mUsesFragColor = true;
    }
    else if (name == "gl_FragData")
    {
        out << "gl_Color";
        mUsesFragData = true;
    }
    else if (name == "gl_DepthRange")
    {
        mUsesDepthRange = true;
        out << name;
    }
    else if (name == "gl_FragCoord")
    {
        mUsesFragCoord = true;
        out << name;
    }
    else if (name == "gl_PointCoord")
    {
        mUsesPointCoord = true;
        out << name;
    }
    else if (name == "gl_FrontFacing")
    {
        mUsesFrontFacing = true;
        out << name;
    }
    else if (name == "gl_PointSize")
    {
        mUsesPointSize = true;
        out << name;
    }
    else if (name == "gl_FragDepthEXT")
    {
        mUsesFragDepth = true;
        out << "gl_Depth";
    }
    else
    {
        TQualifier qualifier = node->getQualifier();

        if (qualifier == EvqUniform)
        {
            mReferencedUniforms[name] = node;
            out << decorateUniform(name, node->getType());
        }
        else if (qualifier == EvqAttribute)
        {
            mReferencedAttributes[name] = node;
            out << decorate(name);
        }
        else if (qualifier == EvqVaryingOut || qualifier == EvqInvariantVaryingOut || qualifier == EvqVaryingIn || qualifier == EvqInvariantVaryingIn)
        {
            mReferencedVaryings[name] = node;
            out << decorate(name);
        }
        else
        {
            out << decorate(name);
        }
    }
}

bool OutputHLSL::visitBinary(Visit visit, TIntermBinary *node)
{
    TInfoSinkBase &out = mBody;

    switch (node->getOp())
    {
      case EOpAssign:                  outputTriplet(visit, "(", " = ", ")");           break;
      case EOpInitialize:
        if (visit == PreVisit)
        {
            // GLSL allows to write things like "float x = x;" where a new variable x is defined
            // and the value of an existing variable x is assigned. HLSL uses C semantics (the
            // new variable is created before the assignment is evaluated), so we need to convert
            // this to "float t = x, x = t;".

            TIntermSymbol *symbolNode = node->getLeft()->getAsSymbolNode();
            TIntermTyped *expression = node->getRight();

            sh::SearchSymbol searchSymbol(symbolNode->getSymbol());
            expression->traverse(&searchSymbol);
            bool sameSymbol = searchSymbol.foundMatch();

            if (sameSymbol)
            {
                // Type already printed
                out << "t" + str(mUniqueIndex) + " = ";
                expression->traverse(this);
                out << ", ";
                symbolNode->traverse(this);
                out << " = t" + str(mUniqueIndex);

                mUniqueIndex++;
                return false;
            }
        }
        else if (visit == InVisit)
        {
            out << " = ";
        }
        break;
      case EOpAddAssign:               outputTriplet(visit, "(", " += ", ")");          break;
      case EOpSubAssign:               outputTriplet(visit, "(", " -= ", ")");          break;
      case EOpMulAssign:               outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpVectorTimesScalarAssign: outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpMatrixTimesScalarAssign: outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpVectorTimesMatrixAssign:
        if (visit == PreVisit)
        {
            out << "(";
        }
        else if (visit == InVisit)
        {
            out << " = mul(";
            node->getLeft()->traverse(this);
            out << ", transpose(";   
        }
        else
        {
            out << ")))";
        }
        break;
      case EOpMatrixTimesMatrixAssign:
        if (visit == PreVisit)
        {
            out << "(";
        }
        else if (visit == InVisit)
        {
            out << " = mul(";
            node->getLeft()->traverse(this);
            out << ", ";   
        }
        else
        {
            out << "))";
        }
        break;
      case EOpDivAssign:               outputTriplet(visit, "(", " /= ", ")");          break;
      case EOpIndexDirect:             outputTriplet(visit, "", "[", "]");              break;
      case EOpIndexIndirect:           outputTriplet(visit, "", "[", "]");              break;
      case EOpIndexDirectStruct:
        if (visit == InVisit)
        {
            const TStructure* structure = node->getLeft()->getType().getStruct();
            const TIntermConstantUnion* index = node->getRight()->getAsConstantUnion();
            const TField* field = structure->fields()[index->getIConst(0)];
            out << "." + decorateField(field->name(), node->getLeft()->getType());

            return false;
        }
        break;
      case EOpVectorSwizzle:
        if (visit == InVisit)
        {
            out << ".";

            TIntermAggregate *swizzle = node->getRight()->getAsAggregate();

            if (swizzle)
            {
                TIntermSequence &sequence = swizzle->getSequence();

                for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
                {
                    TIntermConstantUnion *element = (*sit)->getAsConstantUnion();

                    if (element)
                    {
                        int i = element->getIConst(0);

                        switch (i)
                        {
                        case 0: out << "x"; break;
                        case 1: out << "y"; break;
                        case 2: out << "z"; break;
                        case 3: out << "w"; break;
                        default: UNREACHABLE();
                        }
                    }
                    else UNREACHABLE();
                }
            }
            else UNREACHABLE();

            return false;   // Fully processed
        }
        break;
      case EOpAdd:               outputTriplet(visit, "(", " + ", ")"); break;
      case EOpSub:               outputTriplet(visit, "(", " - ", ")"); break;
      case EOpMul:               outputTriplet(visit, "(", " * ", ")"); break;
      case EOpDiv:               outputTriplet(visit, "(", " / ", ")"); break;
      case EOpEqual:
      case EOpNotEqual:
        if (node->getLeft()->isScalar())
        {
            if (node->getOp() == EOpEqual)
            {
                outputTriplet(visit, "(", " == ", ")");
            }
            else
            {
                outputTriplet(visit, "(", " != ", ")");
            }
        }
        else if (node->getLeft()->getBasicType() == EbtStruct)
        {
            if (node->getOp() == EOpEqual)
            {
                out << "(";
            }
            else
            {
                out << "!(";
            }

            const TFieldList &fields = node->getLeft()->getType().getStruct()->fields();

            for (size_t i = 0; i < fields.size(); i++)
            {
                const TField *field = fields[i];

                node->getLeft()->traverse(this);
                out << "." + decorateField(field->name(), node->getLeft()->getType()) + " == ";
                node->getRight()->traverse(this);
                out << "." + decorateField(field->name(), node->getLeft()->getType());

                if (i < fields.size() - 1)
                {
                    out << " && ";
                }
            }

            out << ")";

            return false;
        }
        else
        {
            if (node->getLeft()->isMatrix())
            {
                switch (node->getLeft()->getNominalSize())
                {
                  case 2: mUsesEqualMat2 = true; break;
                  case 3: mUsesEqualMat3 = true; break;
                  case 4: mUsesEqualMat4 = true; break;
                  default: UNREACHABLE();
                }
            }
            else if (node->getLeft()->isVector())
            {
                switch (node->getLeft()->getBasicType())
                {
                  case EbtFloat:
                    switch (node->getLeft()->getNominalSize())
                    {
                      case 2: mUsesEqualVec2 = true; break;
                      case 3: mUsesEqualVec3 = true; break;
                      case 4: mUsesEqualVec4 = true; break;
                      default: UNREACHABLE();
                    }
                    break;
                  case EbtInt:
                    switch (node->getLeft()->getNominalSize())
                    {
                      case 2: mUsesEqualIVec2 = true; break;
                      case 3: mUsesEqualIVec3 = true; break;
                      case 4: mUsesEqualIVec4 = true; break;
                      default: UNREACHABLE();
                    }
                    break;
                  case EbtBool:
                    switch (node->getLeft()->getNominalSize())
                    {
                      case 2: mUsesEqualBVec2 = true; break;
                      case 3: mUsesEqualBVec3 = true; break;
                      case 4: mUsesEqualBVec4 = true; break;
                      default: UNREACHABLE();
                    }
                    break;
                  default: UNREACHABLE();
                }
            }
            else UNREACHABLE();

            if (node->getOp() == EOpEqual)
            {
                outputTriplet(visit, "equal(", ", ", ")");
            }
            else
            {
                outputTriplet(visit, "!equal(", ", ", ")");
            }
        }
        break;
      case EOpLessThan:          outputTriplet(visit, "(", " < ", ")");   break;
      case EOpGreaterThan:       outputTriplet(visit, "(", " > ", ")");   break;
      case EOpLessThanEqual:     outputTriplet(visit, "(", " <= ", ")");  break;
      case EOpGreaterThanEqual:  outputTriplet(visit, "(", " >= ", ")");  break;
      case EOpVectorTimesScalar: outputTriplet(visit, "(", " * ", ")");   break;
      case EOpMatrixTimesScalar: outputTriplet(visit, "(", " * ", ")");   break;
      case EOpVectorTimesMatrix: outputTriplet(visit, "mul(", ", transpose(", "))"); break;
      case EOpMatrixTimesVector: outputTriplet(visit, "mul(transpose(", "), ", ")"); break;
      case EOpMatrixTimesMatrix: outputTriplet(visit, "transpose(mul(transpose(", "), transpose(", ")))"); break;
      case EOpLogicalOr:
        out << "s" << mUnfoldShortCircuit->getNextTemporaryIndex();
        return false;
      case EOpLogicalXor:
        mUsesXor = true;
        outputTriplet(visit, "xor(", ", ", ")");
        break;
      case EOpLogicalAnd:
        out << "s" << mUnfoldShortCircuit->getNextTemporaryIndex();
        return false;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitUnary(Visit visit, TIntermUnary *node)
{
    switch (node->getOp())
    {
      case EOpNegative:         outputTriplet(visit, "(-", "", ")");  break;
      case EOpVectorLogicalNot: outputTriplet(visit, "(!", "", ")");  break;
      case EOpLogicalNot:       outputTriplet(visit, "(!", "", ")");  break;
      case EOpPostIncrement:    outputTriplet(visit, "(", "", "++)"); break;
      case EOpPostDecrement:    outputTriplet(visit, "(", "", "--)"); break;
      case EOpPreIncrement:     outputTriplet(visit, "(++", "", ")"); break;
      case EOpPreDecrement:     outputTriplet(visit, "(--", "", ")"); break;
      case EOpConvIntToBool:
      case EOpConvFloatToBool:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "bool(", "", ")");  break;
          case 2:    outputTriplet(visit, "bool2(", "", ")"); break;
          case 3:    outputTriplet(visit, "bool3(", "", ")"); break;
          case 4:    outputTriplet(visit, "bool4(", "", ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpConvBoolToFloat:
      case EOpConvIntToFloat:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "float(", "", ")");  break;
          case 2:    outputTriplet(visit, "float2(", "", ")"); break;
          case 3:    outputTriplet(visit, "float3(", "", ")"); break;
          case 4:    outputTriplet(visit, "float4(", "", ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpConvFloatToInt:
      case EOpConvBoolToInt:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "int(", "", ")");  break;
          case 2:    outputTriplet(visit, "int2(", "", ")"); break;
          case 3:    outputTriplet(visit, "int3(", "", ")"); break;
          case 4:    outputTriplet(visit, "int4(", "", ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpRadians:          outputTriplet(visit, "radians(", "", ")");   break;
      case EOpDegrees:          outputTriplet(visit, "degrees(", "", ")");   break;
      case EOpSin:              outputTriplet(visit, "sin(", "", ")");       break;
      case EOpCos:              outputTriplet(visit, "cos(", "", ")");       break;
      case EOpTan:              outputTriplet(visit, "tan(", "", ")");       break;
      case EOpAsin:             outputTriplet(visit, "asin(", "", ")");      break;
      case EOpAcos:             outputTriplet(visit, "acos(", "", ")");      break;
      case EOpAtan:             outputTriplet(visit, "atan(", "", ")");      break;
      case EOpExp:              outputTriplet(visit, "exp(", "", ")");       break;
      case EOpLog:              outputTriplet(visit, "log(", "", ")");       break;
      case EOpExp2:             outputTriplet(visit, "exp2(", "", ")");      break;
      case EOpLog2:             outputTriplet(visit, "log2(", "", ")");      break;
      case EOpSqrt:             outputTriplet(visit, "sqrt(", "", ")");      break;
      case EOpInverseSqrt:      outputTriplet(visit, "rsqrt(", "", ")");     break;
      case EOpAbs:              outputTriplet(visit, "abs(", "", ")");       break;
      case EOpSign:             outputTriplet(visit, "sign(", "", ")");      break;
      case EOpFloor:            outputTriplet(visit, "floor(", "", ")");     break;
      case EOpCeil:             outputTriplet(visit, "ceil(", "", ")");      break;
      case EOpFract:            outputTriplet(visit, "frac(", "", ")");      break;
      case EOpLength:           outputTriplet(visit, "length(", "", ")");    break;
      case EOpNormalize:        outputTriplet(visit, "normalize(", "", ")"); break;
      case EOpDFdx:
        if(mInsideDiscontinuousLoop || mOutputLod0Function)
        {
            outputTriplet(visit, "(", "", ", 0.0)");
        }
        else
        {
            outputTriplet(visit, "ddx(", "", ")");
        }
        break;
      case EOpDFdy:
        if(mInsideDiscontinuousLoop || mOutputLod0Function)
        {
            outputTriplet(visit, "(", "", ", 0.0)");
        }
        else
        {
           outputTriplet(visit, "ddy(", "", ")");
        }
        break;
      case EOpFwidth:
        if(mInsideDiscontinuousLoop || mOutputLod0Function)
        {
            outputTriplet(visit, "(", "", ", 0.0)");
        }
        else
        {
            outputTriplet(visit, "fwidth(", "", ")");
        }
        break;
      case EOpAny:              outputTriplet(visit, "any(", "", ")");       break;
      case EOpAll:              outputTriplet(visit, "all(", "", ")");       break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitAggregate(Visit visit, TIntermAggregate *node)
{
    TInfoSinkBase &out = mBody;

    switch (node->getOp())
    {
      case EOpSequence:
        {
            if (mInsideFunction)
            {
                outputLineDirective(node->getLine().first_line);
                out << "{\n";

                mScopeDepth++;

                if (mScopeBracket.size() < mScopeDepth)
                {
                    mScopeBracket.push_back(0);   // New scope level
                }
                else
                {
                    mScopeBracket[mScopeDepth - 1]++;   // New scope at existing level
                }
            }

            for (TIntermSequence::iterator sit = node->getSequence().begin(); sit != node->getSequence().end(); sit++)
            {
                outputLineDirective((*sit)->getLine().first_line);

                traverseStatements(*sit);

                out << ";\n";
            }

            if (mInsideFunction)
            {
                outputLineDirective(node->getLine().last_line);
                out << "}\n";

                mScopeDepth--;
            }

            return false;
        }
      case EOpDeclaration:
        if (visit == PreVisit)
        {
            TIntermSequence &sequence = node->getSequence();
            TIntermTyped *variable = sequence[0]->getAsTyped();

            if (variable && (variable->getQualifier() == EvqTemporary || variable->getQualifier() == EvqGlobal))
            {
                if (variable->getType().getStruct())
                {
                    addConstructor(variable->getType(), scopedStruct(variable->getType().getStruct()->name()), NULL);
                }

                if (!variable->getAsSymbolNode() || variable->getAsSymbolNode()->getSymbol() != "")   // Variable declaration
                {
                    if (!mInsideFunction)
                    {
                        out << "static ";
                    }

                    out << typeString(variable->getType()) + " ";

                    for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
                    {
                        TIntermSymbol *symbol = (*sit)->getAsSymbolNode();

                        if (symbol)
                        {
                            symbol->traverse(this);
                            out << arrayString(symbol->getType());
                            out << " = " + initializer(variable->getType());
                        }
                        else
                        {
                            (*sit)->traverse(this);
                        }

                        if (*sit != sequence.back())
                        {
                            out << ", ";
                        }
                    }
                }
                else if (variable->getAsSymbolNode() && variable->getAsSymbolNode()->getSymbol() == "")   // Type (struct) declaration
                {
                    // Already added to constructor map
                }
                else UNREACHABLE();
            }
            else if (variable && (variable->getQualifier() == EvqVaryingOut || variable->getQualifier() == EvqInvariantVaryingOut))
            {
                for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
                {
                    TIntermSymbol *symbol = (*sit)->getAsSymbolNode();

                    if (symbol)
                    {
                        // Vertex (output) varyings which are declared but not written to should still be declared to allow successful linking
                        mReferencedVaryings[symbol->getSymbol()] = symbol;
                    }
                    else
                    {
                        (*sit)->traverse(this);
                    }
                }
            }

            return false;
        }
        else if (visit == InVisit)
        {
            out << ", ";
        }
        break;
      case EOpPrototype:
        if (visit == PreVisit)
        {
            out << typeString(node->getType()) << " " << decorate(node->getName()) << (mOutputLod0Function ? "Lod0(" : "(");

            TIntermSequence &arguments = node->getSequence();

            for (unsigned int i = 0; i < arguments.size(); i++)
            {
                TIntermSymbol *symbol = arguments[i]->getAsSymbolNode();

                if (symbol)
                {
                    out << argumentString(symbol);

                    if (i < arguments.size() - 1)
                    {
                        out << ", ";
                    }
                }
                else UNREACHABLE();
            }

            out << ");\n";

            // Also prototype the Lod0 variant if needed
            if (mContainsLoopDiscontinuity && !mOutputLod0Function)
            {
                mOutputLod0Function = true;
                node->traverse(this);
                mOutputLod0Function = false;
            }

            return false;
        }
        break;
      case EOpComma:            outputTriplet(visit, "(", ", ", ")");                break;
      case EOpFunction:
        {
            TString name = TFunction::unmangleName(node->getName());

            out << typeString(node->getType()) << " ";

            if (name == "main")
            {
                out << "gl_main(";
            }
            else
            {
                out << decorate(name) << (mOutputLod0Function ? "Lod0(" : "(");
            }

            TIntermSequence &sequence = node->getSequence();
            TIntermSequence &arguments = sequence[0]->getAsAggregate()->getSequence();

            for (unsigned int i = 0; i < arguments.size(); i++)
            {
                TIntermSymbol *symbol = arguments[i]->getAsSymbolNode();

                if (symbol)
                {
                    if (symbol->getType().getStruct())
                    {
                        addConstructor(symbol->getType(), scopedStruct(symbol->getType().getStruct()->name()), NULL);
                    }

                    out << argumentString(symbol);

                    if (i < arguments.size() - 1)
                    {
                        out << ", ";
                    }
                }
                else UNREACHABLE();
            }

            out << ")\n"
                "{\n";
            
            if (sequence.size() > 1)
            {
                mInsideFunction = true;
                sequence[1]->traverse(this);
                mInsideFunction = false;
            }
            
            out << "}\n";

            if (mContainsLoopDiscontinuity && !mOutputLod0Function)
            {
                if (name != "main")
                {
                    mOutputLod0Function = true;
                    node->traverse(this);
                    mOutputLod0Function = false;
                }
            }

            return false;
        }
        break;
      case EOpFunctionCall:
        {
            TString name = TFunction::unmangleName(node->getName());
            bool lod0 = mInsideDiscontinuousLoop || mOutputLod0Function;

            if (node->isUserDefined())
            {
                out << decorate(name) << (lod0 ? "Lod0(" : "(");
            }
            else
            {
                if (name == "texture2D")
                {
                    if (!lod0)
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTexture2D = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTexture2D_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_texture2D(";
                    }
                    else
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTexture2DLod0 = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTexture2DLod0_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_texture2DLod0(";
                    }
                }
                else if (name == "texture2DProj")
                {
                    if (!lod0)
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTexture2DProj = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTexture2DProj_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_texture2DProj(";
                    }
                    else
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTexture2DProjLod0 = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTexture2DProjLod0_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_texture2DProjLod0(";
                    }
                }
                else if (name == "textureCube")
                {
                    if (!lod0)
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTextureCube = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTextureCube_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_textureCube(";
                    }
                    else
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTextureCubeLod0 = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTextureCubeLod0_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_textureCubeLod0(";
                    }
                }
                else if (name == "texture2DLod")
                {
                    if (node->getSequence().size() == 3)
                    {
                        mUsesTexture2DLod = true;
                    }
                    else UNREACHABLE();

                    out << "gl_texture2DLod(";
                }
                else if (name == "texture2DProjLod")
                {
                    if (node->getSequence().size() == 3)
                    {
                        mUsesTexture2DProjLod = true;
                    }
                    else UNREACHABLE();

                    out << "gl_texture2DProjLod(";
                }
                else if (name == "textureCubeLod")
                {
                    if (node->getSequence().size() == 3)
                    {
                        mUsesTextureCubeLod = true;
                    }
                    else UNREACHABLE();

                    out << "gl_textureCubeLod(";
                }
                else UNREACHABLE();
            }

            TIntermSequence &arguments = node->getSequence();

            for (TIntermSequence::iterator arg = arguments.begin(); arg != arguments.end(); arg++)
            {
                if (mOutputType == SH_HLSL11_OUTPUT && IsSampler((*arg)->getAsTyped()->getBasicType()))
                {
                    out << "texture_";
                    (*arg)->traverse(this);
                    out << ", sampler_";
                }

                (*arg)->traverse(this);

                if (arg < arguments.end() - 1)
                {
                    out << ", ";
                }
            }

            out << ")";

            return false;
        }
        break;
      case EOpParameters:       outputTriplet(visit, "(", ", ", ")\n{\n");             break;
      case EOpConstructFloat:
        addConstructor(node->getType(), "vec1", &node->getSequence());
        outputTriplet(visit, "vec1(", "", ")");
        break;
      case EOpConstructVec2:
        addConstructor(node->getType(), "vec2", &node->getSequence());
        outputTriplet(visit, "vec2(", ", ", ")");
        break;
      case EOpConstructVec3:
        addConstructor(node->getType(), "vec3", &node->getSequence());
        outputTriplet(visit, "vec3(", ", ", ")");
        break;
      case EOpConstructVec4:
        addConstructor(node->getType(), "vec4", &node->getSequence());
        outputTriplet(visit, "vec4(", ", ", ")");
        break;
      case EOpConstructBool:
        addConstructor(node->getType(), "bvec1", &node->getSequence());
        outputTriplet(visit, "bvec1(", "", ")");
        break;
      case EOpConstructBVec2:
        addConstructor(node->getType(), "bvec2", &node->getSequence());
        outputTriplet(visit, "bvec2(", ", ", ")");
        break;
      case EOpConstructBVec3:
        addConstructor(node->getType(), "bvec3", &node->getSequence());
        outputTriplet(visit, "bvec3(", ", ", ")");
        break;
      case EOpConstructBVec4:
        addConstructor(node->getType(), "bvec4", &node->getSequence());
        outputTriplet(visit, "bvec4(", ", ", ")");
        break;
      case EOpConstructInt:
        addConstructor(node->getType(), "ivec1", &node->getSequence());
        outputTriplet(visit, "ivec1(", "", ")");
        break;
      case EOpConstructIVec2:
        addConstructor(node->getType(), "ivec2", &node->getSequence());
        outputTriplet(visit, "ivec2(", ", ", ")");
        break;
      case EOpConstructIVec3:
        addConstructor(node->getType(), "ivec3", &node->getSequence());
        outputTriplet(visit, "ivec3(", ", ", ")");
        break;
      case EOpConstructIVec4:
        addConstructor(node->getType(), "ivec4", &node->getSequence());
        outputTriplet(visit, "ivec4(", ", ", ")");
        break;
      case EOpConstructMat2:
        addConstructor(node->getType(), "mat2", &node->getSequence());
        outputTriplet(visit, "mat2(", ", ", ")");
        break;
      case EOpConstructMat3:
        addConstructor(node->getType(), "mat3", &node->getSequence());
        outputTriplet(visit, "mat3(", ", ", ")");
        break;
      case EOpConstructMat4: 
        addConstructor(node->getType(), "mat4", &node->getSequence());
        outputTriplet(visit, "mat4(", ", ", ")");
        break;
      case EOpConstructStruct:
        addConstructor(node->getType(), scopedStruct(node->getType().getStruct()->name()), &node->getSequence());
        outputTriplet(visit, structLookup(node->getType().getStruct()->name()) + "_ctor(", ", ", ")");
        break;
      case EOpLessThan:         outputTriplet(visit, "(", " < ", ")");                 break;
      case EOpGreaterThan:      outputTriplet(visit, "(", " > ", ")");                 break;
      case EOpLessThanEqual:    outputTriplet(visit, "(", " <= ", ")");                break;
      case EOpGreaterThanEqual: outputTriplet(visit, "(", " >= ", ")");                break;
      case EOpVectorEqual:      outputTriplet(visit, "(", " == ", ")");                break;
      case EOpVectorNotEqual:   outputTriplet(visit, "(", " != ", ")");                break;
      case EOpMod:
        {
            // We need to look at the number of components in both arguments
            switch (node->getSequence()[0]->getAsTyped()->getNominalSize() * 10
                     + node->getSequence()[1]->getAsTyped()->getNominalSize())
            {
              case 11: mUsesMod1 = true; break;
              case 22: mUsesMod2v = true; break;
              case 21: mUsesMod2f = true; break;
              case 33: mUsesMod3v = true; break;
              case 31: mUsesMod3f = true; break;
              case 44: mUsesMod4v = true; break;
              case 41: mUsesMod4f = true; break;
              default: UNREACHABLE();
            }

            outputTriplet(visit, "mod(", ", ", ")");
        }
        break;
      case EOpPow:              outputTriplet(visit, "pow(", ", ", ")");               break;
      case EOpAtan:
        ASSERT(node->getSequence().size() == 2);   // atan(x) is a unary operator
        switch (node->getSequence()[0]->getAsTyped()->getNominalSize())
        {
          case 1: mUsesAtan2_1 = true; break;
          case 2: mUsesAtan2_2 = true; break;
          case 3: mUsesAtan2_3 = true; break;
          case 4: mUsesAtan2_4 = true; break;
          default: UNREACHABLE();
        }
        outputTriplet(visit, "atanyx(", ", ", ")");
        break;
      case EOpMin:           outputTriplet(visit, "min(", ", ", ")");           break;
      case EOpMax:           outputTriplet(visit, "max(", ", ", ")");           break;
      case EOpClamp:         outputTriplet(visit, "clamp(", ", ", ")");         break;
      case EOpMix:           outputTriplet(visit, "lerp(", ", ", ")");          break;
      case EOpStep:          outputTriplet(visit, "step(", ", ", ")");          break;
      case EOpSmoothStep:    outputTriplet(visit, "smoothstep(", ", ", ")");    break;
      case EOpDistance:      outputTriplet(visit, "distance(", ", ", ")");      break;
      case EOpDot:           outputTriplet(visit, "dot(", ", ", ")");           break;
      case EOpCross:         outputTriplet(visit, "cross(", ", ", ")");         break;
      case EOpFaceForward:
        {
            switch (node->getSequence()[0]->getAsTyped()->getNominalSize())   // Number of components in the first argument
            {
            case 1: mUsesFaceforward1 = true; break;
            case 2: mUsesFaceforward2 = true; break;
            case 3: mUsesFaceforward3 = true; break;
            case 4: mUsesFaceforward4 = true; break;
            default: UNREACHABLE();
            }
            
            outputTriplet(visit, "faceforward(", ", ", ")");
        }
        break;
      case EOpReflect:       outputTriplet(visit, "reflect(", ", ", ")");       break;
      case EOpRefract:       outputTriplet(visit, "refract(", ", ", ")");       break;
      case EOpMul:           outputTriplet(visit, "(", " * ", ")");             break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitSelection(Visit visit, TIntermSelection *node)
{
    TInfoSinkBase &out = mBody;

    if (node->usesTernaryOperator())
    {
        out << "s" << mUnfoldShortCircuit->getNextTemporaryIndex();
    }
    else  // if/else statement
    {
        mUnfoldShortCircuit->traverse(node->getCondition());

        out << "if(";

        node->getCondition()->traverse(this);

        out << ")\n";
        
        outputLineDirective(node->getLine().first_line);
        out << "{\n";

        if (node->getTrueBlock())
        {
            traverseStatements(node->getTrueBlock());
        }

        outputLineDirective(node->getLine().first_line);
        out << ";\n}\n";

        if (node->getFalseBlock())
        {
            out << "else\n";

            outputLineDirective(node->getFalseBlock()->getLine().first_line);
            out << "{\n";

            outputLineDirective(node->getFalseBlock()->getLine().first_line);
            traverseStatements(node->getFalseBlock());

            outputLineDirective(node->getFalseBlock()->getLine().first_line);
            out << ";\n}\n";
        }
    }

    return false;
}

void OutputHLSL::visitConstantUnion(TIntermConstantUnion *node)
{
    writeConstantUnion(node->getType(), node->getUnionArrayPointer());
}

bool OutputHLSL::visitLoop(Visit visit, TIntermLoop *node)
{
    bool wasDiscontinuous = mInsideDiscontinuousLoop;

    if (mContainsLoopDiscontinuity && !mInsideDiscontinuousLoop)
    {
        mInsideDiscontinuousLoop = containsLoopDiscontinuity(node);
    }

    if (mOutputType == SH_HLSL9_OUTPUT)
    {
        if (handleExcessiveLoop(node))
        {
            return false;
        }
    }

    TInfoSinkBase &out = mBody;

    if (node->getType() == ELoopDoWhile)
    {
        out << "{do\n";

        outputLineDirective(node->getLine().first_line);
        out << "{\n";
    }
    else
    {
        out << "{for(";
        
        if (node->getInit())
        {
            node->getInit()->traverse(this);
        }

        out << "; ";

        if (node->getCondition())
        {
            node->getCondition()->traverse(this);
        }

        out << "; ";

        if (node->getExpression())
        {
            node->getExpression()->traverse(this);
        }

        out << ")\n";
        
        outputLineDirective(node->getLine().first_line);
        out << "{\n";
    }

    if (node->getBody())
    {
        traverseStatements(node->getBody());
    }

    outputLineDirective(node->getLine().first_line);
    out << ";}\n";

    if (node->getType() == ELoopDoWhile)
    {
        outputLineDirective(node->getCondition()->getLine().first_line);
        out << "while(\n";

        node->getCondition()->traverse(this);

        out << ");";
    }

    out << "}\n";

    mInsideDiscontinuousLoop = wasDiscontinuous;

    return false;
}

bool OutputHLSL::visitBranch(Visit visit, TIntermBranch *node)
{
    TInfoSinkBase &out = mBody;

    switch (node->getFlowOp())
    {
      case EOpKill:     outputTriplet(visit, "discard;\n", "", "");  break;
      case EOpBreak:
        if (visit == PreVisit)
        {
            if (mExcessiveLoopIndex)
            {
                out << "{Break";
                mExcessiveLoopIndex->traverse(this);
                out << " = true; break;}\n";
            }
            else
            {
                out << "break;\n";
            }
        }
        break;
      case EOpContinue: outputTriplet(visit, "continue;\n", "", ""); break;
      case EOpReturn:
        if (visit == PreVisit)
        {
            if (node->getExpression())
            {
                out << "return ";
            }
            else
            {
                out << "return;\n";
            }
        }
        else if (visit == PostVisit)
        {
            if (node->getExpression())
            {
                out << ";\n";
            }
        }
        break;
      default: UNREACHABLE();
    }

    return true;
}

void OutputHLSL::traverseStatements(TIntermNode *node)
{
    if (isSingleStatement(node))
    {
        mUnfoldShortCircuit->traverse(node);
    }

    node->traverse(this);
}

bool OutputHLSL::isSingleStatement(TIntermNode *node)
{
    TIntermAggregate *aggregate = node->getAsAggregate();

    if (aggregate)
    {
        if (aggregate->getOp() == EOpSequence)
        {
            return false;
        }
        else
        {
            for (TIntermSequence::iterator sit = aggregate->getSequence().begin(); sit != aggregate->getSequence().end(); sit++)
            {
                if (!isSingleStatement(*sit))
                {
                    return false;
                }
            }

            return true;
        }
    }

    return true;
}

// Handle loops with more than 254 iterations (unsupported by D3D9) by splitting them
// (The D3D documentation says 255 iterations, but the compiler complains at anything more than 254).
bool OutputHLSL::handleExcessiveLoop(TIntermLoop *node)
{
    const int MAX_LOOP_ITERATIONS = 254;
    TInfoSinkBase &out = mBody;

    // Parse loops of the form:
    // for(int index = initial; index [comparator] limit; index += increment)
    TIntermSymbol *index = NULL;
    TOperator comparator = EOpNull;
    int initial = 0;
    int limit = 0;
    int increment = 0;

    // Parse index name and intial value
    if (node->getInit())
    {
        TIntermAggregate *init = node->getInit()->getAsAggregate();

        if (init)
        {
            TIntermSequence &sequence = init->getSequence();
            TIntermTyped *variable = sequence[0]->getAsTyped();

            if (variable && variable->getQualifier() == EvqTemporary)
            {
                TIntermBinary *assign = variable->getAsBinaryNode();

                if (assign->getOp() == EOpInitialize)
                {
                    TIntermSymbol *symbol = assign->getLeft()->getAsSymbolNode();
                    TIntermConstantUnion *constant = assign->getRight()->getAsConstantUnion();

                    if (symbol && constant)
                    {
                        if (constant->getBasicType() == EbtInt && constant->getNominalSize() == 1)
                        {
                            index = symbol;
                            initial = constant->getIConst(0);
                        }
                    }
                }
            }
        }
    }

    // Parse comparator and limit value
    if (index != NULL && node->getCondition())
    {
        TIntermBinary *test = node->getCondition()->getAsBinaryNode();
        
        if (test && test->getLeft()->getAsSymbolNode()->getId() == index->getId())
        {
            TIntermConstantUnion *constant = test->getRight()->getAsConstantUnion();

            if (constant)
            {
                if (constant->getBasicType() == EbtInt && constant->getNominalSize() == 1)
                {
                    comparator = test->getOp();
                    limit = constant->getIConst(0);
                }
            }
        }
    }

    // Parse increment
    if (index != NULL && comparator != EOpNull && node->getExpression())
    {
        TIntermBinary *binaryTerminal = node->getExpression()->getAsBinaryNode();
        TIntermUnary *unaryTerminal = node->getExpression()->getAsUnaryNode();
        
        if (binaryTerminal)
        {
            TOperator op = binaryTerminal->getOp();
            TIntermConstantUnion *constant = binaryTerminal->getRight()->getAsConstantUnion();

            if (constant)
            {
                if (constant->getBasicType() == EbtInt && constant->getNominalSize() == 1)
                {
                    int value = constant->getIConst(0);

                    switch (op)
                    {
                      case EOpAddAssign: increment = value;  break;
                      case EOpSubAssign: increment = -value; break;
                      default: UNIMPLEMENTED();
                    }
                }
            }
        }
        else if (unaryTerminal)
        {
            TOperator op = unaryTerminal->getOp();

            switch (op)
            {
              case EOpPostIncrement: increment = 1;  break;
              case EOpPostDecrement: increment = -1; break;
              case EOpPreIncrement:  increment = 1;  break;
              case EOpPreDecrement:  increment = -1; break;
              default: UNIMPLEMENTED();
            }
        }
    }

    if (index != NULL && comparator != EOpNull && increment != 0)
    {
        if (comparator == EOpLessThanEqual)
        {
            comparator = EOpLessThan;
            limit += 1;
        }

        if (comparator == EOpLessThan)
        {
            int iterations = (limit - initial) / increment;

            if (iterations <= MAX_LOOP_ITERATIONS)
            {
                return false;   // Not an excessive loop
            }

            TIntermSymbol *restoreIndex = mExcessiveLoopIndex;
            mExcessiveLoopIndex = index;

            out << "{int ";
            index->traverse(this);
            out << ";\n"
                   "bool Break";
            index->traverse(this);
            out << " = false;\n";

            bool firstLoopFragment = true;

            while (iterations > 0)
            {
                int clampedLimit = initial + increment * std::min(MAX_LOOP_ITERATIONS, iterations);

                if (!firstLoopFragment)
                {
                    out << "if(!Break";
                    index->traverse(this);
                    out << ") {\n";
                }

                if (iterations <= MAX_LOOP_ITERATIONS)   // Last loop fragment
                {
                    mExcessiveLoopIndex = NULL;   // Stops setting the Break flag
                }
                
                // for(int index = initial; index < clampedLimit; index += increment)

                out << "for(";
                index->traverse(this);
                out << " = ";
                out << initial;

                out << "; ";
                index->traverse(this);
                out << " < ";
                out << clampedLimit;

                out << "; ";
                index->traverse(this);
                out << " += ";
                out << increment;
                out << ")\n";
                
                outputLineDirective(node->getLine().first_line);
                out << "{\n";

                if (node->getBody())
                {
                    node->getBody()->traverse(this);
                }

                outputLineDirective(node->getLine().first_line);
                out << ";}\n";

                if (!firstLoopFragment)
                {
                    out << "}\n";
                }

                firstLoopFragment = false;

                initial += MAX_LOOP_ITERATIONS * increment;
                iterations -= MAX_LOOP_ITERATIONS;
            }
            
            out << "}";

            mExcessiveLoopIndex = restoreIndex;

            return true;
        }
        else UNIMPLEMENTED();
    }

    return false;   // Not handled as an excessive loop
}

void OutputHLSL::outputTriplet(Visit visit, const TString &preString, const TString &inString, const TString &postString)
{
    TInfoSinkBase &out = mBody;

    if (visit == PreVisit)
    {
        out << preString;
    }
    else if (visit == InVisit)
    {
        out << inString;
    }
    else if (visit == PostVisit)
    {
        out << postString;
    }
}

void OutputHLSL::outputLineDirective(int line)
{
    if ((mContext.compileOptions & SH_LINE_DIRECTIVES) && (line > 0))
    {
        mBody << "\n";
        mBody << "#line " << line;

        if (mContext.sourcePath)
        {
            mBody << " \"" << mContext.sourcePath << "\"";
        }
        
        mBody << "\n";
    }
}

TString OutputHLSL::argumentString(const TIntermSymbol *symbol)
{
    TQualifier qualifier = symbol->getQualifier();
    const TType &type = symbol->getType();
    TString name = symbol->getSymbol();

    if (name.empty())   // HLSL demands named arguments, also for prototypes
    {
        name = "x" + str(mUniqueIndex++);
    }
    else
    {
        name = decorate(name);
    }

    if (mOutputType == SH_HLSL11_OUTPUT && IsSampler(type.getBasicType()))
    {
       return qualifierString(qualifier) + " " + textureString(type) + " texture_" + name + arrayString(type) + ", " +
              qualifierString(qualifier) + " SamplerState sampler_" + name + arrayString(type);
    }

    return qualifierString(qualifier) + " " + typeString(type) + " " + name + arrayString(type);
}

TString OutputHLSL::qualifierString(TQualifier qualifier)
{
    switch(qualifier)
    {
      case EvqIn:            return "in";
      case EvqOut:           return "out";
      case EvqInOut:         return "inout";
      case EvqConstReadOnly: return "const";
      default: UNREACHABLE();
    }

    return "";
}

TString OutputHLSL::typeString(const TType &type)
{
    if (type.getBasicType() == EbtStruct)
    {
        const TString& typeName = type.getStruct()->name();
        if (typeName != "")
        {
            return structLookup(typeName);
        }
        else   // Nameless structure, define in place
        {
            const TFieldList &fields = type.getStruct()->fields();

            TString string = "struct\n"
                             "{\n";

            for (unsigned int i = 0; i < fields.size(); i++)
            {
                const TField *field = fields[i];

                string += "    " + typeString(*field->type()) + " " + decorate(field->name()) + arrayString(*field->type()) + ";\n";
            }

            string += "} ";

            return string;
        }
    }
    else if (type.isMatrix())
    {
        switch (type.getNominalSize())
        {
          case 2: return "float2x2";
          case 3: return "float3x3";
          case 4: return "float4x4";
        }
    }
    else
    {
        switch (type.getBasicType())
        {
          case EbtFloat:
            switch (type.getNominalSize())
            {
              case 1: return "float";
              case 2: return "float2";
              case 3: return "float3";
              case 4: return "float4";
            }
          case EbtInt:
            switch (type.getNominalSize())
            {
              case 1: return "int";
              case 2: return "int2";
              case 3: return "int3";
              case 4: return "int4";
            }
          case EbtBool:
            switch (type.getNominalSize())
            {
              case 1: return "bool";
              case 2: return "bool2";
              case 3: return "bool3";
              case 4: return "bool4";
            }
          case EbtVoid:
            return "void";
          case EbtSampler2D:
            return "sampler2D";
          case EbtSamplerCube:
            return "samplerCUBE";
          case EbtSamplerExternalOES:
            return "sampler2D";
          default:
            break;
        }
    }

    UNREACHABLE();
    return "<unknown type>";
}

TString OutputHLSL::textureString(const TType &type)
{
    switch (type.getBasicType())
    {
      case EbtSampler2D:
        return "Texture2D";
      case EbtSamplerCube:
        return "TextureCube";
      case EbtSamplerExternalOES:
        return "Texture2D";
      default:
        break;
    }

    UNREACHABLE();
    return "<unknown texture type>";
}

TString OutputHLSL::arrayString(const TType &type)
{
    if (!type.isArray())
    {
        return "";
    }

    return "[" + str(type.getArraySize()) + "]";
}

TString OutputHLSL::initializer(const TType &type)
{
    TString string;

    size_t size = type.getObjectSize();
    for (size_t component = 0; component < size; component++)
    {
        string += "0";

        if (component + 1 < size)
        {
            string += ", ";
        }
    }

    return "{" + string + "}";
}

void OutputHLSL::addConstructor(const TType &type, const TString &name, const TIntermSequence *parameters)
{
    if (name == "")
    {
        return;   // Nameless structures don't have constructors
    }

    if (type.getStruct() && mStructNames.find(decorate(name)) != mStructNames.end())
    {
        return;   // Already added
    }

    TType ctorType = type;
    ctorType.clearArrayness();
    ctorType.setPrecision(EbpHigh);
    ctorType.setQualifier(EvqTemporary);

    TString ctorName = type.getStruct() ? decorate(name) : name;

    typedef std::vector<TType> ParameterArray;
    ParameterArray ctorParameters;

    if (type.getStruct())
    {
        mStructNames.insert(decorate(name));

        TString structure;
        structure += "struct " + decorate(name) + "\n"
                     "{\n";

        const TFieldList &fields = type.getStruct()->fields();

        for (unsigned int i = 0; i < fields.size(); i++)
        {
            const TField *field = fields[i];

            structure += "    " + typeString(*field->type()) + " " + decorateField(field->name(), type) + arrayString(*field->type()) + ";\n";
        }

        structure += "};\n";

        if (std::find(mStructDeclarations.begin(), mStructDeclarations.end(), structure) == mStructDeclarations.end())
        {
            mStructDeclarations.push_back(structure);
        }

        for (unsigned int i = 0; i < fields.size(); i++)
        {
            ctorParameters.push_back(*fields[i]->type());
        }
    }
    else if (parameters)
    {
        for (TIntermSequence::const_iterator parameter = parameters->begin(); parameter != parameters->end(); parameter++)
        {
            ctorParameters.push_back((*parameter)->getAsTyped()->getType());
        }
    }
    else UNREACHABLE();

    TString constructor;

    if (ctorType.getStruct())
    {
        constructor += ctorName + " " + ctorName + "_ctor(";
    }
    else   // Built-in type
    {
        constructor += typeString(ctorType) + " " + ctorName + "(";
    }

    for (unsigned int parameter = 0; parameter < ctorParameters.size(); parameter++)
    {
        const TType &type = ctorParameters[parameter];

        constructor += typeString(type) + " x" + str(parameter) + arrayString(type);

        if (parameter < ctorParameters.size() - 1)
        {
            constructor += ", ";
        }
    }

    constructor += ")\n"
                   "{\n";

    if (ctorType.getStruct())
    {
        constructor += "    " + ctorName + " structure = {";
    }
    else
    {
        constructor += "    return " + typeString(ctorType) + "(";
    }

    if (ctorType.isMatrix() && ctorParameters.size() == 1)
    {
        int dim = ctorType.getNominalSize();
        const TType &parameter = ctorParameters[0];

        if (parameter.isScalar())
        {
            for (int row = 0; row < dim; row++)
            {
                for (int col = 0; col < dim; col++)
                {
                    constructor += TString((row == col) ? "x0" : "0.0");
                    
                    if (row < dim - 1 || col < dim - 1)
                    {
                        constructor += ", ";
                    }
                }
            }
        }
        else if (parameter.isMatrix())
        {
            for (int row = 0; row < dim; row++)
            {
                for (int col = 0; col < dim; col++)
                {
                    if (row < parameter.getNominalSize() && col < parameter.getNominalSize())
                    {
                        constructor += TString("x0") + "[" + str(row) + "]" + "[" + str(col) + "]";
                    }
                    else
                    {
                        constructor += TString((row == col) ? "1.0" : "0.0");
                    }

                    if (row < dim - 1 || col < dim - 1)
                    {
                        constructor += ", ";
                    }
                }
            }
        }
        else UNREACHABLE();
    }
    else
    {
        size_t remainingComponents = ctorType.getObjectSize();
        size_t parameterIndex = 0;

        while (remainingComponents > 0)
        {
            const TType &parameter = ctorParameters[parameterIndex];
            const size_t parameterSize = parameter.getObjectSize();
            bool moreParameters = parameterIndex + 1 < ctorParameters.size();

            constructor += "x" + str(parameterIndex);

            if (parameter.isScalar())
            {
                ASSERT(parameterSize <= remainingComponents);
                remainingComponents -= parameterSize;
            }
            else if (parameter.isVector())
            {
                if (remainingComponents == parameterSize || moreParameters)
                {
                    ASSERT(parameterSize <= remainingComponents);
                    remainingComponents -= parameterSize;
                }
                else if (remainingComponents < static_cast<size_t>(parameter.getNominalSize()))
                {
                    switch (remainingComponents)
                    {
                      case 1: constructor += ".x";    break;
                      case 2: constructor += ".xy";   break;
                      case 3: constructor += ".xyz";  break;
                      case 4: constructor += ".xyzw"; break;
                      default: UNREACHABLE();
                    }

                    remainingComponents = 0;
                }
                else UNREACHABLE();
            }
            else if (parameter.isMatrix() || parameter.getStruct())
            {
                ASSERT(remainingComponents == parameterSize || moreParameters);
                ASSERT(parameterSize <= remainingComponents);
                
                remainingComponents -= parameterSize;
            }
            else UNREACHABLE();

            if (moreParameters)
            {
                parameterIndex++;
            }

            if (remainingComponents)
            {
                constructor += ", ";
            }
        }
    }

    if (ctorType.getStruct())
    {
        constructor += "};\n"
                       "    return structure;\n"
                       "}\n";
    }
    else
    {
        constructor += ");\n"
                       "}\n";
    }

    mConstructors.insert(constructor);
}

const ConstantUnion *OutputHLSL::writeConstantUnion(const TType &type, const ConstantUnion *constUnion)
{
    TInfoSinkBase &out = mBody;

    if (type.getBasicType() == EbtStruct)
    {
        out << structLookup(type.getStruct()->name()) + "_ctor(";
        
        const TFieldList &fields = type.getStruct()->fields();

        for (size_t i = 0; i < fields.size(); i++)
        {
            const TType *fieldType = fields[i]->type();

            constUnion = writeConstantUnion(*fieldType, constUnion);

            if (i != fields.size() - 1)
            {
                out << ", ";
            }
        }

        out << ")";
    }
    else
    {
        size_t size = type.getObjectSize();
        bool writeType = size > 1;
        
        if (writeType)
        {
            out << typeString(type) << "(";
        }

        for (size_t i = 0; i < size; i++, constUnion++)
        {
            switch (constUnion->getType())
            {
              case EbtFloat: out << std::min(FLT_MAX, std::max(-FLT_MAX, constUnion->getFConst())); break;
              case EbtInt:   out << constUnion->getIConst(); break;
              case EbtBool:  out << constUnion->getBConst(); break;
              default: UNREACHABLE();
            }

            if (i != size - 1)
            {
                out << ", ";
            }
        }

        if (writeType)
        {
            out << ")";
        }
    }

    return constUnion;
}

TString OutputHLSL::scopeString(unsigned int depthLimit)
{
    TString string;

    for (unsigned int i = 0; i < mScopeBracket.size() && i < depthLimit; i++)
    {
        string += "_" + str(i);
    }

    return string;
}

TString OutputHLSL::scopedStruct(const TString &typeName)
{
    if (typeName == "")
    {
        return typeName;
    }

    return typeName + scopeString(mScopeDepth);
}

TString OutputHLSL::structLookup(const TString &typeName)
{
    for (int depth = mScopeDepth; depth >= 0; depth--)
    {
        TString scopedName = decorate(typeName + scopeString(depth));

        for (StructNames::iterator structName = mStructNames.begin(); structName != mStructNames.end(); structName++)
        {
            if (*structName == scopedName)
            {
                return scopedName;
            }
        }
    }

    UNREACHABLE();   // Should have found a matching constructor

    return typeName;
}

TString OutputHLSL::decorate(const TString &string)
{
    if (string.compare(0, 3, "gl_") != 0 && string.compare(0, 3, "dx_") != 0)
    {
        return "_" + string;
    }
    
    return string;
}

TString OutputHLSL::decorateUniform(const TString &string, const TType &type)
{
    if (type.getBasicType() == EbtSamplerExternalOES)
    {
        return "ex_" + string;
    }
    
    return decorate(string);
}

TString OutputHLSL::decorateField(const TString &string, const TType &structure)
{
    if (structure.getStruct()->name().compare(0, 3, "gl_") != 0)
    {
        return decorate(string);
    }

    return string;
}

TString OutputHLSL::registerString(TIntermSymbol *operand)
{
    ASSERT(operand->getQualifier() == EvqUniform);

    if (IsSampler(operand->getBasicType()))
    {
        return "s" + str(samplerRegister(operand));
    }

    return "c" + str(uniformRegister(operand));
}

int OutputHLSL::samplerRegister(TIntermSymbol *sampler)
{
    const TType &type = sampler->getType();
    ASSERT(IsSampler(type.getBasicType()));

    int index = mSamplerRegister;
    mSamplerRegister += sampler->totalRegisterCount();

    declareUniform(type, sampler->getSymbol(), index);

    return index;
}

int OutputHLSL::uniformRegister(TIntermSymbol *uniform)
{
    const TType &type = uniform->getType();
    ASSERT(!IsSampler(type.getBasicType()));

    int index = mUniformRegister;
    mUniformRegister += uniform->totalRegisterCount();

    declareUniform(type, uniform->getSymbol(), index);

    return index;
}

void OutputHLSL::declareUniform(const TType &type, const TString &name, int index)
{
    TStructure *structure = type.getStruct();

    if (!structure)
    {
        mActiveUniforms.push_back(Uniform(glVariableType(type), glVariablePrecision(type), name.c_str(), type.getArraySize(), index));
    }
    else
    {
        const TFieldList &fields = structure->fields();

        if (type.isArray())
        {
            int elementIndex = index;

            for (int i = 0; i < type.getArraySize(); i++)
            {
                for (size_t j = 0; j < fields.size(); j++)
                {
                    const TType &fieldType = *fields[j]->type();
                    const TString uniformName = name + "[" + str(i) + "]." + fields[j]->name();
                    declareUniform(fieldType, uniformName, elementIndex);
                    elementIndex += fieldType.totalRegisterCount();
                }
            }
        }
        else
        {
            int fieldIndex = index;

            for (size_t i = 0; i < fields.size(); i++)
            {
                const TType &fieldType = *fields[i]->type();
                const TString uniformName = name + "." + fields[i]->name();
                declareUniform(fieldType, uniformName, fieldIndex);
                fieldIndex += fieldType.totalRegisterCount();
            }
        }
    }
}

GLenum OutputHLSL::glVariableType(const TType &type)
{
    if (type.getBasicType() == EbtFloat)
    {
        if (type.isScalar())
        {
            return GL_FLOAT;
        }
        else if (type.isVector())
        {
            switch(type.getNominalSize())
            {
              case 2: return GL_FLOAT_VEC2;
              case 3: return GL_FLOAT_VEC3;
              case 4: return GL_FLOAT_VEC4;
              default: UNREACHABLE();
            }
        }
        else if (type.isMatrix())
        {
            switch(type.getNominalSize())
            {
              case 2: return GL_FLOAT_MAT2;
              case 3: return GL_FLOAT_MAT3;
              case 4: return GL_FLOAT_MAT4;
              default: UNREACHABLE();
            }
        }
        else UNREACHABLE();
    }
    else if (type.getBasicType() == EbtInt)
    {
        if (type.isScalar())
        {
            return GL_INT;
        }
        else if (type.isVector())
        {
            switch(type.getNominalSize())
            {
              case 2: return GL_INT_VEC2;
              case 3: return GL_INT_VEC3;
              case 4: return GL_INT_VEC4;
              default: UNREACHABLE();
            }
        }
        else UNREACHABLE();
    }
    else if (type.getBasicType() == EbtBool)
    {
        if (type.isScalar())
        {
            return GL_BOOL;
        }
        else if (type.isVector())
        {
            switch(type.getNominalSize())
            {
              case 2: return GL_BOOL_VEC2;
              case 3: return GL_BOOL_VEC3;
              case 4: return GL_BOOL_VEC4;
              default: UNREACHABLE();
            }
        }
        else UNREACHABLE();
    }
    else if (type.getBasicType() == EbtSampler2D)
    {
        return GL_SAMPLER_2D;
    }
    else if (type.getBasicType() == EbtSamplerCube)
    {
        return GL_SAMPLER_CUBE;
    }
    else UNREACHABLE();

    return GL_NONE;
}

GLenum OutputHLSL::glVariablePrecision(const TType &type)
{
    if (type.getBasicType() == EbtFloat)
    {
        switch (type.getPrecision())
        {
          case EbpHigh:   return GL_HIGH_FLOAT;
          case EbpMedium: return GL_MEDIUM_FLOAT;
          case EbpLow:    return GL_LOW_FLOAT;
          case EbpUndefined:
            // Should be defined as the default precision by the parser
          default: UNREACHABLE();
        }
    }
    else if (type.getBasicType() == EbtInt)
    {
        switch (type.getPrecision())
        {
          case EbpHigh:   return GL_HIGH_INT;
          case EbpMedium: return GL_MEDIUM_INT;
          case EbpLow:    return GL_LOW_INT;
          case EbpUndefined:
            // Should be defined as the default precision by the parser
          default: UNREACHABLE();
        }
    }

    // Other types (boolean, sampler) don't have a precision
    return GL_NONE;
}

}
