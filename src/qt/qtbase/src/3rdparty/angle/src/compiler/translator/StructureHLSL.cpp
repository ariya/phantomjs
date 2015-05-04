//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// StructureHLSL.cpp:
//   Definitions of methods for HLSL translation of GLSL structures.
//

#include "compiler/translator/StructureHLSL.h"
#include "common/utilities.h"
#include "compiler/translator/OutputHLSL.h"
#include "compiler/translator/Types.h"
#include "compiler/translator/util.h"
#include "compiler/translator/UtilsHLSL.h"

namespace sh
{

Std140PaddingHelper::Std140PaddingHelper(const std::map<TString, int> &structElementIndexes,
                                         unsigned *uniqueCounter)
    : mPaddingCounter(uniqueCounter),
      mElementIndex(0),
      mStructElementIndexes(structElementIndexes)
{}

TString Std140PaddingHelper::next()
{
    unsigned value = (*mPaddingCounter)++;
    return str(value);
}

int Std140PaddingHelper::prePadding(const TType &type)
{
    if (type.getBasicType() == EbtStruct || type.isMatrix() || type.isArray())
    {
        // no padding needed, HLSL will align the field to a new register
        mElementIndex = 0;
        return 0;
    }

    const GLenum glType = GLVariableType(type);
    const int numComponents = gl::VariableComponentCount(glType);

    if (numComponents >= 4)
    {
        // no padding needed, HLSL will align the field to a new register
        mElementIndex = 0;
        return 0;
    }

    if (mElementIndex + numComponents > 4)
    {
        // no padding needed, HLSL will align the field to a new register
        mElementIndex = numComponents;
        return 0;
    }

    const int alignment = numComponents == 3 ? 4 : numComponents;
    const int paddingOffset = (mElementIndex % alignment);
    const int paddingCount = (paddingOffset != 0 ? (alignment - paddingOffset) : 0);

    mElementIndex += paddingCount;
    mElementIndex += numComponents;
    mElementIndex %= 4;

    return paddingCount;
}

TString Std140PaddingHelper::prePaddingString(const TType &type)
{
    int paddingCount = prePadding(type);

    TString padding;

    for (int paddingIndex = 0; paddingIndex < paddingCount; paddingIndex++)
    {
        padding += "    float pad_" + next() + ";\n";
    }

    return padding;
}

TString Std140PaddingHelper::postPaddingString(const TType &type, bool useHLSLRowMajorPacking)
{
    if (!type.isMatrix() && !type.isArray() && type.getBasicType() != EbtStruct)
    {
        return "";
    }

    int numComponents = 0;
    TStructure *structure = type.getStruct();

    if (type.isMatrix())
    {
        // This method can also be called from structureString, which does not use layout qualifiers.
        // Thus, use the method parameter for determining the matrix packing.
        //
        // Note HLSL row major packing corresponds to GL API column-major, and vice-versa, since we
        // wish to always transpose GL matrices to play well with HLSL's matrix array indexing.
        //
        const bool isRowMajorMatrix = !useHLSLRowMajorPacking;
        const GLenum glType = GLVariableType(type);
        numComponents = gl::MatrixComponentCount(glType, isRowMajorMatrix);
    }
    else if (structure)
    {
        const TString &structName = QualifiedStructNameString(*structure,
                                                              useHLSLRowMajorPacking, true);
        numComponents = mStructElementIndexes.find(structName)->second;

        if (numComponents == 0)
        {
            return "";
        }
    }
    else
    {
        const GLenum glType = GLVariableType(type);
        numComponents = gl::VariableComponentCount(glType);
    }

    TString padding;
    for (int paddingOffset = numComponents; paddingOffset < 4; paddingOffset++)
    {
        padding += "    float pad_" + next() + ";\n";
    }
    return padding;
}

StructureHLSL::StructureHLSL()
    : mUniquePaddingCounter(0)
{}

Std140PaddingHelper StructureHLSL::getPaddingHelper()
{
    return Std140PaddingHelper(mStd140StructElementIndexes, &mUniquePaddingCounter);
}

TString StructureHLSL::defineQualified(const TStructure &structure, bool useHLSLRowMajorPacking, bool useStd140Packing)
{
    if (useStd140Packing)
    {
        Std140PaddingHelper padHelper = getPaddingHelper();
        return define(structure, useHLSLRowMajorPacking, useStd140Packing, &padHelper);
    }
    else
    {
        return define(structure, useHLSLRowMajorPacking, useStd140Packing, NULL);
    }
}

TString StructureHLSL::defineNameless(const TStructure &structure)
{
    return define(structure, false, false, NULL);
}

TString StructureHLSL::define(const TStructure &structure, bool useHLSLRowMajorPacking,
                              bool useStd140Packing, Std140PaddingHelper *padHelper)
{
    const TFieldList &fields = structure.fields();
    const bool isNameless = (structure.name() == "");
    const TString &structName = QualifiedStructNameString(structure, useHLSLRowMajorPacking,
                                                          useStd140Packing);
    const TString declareString = (isNameless ? "struct" : "struct " + structName);

    TString string;
    string += declareString + "\n"
              "{\n";

    for (unsigned int i = 0; i < fields.size(); i++)
    {
        const TField &field = *fields[i];
        const TType &fieldType = *field.type();
        const TStructure *fieldStruct = fieldType.getStruct();
        const TString &fieldTypeString = fieldStruct ?
                                         QualifiedStructNameString(*fieldStruct, useHLSLRowMajorPacking,
                                                                   useStd140Packing) :
                                         TypeString(fieldType);

        if (padHelper)
        {
            string += padHelper->prePaddingString(fieldType);
        }

        string += "    " + fieldTypeString + " " + DecorateField(field.name(), structure) + ArrayString(fieldType) + ";\n";

        if (padHelper)
        {
            string += padHelper->postPaddingString(fieldType, useHLSLRowMajorPacking);
        }
    }

    // Nameless structs do not finish with a semicolon and newline, to leave room for an instance variable
    string += (isNameless ? "} " : "};\n");

    return string;
}

void StructureHLSL::addConstructor(const TType &type, const TString &name, const TIntermSequence *parameters)
{
    if (name == "")
    {
        return;   // Nameless structures don't have constructors
    }

    if (type.getStruct() && mStructNames.find(name) != mStructNames.end())
    {
        return;   // Already added
    }

    TType ctorType = type;
    ctorType.clearArrayness();
    ctorType.setPrecision(EbpHigh);
    ctorType.setQualifier(EvqTemporary);

    typedef std::vector<TType> ParameterArray;
    ParameterArray ctorParameters;

    const TStructure* structure = type.getStruct();
    if (structure)
    {
        mStructNames.insert(name);

        // Add element index
        storeStd140ElementIndex(*structure, false);
        storeStd140ElementIndex(*structure, true);

        const TString &structString = defineQualified(*structure, false, false);

        if (std::find(mStructDeclarations.begin(), mStructDeclarations.end(), structString) == mStructDeclarations.end())
        {
            // Add row-major packed struct for interface blocks
            TString rowMajorString = "#pragma pack_matrix(row_major)\n" +
                defineQualified(*structure, true, false) +
                "#pragma pack_matrix(column_major)\n";

            TString std140String = defineQualified(*structure, false, true);
            TString std140RowMajorString = "#pragma pack_matrix(row_major)\n" +
                defineQualified(*structure, true, true) +
                "#pragma pack_matrix(column_major)\n";

            mStructDeclarations.push_back(structString);
            mStructDeclarations.push_back(rowMajorString);
            mStructDeclarations.push_back(std140String);
            mStructDeclarations.push_back(std140RowMajorString);
        }

        const TFieldList &fields = structure->fields();
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
        constructor += name + " " + name + "_ctor(";
    }
    else   // Built-in type
    {
        constructor += TypeString(ctorType) + " " + name + "(";
    }

    for (unsigned int parameter = 0; parameter < ctorParameters.size(); parameter++)
    {
        const TType &type = ctorParameters[parameter];

        constructor += TypeString(type) + " x" + str(parameter) + ArrayString(type);

        if (parameter < ctorParameters.size() - 1)
        {
            constructor += ", ";
        }
    }

    constructor += ")\n"
                   "{\n";

    if (ctorType.getStruct())
    {
        constructor += "    " + name + " structure = {";
    }
    else
    {
        constructor += "    return " + TypeString(ctorType) + "(";
    }

    if (ctorType.isMatrix() && ctorParameters.size() == 1)
    {
        int rows = ctorType.getRows();
        int cols = ctorType.getCols();
        const TType &parameter = ctorParameters[0];

        if (parameter.isScalar())
        {
            for (int col = 0; col < cols; col++)
            {
                for (int row = 0; row < rows; row++)
                {
                    constructor += TString((row == col) ? "x0" : "0.0");

                    if (row < rows - 1 || col < cols - 1)
                    {
                        constructor += ", ";
                    }
                }
            }
        }
        else if (parameter.isMatrix())
        {
            for (int col = 0; col < cols; col++)
            {
                for (int row = 0; row < rows; row++)
                {
                    if (row < parameter.getRows() && col < parameter.getCols())
                    {
                        constructor += TString("x0") + "[" + str(col) + "][" + str(row) + "]";
                    }
                    else
                    {
                        constructor += TString((row == col) ? "1.0" : "0.0");
                    }

                    if (row < rows - 1 || col < cols - 1)
                    {
                        constructor += ", ";
                    }
                }
            }
        }
        else
        {
            ASSERT(rows == 2 && cols == 2 && parameter.isVector() && parameter.getNominalSize() == 4);

            constructor += "x0";
        }
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

            if (ctorType.getStruct())
            {
                ASSERT(remainingComponents == parameterSize || moreParameters);
                ASSERT(parameterSize <= remainingComponents);

                remainingComponents -= parameterSize;
            }
            else if (parameter.isScalar())
            {
                remainingComponents -= parameter.getObjectSize();
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
            else if (parameter.isMatrix())
            {
                int column = 0;
                while (remainingComponents > 0 && column < parameter.getCols())
                {
                    constructor += "[" + str(column) + "]";

                    if (remainingComponents < static_cast<size_t>(parameter.getRows()))
                    {
                        switch (remainingComponents)
                        {
                          case 1:  constructor += ".x";    break;
                          case 2:  constructor += ".xy";   break;
                          case 3:  constructor += ".xyz";  break;
                          default: UNREACHABLE();
                        }

                        remainingComponents = 0;
                    }
                    else
                    {
                        remainingComponents -= parameter.getRows();

                        if (remainingComponents > 0)
                        {
                            constructor += ", x" + str(parameterIndex);
                        }
                    }

                    column++;
                }
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

std::string StructureHLSL::structsHeader() const
{
    TInfoSinkBase out;

    for (size_t structIndex = 0; structIndex < mStructDeclarations.size(); structIndex++)
    {
        out << mStructDeclarations[structIndex];
    }

    for (Constructors::const_iterator constructor = mConstructors.begin();
         constructor != mConstructors.end();
         constructor++)
    {
        out << *constructor;
    }

    return out.str();
}

void StructureHLSL::storeStd140ElementIndex(const TStructure &structure, bool useHLSLRowMajorPacking)
{
    Std140PaddingHelper padHelper = getPaddingHelper();
    const TFieldList &fields = structure.fields();

    for (unsigned int i = 0; i < fields.size(); i++)
    {
        padHelper.prePadding(*fields[i]->type());
    }

    // Add remaining element index to the global map, for use with nested structs in standard layouts
    const TString &structName = QualifiedStructNameString(structure, useHLSLRowMajorPacking, true);
    mStd140StructElementIndexes[structName] = padHelper.elementIndex();
}

}
