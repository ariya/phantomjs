//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/VariableInfo.h"

namespace {

TString arrayBrackets(int index)
{
    TStringStream stream;
    stream << "[" << index << "]";
    return stream.str();
}

// Returns the data type for an attribute, uniform, or varying.
ShDataType getVariableDataType(const TType& type)
{
    switch (type.getBasicType()) {
      case EbtFloat:
          if (type.isMatrix()) {
              switch (type.getNominalSize()) {
                case 2: return SH_FLOAT_MAT2;
                case 3: return SH_FLOAT_MAT3;
                case 4: return SH_FLOAT_MAT4;
                default: UNREACHABLE();
              }
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_FLOAT_VEC2;
                case 3: return SH_FLOAT_VEC3;
                case 4: return SH_FLOAT_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_FLOAT;
          }
      case EbtInt:
          if (type.isMatrix()) {
              UNREACHABLE();
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_INT_VEC2;
                case 3: return SH_INT_VEC3;
                case 4: return SH_INT_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_INT;
          }
      case EbtBool:
          if (type.isMatrix()) {
              UNREACHABLE();
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_BOOL_VEC2;
                case 3: return SH_BOOL_VEC3;
                case 4: return SH_BOOL_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_BOOL;
          }
      case EbtSampler2D: return SH_SAMPLER_2D;
      case EbtSamplerCube: return SH_SAMPLER_CUBE;
      case EbtSamplerExternalOES: return SH_SAMPLER_EXTERNAL_OES;
      case EbtSampler2DRect: return SH_SAMPLER_2D_RECT_ARB;
      default: UNREACHABLE();
    }
    return SH_NONE;
}

void getBuiltInVariableInfo(const TType& type,
                            const TString& name,
                            const TString& mappedName,
                            TVariableInfoList& infoList);
void getUserDefinedVariableInfo(const TType& type,
                                const TString& name,
                                const TString& mappedName,
                                TVariableInfoList& infoList,
                                ShHashFunction64 hashFunction);

// Returns info for an attribute, uniform, or varying.
void getVariableInfo(const TType& type,
                     const TString& name,
                     const TString& mappedName,
                     TVariableInfoList& infoList,
                     ShHashFunction64 hashFunction)
{
    if (type.getBasicType() == EbtStruct) {
        if (type.isArray()) {
            for (int i = 0; i < type.getArraySize(); ++i) {
                TString lname = name + arrayBrackets(i);
                TString lmappedName = mappedName + arrayBrackets(i);
                getUserDefinedVariableInfo(type, lname, lmappedName, infoList, hashFunction);
            }
        } else {
            getUserDefinedVariableInfo(type, name, mappedName, infoList, hashFunction);
        }
    } else {
        getBuiltInVariableInfo(type, name, mappedName, infoList);
    }
}

void getBuiltInVariableInfo(const TType& type,
                            const TString& name,
                            const TString& mappedName,
                            TVariableInfoList& infoList)
{
    ASSERT(type.getBasicType() != EbtStruct);

    TVariableInfo varInfo;
    if (type.isArray()) {
        varInfo.name = (name + "[0]").c_str();
        varInfo.mappedName = (mappedName + "[0]").c_str();
        varInfo.size = type.getArraySize();
        varInfo.isArray = true;
    } else {
        varInfo.name = name.c_str();
        varInfo.mappedName = mappedName.c_str();
        varInfo.size = 1;
        varInfo.isArray = false;
    }
    varInfo.precision = type.getPrecision();
    varInfo.type = getVariableDataType(type);
    infoList.push_back(varInfo);
}

void getUserDefinedVariableInfo(const TType& type,
                                const TString& name,
                                const TString& mappedName,
                                TVariableInfoList& infoList,
                                ShHashFunction64 hashFunction)
{
    ASSERT(type.getBasicType() == EbtStruct);

    const TFieldList& fields = type.getStruct()->fields();
    for (size_t i = 0; i < fields.size(); ++i) {
        const TType& fieldType = *(fields[i]->type());
        const TString& fieldName = fields[i]->name();
        getVariableInfo(fieldType,
                        name + "." + fieldName,
                        mappedName + "." + TIntermTraverser::hash(fieldName, hashFunction),
                        infoList,
                        hashFunction);
    }
}

TVariableInfo* findVariable(const TType& type,
                            const TString& name,
                            TVariableInfoList& infoList)
{
    // TODO(zmo): optimize this function.
    TString myName = name;
    if (type.isArray())
        myName += "[0]";
    for (size_t ii = 0; ii < infoList.size(); ++ii)
    {
        if (infoList[ii].name.c_str() == myName)
            return &(infoList[ii]);
    }
    return NULL;
}

}  // namespace anonymous

TVariableInfo::TVariableInfo()
    : type(SH_NONE),
      size(0),
      isArray(false),
      precision(EbpUndefined),
      staticUse(false)
{
}

TVariableInfo::TVariableInfo(ShDataType type, int size)
    : type(type),
      size(size),
      isArray(false),
      precision(EbpUndefined),
      staticUse(false)
{
}

CollectVariables::CollectVariables(TVariableInfoList& attribs,
                                   TVariableInfoList& uniforms,
                                   TVariableInfoList& varyings,
                                   ShHashFunction64 hashFunction)
    : mAttribs(attribs),
      mUniforms(uniforms),
      mVaryings(varyings),
      mPointCoordAdded(false),
      mFrontFacingAdded(false),
      mFragCoordAdded(false),
      mHashFunction(hashFunction)
{
}

// We want to check whether a uniform/varying is statically used
// because we only count the used ones in packing computing.
// Also, gl_FragCoord, gl_PointCoord, and gl_FrontFacing count
// toward varying counting if they are statically used in a fragment
// shader.
void CollectVariables::visitSymbol(TIntermSymbol* symbol)
{
    ASSERT(symbol != NULL);
    TVariableInfo* var = NULL;
    switch (symbol->getQualifier())
    {
    case EvqVaryingOut:
    case EvqInvariantVaryingOut:
    case EvqVaryingIn:
    case EvqInvariantVaryingIn:
        var = findVariable(symbol->getType(), symbol->getSymbol(), mVaryings);
        break;
    case EvqUniform:
        var = findVariable(symbol->getType(), symbol->getSymbol(), mUniforms);
        break;
    case EvqFragCoord:
        if (!mFragCoordAdded) {
            TVariableInfo info;
            info.name = "gl_FragCoord";
            info.mappedName = "gl_FragCoord";
            info.type = SH_FLOAT_VEC4;
            info.size = 1;
            info.precision = EbpMedium;  // Use mediump as it doesn't really matter.
            info.staticUse = true;
	    mVaryings.push_back(info);
            mFragCoordAdded = true;
        }
        return;
    case EvqFrontFacing:
        if (!mFrontFacingAdded) {
            TVariableInfo info;
            info.name = "gl_FrontFacing";
            info.mappedName = "gl_FrontFacing";
            info.type = SH_BOOL;
            info.size = 1;
            info.precision = EbpUndefined;
            info.staticUse = true;
	    mVaryings.push_back(info);
            mFrontFacingAdded = true;
        }
        return;
    case EvqPointCoord:
        if (!mPointCoordAdded) {
            TVariableInfo info;
            info.name = "gl_PointCoord";
            info.mappedName = "gl_PointCoord";
            info.type = SH_FLOAT_VEC2;
            info.size = 1;
            info.precision = EbpMedium;  // Use mediump as it doesn't really matter.
            info.staticUse = true;
	    mVaryings.push_back(info);
            mPointCoordAdded = true;
        }
        return;
    default:
        break;
    }
    if (var)
        var->staticUse = true;
}

bool CollectVariables::visitAggregate(Visit, TIntermAggregate* node)
{
    bool visitChildren = true;

    switch (node->getOp())
    {
    case EOpDeclaration: {
        const TIntermSequence& sequence = node->getSequence();
        TQualifier qualifier = sequence.front()->getAsTyped()->getQualifier();
        if (qualifier == EvqAttribute || qualifier == EvqUniform ||
            qualifier == EvqVaryingIn || qualifier == EvqVaryingOut ||
            qualifier == EvqInvariantVaryingIn || qualifier == EvqInvariantVaryingOut)
        {
            TVariableInfoList& infoList = qualifier == EvqAttribute ? mAttribs :
                (qualifier == EvqUniform ? mUniforms : mVaryings);
            for (TIntermSequence::const_iterator i = sequence.begin();
                 i != sequence.end(); ++i)
            {
                const TIntermSymbol* variable = (*i)->getAsSymbolNode();
                // The only case in which the sequence will not contain a
                // TIntermSymbol node is initialization. It will contain a
                // TInterBinary node in that case. Since attributes, uniforms,
                // and varyings cannot be initialized in a shader, we must have
                // only TIntermSymbol nodes in the sequence.
                ASSERT(variable != NULL);
                TString processedSymbol;
                if (mHashFunction == NULL)
                    processedSymbol = variable->getSymbol();
                else
                    processedSymbol = TIntermTraverser::hash(variable->getOriginalSymbol(), mHashFunction);
                getVariableInfo(variable->getType(),
                                variable->getOriginalSymbol(),
                                processedSymbol,
                                infoList,
                                mHashFunction);
                visitChildren = false;
            }
        }
        break;
    }
    default: break;
    }

    return visitChildren;
}

