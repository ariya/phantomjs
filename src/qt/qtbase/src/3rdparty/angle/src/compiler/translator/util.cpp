//
// Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/util.h"

#include <limits>

#include "compiler/preprocessor/numeric_lex.h"
#include "compiler/translator/SymbolTable.h"
#include "common/utilities.h"

bool atof_clamp(const char *str, float *value)
{
    bool success = pp::numeric_lex_float(str, value);
    if (!success)
        *value = std::numeric_limits<float>::max();
    return success;
}

bool atoi_clamp(const char *str, int *value)
{
    bool success = pp::numeric_lex_int(str, value);
    if (!success)
        *value = std::numeric_limits<int>::max();
    return success;
}

namespace sh
{

GLenum GLVariableType(const TType &type)
{
    if (type.getBasicType() == EbtFloat)
    {
        if (type.isScalar())
        {
            return GL_FLOAT;
        }
        else if (type.isVector())
        {
            switch (type.getNominalSize())
            {
              case 2: return GL_FLOAT_VEC2;
              case 3: return GL_FLOAT_VEC3;
              case 4: return GL_FLOAT_VEC4;
              default: UNREACHABLE();
            }
        }
        else if (type.isMatrix())
        {
            switch (type.getCols())
            {
              case 2:
                switch (type.getRows())
                {
                  case 2: return GL_FLOAT_MAT2;
                  case 3: return GL_FLOAT_MAT2x3;
                  case 4: return GL_FLOAT_MAT2x4;
                  default: UNREACHABLE();
                }

              case 3:
                switch (type.getRows())
                {
                  case 2: return GL_FLOAT_MAT3x2;
                  case 3: return GL_FLOAT_MAT3;
                  case 4: return GL_FLOAT_MAT3x4;
                  default: UNREACHABLE();
                }

              case 4:
                switch (type.getRows())
                {
                  case 2: return GL_FLOAT_MAT4x2;
                  case 3: return GL_FLOAT_MAT4x3;
                  case 4: return GL_FLOAT_MAT4;
                  default: UNREACHABLE();
                }

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
            switch (type.getNominalSize())
            {
              case 2: return GL_INT_VEC2;
              case 3: return GL_INT_VEC3;
              case 4: return GL_INT_VEC4;
              default: UNREACHABLE();
            }
        }
        else UNREACHABLE();
    }
    else if (type.getBasicType() == EbtUInt)
    {
        if (type.isScalar())
        {
            return GL_UNSIGNED_INT;
        }
        else if (type.isVector())
        {
            switch (type.getNominalSize())
            {
              case 2: return GL_UNSIGNED_INT_VEC2;
              case 3: return GL_UNSIGNED_INT_VEC3;
              case 4: return GL_UNSIGNED_INT_VEC4;
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
            switch (type.getNominalSize())
            {
              case 2: return GL_BOOL_VEC2;
              case 3: return GL_BOOL_VEC3;
              case 4: return GL_BOOL_VEC4;
              default: UNREACHABLE();
            }
        }
        else UNREACHABLE();
    }

    switch (type.getBasicType())
    {
      case EbtSampler2D:            return GL_SAMPLER_2D;
      case EbtSampler3D:            return GL_SAMPLER_3D;
      case EbtSamplerCube:          return GL_SAMPLER_CUBE;
      case EbtSamplerExternalOES:   return GL_SAMPLER_EXTERNAL_OES;
      case EbtSampler2DRect:        return GL_SAMPLER_2D_RECT_ARB;
      case EbtSampler2DArray:       return GL_SAMPLER_2D_ARRAY;
      case EbtISampler2D:           return GL_INT_SAMPLER_2D;
      case EbtISampler3D:           return GL_INT_SAMPLER_3D;
      case EbtISamplerCube:         return GL_INT_SAMPLER_CUBE;
      case EbtISampler2DArray:      return GL_INT_SAMPLER_2D_ARRAY;
      case EbtUSampler2D:           return GL_UNSIGNED_INT_SAMPLER_2D;
      case EbtUSampler3D:           return GL_UNSIGNED_INT_SAMPLER_3D;
      case EbtUSamplerCube:         return GL_UNSIGNED_INT_SAMPLER_CUBE;
      case EbtUSampler2DArray:      return GL_UNSIGNED_INT_SAMPLER_2D_ARRAY;
      case EbtSampler2DShadow:      return GL_SAMPLER_2D_SHADOW;
      case EbtSamplerCubeShadow:    return GL_SAMPLER_CUBE_SHADOW;
      case EbtSampler2DArrayShadow: return GL_SAMPLER_2D_ARRAY_SHADOW;
      default: UNREACHABLE();
    }

    return GL_NONE;
}

GLenum GLVariablePrecision(const TType &type)
{
    if (type.getBasicType() == EbtFloat)
    {
        switch (type.getPrecision())
        {
          case EbpHigh:
            return GL_HIGH_FLOAT;
          case EbpMedium:
            return GL_MEDIUM_FLOAT;
          case EbpLow:
            return GL_LOW_FLOAT;
          case EbpUndefined:
          // Should be defined as the default precision by the parser
          default:
            UNREACHABLE();
        }
    }
    else if (type.getBasicType() == EbtInt || type.getBasicType() == EbtUInt)
    {
        switch (type.getPrecision())
        {
          case EbpHigh:
            return GL_HIGH_INT;
          case EbpMedium:
            return GL_MEDIUM_INT;
          case EbpLow:
            return GL_LOW_INT;
          case EbpUndefined:
          // Should be defined as the default precision by the parser
          default:
            UNREACHABLE();
        }
    }

    // Other types (boolean, sampler) don't have a precision
    return GL_NONE;
}

TString ArrayString(const TType &type)
{
    if (!type.isArray())
    {
        return "";
    }

    return "[" + str(type.getArraySize()) + "]";
}

bool IsVaryingOut(TQualifier qualifier)
{
    switch (qualifier)
    {
      case EvqVaryingOut:
      case EvqInvariantVaryingOut:
      case EvqSmoothOut:
      case EvqFlatOut:
      case EvqCentroidOut:
      case EvqVertexOut:
        return true;

      default: break;
    }

    return false;
}

bool IsVaryingIn(TQualifier qualifier)
{
    switch (qualifier)
    {
      case EvqVaryingIn:
      case EvqInvariantVaryingIn:
      case EvqSmoothIn:
      case EvqFlatIn:
      case EvqCentroidIn:
      case EvqFragmentIn:
        return true;

      default: break;
    }

    return false;
}

bool IsVarying(TQualifier qualifier)
{
    return IsVaryingIn(qualifier) || IsVaryingOut(qualifier);
}

InterpolationType GetInterpolationType(TQualifier qualifier)
{
    switch (qualifier)
    {
      case EvqFlatIn:
      case EvqFlatOut:
        return INTERPOLATION_FLAT;

      case EvqSmoothIn:
      case EvqSmoothOut:
      case EvqVertexOut:
      case EvqFragmentIn:
      case EvqVaryingIn:
      case EvqVaryingOut:
      case EvqInvariantVaryingIn:
      case EvqInvariantVaryingOut:
        return INTERPOLATION_SMOOTH;

      case EvqCentroidIn:
      case EvqCentroidOut:
        return INTERPOLATION_CENTROID;

      default: UNREACHABLE();
        return INTERPOLATION_SMOOTH;
    }
}

GetVariableTraverser::GetVariableTraverser(const TSymbolTable &symbolTable)
    : mSymbolTable(symbolTable)
{
}

template void GetVariableTraverser::setTypeSpecificInfo(
    const TType &type, const TString& name, InterfaceBlockField *variable);
template void GetVariableTraverser::setTypeSpecificInfo(
    const TType &type, const TString& name, ShaderVariable *variable);
template void GetVariableTraverser::setTypeSpecificInfo(
    const TType &type, const TString& name, Uniform *variable);

template<>
void GetVariableTraverser::setTypeSpecificInfo(
    const TType &type, const TString& name, Varying *variable)
{
    ASSERT(variable);
    switch (type.getQualifier())
    {
      case EvqInvariantVaryingIn:
      case EvqInvariantVaryingOut:
        variable->isInvariant = true;
        break;
      case EvqVaryingIn:
      case EvqVaryingOut:
        if (mSymbolTable.isVaryingInvariant(name))
        {
            variable->isInvariant = true;
        }
        break;
      default:
        break;
    }

    variable->interpolation = GetInterpolationType(type.getQualifier());
}

template <typename VarT>
void GetVariableTraverser::traverse(const TType &type,
                                    const TString &name,
                                    std::vector<VarT> *output)
{
    const TStructure *structure = type.getStruct();

    VarT variable;
    variable.name = name.c_str();
    variable.arraySize = static_cast<unsigned int>(type.getArraySize());

    if (!structure)
    {
        variable.type = GLVariableType(type);
        variable.precision = GLVariablePrecision(type);
    }
    else
    {
        // Note: this enum value is not exposed outside ANGLE
        variable.type = GL_STRUCT_ANGLEX;
        variable.structName = structure->name().c_str();

        const TFieldList &fields = structure->fields();

        for (size_t fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
        {
            TField *field = fields[fieldIndex];
            traverse(*field->type(), field->name(), &variable.fields);
        }
    }
    setTypeSpecificInfo(type, name, &variable);
    visitVariable(&variable);

    ASSERT(output);
    output->push_back(variable);
}

template void GetVariableTraverser::traverse(const TType &, const TString &, std::vector<InterfaceBlockField> *);
template void GetVariableTraverser::traverse(const TType &, const TString &, std::vector<ShaderVariable> *);
template void GetVariableTraverser::traverse(const TType &, const TString &, std::vector<Uniform> *);
template void GetVariableTraverser::traverse(const TType &, const TString &, std::vector<Varying> *);

}
