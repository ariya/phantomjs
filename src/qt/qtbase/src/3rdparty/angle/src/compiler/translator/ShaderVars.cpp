//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderVars.cpp:
//  Methods for GL variable types (varyings, uniforms, etc)
//

#include <GLSLANG/ShaderLang.h>

#include "compiler/translator/compilerdebug.h"

namespace sh
{

ShaderVariable::ShaderVariable()
    : type(0),
      precision(0),
      arraySize(0),
      staticUse(false)
{}

ShaderVariable::ShaderVariable(GLenum typeIn, unsigned int arraySizeIn)
    : type(typeIn),
      precision(0),
      arraySize(arraySizeIn),
      staticUse(false)
{}

ShaderVariable::~ShaderVariable()
{}

ShaderVariable::ShaderVariable(const ShaderVariable &other)
    : type(other.type),
      precision(other.precision),
      name(other.name),
      mappedName(other.mappedName),
      arraySize(other.arraySize),
      staticUse(other.staticUse),
      fields(other.fields),
      structName(other.structName)
{}

ShaderVariable &ShaderVariable::operator=(const ShaderVariable &other)
{
    type = other.type;
    precision = other.precision;
    name = other.name;
    mappedName = other.mappedName;
    arraySize = other.arraySize;
    staticUse = other.staticUse;
    fields = other.fields;
    structName = other.structName;
    return *this;
}

bool ShaderVariable::operator==(const ShaderVariable &other) const
{
    if (type != other.type ||
        precision != other.precision ||
        name != other.name ||
        mappedName != other.mappedName ||
        arraySize != other.arraySize ||
        staticUse != other.staticUse ||
        fields.size() != other.fields.size() ||
        structName != other.structName)
    {
        return false;
    }
    for (size_t ii = 0; ii < fields.size(); ++ii)
    {
        if (fields[ii] != other.fields[ii])
            return false;
    }
    return true;
}

bool ShaderVariable::findInfoByMappedName(
    const std::string &mappedFullName,
    const ShaderVariable **leafVar, std::string *originalFullName) const
{
    ASSERT(leafVar && originalFullName);
    // There are three cases:
    // 1) the top variable is of struct type;
    // 2) the top variable is an array;
    // 3) otherwise.
    size_t pos = mappedFullName.find_first_of(".[");
    std::string topName;

    if (pos == std::string::npos)
    {
        // Case 3.
        if (mappedFullName != this->mappedName)
            return false;
        *originalFullName = this->name;
        *leafVar = this;
        return true;
    }
    else
    {
        std::string topName = mappedFullName.substr(0, pos);
        if (topName != this->mappedName)
            return false;
        std::string originalName = this->name;
        std::string remaining;
        if (mappedFullName[pos] == '[')
        {
            // Case 2.
            size_t closePos = mappedFullName.find_first_of(']');
            if (closePos < pos || closePos == std::string::npos)
                return false;
            // Append '[index]'.
            originalName += mappedFullName.substr(pos, closePos - pos + 1);
            if (closePos + 1 == mappedFullName.size())
            {
                *originalFullName = originalName;
                *leafVar = this;
                return true;
            }
            else
            {
                // In the form of 'a[0].b', so after ']', '.' is expected.
                if (mappedFullName[closePos + 1]  != '.')
                    return false;
                remaining = mappedFullName.substr(closePos + 2);  // Skip "]."
            }
        }
        else
        {
            // Case 1.
            remaining = mappedFullName.substr(pos + 1);  // Skip "."
        }
        for (size_t ii = 0; ii < this->fields.size(); ++ii)
        {
            const ShaderVariable *fieldVar = NULL;
            std::string originalFieldName;
            bool found = fields[ii].findInfoByMappedName(
                remaining, &fieldVar, &originalFieldName);
            if (found)
            {
                *originalFullName = originalName + "." + originalFieldName;
                *leafVar = fieldVar;
                return true;
            }
        }
        return false;
    }
}

bool ShaderVariable::isSameVariableAtLinkTime(
    const ShaderVariable &other, bool matchPrecision) const
{
    if (type != other.type)
        return false;
    if (matchPrecision && precision != other.precision)
        return false;
    if (name != other.name)
        return false;
    ASSERT(mappedName == other.mappedName);
    if (arraySize != other.arraySize)
        return false;
    if (fields.size() != other.fields.size())
        return false;
    for (size_t ii = 0; ii < fields.size(); ++ii)
    {
        if (!fields[ii].isSameVariableAtLinkTime(other.fields[ii],
                                                 matchPrecision))
        {
            return false;
        }
    }
    if (structName != other.structName)
        return false;
    return true;
}

Uniform::Uniform()
{}

Uniform::~Uniform()
{}

Uniform::Uniform(const Uniform &other)
    : ShaderVariable(other)
{}

Uniform &Uniform::operator=(const Uniform &other)
{
    ShaderVariable::operator=(other);
    return *this;
}

bool Uniform::operator==(const Uniform &other) const
{
    return ShaderVariable::operator==(other);
}

bool Uniform::isSameUniformAtLinkTime(const Uniform &other) const
{
    return ShaderVariable::isSameVariableAtLinkTime(other, true);
}

Attribute::Attribute()
    : location(-1)
{}

Attribute::~Attribute()
{}

Attribute::Attribute(const Attribute &other)
    : ShaderVariable(other),
      location(other.location)
{}

Attribute &Attribute::operator=(const Attribute &other)
{
    ShaderVariable::operator=(other);
    location = other.location;
    return *this;
}

bool Attribute::operator==(const Attribute &other) const
{
    return (ShaderVariable::operator==(other) &&
            location == other.location);
}

InterfaceBlockField::InterfaceBlockField()
    : isRowMajorLayout(false)
{}

InterfaceBlockField::~InterfaceBlockField()
{}

InterfaceBlockField::InterfaceBlockField(const InterfaceBlockField &other)
    : ShaderVariable(other),
      isRowMajorLayout(other.isRowMajorLayout)
{}

InterfaceBlockField &InterfaceBlockField::operator=(const InterfaceBlockField &other)
{
    ShaderVariable::operator=(other);
    isRowMajorLayout = other.isRowMajorLayout;
    return *this;
}

bool InterfaceBlockField::operator==(const InterfaceBlockField &other) const
{
    return (ShaderVariable::operator==(other) &&
            isRowMajorLayout == other.isRowMajorLayout);
}

bool InterfaceBlockField::isSameInterfaceBlockFieldAtLinkTime(
    const InterfaceBlockField &other) const
{
    return (ShaderVariable::isSameVariableAtLinkTime(other, true) &&
            isRowMajorLayout == other.isRowMajorLayout);
}

Varying::Varying()
    : interpolation(INTERPOLATION_SMOOTH),
      isInvariant(false)
{}

Varying::~Varying()
{}

Varying::Varying(const Varying &other)
    : ShaderVariable(other),
      interpolation(other.interpolation),
      isInvariant(other.isInvariant)
{}

Varying &Varying::operator=(const Varying &other)
{
    ShaderVariable::operator=(other);
    interpolation = other.interpolation;
    isInvariant = other.isInvariant;
    return *this;
}

bool Varying::operator==(const Varying &other) const
{
    return (ShaderVariable::operator==(other) &&
            interpolation == other.interpolation &&
            isInvariant == other.isInvariant);
}

bool Varying::isSameVaryingAtLinkTime(const Varying &other) const
{
    return (ShaderVariable::isSameVariableAtLinkTime(other, false) &&
            interpolation == other.interpolation &&
            isInvariant == other.isInvariant);
}

InterfaceBlock::InterfaceBlock()
    : arraySize(0),
      layout(BLOCKLAYOUT_PACKED),
      isRowMajorLayout(false),
      staticUse(false)
{}

InterfaceBlock::~InterfaceBlock()
{}

InterfaceBlock::InterfaceBlock(const InterfaceBlock &other)
    : name(other.name),
      mappedName(other.mappedName),
      instanceName(other.instanceName),
      arraySize(other.arraySize),
      layout(other.layout),
      isRowMajorLayout(other.isRowMajorLayout),
      staticUse(other.staticUse),
      fields(other.fields)
{}

InterfaceBlock &InterfaceBlock::operator=(const InterfaceBlock &other)
{
    name = other.name;
    mappedName = other.mappedName;
    instanceName = other.instanceName;
    arraySize = other.arraySize;
    layout = other.layout;
    isRowMajorLayout = other.isRowMajorLayout;
    staticUse = other.staticUse;
    fields = other.fields;
    return *this;
}

}
