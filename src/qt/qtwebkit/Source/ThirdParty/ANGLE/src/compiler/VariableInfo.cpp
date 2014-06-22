//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/VariableInfo.h"

static TString arrayBrackets(int index)
{
    TStringStream stream;
    stream << "[" << index << "]";
    return stream.str();
}

// Returns the data type for an attribute or uniform.
static ShDataType getVariableDataType(const TType& type)
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

static void getBuiltInVariableInfo(const TType& type,
                                   const TString& name,
                                   const TString& mappedName,
                                   TVariableInfoList& infoList);
static void getUserDefinedVariableInfo(const TType& type,
                                       const TString& name,
                                       const TString& mappedName,
                                       TVariableInfoList& infoList,
                                       ShHashFunction64 hashFunction);

// Returns info for an attribute or uniform.
static void getVariableInfo(const TType& type,
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
    } else {
        varInfo.name = name.c_str();
        varInfo.mappedName = mappedName.c_str();
        varInfo.size = 1;
    }
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

TVariableInfo::TVariableInfo()
{
}

TVariableInfo::TVariableInfo(ShDataType type, int size)
    : type(type),
      size(size)
{
}

CollectAttribsUniforms::CollectAttribsUniforms(TVariableInfoList& attribs,
                                               TVariableInfoList& uniforms,
                                               ShHashFunction64 hashFunction)
    : mAttribs(attribs),
      mUniforms(uniforms),
      mHashFunction(hashFunction)
{
}

// We are only interested in attribute and uniform variable declaration.
void CollectAttribsUniforms::visitSymbol(TIntermSymbol*)
{
}

void CollectAttribsUniforms::visitConstantUnion(TIntermConstantUnion*)
{
}

bool CollectAttribsUniforms::visitBinary(Visit, TIntermBinary*)
{
    return false;
}

bool CollectAttribsUniforms::visitUnary(Visit, TIntermUnary*)
{
    return false;
}

bool CollectAttribsUniforms::visitSelection(Visit, TIntermSelection*)
{
    return false;
}

bool CollectAttribsUniforms::visitAggregate(Visit, TIntermAggregate* node)
{
    bool visitChildren = false;

    switch (node->getOp())
    {
    case EOpSequence:
        // We need to visit sequence children to get to variable declarations.
        visitChildren = true;
        break;
    case EOpDeclaration: {
        const TIntermSequence& sequence = node->getSequence();
        TQualifier qualifier = sequence.front()->getAsTyped()->getQualifier();
        if (qualifier == EvqAttribute || qualifier == EvqUniform)
        {
            TVariableInfoList& infoList = qualifier == EvqAttribute ?
                mAttribs : mUniforms;
            for (TIntermSequence::const_iterator i = sequence.begin();
                 i != sequence.end(); ++i)
            {
                const TIntermSymbol* variable = (*i)->getAsSymbolNode();
                // The only case in which the sequence will not contain a
                // TIntermSymbol node is initialization. It will contain a
                // TInterBinary node in that case. Since attributes and unifroms
                // cannot be initialized in a shader, we must have only
                // TIntermSymbol nodes in the sequence.
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
            }
        }
        break;
    }
    default: break;
    }

    return visitChildren;
}

bool CollectAttribsUniforms::visitLoop(Visit, TIntermLoop*)
{
    return false;
}

bool CollectAttribsUniforms::visitBranch(Visit, TIntermBranch*)
{
    return false;
}

