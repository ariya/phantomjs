//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/VariableInfo.h"
#include "compiler/translator/util.h"
#include "common/utilities.h"

namespace sh
{

namespace
{

TString InterfaceBlockFieldName(const TInterfaceBlock &interfaceBlock, const TField &field)
{
    if (interfaceBlock.hasInstanceName())
    {
        return interfaceBlock.name() + "." + field.name();
    }
    else
    {
        return field.name();
    }
}

BlockLayoutType GetBlockLayoutType(TLayoutBlockStorage blockStorage)
{
    switch (blockStorage)
    {
      case EbsPacked:         return BLOCKLAYOUT_PACKED;
      case EbsShared:         return BLOCKLAYOUT_SHARED;
      case EbsStd140:         return BLOCKLAYOUT_STANDARD;
      default: UNREACHABLE(); return BLOCKLAYOUT_SHARED;
    }
}

void ExpandUserDefinedVariable(const ShaderVariable &variable,
                               const std::string &name,
                               const std::string &mappedName,
                               bool markStaticUse,
                               std::vector<ShaderVariable> *expanded);

void ExpandVariable(const ShaderVariable &variable,
                    const std::string &name,
                    const std::string &mappedName,
                    bool markStaticUse,
                    std::vector<ShaderVariable> *expanded)
{
    if (variable.isStruct())
    {
        if (variable.isArray())
        {
            for (size_t elementIndex = 0; elementIndex < variable.elementCount(); elementIndex++)
            {
                std::string lname = name + ::ArrayString(elementIndex);
                std::string lmappedName = mappedName + ::ArrayString(elementIndex);
                ExpandUserDefinedVariable(variable, lname, lmappedName, markStaticUse, expanded);
            }
        }
        else
        {
            ExpandUserDefinedVariable(variable, name, mappedName, markStaticUse, expanded);
        }
    }
    else
    {
        ShaderVariable expandedVar = variable;

        expandedVar.name = name;
        expandedVar.mappedName = mappedName;

        // Mark all expanded fields as used if the parent is used
        if (markStaticUse)
        {
            expandedVar.staticUse = true;
        }

        if (expandedVar.isArray())
        {
            expandedVar.name += "[0]";
            expandedVar.mappedName += "[0]";
        }

        expanded->push_back(expandedVar);
    }
}

void ExpandUserDefinedVariable(const ShaderVariable &variable,
                               const std::string &name,
                               const std::string &mappedName,
                               bool markStaticUse,
                               std::vector<ShaderVariable> *expanded)
{
    ASSERT(variable.isStruct());

    const std::vector<ShaderVariable> &fields = variable.fields;

    for (size_t fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
    {
        const ShaderVariable &field = fields[fieldIndex];
        ExpandVariable(field,
                       name + "." + field.name,
                       mappedName + "." + field.mappedName,
                       markStaticUse,
                       expanded);
    }
}

template <class VarT>
VarT *FindVariable(const TString &name,
                  std::vector<VarT> *infoList)
{
    // TODO(zmo): optimize this function.
    for (size_t ii = 0; ii < infoList->size(); ++ii)
    {
        if ((*infoList)[ii].name.c_str() == name)
            return &((*infoList)[ii]);
    }

    return NULL;
}

}

CollectVariables::CollectVariables(std::vector<sh::Attribute> *attribs,
                                   std::vector<sh::Attribute> *outputVariables,
                                   std::vector<sh::Uniform> *uniforms,
                                   std::vector<sh::Varying> *varyings,
                                   std::vector<sh::InterfaceBlock> *interfaceBlocks,
                                   ShHashFunction64 hashFunction,
                                   const TSymbolTable &symbolTable)
    : mAttribs(attribs),
      mOutputVariables(outputVariables),
      mUniforms(uniforms),
      mVaryings(varyings),
      mInterfaceBlocks(interfaceBlocks),
      mPointCoordAdded(false),
      mFrontFacingAdded(false),
      mFragCoordAdded(false),
      mPositionAdded(false),
      mPointSizeAdded(false),
      mHashFunction(hashFunction),
      mSymbolTable(symbolTable)
{
}

// We want to check whether a uniform/varying is statically used
// because we only count the used ones in packing computing.
// Also, gl_FragCoord, gl_PointCoord, and gl_FrontFacing count
// toward varying counting if they are statically used in a fragment
// shader.
void CollectVariables::visitSymbol(TIntermSymbol *symbol)
{
    ASSERT(symbol != NULL);
    ShaderVariable *var = NULL;
    const TString &symbolName = symbol->getSymbol();

    if (IsVarying(symbol->getQualifier()))
    {
        var = FindVariable(symbolName, mVaryings);
    }
    else if (symbol->getType().getBasicType() == EbtInterfaceBlock)
    {
        UNREACHABLE();
    }
    else
    {
        switch (symbol->getQualifier())
        {
          case EvqAttribute:
          case EvqVertexIn:
            var = FindVariable(symbolName, mAttribs);
            break;
          case EvqFragmentOut:
            var = FindVariable(symbolName, mOutputVariables);
            break;
          case EvqUniform:
            {
                const TInterfaceBlock *interfaceBlock = symbol->getType().getInterfaceBlock();
                if (interfaceBlock)
                {
                    InterfaceBlock *namedBlock = FindVariable(interfaceBlock->name(), mInterfaceBlocks);
                    ASSERT(namedBlock);
                    var = FindVariable(symbolName, &namedBlock->fields);

                    // Set static use on the parent interface block here
                    namedBlock->staticUse = true;

                }
                else
                {
                    var = FindVariable(symbolName, mUniforms);
                }

                // It's an internal error to reference an undefined user uniform
                ASSERT(symbolName.compare(0, 3, "gl_") == 0 || var);
            }
            break;
          case EvqFragCoord:
            if (!mFragCoordAdded)
            {
                Varying info;
                const char kName[] = "gl_FragCoord";
                info.name = kName;
                info.mappedName = kName;
                info.type = GL_FLOAT_VEC4;
                info.arraySize = 0;
                info.precision = GL_MEDIUM_FLOAT;  // Defined by spec.
                info.staticUse = true;
                info.isInvariant = mSymbolTable.isVaryingInvariant(kName);
                mVaryings->push_back(info);
                mFragCoordAdded = true;
            }
            return;
          case EvqFrontFacing:
            if (!mFrontFacingAdded)
            {
                Varying info;
                const char kName[] = "gl_FrontFacing";
                info.name = kName;
                info.mappedName = kName;
                info.type = GL_BOOL;
                info.arraySize = 0;
                info.precision = GL_NONE;
                info.staticUse = true;
                info.isInvariant = mSymbolTable.isVaryingInvariant(kName);
                mVaryings->push_back(info);
                mFrontFacingAdded = true;
            }
            return;
          case EvqPointCoord:
            if (!mPointCoordAdded)
            {
                Varying info;
                const char kName[] = "gl_PointCoord";
                info.name = kName;
                info.mappedName = kName;
                info.type = GL_FLOAT_VEC2;
                info.arraySize = 0;
                info.precision = GL_MEDIUM_FLOAT;  // Defined by spec.
                info.staticUse = true;
                info.isInvariant = mSymbolTable.isVaryingInvariant(kName);
                mVaryings->push_back(info);
                mPointCoordAdded = true;
            }
            return;
          case EvqPosition:
            if (!mPositionAdded)
            {
                Varying info;
                const char kName[] = "gl_Position";
                info.name = kName;
                info.mappedName = kName;
                info.type = GL_FLOAT_VEC4;
                info.arraySize = 0;
                info.precision = GL_HIGH_FLOAT;  // Defined by spec.
                info.staticUse = true;
                info.isInvariant = mSymbolTable.isVaryingInvariant(kName);
                mVaryings->push_back(info);
                mPositionAdded = true;
            }
            return;
          case EvqPointSize:
            if (!mPointSizeAdded)
            {
                Varying info;
                const char kName[] = "gl_PointSize";
                info.name = kName;
                info.mappedName = kName;
                info.type = GL_FLOAT;
                info.arraySize = 0;
                info.precision = GL_MEDIUM_FLOAT;  // Defined by spec.
                info.staticUse = true;
                info.isInvariant = mSymbolTable.isVaryingInvariant(kName);
                mVaryings->push_back(info);
                mPointSizeAdded = true;
            }
            return;
          default:
            break;
        }
    }
    if (var)
    {
        var->staticUse = true;
    }
}

class NameHashingTraverser : public GetVariableTraverser
{
  public:
    NameHashingTraverser(ShHashFunction64 hashFunction,
                         const TSymbolTable &symbolTable)
        : GetVariableTraverser(symbolTable),
          mHashFunction(hashFunction)
    {}

  private:
    DISALLOW_COPY_AND_ASSIGN(NameHashingTraverser);

    virtual void visitVariable(ShaderVariable *variable)
    {
        TString stringName = TString(variable->name.c_str());
        variable->mappedName = TIntermTraverser::hash(stringName, mHashFunction).c_str();
    }

    ShHashFunction64 mHashFunction;
};

// Attributes, which cannot have struct fields, are a special case
template <>
void CollectVariables::visitVariable(const TIntermSymbol *variable,
                                     std::vector<Attribute> *infoList) const
{
    ASSERT(variable);
    const TType &type = variable->getType();
    ASSERT(!type.getStruct());

    Attribute attribute;

    attribute.type = GLVariableType(type);
    attribute.precision = GLVariablePrecision(type);
    attribute.name = variable->getSymbol().c_str();
    attribute.arraySize = static_cast<unsigned int>(type.getArraySize());
    attribute.mappedName = TIntermTraverser::hash(variable->getSymbol(), mHashFunction).c_str();
    attribute.location = variable->getType().getLayoutQualifier().location;

    infoList->push_back(attribute);
}

template <>
void CollectVariables::visitVariable(const TIntermSymbol *variable,
                                     std::vector<InterfaceBlock> *infoList) const
{
    InterfaceBlock interfaceBlock;
    const TInterfaceBlock *blockType = variable->getType().getInterfaceBlock();
    ASSERT(blockType);

    interfaceBlock.name = blockType->name().c_str();
    interfaceBlock.mappedName = TIntermTraverser::hash(variable->getSymbol(), mHashFunction).c_str();
    interfaceBlock.instanceName = (blockType->hasInstanceName() ? blockType->instanceName().c_str() : "");
    interfaceBlock.arraySize = variable->getArraySize();
    interfaceBlock.isRowMajorLayout = (blockType->matrixPacking() == EmpRowMajor);
    interfaceBlock.layout = GetBlockLayoutType(blockType->blockStorage());

    // Gather field information
    const TFieldList &fieldList = blockType->fields();

    for (size_t fieldIndex = 0; fieldIndex < fieldList.size(); ++fieldIndex)
    {
        const TField &field = *fieldList[fieldIndex];
        const TString &fullFieldName = InterfaceBlockFieldName(*blockType, field);
        const TType &fieldType = *field.type();

        GetVariableTraverser traverser(mSymbolTable);
        traverser.traverse(fieldType, fullFieldName, &interfaceBlock.fields);

        interfaceBlock.fields.back().isRowMajorLayout = (fieldType.getLayoutQualifier().matrixPacking == EmpRowMajor);
    }

    infoList->push_back(interfaceBlock);
}

template <typename VarT>
void CollectVariables::visitVariable(const TIntermSymbol *variable,
                                     std::vector<VarT> *infoList) const
{
    NameHashingTraverser traverser(mHashFunction, mSymbolTable);
    traverser.traverse(variable->getType(), variable->getSymbol(), infoList);
}

template <typename VarT>
void CollectVariables::visitInfoList(const TIntermSequence &sequence,
                                     std::vector<VarT> *infoList) const
{
    for (size_t seqIndex = 0; seqIndex < sequence.size(); seqIndex++)
    {
        const TIntermSymbol *variable = sequence[seqIndex]->getAsSymbolNode();
        // The only case in which the sequence will not contain a
        // TIntermSymbol node is initialization. It will contain a
        // TInterBinary node in that case. Since attributes, uniforms,
        // and varyings cannot be initialized in a shader, we must have
        // only TIntermSymbol nodes in the sequence.
        ASSERT(variable != NULL);
        visitVariable(variable, infoList);
    }
}

bool CollectVariables::visitAggregate(Visit, TIntermAggregate *node)
{
    bool visitChildren = true;

    switch (node->getOp())
    {
      case EOpDeclaration:
        {
            const TIntermSequence &sequence = *(node->getSequence());
            ASSERT(!sequence.empty());

            const TIntermTyped &typedNode = *(sequence.front()->getAsTyped());
            TQualifier qualifier = typedNode.getQualifier();

            if (typedNode.getBasicType() == EbtInterfaceBlock)
            {
                visitInfoList(sequence, mInterfaceBlocks);
                visitChildren = false;
            }
            else if (qualifier == EvqAttribute || qualifier == EvqVertexIn ||
                     qualifier == EvqFragmentOut || qualifier == EvqUniform ||
                     IsVarying(qualifier))
            {
                switch (qualifier)
                {
                  case EvqAttribute:
                  case EvqVertexIn:
                    visitInfoList(sequence, mAttribs);
                    break;
                  case EvqFragmentOut:
                    visitInfoList(sequence, mOutputVariables);
                    break;
                  case EvqUniform:
                    visitInfoList(sequence, mUniforms);
                    break;
                  default:
                    visitInfoList(sequence, mVaryings);
                    break;
                }

                visitChildren = false;
            }
            break;
        }
      default: break;
    }

    return visitChildren;
}

bool CollectVariables::visitBinary(Visit, TIntermBinary *binaryNode)
{
    if (binaryNode->getOp() == EOpIndexDirectInterfaceBlock)
    {
        // NOTE: we do not determine static use for individual blocks of an array
        TIntermTyped *blockNode = binaryNode->getLeft()->getAsTyped();
        ASSERT(blockNode);

        TIntermConstantUnion *constantUnion = binaryNode->getRight()->getAsConstantUnion();
        ASSERT(constantUnion);

        const TInterfaceBlock *interfaceBlock = blockNode->getType().getInterfaceBlock();
        InterfaceBlock *namedBlock = FindVariable(interfaceBlock->name(), mInterfaceBlocks);
        ASSERT(namedBlock);
        namedBlock->staticUse = true;

        unsigned int fieldIndex = constantUnion->getUConst(0);
        ASSERT(fieldIndex < namedBlock->fields.size());
        namedBlock->fields[fieldIndex].staticUse = true;
        return false;
    }

    return true;
}

void ExpandUniforms(const std::vector<Uniform> &compact,
                    std::vector<ShaderVariable> *expanded)
{
    for (size_t variableIndex = 0; variableIndex < compact.size(); variableIndex++)
    {
        const ShaderVariable &variable = compact[variableIndex];
        ExpandVariable(variable, variable.name, variable.mappedName, variable.staticUse, expanded);
    }
}

}
