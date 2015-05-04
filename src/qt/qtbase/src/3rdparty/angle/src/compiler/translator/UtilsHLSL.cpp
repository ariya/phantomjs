//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UtilsHLSL.cpp:
//   Utility methods for GLSL to HLSL translation.
//

#include "compiler/translator/UtilsHLSL.h"
#include "compiler/translator/StructureHLSL.h"
#include "compiler/translator/SymbolTable.h"

namespace sh
{

TString SamplerString(const TType &type)
{
    if (IsShadowSampler(type.getBasicType()))
    {
        return "SamplerComparisonState";
    }
    else
    {
        return "SamplerState";
    }
}

TString TextureString(const TType &type)
{
    switch (type.getBasicType())
    {
      case EbtSampler2D:            return "Texture2D";
      case EbtSamplerCube:          return "TextureCube";
      case EbtSamplerExternalOES:   return "Texture2D";
      case EbtSampler2DArray:       return "Texture2DArray";
      case EbtSampler3D:            return "Texture3D";
      case EbtISampler2D:           return "Texture2D<int4>";
      case EbtISampler3D:           return "Texture3D<int4>";
      case EbtISamplerCube:         return "Texture2DArray<int4>";
      case EbtISampler2DArray:      return "Texture2DArray<int4>";
      case EbtUSampler2D:           return "Texture2D<uint4>";
      case EbtUSampler3D:           return "Texture3D<uint4>";
      case EbtUSamplerCube:         return "Texture2DArray<uint4>";
      case EbtUSampler2DArray:      return "Texture2DArray<uint4>";
      case EbtSampler2DShadow:      return "Texture2D";
      case EbtSamplerCubeShadow:    return "TextureCube";
      case EbtSampler2DArrayShadow: return "Texture2DArray";
      default: UNREACHABLE();
    }

    return "<unknown texture type>";
}

TString DecorateUniform(const TString &string, const TType &type)
{
    if (type.getBasicType() == EbtSamplerExternalOES)
    {
        return "ex_" + string;
    }

    return Decorate(string);
}

TString DecorateField(const TString &string, const TStructure &structure)
{
    if (structure.name().compare(0, 3, "gl_") != 0)
    {
        return Decorate(string);
    }

    return string;
}

TString DecoratePrivate(const TString &privateText)
{
    return "dx_" + privateText;
}

TString Decorate(const TString &string)
{
    if (string.compare(0, 3, "gl_") != 0)
    {
        return "_" + string;
    }

    return string;
}

TString TypeString(const TType &type)
{
    const TStructure* structure = type.getStruct();
    if (structure)
    {
        const TString& typeName = structure->name();
        if (typeName != "")
        {
            return StructNameString(*structure);
        }
        else   // Nameless structure, define in place
        {
            return StructureHLSL::defineNameless(*structure);
        }
    }
    else if (type.isMatrix())
    {
        int cols = type.getCols();
        int rows = type.getRows();
        return "float" + str(cols) + "x" + str(rows);
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
          case EbtUInt:
            switch (type.getNominalSize())
            {
              case 1: return "uint";
              case 2: return "uint2";
              case 3: return "uint3";
              case 4: return "uint4";
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
          case EbtISampler2D:
          case EbtUSampler2D:
          case EbtSampler2DArray:
          case EbtISampler2DArray:
          case EbtUSampler2DArray:
            return "sampler2D";
          case EbtSamplerCube:
          case EbtISamplerCube:
          case EbtUSamplerCube:
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

TString StructNameString(const TStructure &structure)
{
    if (structure.name().empty())
    {
        return "";
    }

    return "ss" + str(structure.uniqueId()) + "_" + structure.name();
}

TString QualifiedStructNameString(const TStructure &structure, bool useHLSLRowMajorPacking,
                                  bool useStd140Packing)
{
    if (structure.name() == "")
    {
        return "";
    }

    TString prefix = "";

    // Structs packed with row-major matrices in HLSL are prefixed with "rm"
    // GLSL column-major maps to HLSL row-major, and the converse is true

    if (useStd140Packing)
    {
        prefix += "std_";
    }

    if (useHLSLRowMajorPacking)
    {
        prefix += "rm_";
    }

    return prefix + StructNameString(structure);
}

TString InterpolationString(TQualifier qualifier)
{
    switch (qualifier)
    {
      case EvqVaryingIn:           return "";
      case EvqFragmentIn:          return "";
      case EvqInvariantVaryingIn:  return "";
      case EvqSmoothIn:            return "linear";
      case EvqFlatIn:              return "nointerpolation";
      case EvqCentroidIn:          return "centroid";
      case EvqVaryingOut:          return "";
      case EvqVertexOut:           return "";
      case EvqInvariantVaryingOut: return "";
      case EvqSmoothOut:           return "linear";
      case EvqFlatOut:             return "nointerpolation";
      case EvqCentroidOut:         return "centroid";
      default: UNREACHABLE();
    }

    return "";
}

TString QualifierString(TQualifier qualifier)
{
    switch (qualifier)
    {
      case EvqIn:            return "in";
      case EvqOut:           return "inout"; // 'out' results in an HLSL error if not all fields are written, for GLSL it's undefined
      case EvqInOut:         return "inout";
      case EvqConstReadOnly: return "const";
      default: UNREACHABLE();
    }

    return "";
}

}
