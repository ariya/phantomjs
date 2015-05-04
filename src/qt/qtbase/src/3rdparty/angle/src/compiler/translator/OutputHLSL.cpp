//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/OutputHLSL.h"

#include "common/angleutils.h"
#include "common/utilities.h"
#include "common/blocklayout.h"
#include "compiler/translator/compilerdebug.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/DetectDiscontinuity.h"
#include "compiler/translator/SearchSymbol.h"
#include "compiler/translator/UnfoldShortCircuit.h"
#include "compiler/translator/FlagStd140Structs.h"
#include "compiler/translator/NodeSearch.h"
#include "compiler/translator/RewriteElseBlocks.h"
#include "compiler/translator/UtilsHLSL.h"
#include "compiler/translator/util.h"
#include "compiler/translator/UniformHLSL.h"
#include "compiler/translator/StructureHLSL.h"
#include "compiler/translator/TranslatorHLSL.h"

#include <algorithm>
#include <cfloat>
#include <stdio.h>

namespace sh
{

TString OutputHLSL::TextureFunction::name() const
{
    TString name = "gl_texture";

    if (IsSampler2D(sampler))
    {
        name += "2D";
    }
    else if (IsSampler3D(sampler))
    {
        name += "3D";
    }
    else if (IsSamplerCube(sampler))
    {
        name += "Cube";
    }
    else UNREACHABLE();

    if (proj)
    {
        name += "Proj";
    }

    if (offset)
    {
        name += "Offset";
    }

    switch(method)
    {
      case IMPLICIT:                  break;
      case BIAS:                      break;   // Extra parameter makes the signature unique
      case LOD:      name += "Lod";   break;
      case LOD0:     name += "Lod0";  break;
      case LOD0BIAS: name += "Lod0";  break;   // Extra parameter makes the signature unique
      case SIZE:     name += "Size";  break;
      case FETCH:    name += "Fetch"; break;
      case GRAD:     name += "Grad";  break;
      default: UNREACHABLE();
    }

    return name + "(";
}

bool OutputHLSL::TextureFunction::operator<(const TextureFunction &rhs) const
{
    if (sampler < rhs.sampler) return true;
    if (sampler > rhs.sampler) return false;

    if (coords < rhs.coords)   return true;
    if (coords > rhs.coords)   return false;

    if (!proj && rhs.proj)     return true;
    if (proj && !rhs.proj)     return false;

    if (!offset && rhs.offset) return true;
    if (offset && !rhs.offset) return false;

    if (method < rhs.method)   return true;
    if (method > rhs.method)   return false;

    return false;
}

OutputHLSL::OutputHLSL(TParseContext &context, TranslatorHLSL *parentTranslator)
    : TIntermTraverser(true, true, true),
      mContext(context),
      mOutputType(parentTranslator->getOutputType())
{
    mUnfoldShortCircuit = new UnfoldShortCircuit(context, this);
    mInsideFunction = false;

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
    mUsesAtan2_1 = false;
    mUsesAtan2_2 = false;
    mUsesAtan2_3 = false;
    mUsesAtan2_4 = false;
    mUsesDiscardRewriting = false;
    mUsesNestedBreak = false;

    const ShBuiltInResources &resources = parentTranslator->getResources();
    mNumRenderTargets = resources.EXT_draw_buffers ? resources.MaxDrawBuffers : 1;

    mUniqueIndex = 0;

    mContainsLoopDiscontinuity = false;
    mContainsAnyLoop = false;
    mOutputLod0Function = false;
    mInsideDiscontinuousLoop = false;
    mNestedLoopDepth = 0;

    mExcessiveLoopIndex = NULL;

    mStructureHLSL = new StructureHLSL;
    mUniformHLSL = new UniformHLSL(mStructureHLSL, parentTranslator);

    if (mOutputType == SH_HLSL9_OUTPUT)
    {
        if (mContext.shaderType == GL_FRAGMENT_SHADER)
        {
            // Reserve registers for dx_DepthRange, dx_ViewCoords and dx_DepthFront
            mUniformHLSL->reserveUniformRegisters(3);
        }
        else
        {
            // Reserve registers for dx_DepthRange and dx_ViewAdjust
            mUniformHLSL->reserveUniformRegisters(2);
        }
    }

    // Reserve registers for the default uniform block and driver constants
    mUniformHLSL->reserveInterfaceBlockRegisters(2);
}

OutputHLSL::~OutputHLSL()
{
    SafeDelete(mUnfoldShortCircuit);
    SafeDelete(mStructureHLSL);
    SafeDelete(mUniformHLSL);
}

void OutputHLSL::output()
{
    mContainsLoopDiscontinuity = mContext.shaderType == GL_FRAGMENT_SHADER && containsLoopDiscontinuity(mContext.treeRoot);
    mContainsAnyLoop = containsAnyLoop(mContext.treeRoot);
    const std::vector<TIntermTyped*> &flaggedStructs = FlagStd140ValueStructs(mContext.treeRoot);
    makeFlaggedStructMaps(flaggedStructs);

    // Work around D3D9 bug that would manifest in vertex shaders with selection blocks which
    // use a vertex attribute as a condition, and some related computation in the else block.
    if (mOutputType == SH_HLSL9_OUTPUT && mContext.shaderType == GL_VERTEX_SHADER)
    {
        RewriteElseBlocks(mContext.treeRoot);
    }

    mContext.treeRoot->traverse(this);   // Output the body first to determine what has to go in the header
    header();

    mContext.infoSink().obj << mHeader.c_str();
    mContext.infoSink().obj << mBody.c_str();
}

void OutputHLSL::makeFlaggedStructMaps(const std::vector<TIntermTyped *> &flaggedStructs)
{
    for (unsigned int structIndex = 0; structIndex < flaggedStructs.size(); structIndex++)
    {
        TIntermTyped *flaggedNode = flaggedStructs[structIndex];

        // This will mark the necessary block elements as referenced
        flaggedNode->traverse(this);
        TString structName(mBody.c_str());
        mBody.erase();

        mFlaggedStructOriginalNames[flaggedNode] = structName;

        for (size_t pos = structName.find('.'); pos != std::string::npos; pos = structName.find('.'))
        {
            structName.erase(pos, 1);
        }

        mFlaggedStructMappedNames[flaggedNode] = "map" + structName;
    }
}

TInfoSinkBase &OutputHLSL::getBodyStream()
{
    return mBody;
}

const std::map<std::string, unsigned int> &OutputHLSL::getInterfaceBlockRegisterMap() const
{
    return mUniformHLSL->getInterfaceBlockRegisterMap();
}

const std::map<std::string, unsigned int> &OutputHLSL::getUniformRegisterMap() const
{
    return mUniformHLSL->getUniformRegisterMap();
}

int OutputHLSL::vectorSize(const TType &type) const
{
    int elementSize = type.isMatrix() ? type.getCols() : 1;
    int arraySize = type.isArray() ? type.getArraySize() : 1;

    return elementSize * arraySize;
}

TString OutputHLSL::structInitializerString(int indent, const TStructure &structure, const TString &rhsStructName)
{
    TString init;

    TString preIndentString;
    TString fullIndentString;

    for (int spaces = 0; spaces < (indent * 4); spaces++)
    {
        preIndentString += ' ';
    }

    for (int spaces = 0; spaces < ((indent+1) * 4); spaces++)
    {
        fullIndentString += ' ';
    }

    init += preIndentString + "{\n";

    const TFieldList &fields = structure.fields();
    for (unsigned int fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
    {
        const TField &field = *fields[fieldIndex];
        const TString &fieldName = rhsStructName + "." + Decorate(field.name());
        const TType &fieldType = *field.type();

        if (fieldType.getStruct())
        {
            init += structInitializerString(indent + 1, *fieldType.getStruct(), fieldName);
        }
        else
        {
            init += fullIndentString + fieldName + ",\n";
        }
    }

    init += preIndentString + "}" + (indent == 0 ? ";" : ",") + "\n";

    return init;
}

void OutputHLSL::header()
{
    TInfoSinkBase &out = mHeader;

    TString varyings;
    TString attributes;
    TString flaggedStructs;

    for (std::map<TIntermTyped*, TString>::const_iterator flaggedStructIt = mFlaggedStructMappedNames.begin(); flaggedStructIt != mFlaggedStructMappedNames.end(); flaggedStructIt++)
    {
        TIntermTyped *structNode = flaggedStructIt->first;
        const TString &mappedName = flaggedStructIt->second;
        const TStructure &structure = *structNode->getType().getStruct();
        const TString &originalName = mFlaggedStructOriginalNames[structNode];

        flaggedStructs += "static " + Decorate(structure.name()) + " " + mappedName + " =\n";
        flaggedStructs += structInitializerString(0, structure, originalName);
        flaggedStructs += "\n";
    }

    for (ReferencedSymbols::const_iterator varying = mReferencedVaryings.begin(); varying != mReferencedVaryings.end(); varying++)
    {
        const TType &type = varying->second->getType();
        const TString &name = varying->second->getSymbol();

        // Program linking depends on this exact format
        varyings += "static " + InterpolationString(type.getQualifier()) + " " + TypeString(type) + " " +
                    Decorate(name) + ArrayString(type) + " = " + initializer(type) + ";\n";
    }

    for (ReferencedSymbols::const_iterator attribute = mReferencedAttributes.begin(); attribute != mReferencedAttributes.end(); attribute++)
    {
        const TType &type = attribute->second->getType();
        const TString &name = attribute->second->getSymbol();

        attributes += "static " + TypeString(type) + " " + Decorate(name) + ArrayString(type) + " = " + initializer(type) + ";\n";
    }

    out << mStructureHLSL->structsHeader();

    out << mUniformHLSL->uniformsHeader(mOutputType, mReferencedUniforms);
    out << mUniformHLSL->interfaceBlocksHeader(mReferencedInterfaceBlocks);

    if (mUsesDiscardRewriting)
    {
        out << "#define ANGLE_USES_DISCARD_REWRITING\n";
    }

    if (mUsesNestedBreak)
    {
        out << "#define ANGLE_USES_NESTED_BREAK\n";
    }

    out << "#ifdef ANGLE_ENABLE_LOOP_FLATTEN\n"
           "#define LOOP [loop]\n"
           "#define FLATTEN [flatten]\n"
           "#else\n"
           "#define LOOP\n"
           "#define FLATTEN\n"
           "#endif\n";

    if (mContext.shaderType == GL_FRAGMENT_SHADER)
    {
        TExtensionBehavior::const_iterator iter = mContext.extensionBehavior().find("GL_EXT_draw_buffers");
        const bool usingMRTExtension = (iter != mContext.extensionBehavior().end() && (iter->second == EBhEnable || iter->second == EBhRequire));

        out << "// Varyings\n";
        out <<  varyings;
        out << "\n";

        if (mContext.getShaderVersion() >= 300)
        {
            for (ReferencedSymbols::const_iterator outputVariableIt = mReferencedOutputVariables.begin(); outputVariableIt != mReferencedOutputVariables.end(); outputVariableIt++)
            {
                const TString &variableName = outputVariableIt->first;
                const TType &variableType = outputVariableIt->second->getType();

                out << "static " + TypeString(variableType) + " out_" + variableName + ArrayString(variableType) +
                       " = " + initializer(variableType) + ";\n";
            }
        }
        else
        {
            const unsigned int numColorValues = usingMRTExtension ? mNumRenderTargets : 1;

            out << "static float4 gl_Color[" << numColorValues << "] =\n"
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
        }

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

        if (!flaggedStructs.empty())
        {
            out << "// Std140 Structures accessed by value\n";
            out << "\n";
            out << flaggedStructs;
            out << "\n";
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

        if (!flaggedStructs.empty())
        {
            out << "// Std140 Structures accessed by value\n";
            out << "\n";
            out << flaggedStructs;
            out << "\n";
        }
    }

    for (TextureFunctionSet::const_iterator textureFunction = mUsesTexture.begin(); textureFunction != mUsesTexture.end(); textureFunction++)
    {
        // Return type
        if (textureFunction->method == TextureFunction::SIZE)
        {
            switch(textureFunction->sampler)
            {
              case EbtSampler2D:            out << "int2 "; break;
              case EbtSampler3D:            out << "int3 "; break;
              case EbtSamplerCube:          out << "int2 "; break;
              case EbtSampler2DArray:       out << "int3 "; break;
              case EbtISampler2D:           out << "int2 "; break;
              case EbtISampler3D:           out << "int3 "; break;
              case EbtISamplerCube:         out << "int2 "; break;
              case EbtISampler2DArray:      out << "int3 "; break;
              case EbtUSampler2D:           out << "int2 "; break;
              case EbtUSampler3D:           out << "int3 "; break;
              case EbtUSamplerCube:         out << "int2 "; break;
              case EbtUSampler2DArray:      out << "int3 "; break;
              case EbtSampler2DShadow:      out << "int2 "; break;
              case EbtSamplerCubeShadow:    out << "int2 "; break;
              case EbtSampler2DArrayShadow: out << "int3 "; break;
              default: UNREACHABLE();
            }
        }
        else   // Sampling function
        {
            switch(textureFunction->sampler)
            {
              case EbtSampler2D:            out << "float4 "; break;
              case EbtSampler3D:            out << "float4 "; break;
              case EbtSamplerCube:          out << "float4 "; break;
              case EbtSampler2DArray:       out << "float4 "; break;
              case EbtISampler2D:           out << "int4 ";   break;
              case EbtISampler3D:           out << "int4 ";   break;
              case EbtISamplerCube:         out << "int4 ";   break;
              case EbtISampler2DArray:      out << "int4 ";   break;
              case EbtUSampler2D:           out << "uint4 ";  break;
              case EbtUSampler3D:           out << "uint4 ";  break;
              case EbtUSamplerCube:         out << "uint4 ";  break;
              case EbtUSampler2DArray:      out << "uint4 ";  break;
              case EbtSampler2DShadow:      out << "float ";  break;
              case EbtSamplerCubeShadow:    out << "float ";  break;
              case EbtSampler2DArrayShadow: out << "float ";  break;
              default: UNREACHABLE();
            }
        }

        // Function name
        out << textureFunction->name();

        // Argument list
        int hlslCoords = 4;

        if (mOutputType == SH_HLSL9_OUTPUT)
        {
            switch(textureFunction->sampler)
            {
              case EbtSampler2D:   out << "sampler2D s";   hlslCoords = 2; break;
              case EbtSamplerCube: out << "samplerCUBE s"; hlslCoords = 3; break;
              default: UNREACHABLE();
            }

            switch(textureFunction->method)
            {
              case TextureFunction::IMPLICIT:                 break;
              case TextureFunction::BIAS:     hlslCoords = 4; break;
              case TextureFunction::LOD:      hlslCoords = 4; break;
              case TextureFunction::LOD0:     hlslCoords = 4; break;
              case TextureFunction::LOD0BIAS: hlslCoords = 4; break;
              default: UNREACHABLE();
            }
        }
        else if (mOutputType == SH_HLSL11_OUTPUT)
        {
            switch(textureFunction->sampler)
            {
              case EbtSampler2D:            out << "Texture2D x, SamplerState s";                hlslCoords = 2; break;
              case EbtSampler3D:            out << "Texture3D x, SamplerState s";                hlslCoords = 3; break;
              case EbtSamplerCube:          out << "TextureCube x, SamplerState s";              hlslCoords = 3; break;
              case EbtSampler2DArray:       out << "Texture2DArray x, SamplerState s";           hlslCoords = 3; break;
              case EbtISampler2D:           out << "Texture2D<int4> x, SamplerState s";          hlslCoords = 2; break;
              case EbtISampler3D:           out << "Texture3D<int4> x, SamplerState s";          hlslCoords = 3; break;
              case EbtISamplerCube:         out << "Texture2DArray<int4> x, SamplerState s";     hlslCoords = 3; break;
              case EbtISampler2DArray:      out << "Texture2DArray<int4> x, SamplerState s";     hlslCoords = 3; break;
              case EbtUSampler2D:           out << "Texture2D<uint4> x, SamplerState s";         hlslCoords = 2; break;
              case EbtUSampler3D:           out << "Texture3D<uint4> x, SamplerState s";         hlslCoords = 3; break;
              case EbtUSamplerCube:         out << "Texture2DArray<uint4> x, SamplerState s";    hlslCoords = 3; break;
              case EbtUSampler2DArray:      out << "Texture2DArray<uint4> x, SamplerState s";    hlslCoords = 3; break;
              case EbtSampler2DShadow:      out << "Texture2D x, SamplerComparisonState s";      hlslCoords = 2; break;
              case EbtSamplerCubeShadow:    out << "TextureCube x, SamplerComparisonState s";    hlslCoords = 3; break;
              case EbtSampler2DArrayShadow: out << "Texture2DArray x, SamplerComparisonState s"; hlslCoords = 3; break;
              default: UNREACHABLE();
            }
        }
        else UNREACHABLE();

        if (textureFunction->method == TextureFunction::FETCH)   // Integer coordinates
        {
            switch(textureFunction->coords)
            {
              case 2: out << ", int2 t"; break;
              case 3: out << ", int3 t"; break;
              default: UNREACHABLE();
            }
        }
        else   // Floating-point coordinates (except textureSize)
        {
            switch(textureFunction->coords)
            {
              case 1: out << ", int lod";  break;   // textureSize()
              case 2: out << ", float2 t"; break;
              case 3: out << ", float3 t"; break;
              case 4: out << ", float4 t"; break;
              default: UNREACHABLE();
            }
        }

        if (textureFunction->method == TextureFunction::GRAD)
        {
            switch(textureFunction->sampler)
            {
              case EbtSampler2D:
              case EbtISampler2D:
              case EbtUSampler2D:
              case EbtSampler2DArray:
              case EbtISampler2DArray:
              case EbtUSampler2DArray:
              case EbtSampler2DShadow:
              case EbtSampler2DArrayShadow:
                out << ", float2 ddx, float2 ddy";
                break;
              case EbtSampler3D:
              case EbtISampler3D:
              case EbtUSampler3D:
              case EbtSamplerCube:
              case EbtISamplerCube:
              case EbtUSamplerCube:
              case EbtSamplerCubeShadow:
                out << ", float3 ddx, float3 ddy";
                break;
              default: UNREACHABLE();
            }
        }

        switch(textureFunction->method)
        {
          case TextureFunction::IMPLICIT:                        break;
          case TextureFunction::BIAS:                            break;   // Comes after the offset parameter
          case TextureFunction::LOD:      out << ", float lod";  break;
          case TextureFunction::LOD0:                            break;
          case TextureFunction::LOD0BIAS:                        break;   // Comes after the offset parameter
          case TextureFunction::SIZE:                            break;
          case TextureFunction::FETCH:    out << ", int mip";    break;
          case TextureFunction::GRAD:                            break;
          default: UNREACHABLE();
        }

        if (textureFunction->offset)
        {
            switch(textureFunction->sampler)
            {
              case EbtSampler2D:            out << ", int2 offset"; break;
              case EbtSampler3D:            out << ", int3 offset"; break;
              case EbtSampler2DArray:       out << ", int2 offset"; break;
              case EbtISampler2D:           out << ", int2 offset"; break;
              case EbtISampler3D:           out << ", int3 offset"; break;
              case EbtISampler2DArray:      out << ", int2 offset"; break;
              case EbtUSampler2D:           out << ", int2 offset"; break;
              case EbtUSampler3D:           out << ", int3 offset"; break;
              case EbtUSampler2DArray:      out << ", int2 offset"; break;
              case EbtSampler2DShadow:      out << ", int2 offset"; break;
              case EbtSampler2DArrayShadow: out << ", int2 offset"; break;
              default: UNREACHABLE();
            }
        }

        if (textureFunction->method == TextureFunction::BIAS ||
            textureFunction->method == TextureFunction::LOD0BIAS)
        {
            out << ", float bias";
        }

        out << ")\n"
               "{\n";

        if (textureFunction->method == TextureFunction::SIZE)
        {
            if (IsSampler2D(textureFunction->sampler) || IsSamplerCube(textureFunction->sampler))
            {
                if (IsSamplerArray(textureFunction->sampler))
                {
                    out << "    uint width; uint height; uint layers; uint numberOfLevels;\n"
                           "    x.GetDimensions(lod, width, height, layers, numberOfLevels);\n";
                }
                else
                {
                    out << "    uint width; uint height; uint numberOfLevels;\n"
                           "    x.GetDimensions(lod, width, height, numberOfLevels);\n";
                }
            }
            else if (IsSampler3D(textureFunction->sampler))
            {
                out << "    uint width; uint height; uint depth; uint numberOfLevels;\n"
                       "    x.GetDimensions(lod, width, height, depth, numberOfLevels);\n";
            }
            else UNREACHABLE();

            switch(textureFunction->sampler)
            {
              case EbtSampler2D:            out << "    return int2(width, height);";         break;
              case EbtSampler3D:            out << "    return int3(width, height, depth);";  break;
              case EbtSamplerCube:          out << "    return int2(width, height);";         break;
              case EbtSampler2DArray:       out << "    return int3(width, height, layers);"; break;
              case EbtISampler2D:           out << "    return int2(width, height);";         break;
              case EbtISampler3D:           out << "    return int3(width, height, depth);";  break;
              case EbtISamplerCube:         out << "    return int2(width, height);";         break;
              case EbtISampler2DArray:      out << "    return int3(width, height, layers);"; break;
              case EbtUSampler2D:           out << "    return int2(width, height);";         break;
              case EbtUSampler3D:           out << "    return int3(width, height, depth);";  break;
              case EbtUSamplerCube:         out << "    return int2(width, height);";         break;
              case EbtUSampler2DArray:      out << "    return int3(width, height, layers);"; break;
              case EbtSampler2DShadow:      out << "    return int2(width, height);";         break;
              case EbtSamplerCubeShadow:    out << "    return int2(width, height);";         break;
              case EbtSampler2DArrayShadow: out << "    return int3(width, height, layers);"; break;
              default: UNREACHABLE();
            }
        }
        else
        {
            if (IsIntegerSampler(textureFunction->sampler) && IsSamplerCube(textureFunction->sampler))
            {
                out << "    float width; float height; float layers; float levels;\n";

                out << "    uint mip = 0;\n";

                out << "    x.GetDimensions(mip, width, height, layers, levels);\n";

                out << "    bool xMajor = abs(t.x) > abs(t.y) && abs(t.x) > abs(t.z);\n";
                out << "    bool yMajor = abs(t.y) > abs(t.z) && abs(t.y) > abs(t.x);\n";
                out << "    bool zMajor = abs(t.z) > abs(t.x) && abs(t.z) > abs(t.y);\n";
                out << "    bool negative = (xMajor && t.x < 0.0f) || (yMajor && t.y < 0.0f) || (zMajor && t.z < 0.0f);\n";

                // FACE_POSITIVE_X = 000b
                // FACE_NEGATIVE_X = 001b
                // FACE_POSITIVE_Y = 010b
                // FACE_NEGATIVE_Y = 011b
                // FACE_POSITIVE_Z = 100b
                // FACE_NEGATIVE_Z = 101b
                out << "    int face = (int)negative + (int)yMajor * 2 + (int)zMajor * 4;\n";

                out << "    float u = xMajor ? -t.z : (yMajor && t.y < 0.0f ? -t.x : t.x);\n";
                out << "    float v = yMajor ? t.z : (negative ? t.y : -t.y);\n";
                out << "    float m = xMajor ? t.x : (yMajor ? t.y : t.z);\n";

                out << "    t.x = (u * 0.5f / m) + 0.5f;\n";
                out << "    t.y = (v * 0.5f / m) + 0.5f;\n";
            }
            else if (IsIntegerSampler(textureFunction->sampler) &&
                     textureFunction->method != TextureFunction::FETCH)
            {
                if (IsSampler2D(textureFunction->sampler))
                {
                    if (IsSamplerArray(textureFunction->sampler))
                    {
                        out << "    float width; float height; float layers; float levels;\n";

                        if (textureFunction->method == TextureFunction::LOD0)
                        {
                            out << "    uint mip = 0;\n";
                        }
                        else if (textureFunction->method == TextureFunction::LOD0BIAS)
                        {
                            out << "    uint mip = bias;\n";
                        }
                        else
                        {
                            if (textureFunction->method == TextureFunction::IMPLICIT ||
                                textureFunction->method == TextureFunction::BIAS)
                            {
                                out << "    x.GetDimensions(0, width, height, layers, levels);\n"
                                       "    float2 tSized = float2(t.x * width, t.y * height);\n"
                                       "    float dx = length(ddx(tSized));\n"
                                       "    float dy = length(ddy(tSized));\n"
                                       "    float lod = log2(max(dx, dy));\n";

                                if (textureFunction->method == TextureFunction::BIAS)
                                {
                                    out << "    lod += bias;\n";
                                }
                            }
                            else if (textureFunction->method == TextureFunction::GRAD)
                            {
                                out << "    x.GetDimensions(0, width, height, layers, levels);\n"
                                       "    float lod = log2(max(length(ddx), length(ddy)));\n";
                            }

                            out << "    uint mip = uint(min(max(round(lod), 0), levels - 1));\n";
                        }

                        out << "    x.GetDimensions(mip, width, height, layers, levels);\n";
                    }
                    else
                    {
                        out << "    float width; float height; float levels;\n";

                        if (textureFunction->method == TextureFunction::LOD0)
                        {
                            out << "    uint mip = 0;\n";
                        }
                        else if (textureFunction->method == TextureFunction::LOD0BIAS)
                        {
                            out << "    uint mip = bias;\n";
                        }
                        else
                        {
                            if (textureFunction->method == TextureFunction::IMPLICIT ||
                                textureFunction->method == TextureFunction::BIAS)
                            {
                                out << "    x.GetDimensions(0, width, height, levels);\n"
                                       "    float2 tSized = float2(t.x * width, t.y * height);\n"
                                       "    float dx = length(ddx(tSized));\n"
                                       "    float dy = length(ddy(tSized));\n"
                                       "    float lod = log2(max(dx, dy));\n";

                                if (textureFunction->method == TextureFunction::BIAS)
                                {
                                    out << "    lod += bias;\n";
                                }
                            }
                            else if (textureFunction->method == TextureFunction::LOD)
                            {
                                out << "    x.GetDimensions(0, width, height, levels);\n";
                            }
                            else if (textureFunction->method == TextureFunction::GRAD)
                            {
                                out << "    x.GetDimensions(0, width, height, levels);\n"
                                       "    float lod = log2(max(length(ddx), length(ddy)));\n";
                            }

                            out << "    uint mip = uint(min(max(round(lod), 0), levels - 1));\n";
                        }

                        out << "    x.GetDimensions(mip, width, height, levels);\n";
                    }
                }
                else if (IsSampler3D(textureFunction->sampler))
                {
                    out << "    float width; float height; float depth; float levels;\n";

                    if (textureFunction->method == TextureFunction::LOD0)
                    {
                        out << "    uint mip = 0;\n";
                    }
                    else if (textureFunction->method == TextureFunction::LOD0BIAS)
                    {
                        out << "    uint mip = bias;\n";
                    }
                    else
                    {
                        if (textureFunction->method == TextureFunction::IMPLICIT ||
                            textureFunction->method == TextureFunction::BIAS)
                        {
                            out << "    x.GetDimensions(0, width, height, depth, levels);\n"
                                   "    float3 tSized = float3(t.x * width, t.y * height, t.z * depth);\n"
                                   "    float dx = length(ddx(tSized));\n"
                                   "    float dy = length(ddy(tSized));\n"
                                   "    float lod = log2(max(dx, dy));\n";

                            if (textureFunction->method == TextureFunction::BIAS)
                            {
                                out << "    lod += bias;\n";
                            }
                        }
                        else if (textureFunction->method == TextureFunction::GRAD)
                        {
                            out << "    x.GetDimensions(0, width, height, depth, levels);\n"
                                   "    float lod = log2(max(length(ddx), length(ddy)));\n";
                        }

                        out << "    uint mip = uint(min(max(round(lod), 0), levels - 1));\n";
                    }

                    out << "    x.GetDimensions(mip, width, height, depth, levels);\n";
                }
                else UNREACHABLE();
            }

            out << "    return ";

            // HLSL intrinsic
            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                switch(textureFunction->sampler)
                {
                  case EbtSampler2D:   out << "tex2D";   break;
                  case EbtSamplerCube: out << "texCUBE"; break;
                  default: UNREACHABLE();
                }

                switch(textureFunction->method)
                {
                  case TextureFunction::IMPLICIT: out << "(s, ";     break;
                  case TextureFunction::BIAS:     out << "bias(s, "; break;
                  case TextureFunction::LOD:      out << "lod(s, ";  break;
                  case TextureFunction::LOD0:     out << "lod(s, ";  break;
                  case TextureFunction::LOD0BIAS: out << "lod(s, ";  break;
                  default: UNREACHABLE();
                }
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                if (textureFunction->method == TextureFunction::GRAD)
                {
                    if (IsIntegerSampler(textureFunction->sampler))
                    {
                        out << "x.Load(";
                    }
                    else if (IsShadowSampler(textureFunction->sampler))
                    {
                        out << "x.SampleCmpLevelZero(s, ";
                    }
                    else
                    {
                        out << "x.SampleGrad(s, ";
                    }
                }
                else if (IsIntegerSampler(textureFunction->sampler) ||
                         textureFunction->method == TextureFunction::FETCH)
                {
                    out << "x.Load(";
                }
                else if (IsShadowSampler(textureFunction->sampler))
                {
                    out << "x.SampleCmp(s, ";
                }
                else
                {
                    switch(textureFunction->method)
                    {
                      case TextureFunction::IMPLICIT: out << "x.Sample(s, ";      break;
                      case TextureFunction::BIAS:     out << "x.SampleBias(s, ";  break;
                      case TextureFunction::LOD:      out << "x.SampleLevel(s, "; break;
                      case TextureFunction::LOD0:     out << "x.SampleLevel(s, "; break;
                      case TextureFunction::LOD0BIAS: out << "x.SampleLevel(s, "; break;
                      default: UNREACHABLE();
                    }
                }
            }
            else UNREACHABLE();

            // Integer sampling requires integer addresses
            TString addressx = "";
            TString addressy = "";
            TString addressz = "";
            TString close = "";

            if (IsIntegerSampler(textureFunction->sampler) ||
                textureFunction->method == TextureFunction::FETCH)
            {
                switch(hlslCoords)
                {
                  case 2: out << "int3("; break;
                  case 3: out << "int4("; break;
                  default: UNREACHABLE();
                }

                // Convert from normalized floating-point to integer
                if (textureFunction->method != TextureFunction::FETCH)
                {
                    addressx = "int(floor(width * frac((";
                    addressy = "int(floor(height * frac((";

                    if (IsSamplerArray(textureFunction->sampler))
                    {
                        addressz = "int(max(0, min(layers - 1, floor(0.5 + ";
                    }
                    else if (IsSamplerCube(textureFunction->sampler))
                    {
                        addressz = "((((";
                    }
                    else
                    {
                        addressz = "int(floor(depth * frac((";
                    }

                    close = "))))";
                }
            }
            else
            {
                switch(hlslCoords)
                {
                  case 2: out << "float2("; break;
                  case 3: out << "float3("; break;
                  case 4: out << "float4("; break;
                  default: UNREACHABLE();
                }
            }

            TString proj = "";   // Only used for projected textures

            if (textureFunction->proj)
            {
                switch(textureFunction->coords)
                {
                  case 3: proj = " / t.z"; break;
                  case 4: proj = " / t.w"; break;
                  default: UNREACHABLE();
                }
            }

            out << addressx + ("t.x" + proj) + close + ", " + addressy + ("t.y" + proj) + close;

            if (mOutputType == SH_HLSL9_OUTPUT)
            {
                if (hlslCoords >= 3)
                {
                    if (textureFunction->coords < 3)
                    {
                        out << ", 0";
                    }
                    else
                    {
                        out << ", t.z" + proj;
                    }
                }

                if (hlslCoords == 4)
                {
                    switch(textureFunction->method)
                    {
                      case TextureFunction::BIAS:     out << ", bias"; break;
                      case TextureFunction::LOD:      out << ", lod";  break;
                      case TextureFunction::LOD0:     out << ", 0";    break;
                      case TextureFunction::LOD0BIAS: out << ", bias"; break;
                      default: UNREACHABLE();
                    }
                }

                out << "));\n";
            }
            else if (mOutputType == SH_HLSL11_OUTPUT)
            {
                if (hlslCoords >= 3)
                {
                    if (IsIntegerSampler(textureFunction->sampler) && IsSamplerCube(textureFunction->sampler))
                    {
                        out << ", face";
                    }
                    else
                    {
                        out << ", " + addressz + ("t.z" + proj) + close;
                    }
                }

                if (textureFunction->method == TextureFunction::GRAD)
                {
                    if (IsIntegerSampler(textureFunction->sampler))
                    {
                        out << ", mip)";
                    }
                    else if (IsShadowSampler(textureFunction->sampler))
                    {
                        // Compare value
                        switch(textureFunction->coords)
                        {
                          case 3: out << "), t.z"; break;
                          case 4: out << "), t.w"; break;
                          default: UNREACHABLE();
                        }
                    }
                    else
                    {
                        out << "), ddx, ddy";
                    }
                }
                else if (IsIntegerSampler(textureFunction->sampler) ||
                         textureFunction->method == TextureFunction::FETCH)
                {
                    out << ", mip)";
                }
                else if (IsShadowSampler(textureFunction->sampler))
                {
                    // Compare value
                    switch(textureFunction->coords)
                    {
                      case 3: out << "), t.z"; break;
                      case 4: out << "), t.w"; break;
                      default: UNREACHABLE();
                    }
                }
                else
                {
                    switch(textureFunction->method)
                    {
                      case TextureFunction::IMPLICIT: out << ")";       break;
                      case TextureFunction::BIAS:     out << "), bias"; break;
                      case TextureFunction::LOD:      out << "), lod";  break;
                      case TextureFunction::LOD0:     out << "), 0";    break;
                      case TextureFunction::LOD0BIAS: out << "), bias"; break;
                      default: UNREACHABLE();
                    }
                }

                if (textureFunction->offset)
                {
                    out << ", offset";
                }

                out << ");";
            }
            else UNREACHABLE();
        }

        out << "\n"
               "}\n"
               "\n";
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

    // Handle accessing std140 structs by value
    if (mFlaggedStructMappedNames.count(node) > 0)
    {
        out << mFlaggedStructMappedNames[node];
        return;
    }

    TString name = node->getSymbol();

    if (name == "gl_DepthRange")
    {
        mUsesDepthRange = true;
        out << name;
    }
    else
    {
        TQualifier qualifier = node->getQualifier();

        if (qualifier == EvqUniform)
        {
            const TType& nodeType = node->getType();
            const TInterfaceBlock* interfaceBlock = nodeType.getInterfaceBlock();

            if (interfaceBlock)
            {
                mReferencedInterfaceBlocks[interfaceBlock->name()] = node;
            }
            else
            {
                mReferencedUniforms[name] = node;
            }

            out << DecorateUniform(name, nodeType);
        }
        else if (qualifier == EvqAttribute || qualifier == EvqVertexIn)
        {
            mReferencedAttributes[name] = node;
            out << Decorate(name);
        }
        else if (IsVarying(qualifier))
        {
            mReferencedVaryings[name] = node;
            out << Decorate(name);
        }
        else if (qualifier == EvqFragmentOut)
        {
            mReferencedOutputVariables[name] = node;
            out << "out_" << name;
        }
        else if (qualifier == EvqFragColor)
        {
            out << "gl_Color[0]";
            mUsesFragColor = true;
        }
        else if (qualifier == EvqFragData)
        {
            out << "gl_Color";
            mUsesFragData = true;
        }
        else if (qualifier == EvqFragCoord)
        {
            mUsesFragCoord = true;
            out << name;
        }
        else if (qualifier == EvqPointCoord)
        {
            mUsesPointCoord = true;
            out << name;
        }
        else if (qualifier == EvqFrontFacing)
        {
            mUsesFrontFacing = true;
            out << name;
        }
        else if (qualifier == EvqPointSize)
        {
            mUsesPointSize = true;
            out << name;
        }
        else if (name == "gl_FragDepthEXT")
        {
            mUsesFragDepth = true;
            out << "gl_Depth";
        }
        else if (qualifier == EvqInternal)
        {
            out << name;
        }
        else
        {
            out << Decorate(name);
        }
    }
}

void OutputHLSL::visitRaw(TIntermRaw *node)
{
    mBody << node->getRawText();
}

bool OutputHLSL::visitBinary(Visit visit, TIntermBinary *node)
{
    TInfoSinkBase &out = mBody;

    // Handle accessing std140 structs by value
    if (mFlaggedStructMappedNames.count(node) > 0)
    {
        out << mFlaggedStructMappedNames[node];
        return false;
    }

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
      case EOpIndexDirect:
        {
            const TType& leftType = node->getLeft()->getType();
            if (leftType.isInterfaceBlock())
            {
                if (visit == PreVisit)
                {
                    TInterfaceBlock* interfaceBlock = leftType.getInterfaceBlock();
                    const int arrayIndex = node->getRight()->getAsConstantUnion()->getIConst(0);
                    mReferencedInterfaceBlocks[interfaceBlock->instanceName()] = node->getLeft()->getAsSymbolNode();
                    out << mUniformHLSL->interfaceBlockInstanceString(*interfaceBlock, arrayIndex);
                    return false;
                }
            }
            else
            {
                outputTriplet(visit, "", "[", "]");
            }
        }
        break;
      case EOpIndexIndirect:
        // We do not currently support indirect references to interface blocks
        ASSERT(node->getLeft()->getBasicType() != EbtInterfaceBlock);
        outputTriplet(visit, "", "[", "]");
        break;
      case EOpIndexDirectStruct:
        if (visit == InVisit)
        {
            const TStructure* structure = node->getLeft()->getType().getStruct();
            const TIntermConstantUnion* index = node->getRight()->getAsConstantUnion();
            const TField* field = structure->fields()[index->getIConst(0)];
            out << "." + DecorateField(field->name(), *structure);

            return false;
        }
        break;
      case EOpIndexDirectInterfaceBlock:
        if (visit == InVisit)
        {
            const TInterfaceBlock* interfaceBlock = node->getLeft()->getType().getInterfaceBlock();
            const TIntermConstantUnion* index = node->getRight()->getAsConstantUnion();
            const TField* field = interfaceBlock->fields()[index->getIConst(0)];
            out << "." + Decorate(field->name());

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
                TIntermSequence *sequence = swizzle->getSequence();

                for (TIntermSequence::iterator sit = sequence->begin(); sit != sequence->end(); sit++)
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

            const TStructure &structure = *node->getLeft()->getType().getStruct();
            const TFieldList &fields = structure.fields();

            for (size_t i = 0; i < fields.size(); i++)
            {
                const TField *field = fields[i];

                node->getLeft()->traverse(this);
                out << "." + DecorateField(field->name(), structure) + " == ";
                node->getRight()->traverse(this);
                out << "." + DecorateField(field->name(), structure);

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
            ASSERT(node->getLeft()->isMatrix() || node->getLeft()->isVector());

            if (node->getOp() == EOpEqual)
            {
                outputTriplet(visit, "all(", " == ", ")");
            }
            else
            {
                outputTriplet(visit, "!all(", " == ", ")");
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
        if (node->getRight()->hasSideEffects())
        {
            out << "s" << mUnfoldShortCircuit->getNextTemporaryIndex();
            return false;
        }
        else
        {
           outputTriplet(visit, "(", " || ", ")");
           return true;
        }
      case EOpLogicalXor:
        mUsesXor = true;
        outputTriplet(visit, "xor(", ", ", ")");
        break;
      case EOpLogicalAnd:
        if (node->getRight()->hasSideEffects())
        {
            out << "s" << mUnfoldShortCircuit->getNextTemporaryIndex();
            return false;
        }
        else
        {
           outputTriplet(visit, "(", " && ", ")");
           return true;
        }
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitUnary(Visit visit, TIntermUnary *node)
{
    switch (node->getOp())
    {
      case EOpNegative:         outputTriplet(visit, "(-", "", ")");         break;
      case EOpPositive:         outputTriplet(visit, "(+", "", ")");         break;
      case EOpVectorLogicalNot: outputTriplet(visit, "(!", "", ")");         break;
      case EOpLogicalNot:       outputTriplet(visit, "(!", "", ")");         break;
      case EOpPostIncrement:    outputTriplet(visit, "(", "", "++)");        break;
      case EOpPostDecrement:    outputTriplet(visit, "(", "", "--)");        break;
      case EOpPreIncrement:     outputTriplet(visit, "(++", "", ")");        break;
      case EOpPreDecrement:     outputTriplet(visit, "(--", "", ")");        break;
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
            }

            for (TIntermSequence::iterator sit = node->getSequence()->begin(); sit != node->getSequence()->end(); sit++)
            {
                outputLineDirective((*sit)->getLine().first_line);

                traverseStatements(*sit);

                out << ";\n";
            }

            if (mInsideFunction)
            {
                outputLineDirective(node->getLine().last_line);
                out << "}\n";
            }

            return false;
        }
      case EOpDeclaration:
        if (visit == PreVisit)
        {
            TIntermSequence *sequence = node->getSequence();
            TIntermTyped *variable = (*sequence)[0]->getAsTyped();

            if (variable && (variable->getQualifier() == EvqTemporary || variable->getQualifier() == EvqGlobal))
            {
                TStructure *structure = variable->getType().getStruct();

                if (structure)
                {
                    mStructureHLSL->addConstructor(variable->getType(), StructNameString(*structure), NULL);
                }

                if (!variable->getAsSymbolNode() || variable->getAsSymbolNode()->getSymbol() != "")   // Variable declaration
                {
                    for (TIntermSequence::iterator sit = sequence->begin(); sit != sequence->end(); sit++)
                    {
                        if (isSingleStatement(*sit))
                        {
                            mUnfoldShortCircuit->traverse(*sit);
                        }

                        if (!mInsideFunction)
                        {
                            out << "static ";
                        }

                        out << TypeString(variable->getType()) + " ";

                        TIntermSymbol *symbol = (*sit)->getAsSymbolNode();

                        if (symbol)
                        {
                            symbol->traverse(this);
                            out << ArrayString(symbol->getType());
                            out << " = " + initializer(symbol->getType());
                        }
                        else
                        {
                            (*sit)->traverse(this);
                        }

                        if (*sit != sequence->back())
                        {
                            out << ";\n";
                        }
                    }
                }
                else if (variable->getAsSymbolNode() && variable->getAsSymbolNode()->getSymbol() == "")   // Type (struct) declaration
                {
                    // Already added to constructor map
                }
                else UNREACHABLE();
            }
            else if (variable && IsVaryingOut(variable->getQualifier()))
            {
                for (TIntermSequence::iterator sit = sequence->begin(); sit != sequence->end(); sit++)
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
      case EOpInvariantDeclaration:
        // Do not do any translation
        return false;
      case EOpPrototype:
        if (visit == PreVisit)
        {
            out << TypeString(node->getType()) << " " << Decorate(TFunction::unmangleName(node->getName())) << (mOutputLod0Function ? "Lod0(" : "(");

            TIntermSequence *arguments = node->getSequence();

            for (unsigned int i = 0; i < arguments->size(); i++)
            {
                TIntermSymbol *symbol = (*arguments)[i]->getAsSymbolNode();

                if (symbol)
                {
                    out << argumentString(symbol);

                    if (i < arguments->size() - 1)
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

            out << TypeString(node->getType()) << " ";

            if (name == "main")
            {
                out << "gl_main(";
            }
            else
            {
                out << Decorate(name) << (mOutputLod0Function ? "Lod0(" : "(");
            }

            TIntermSequence *sequence = node->getSequence();
            TIntermSequence *arguments = (*sequence)[0]->getAsAggregate()->getSequence();

            for (unsigned int i = 0; i < arguments->size(); i++)
            {
                TIntermSymbol *symbol = (*arguments)[i]->getAsSymbolNode();

                if (symbol)
                {
                    TStructure *structure = symbol->getType().getStruct();

                    if (structure)
                    {
                        mStructureHLSL->addConstructor(symbol->getType(), StructNameString(*structure), NULL);
                    }

                    out << argumentString(symbol);

                    if (i < arguments->size() - 1)
                    {
                        out << ", ";
                    }
                }
                else UNREACHABLE();
            }

            out << ")\n"
                "{\n";

            if (sequence->size() > 1)
            {
                mInsideFunction = true;
                (*sequence)[1]->traverse(this);
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
            TIntermSequence *arguments = node->getSequence();

            if (node->isUserDefined())
            {
                out << Decorate(name) << (lod0 ? "Lod0(" : "(");
            }
            else
            {
                TBasicType samplerType = (*arguments)[0]->getAsTyped()->getType().getBasicType();

                TextureFunction textureFunction;
                textureFunction.sampler = samplerType;
                textureFunction.coords = (*arguments)[1]->getAsTyped()->getNominalSize();
                textureFunction.method = TextureFunction::IMPLICIT;
                textureFunction.proj = false;
                textureFunction.offset = false;

                if (name == "texture2D" || name == "textureCube" || name == "texture")
                {
                    textureFunction.method = TextureFunction::IMPLICIT;
                }
                else if (name == "texture2DProj" || name == "textureProj")
                {
                    textureFunction.method = TextureFunction::IMPLICIT;
                    textureFunction.proj = true;
                }
                else if (name == "texture2DLod" || name == "textureCubeLod" || name == "textureLod" ||
                         name == "texture2DLodEXT" || name == "textureCubeLodEXT")
                {
                    textureFunction.method = TextureFunction::LOD;
                }
                else if (name == "texture2DProjLod" || name == "textureProjLod" || name == "texture2DProjLodEXT")
                {
                    textureFunction.method = TextureFunction::LOD;
                    textureFunction.proj = true;
                }
                else if (name == "textureSize")
                {
                    textureFunction.method = TextureFunction::SIZE;
                }
                else if (name == "textureOffset")
                {
                    textureFunction.method = TextureFunction::IMPLICIT;
                    textureFunction.offset = true;
                }
                else if (name == "textureProjOffset")
                {
                    textureFunction.method = TextureFunction::IMPLICIT;
                    textureFunction.offset = true;
                    textureFunction.proj = true;
                }
                else if (name == "textureLodOffset")
                {
                    textureFunction.method = TextureFunction::LOD;
                    textureFunction.offset = true;
                }
                else if (name == "textureProjLodOffset")
                {
                    textureFunction.method = TextureFunction::LOD;
                    textureFunction.proj = true;
                    textureFunction.offset = true;
                }
                else if (name == "texelFetch")
                {
                    textureFunction.method = TextureFunction::FETCH;
                }
                else if (name == "texelFetchOffset")
                {
                    textureFunction.method = TextureFunction::FETCH;
                    textureFunction.offset = true;
                }
                else if (name == "textureGrad" || name == "texture2DGradEXT")
                {
                    textureFunction.method = TextureFunction::GRAD;
                }
                else if (name == "textureGradOffset")
                {
                    textureFunction.method = TextureFunction::GRAD;
                    textureFunction.offset = true;
                }
                else if (name == "textureProjGrad" || name == "texture2DProjGradEXT" || name == "textureCubeGradEXT")
                {
                    textureFunction.method = TextureFunction::GRAD;
                    textureFunction.proj = true;
                }
                else if (name == "textureProjGradOffset")
                {
                    textureFunction.method = TextureFunction::GRAD;
                    textureFunction.proj = true;
                    textureFunction.offset = true;
                }
                else UNREACHABLE();

                if (textureFunction.method == TextureFunction::IMPLICIT)   // Could require lod 0 or have a bias argument
                {
                    unsigned int mandatoryArgumentCount = 2;   // All functions have sampler and coordinate arguments

                    if (textureFunction.offset)
                    {
                        mandatoryArgumentCount++;
                    }

                    bool bias = (arguments->size() > mandatoryArgumentCount);   // Bias argument is optional

                    if (lod0 || mContext.shaderType == GL_VERTEX_SHADER)
                    {
                        if (bias)
                        {
                            textureFunction.method = TextureFunction::LOD0BIAS;
                        }
                        else
                        {
                            textureFunction.method = TextureFunction::LOD0;
                        }
                    }
                    else if (bias)
                    {
                        textureFunction.method = TextureFunction::BIAS;
                    }
                }

                mUsesTexture.insert(textureFunction);

                out << textureFunction.name();
            }

            for (TIntermSequence::iterator arg = arguments->begin(); arg != arguments->end(); arg++)
            {
                if (mOutputType == SH_HLSL11_OUTPUT && IsSampler((*arg)->getAsTyped()->getBasicType()))
                {
                    out << "texture_";
                    (*arg)->traverse(this);
                    out << ", sampler_";
                }

                (*arg)->traverse(this);

                if (arg < arguments->end() - 1)
                {
                    out << ", ";
                }
            }

            out << ")";

            return false;
        }
        break;
      case EOpParameters:       outputTriplet(visit, "(", ", ", ")\n{\n");                                break;
      case EOpConstructFloat:   outputConstructor(visit, node->getType(), "vec1", node->getSequence());  break;
      case EOpConstructVec2:    outputConstructor(visit, node->getType(), "vec2", node->getSequence());  break;
      case EOpConstructVec3:    outputConstructor(visit, node->getType(), "vec3", node->getSequence());  break;
      case EOpConstructVec4:    outputConstructor(visit, node->getType(), "vec4", node->getSequence());  break;
      case EOpConstructBool:    outputConstructor(visit, node->getType(), "bvec1", node->getSequence()); break;
      case EOpConstructBVec2:   outputConstructor(visit, node->getType(), "bvec2", node->getSequence()); break;
      case EOpConstructBVec3:   outputConstructor(visit, node->getType(), "bvec3", node->getSequence()); break;
      case EOpConstructBVec4:   outputConstructor(visit, node->getType(), "bvec4", node->getSequence()); break;
      case EOpConstructInt:     outputConstructor(visit, node->getType(), "ivec1", node->getSequence()); break;
      case EOpConstructIVec2:   outputConstructor(visit, node->getType(), "ivec2", node->getSequence()); break;
      case EOpConstructIVec3:   outputConstructor(visit, node->getType(), "ivec3", node->getSequence()); break;
      case EOpConstructIVec4:   outputConstructor(visit, node->getType(), "ivec4", node->getSequence()); break;
      case EOpConstructUInt:    outputConstructor(visit, node->getType(), "uvec1", node->getSequence()); break;
      case EOpConstructUVec2:   outputConstructor(visit, node->getType(), "uvec2", node->getSequence()); break;
      case EOpConstructUVec3:   outputConstructor(visit, node->getType(), "uvec3", node->getSequence()); break;
      case EOpConstructUVec4:   outputConstructor(visit, node->getType(), "uvec4", node->getSequence()); break;
      case EOpConstructMat2:    outputConstructor(visit, node->getType(), "mat2", node->getSequence());  break;
      case EOpConstructMat3:    outputConstructor(visit, node->getType(), "mat3", node->getSequence());  break;
      case EOpConstructMat4:    outputConstructor(visit, node->getType(), "mat4", node->getSequence());  break;
      case EOpConstructStruct:
        {
            const TString &structName = StructNameString(*node->getType().getStruct());
            mStructureHLSL->addConstructor(node->getType(), structName, node->getSequence());
            outputTriplet(visit, structName + "_ctor(", ", ", ")");
        }
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
            const int modValue = (*node->getSequence())[0]->getAsTyped()->getNominalSize() * 10 +
                (*node->getSequence())[1]->getAsTyped()->getNominalSize();
            switch (modValue)
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
        ASSERT(node->getSequence()->size() == 2);   // atan(x) is a unary operator
        switch ((*node->getSequence())[0]->getAsTyped()->getNominalSize())
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
            switch ((*node->getSequence())[0]->getAsTyped()->getNominalSize())   // Number of components in the first argument
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

        // D3D errors when there is a gradient operation in a loop in an unflattened if
        // however flattening all the ifs in branch heavy shaders made D3D error too.
        // As a temporary workaround we flatten the ifs only if there is at least a loop
        // present somewhere in the shader.
        if (mContext.shaderType == GL_FRAGMENT_SHADER && mContainsAnyLoop)
        {
            out << "FLATTEN ";
        }

        out << "if (";

        node->getCondition()->traverse(this);

        out << ")\n";

        outputLineDirective(node->getLine().first_line);
        out << "{\n";

        bool discard = false;

        if (node->getTrueBlock())
        {
            traverseStatements(node->getTrueBlock());

            // Detect true discard
            discard = (discard || FindDiscard::search(node->getTrueBlock()));
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

            // Detect false discard
            discard = (discard || FindDiscard::search(node->getFalseBlock()));
        }

        // ANGLE issue 486: Detect problematic conditional discard
        if (discard && FindSideEffectRewriting::search(node))
        {
            mUsesDiscardRewriting = true;
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
    mNestedLoopDepth++;

    bool wasDiscontinuous = mInsideDiscontinuousLoop;

    if (mContainsLoopDiscontinuity && !mInsideDiscontinuousLoop)
    {
        mInsideDiscontinuousLoop = containsLoopDiscontinuity(node);
    }

    if (mOutputType == SH_HLSL9_OUTPUT)
    {
        if (handleExcessiveLoop(node))
        {
            mInsideDiscontinuousLoop = wasDiscontinuous;
            mNestedLoopDepth--;

            return false;
        }
    }

    TInfoSinkBase &out = mBody;

    if (node->getType() == ELoopDoWhile)
    {
        out << "{LOOP do\n";

        outputLineDirective(node->getLine().first_line);
        out << "{\n";
    }
    else
    {
        out << "{LOOP for(";

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
    mNestedLoopDepth--;

    return false;
}

bool OutputHLSL::visitBranch(Visit visit, TIntermBranch *node)
{
    TInfoSinkBase &out = mBody;

    switch (node->getFlowOp())
    {
      case EOpKill:
        outputTriplet(visit, "discard;\n", "", "");
        break;
      case EOpBreak:
        if (visit == PreVisit)
        {
            if (mNestedLoopDepth > 1)
            {
                mUsesNestedBreak = true;
            }

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
        else if (aggregate->getOp() == EOpDeclaration)
        {
            // Declaring multiple comma-separated variables must be considered multiple statements
            // because each individual declaration has side effects which are visible in the next.
            return false;
        }
        else
        {
            for (TIntermSequence::iterator sit = aggregate->getSequence()->begin(); sit != aggregate->getSequence()->end(); sit++)
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
            TIntermSequence *sequence = init->getSequence();
            TIntermTyped *variable = (*sequence)[0]->getAsTyped();

            if (variable && variable->getQualifier() == EvqTemporary)
            {
                TIntermBinary *assign = variable->getAsBinaryNode();

                if (assign->getOp() == EOpInitialize)
                {
                    TIntermSymbol *symbol = assign->getLeft()->getAsSymbolNode();
                    TIntermConstantUnion *constant = assign->getRight()->getAsConstantUnion();

                    if (symbol && constant)
                    {
                        if (constant->getBasicType() == EbtInt && constant->isScalar())
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
                if (constant->getBasicType() == EbtInt && constant->isScalar())
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
                if (constant->getBasicType() == EbtInt && constant->isScalar())
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
                    out << "if (!Break";
                    index->traverse(this);
                    out << ") {\n";
                }

                if (iterations <= MAX_LOOP_ITERATIONS)   // Last loop fragment
                {
                    mExcessiveLoopIndex = NULL;   // Stops setting the Break flag
                }

                // for(int index = initial; index < clampedLimit; index += increment)

                out << "LOOP for(";
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
        name = Decorate(name);
    }

    if (mOutputType == SH_HLSL11_OUTPUT && IsSampler(type.getBasicType()))
    {
        return QualifierString(qualifier) + " " + TextureString(type) + " texture_" + name + ArrayString(type) + ", " +
               QualifierString(qualifier) + " " + SamplerString(type) + " sampler_" + name + ArrayString(type);
    }

    return QualifierString(qualifier) + " " + TypeString(type) + " " + name + ArrayString(type);
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

void OutputHLSL::outputConstructor(Visit visit, const TType &type, const TString &name, const TIntermSequence *parameters)
{
    TInfoSinkBase &out = mBody;

    if (visit == PreVisit)
    {
        mStructureHLSL->addConstructor(type, name, parameters);

        out << name + "(";
    }
    else if (visit == InVisit)
    {
        out << ", ";
    }
    else if (visit == PostVisit)
    {
        out << ")";
    }
}

const ConstantUnion *OutputHLSL::writeConstantUnion(const TType &type, const ConstantUnion *constUnion)
{
    TInfoSinkBase &out = mBody;

    const TStructure* structure = type.getStruct();
    if (structure)
    {
        out << StructNameString(*structure) + "_ctor(";

        const TFieldList& fields = structure->fields();

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
            out << TypeString(type) << "(";
        }

        for (size_t i = 0; i < size; i++, constUnion++)
        {
            switch (constUnion->getType())
            {
              case EbtFloat: out << std::min(FLT_MAX, std::max(-FLT_MAX, constUnion->getFConst())); break;
              case EbtInt:   out << constUnion->getIConst(); break;
              case EbtUInt:  out << constUnion->getUConst(); break;
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

}
