//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/ParseContext.h"

#include <stdarg.h>
#include <stdio.h>

#include "compiler/translator/glslang.h"
#include "compiler/preprocessor/SourceLocation.h"

///////////////////////////////////////////////////////////////////////
//
// Sub- vector and matrix fields
//
////////////////////////////////////////////////////////////////////////

//
// Look at a '.' field selector string and change it into offsets
// for a vector.
//
bool TParseContext::parseVectorFields(const TString& compString, int vecSize, TVectorFields& fields, const TSourceLoc& line)
{
    fields.num = (int) compString.size();
    if (fields.num > 4) {
        error(line, "illegal vector field selection", compString.c_str());
        return false;
    }

    enum {
        exyzw,
        ergba,
        estpq
    } fieldSet[4];

    for (int i = 0; i < fields.num; ++i) {
        switch (compString[i])  {
        case 'x': 
            fields.offsets[i] = 0;
            fieldSet[i] = exyzw;
            break;
        case 'r': 
            fields.offsets[i] = 0;
            fieldSet[i] = ergba;
            break;
        case 's':
            fields.offsets[i] = 0;
            fieldSet[i] = estpq;
            break;
        case 'y': 
            fields.offsets[i] = 1;
            fieldSet[i] = exyzw;
            break;
        case 'g': 
            fields.offsets[i] = 1;
            fieldSet[i] = ergba;
            break;
        case 't':
            fields.offsets[i] = 1;
            fieldSet[i] = estpq;
            break;
        case 'z': 
            fields.offsets[i] = 2;
            fieldSet[i] = exyzw;
            break;
        case 'b': 
            fields.offsets[i] = 2;
            fieldSet[i] = ergba;
            break;
        case 'p':
            fields.offsets[i] = 2;
            fieldSet[i] = estpq;
            break;
        
        case 'w': 
            fields.offsets[i] = 3;
            fieldSet[i] = exyzw;
            break;
        case 'a': 
            fields.offsets[i] = 3;
            fieldSet[i] = ergba;
            break;
        case 'q':
            fields.offsets[i] = 3;
            fieldSet[i] = estpq;
            break;
        default:
            error(line, "illegal vector field selection", compString.c_str());
            return false;
        }
    }

    for (int i = 0; i < fields.num; ++i) {
        if (fields.offsets[i] >= vecSize) {
            error(line, "vector field selection out of range",  compString.c_str());
            return false;
        }

        if (i > 0) {
            if (fieldSet[i] != fieldSet[i-1]) {
                error(line, "illegal - vector component fields not from the same set", compString.c_str());
                return false;
            }
        }
    }

    return true;
}


//
// Look at a '.' field selector string and change it into offsets
// for a matrix.
//
bool TParseContext::parseMatrixFields(const TString& compString, int matCols, int matRows, TMatrixFields& fields, const TSourceLoc& line)
{
    fields.wholeRow = false;
    fields.wholeCol = false;
    fields.row = -1;
    fields.col = -1;

    if (compString.size() != 2) {
        error(line, "illegal length of matrix field selection", compString.c_str());
        return false;
    }

    if (compString[0] == '_') {
        if (compString[1] < '0' || compString[1] > '3') {
            error(line, "illegal matrix field selection", compString.c_str());
            return false;
        }
        fields.wholeCol = true;
        fields.col = compString[1] - '0';
    } else if (compString[1] == '_') {
        if (compString[0] < '0' || compString[0] > '3') {
            error(line, "illegal matrix field selection", compString.c_str());
            return false;
        }
        fields.wholeRow = true;
        fields.row = compString[0] - '0';
    } else {
        if (compString[0] < '0' || compString[0] > '3' ||
            compString[1] < '0' || compString[1] > '3') {
            error(line, "illegal matrix field selection", compString.c_str());
            return false;
        }
        fields.row = compString[0] - '0';
        fields.col = compString[1] - '0';
    }

    if (fields.row >= matRows || fields.col >= matCols) {
        error(line, "matrix field selection out of range", compString.c_str());
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////
//
// Errors
//
////////////////////////////////////////////////////////////////////////

//
// Track whether errors have occurred.
//
void TParseContext::recover()
{
}

//
// Used by flex/bison to output all syntax and parsing errors.
//
void TParseContext::error(const TSourceLoc& loc,
                          const char* reason, const char* token, 
                          const char* extraInfo)
{
    pp::SourceLocation srcLoc;
    srcLoc.file = loc.first_file;
    srcLoc.line = loc.first_line;
    diagnostics.writeInfo(pp::Diagnostics::PP_ERROR,
                          srcLoc, reason, token, extraInfo);

}

void TParseContext::warning(const TSourceLoc& loc,
                            const char* reason, const char* token,
                            const char* extraInfo) {
    pp::SourceLocation srcLoc;
    srcLoc.file = loc.first_file;
    srcLoc.line = loc.first_line;
    diagnostics.writeInfo(pp::Diagnostics::PP_WARNING,
                          srcLoc, reason, token, extraInfo);
}

void TParseContext::trace(const char* str)
{
    diagnostics.writeDebug(str);
}

//
// Same error message for all places assignments don't work.
//
void TParseContext::assignError(const TSourceLoc& line, const char* op, TString left, TString right)
{
    std::stringstream extraInfoStream;
    extraInfoStream << "cannot convert from '" << right << "' to '" << left << "'";
    std::string extraInfo = extraInfoStream.str();
    error(line, "", op, extraInfo.c_str());
}

//
// Same error message for all places unary operations don't work.
//
void TParseContext::unaryOpError(const TSourceLoc& line, const char* op, TString operand)
{
    std::stringstream extraInfoStream;
    extraInfoStream << "no operation '" << op << "' exists that takes an operand of type " << operand 
                    << " (or there is no acceptable conversion)";
    std::string extraInfo = extraInfoStream.str();
    error(line, " wrong operand type", op, extraInfo.c_str());
}

//
// Same error message for all binary operations don't work.
//
void TParseContext::binaryOpError(const TSourceLoc& line, const char* op, TString left, TString right)
{
    std::stringstream extraInfoStream;
    extraInfoStream << "no operation '" << op << "' exists that takes a left-hand operand of type '" << left 
                    << "' and a right operand of type '" << right << "' (or there is no acceptable conversion)";
    std::string extraInfo = extraInfoStream.str();
    error(line, " wrong operand types ", op, extraInfo.c_str()); 
}

bool TParseContext::precisionErrorCheck(const TSourceLoc& line, TPrecision precision, TBasicType type){
    if (!checksPrecisionErrors)
        return false;
    switch( type ){
    case EbtFloat:
        if( precision == EbpUndefined ){
            error( line, "No precision specified for (float)", "" );
            return true;
        }
        break;
    case EbtInt:
        if( precision == EbpUndefined ){
            error( line, "No precision specified (int)", "" );
            return true;
        }
        break;
    default:
        return false;
    }
    return false;
}

//
// Both test and if necessary, spit out an error, to see if the node is really
// an l-value that can be operated on this way.
//
// Returns true if the was an error.
//
bool TParseContext::lValueErrorCheck(const TSourceLoc& line, const char* op, TIntermTyped* node)
{
    TIntermSymbol* symNode = node->getAsSymbolNode();
    TIntermBinary* binaryNode = node->getAsBinaryNode();

    if (binaryNode) {
        bool errorReturn;

        switch(binaryNode->getOp()) {
        case EOpIndexDirect:
        case EOpIndexIndirect:
        case EOpIndexDirectStruct:
        case EOpIndexDirectInterfaceBlock:
            return lValueErrorCheck(line, op, binaryNode->getLeft());
        case EOpVectorSwizzle:
            errorReturn = lValueErrorCheck(line, op, binaryNode->getLeft());
            if (!errorReturn) {
                int offset[4] = {0,0,0,0};

                TIntermTyped* rightNode = binaryNode->getRight();
                TIntermAggregate *aggrNode = rightNode->getAsAggregate();

                for (TIntermSequence::iterator p = aggrNode->getSequence()->begin();
                                               p != aggrNode->getSequence()->end(); p++) {
                    int value = (*p)->getAsTyped()->getAsConstantUnion()->getIConst(0);
                    offset[value]++;
                    if (offset[value] > 1) {
                        error(line, " l-value of swizzle cannot have duplicate components", op);

                        return true;
                    }
                }
            }

            return errorReturn;
        default:
            break;
        }
        error(line, " l-value required", op);

        return true;
    }


    const char* symbol = 0;
    if (symNode != 0)
        symbol = symNode->getSymbol().c_str();

    const char* message = 0;
    switch (node->getQualifier()) {
    case EvqConst:          message = "can't modify a const";        break;
    case EvqConstReadOnly:  message = "can't modify a const";        break;
    case EvqAttribute:      message = "can't modify an attribute";   break;
    case EvqFragmentIn:     message = "can't modify an input";       break;
    case EvqVertexIn:       message = "can't modify an input";       break;
    case EvqUniform:        message = "can't modify a uniform";      break;
    case EvqVaryingIn:      message = "can't modify a varying";      break;
    case EvqFragCoord:      message = "can't modify gl_FragCoord";   break;
    case EvqFrontFacing:    message = "can't modify gl_FrontFacing"; break;
    case EvqPointCoord:     message = "can't modify gl_PointCoord";  break;
    default:

        //
        // Type that can't be written to?
        //
        if (node->getBasicType() == EbtVoid) {
            message = "can't modify void";
        }
        if (IsSampler(node->getBasicType())) {
            message = "can't modify a sampler";
        }
    }

    if (message == 0 && binaryNode == 0 && symNode == 0) {
        error(line, " l-value required", op);

        return true;
    }


    //
    // Everything else is okay, no error.
    //
    if (message == 0)
        return false;

    //
    // If we get here, we have an error and a message.
    //
    if (symNode) {
        std::stringstream extraInfoStream;
        extraInfoStream << "\"" << symbol << "\" (" << message << ")";
        std::string extraInfo = extraInfoStream.str();
        error(line, " l-value required", op, extraInfo.c_str());
    }
    else {
        std::stringstream extraInfoStream;
        extraInfoStream << "(" << message << ")";
        std::string extraInfo = extraInfoStream.str();
        error(line, " l-value required", op, extraInfo.c_str());
    }

    return true;
}

//
// Both test, and if necessary spit out an error, to see if the node is really
// a constant.
//
// Returns true if the was an error.
//
bool TParseContext::constErrorCheck(TIntermTyped* node)
{
    if (node->getQualifier() == EvqConst)
        return false;

    error(node->getLine(), "constant expression required", "");

    return true;
}

//
// Both test, and if necessary spit out an error, to see if the node is really
// an integer.
//
// Returns true if the was an error.
//
bool TParseContext::integerErrorCheck(TIntermTyped* node, const char* token)
{
    if (node->isScalarInt())
        return false;

    error(node->getLine(), "integer expression required", token);

    return true;
}

//
// Both test, and if necessary spit out an error, to see if we are currently
// globally scoped.
//
// Returns true if the was an error.
//
bool TParseContext::globalErrorCheck(const TSourceLoc& line, bool global, const char* token)
{
    if (global)
        return false;

    error(line, "only allowed at global scope", token);

    return true;
}

//
// For now, keep it simple:  if it starts "gl_", it's reserved, independent
// of scope.  Except, if the symbol table is at the built-in push-level,
// which is when we are parsing built-ins.
// Also checks for "webgl_" and "_webgl_" reserved identifiers if parsing a
// webgl shader.
//
// Returns true if there was an error.
//
bool TParseContext::reservedErrorCheck(const TSourceLoc& line, const TString& identifier)
{
    static const char* reservedErrMsg = "reserved built-in name";
    if (!symbolTable.atBuiltInLevel()) {
        if (identifier.compare(0, 3, "gl_") == 0) {
            error(line, reservedErrMsg, "gl_");
            return true;
        }
        if (IsWebGLBasedSpec(shaderSpec)) {
            if (identifier.compare(0, 6, "webgl_") == 0) {
                error(line, reservedErrMsg, "webgl_");
                return true;
            }
            if (identifier.compare(0, 7, "_webgl_") == 0) {
                error(line, reservedErrMsg, "_webgl_");
                return true;
            }
            if (shaderSpec == SH_CSS_SHADERS_SPEC && identifier.compare(0, 4, "css_") == 0) {
                error(line, reservedErrMsg, "css_");
                return true;
            }
        }
        if (identifier.find("__") != TString::npos) {
            error(line, "identifiers containing two consecutive underscores (__) are reserved as possible future keywords", identifier.c_str());
            return true;
        }
    }

    return false;
}

//
// Make sure there is enough data provided to the constructor to build
// something of the type of the constructor.  Also returns the type of
// the constructor.
//
// Returns true if there was an error in construction.
//
bool TParseContext::constructorErrorCheck(const TSourceLoc& line, TIntermNode* node, TFunction& function, TOperator op, TType* type)
{
    *type = function.getReturnType();

    bool constructingMatrix = false;
    switch(op) {
    case EOpConstructMat2:
    case EOpConstructMat3:
    case EOpConstructMat4:
        constructingMatrix = true;
        break;
    default: 
        break;
    }

    //
    // Note: It's okay to have too many components available, but not okay to have unused
    // arguments.  'full' will go to true when enough args have been seen.  If we loop
    // again, there is an extra argument, so 'overfull' will become true.
    //

    size_t size = 0;
    bool constType = true;
    bool full = false;
    bool overFull = false;
    bool matrixInMatrix = false;
    bool arrayArg = false;
    for (size_t i = 0; i < function.getParamCount(); ++i) {
        const TParameter& param = function.getParam(i);
        size += param.type->getObjectSize();
        
        if (constructingMatrix && param.type->isMatrix())
            matrixInMatrix = true;
        if (full)
            overFull = true;
        if (op != EOpConstructStruct && !type->isArray() && size >= type->getObjectSize())
            full = true;
        if (param.type->getQualifier() != EvqConst)
            constType = false;
        if (param.type->isArray())
            arrayArg = true;
    }
    
    if (constType)
        type->setQualifier(EvqConst);

    if (type->isArray() && static_cast<size_t>(type->getArraySize()) != function.getParamCount()) {
        error(line, "array constructor needs one argument per array element", "constructor");
        return true;
    }

    if (arrayArg && op != EOpConstructStruct) {
        error(line, "constructing from a non-dereferenced array", "constructor");
        return true;
    }

    if (matrixInMatrix && !type->isArray()) {
        if (function.getParamCount() != 1) {
          error(line, "constructing matrix from matrix can only take one argument", "constructor");
          return true;
        }
    }

    if (overFull) {
        error(line, "too many arguments", "constructor");
        return true;
    }
    
    if (op == EOpConstructStruct && !type->isArray() && type->getStruct()->fields().size() != function.getParamCount()) {
        error(line, "Number of constructor parameters does not match the number of structure fields", "constructor");
        return true;
    }

    if (!type->isMatrix() || !matrixInMatrix) {
        if ((op != EOpConstructStruct && size != 1 && size < type->getObjectSize()) ||
            (op == EOpConstructStruct && size < type->getObjectSize())) {
            error(line, "not enough data provided for construction", "constructor");
            return true;
        }
    }

    TIntermTyped *typed = node ? node->getAsTyped() : 0;
    if (typed == 0) {
        error(line, "constructor argument does not have a type", "constructor");
        return true;
    }
    if (op != EOpConstructStruct && IsSampler(typed->getBasicType())) {
        error(line, "cannot convert a sampler", "constructor");
        return true;
    }
    if (typed->getBasicType() == EbtVoid) {
        error(line, "cannot convert a void", "constructor");
        return true;
    }

    return false;
}

// This function checks to see if a void variable has been declared and raise an error message for such a case
//
// returns true in case of an error
//
bool TParseContext::voidErrorCheck(const TSourceLoc& line, const TString& identifier, const TPublicType& pubType)
{
    if (pubType.type == EbtVoid) {
        error(line, "illegal use of type 'void'", identifier.c_str());
        return true;
    } 

    return false;
}

// This function checks to see if the node (for the expression) contains a scalar boolean expression or not
//
// returns true in case of an error
//
bool TParseContext::boolErrorCheck(const TSourceLoc& line, const TIntermTyped* type)
{
    if (type->getBasicType() != EbtBool || type->isArray() || type->isMatrix() || type->isVector()) {
        error(line, "boolean expression expected", "");
        return true;
    } 

    return false;
}

// This function checks to see if the node (for the expression) contains a scalar boolean expression or not
//
// returns true in case of an error
//
bool TParseContext::boolErrorCheck(const TSourceLoc& line, const TPublicType& pType)
{
    if (pType.type != EbtBool || pType.isAggregate()) {
        error(line, "boolean expression expected", "");
        return true;
    } 

    return false;
}

bool TParseContext::samplerErrorCheck(const TSourceLoc& line, const TPublicType& pType, const char* reason)
{
    if (pType.type == EbtStruct) {
        if (containsSampler(*pType.userDef)) {
            error(line, reason, getBasicString(pType.type), "(structure contains a sampler)");
        
            return true;
        }
        
        return false;
    } else if (IsSampler(pType.type)) {
        error(line, reason, getBasicString(pType.type));

        return true;
    }

    return false;
}

bool TParseContext::structQualifierErrorCheck(const TSourceLoc& line, const TPublicType& pType)
{
    switch (pType.qualifier)
    {
      case EvqVaryingIn:
      case EvqVaryingOut:
      case EvqAttribute:
      case EvqVertexIn:
      case EvqFragmentOut:
        if (pType.type == EbtStruct)
        {
            error(line, "cannot be used with a structure", getQualifierString(pType.qualifier));
            return true;
        }

      default: break;
    }

    if (pType.qualifier != EvqUniform && samplerErrorCheck(line, pType, "samplers must be uniform"))
        return true;

    return false;
}

bool TParseContext::locationDeclaratorListCheck(const TSourceLoc& line, const TPublicType &pType)
{
    if (pType.layoutQualifier.location != -1)
    {
        error(line, "location must only be specified for a single input or output variable", "location");
        return true;
    }

    return false;
}

bool TParseContext::parameterSamplerErrorCheck(const TSourceLoc& line, TQualifier qualifier, const TType& type)
{
    if ((qualifier == EvqOut || qualifier == EvqInOut) && 
             type.getBasicType() != EbtStruct && IsSampler(type.getBasicType())) {
        error(line, "samplers cannot be output parameters", type.getBasicString());
        return true;
    }

    return false;
}

bool TParseContext::containsSampler(TType& type)
{
    if (IsSampler(type.getBasicType()))
        return true;

    if (type.getBasicType() == EbtStruct || type.isInterfaceBlock()) {
        const TFieldList& fields = type.getStruct()->fields();
        for (unsigned int i = 0; i < fields.size(); ++i) {
            if (containsSampler(*fields[i]->type()))
                return true;
        }
    }

    return false;
}

//
// Do size checking for an array type's size.
//
// Returns true if there was an error.
//
bool TParseContext::arraySizeErrorCheck(const TSourceLoc& line, TIntermTyped* expr, int& size)
{
    TIntermConstantUnion* constant = expr->getAsConstantUnion();

    if (constant == 0 || !constant->isScalarInt())
    {
        error(line, "array size must be a constant integer expression", "");
        return true;
    }

    unsigned int unsignedSize = 0;

    if (constant->getBasicType() == EbtUInt)
    {
        unsignedSize = constant->getUConst(0);
        size = static_cast<int>(unsignedSize);
    }
    else
    {
        size = constant->getIConst(0);

        if (size < 0)
        {
            error(line, "array size must be non-negative", "");
            size = 1;
            return true;
        }

        unsignedSize = static_cast<unsigned int>(size);
    }

    if (size == 0)
    {
        error(line, "array size must be greater than zero", "");
        size = 1;
        return true;
    }

    // The size of arrays is restricted here to prevent issues further down the
    // compiler/translator/driver stack. Shader Model 5 generation hardware is limited to
    // 4096 registers so this should be reasonable even for aggressively optimizable code.
    const unsigned int sizeLimit = 65536;

    if (unsignedSize > sizeLimit)
    {
        error(line, "array size too large", "");
        size = 1;
        return true;
    }

    return false;
}

//
// See if this qualifier can be an array.
//
// Returns true if there is an error.
//
bool TParseContext::arrayQualifierErrorCheck(const TSourceLoc& line, TPublicType type)
{
    if ((type.qualifier == EvqAttribute) || (type.qualifier == EvqVertexIn) || (type.qualifier == EvqConst)) {
        error(line, "cannot declare arrays of this qualifier", TType(type).getCompleteString().c_str());
        return true;
    }

    return false;
}

//
// See if this type can be an array.
//
// Returns true if there is an error.
//
bool TParseContext::arrayTypeErrorCheck(const TSourceLoc& line, TPublicType type)
{
    //
    // Can the type be an array?
    //
    if (type.array) {
        error(line, "cannot declare arrays of arrays", TType(type).getCompleteString().c_str());
        return true;
    }

    return false;
}

//
// Do all the semantic checking for declaring an array, with and 
// without a size, and make the right changes to the symbol table.
//
// size == 0 means no specified size.
//
// Returns true if there was an error.
//
bool TParseContext::arrayErrorCheck(const TSourceLoc& line, const TString& identifier, const TPublicType &type, TVariable*& variable)
{
    //
    // Don't check for reserved word use until after we know it's not in the symbol table,
    // because reserved arrays can be redeclared.
    //

    bool builtIn = false; 
    bool sameScope = false;
    TSymbol* symbol = symbolTable.find(identifier, 0, &builtIn, &sameScope);
    if (symbol == 0 || !sameScope) {
        if (reservedErrorCheck(line, identifier))
            return true;
        
        variable = new TVariable(&identifier, TType(type));

        if (type.arraySize)
            variable->getType().setArraySize(type.arraySize);

        if (! symbolTable.declare(variable)) {
            delete variable;
            error(line, "INTERNAL ERROR inserting new symbol", identifier.c_str());
            return true;
        }
    } else {
        if (! symbol->isVariable()) {
            error(line, "variable expected", identifier.c_str());
            return true;
        }

        variable = static_cast<TVariable*>(symbol);
        if (! variable->getType().isArray()) {
            error(line, "redeclaring non-array as array", identifier.c_str());
            return true;
        }
        if (variable->getType().getArraySize() > 0) {
            error(line, "redeclaration of array with size", identifier.c_str());
            return true;
        }
        
        if (! variable->getType().sameElementType(TType(type))) {
            error(line, "redeclaration of array with a different type", identifier.c_str());
            return true;
        }

        if (type.arraySize)
            variable->getType().setArraySize(type.arraySize);
    } 

    if (voidErrorCheck(line, identifier, type))
        return true;

    return false;
}

//
// Enforce non-initializer type/qualifier rules.
//
// Returns true if there was an error.
//
bool TParseContext::nonInitConstErrorCheck(const TSourceLoc& line, const TString& identifier, TPublicType& type, bool array)
{
    if (type.qualifier == EvqConst)
    {
        // Make the qualifier make sense.
        type.qualifier = EvqTemporary;
        
        if (array)
        {
            error(line, "arrays may not be declared constant since they cannot be initialized", identifier.c_str());
        }
        else if (type.isStructureContainingArrays())
        {
            error(line, "structures containing arrays may not be declared constant since they cannot be initialized", identifier.c_str());
        }
        else
        {
            error(line, "variables with qualifier 'const' must be initialized", identifier.c_str());
        }

        return true;
    }

    return false;
}

//
// Do semantic checking for a variable declaration that has no initializer,
// and update the symbol table.
//
// Returns true if there was an error.
//
bool TParseContext::nonInitErrorCheck(const TSourceLoc& line, const TString& identifier, const TPublicType& type, TVariable*& variable)
{
    if (reservedErrorCheck(line, identifier))
        recover();

    variable = new TVariable(&identifier, TType(type));

    if (! symbolTable.declare(variable)) {
        error(line, "redefinition", variable->getName().c_str());
        delete variable;
        variable = 0;
        return true;
    }

    if (voidErrorCheck(line, identifier, type))
        return true;

    return false;
}

bool TParseContext::paramErrorCheck(const TSourceLoc& line, TQualifier qualifier, TQualifier paramQualifier, TType* type)
{    
    if (qualifier != EvqConst && qualifier != EvqTemporary) {
        error(line, "qualifier not allowed on function parameter", getQualifierString(qualifier));
        return true;
    }
    if (qualifier == EvqConst && paramQualifier != EvqIn) {
        error(line, "qualifier not allowed with ", getQualifierString(qualifier), getQualifierString(paramQualifier));
        return true;
    }

    if (qualifier == EvqConst)
        type->setQualifier(EvqConstReadOnly);
    else
        type->setQualifier(paramQualifier);

    return false;
}

bool TParseContext::extensionErrorCheck(const TSourceLoc& line, const TString& extension)
{
    const TExtensionBehavior& extBehavior = extensionBehavior();
    TExtensionBehavior::const_iterator iter = extBehavior.find(extension.c_str());
    if (iter == extBehavior.end()) {
        error(line, "extension", extension.c_str(), "is not supported");
        return true;
    }
    // In GLSL ES, an extension's default behavior is "disable".
    if (iter->second == EBhDisable || iter->second == EBhUndefined) {
        error(line, "extension", extension.c_str(), "is disabled");
        return true;
    }
    if (iter->second == EBhWarn) {
        warning(line, "extension", extension.c_str(), "is being used");
        return false;
    }

    return false;
}

bool TParseContext::singleDeclarationErrorCheck(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier)
{
    if (structQualifierErrorCheck(identifierLocation, publicType))
        return true;

    // check for layout qualifier issues
    const TLayoutQualifier layoutQualifier = publicType.layoutQualifier;

    if (layoutQualifier.matrixPacking != EmpUnspecified)
    {
        error(identifierLocation, "layout qualifier", getMatrixPackingString(layoutQualifier.matrixPacking), "only valid for interface blocks");
        return true;
    }

    if (layoutQualifier.blockStorage != EbsUnspecified)
    {
        error(identifierLocation, "layout qualifier", getBlockStorageString(layoutQualifier.blockStorage), "only valid for interface blocks");
        return true;
    }

    if (publicType.qualifier != EvqVertexIn && publicType.qualifier != EvqFragmentOut && layoutLocationErrorCheck(identifierLocation, publicType.layoutQualifier))
    {
        return true;
    }

    return false;
}

bool TParseContext::layoutLocationErrorCheck(const TSourceLoc& location, const TLayoutQualifier &layoutQualifier)
{
    if (layoutQualifier.location != -1)
    {
        error(location, "invalid layout qualifier:", "location", "only valid on program inputs and outputs");
        return true;
    }

    return false;
}

bool TParseContext::supportsExtension(const char* extension)
{
    const TExtensionBehavior& extbehavior = extensionBehavior();
    TExtensionBehavior::const_iterator iter = extbehavior.find(extension);
    return (iter != extbehavior.end());
}

bool TParseContext::isExtensionEnabled(const char* extension) const
{
    const TExtensionBehavior& extbehavior = extensionBehavior();
    TExtensionBehavior::const_iterator iter = extbehavior.find(extension);

    if (iter == extbehavior.end())
    {
        return false;
    }

    return (iter->second == EBhEnable || iter->second == EBhRequire);
}

void TParseContext::handleExtensionDirective(const TSourceLoc& loc, const char* extName, const char* behavior)
{
    pp::SourceLocation srcLoc;
    srcLoc.file = loc.first_file;
    srcLoc.line = loc.first_line;
    directiveHandler.handleExtension(srcLoc, extName, behavior);
}

void TParseContext::handlePragmaDirective(const TSourceLoc& loc, const char* name, const char* value, bool stdgl)
{
    pp::SourceLocation srcLoc;
    srcLoc.file = loc.first_file;
    srcLoc.line = loc.first_line;
    directiveHandler.handlePragma(srcLoc, name, value, stdgl);
}

/////////////////////////////////////////////////////////////////////////////////
//
// Non-Errors.
//
/////////////////////////////////////////////////////////////////////////////////

const TVariable *TParseContext::getNamedVariable(const TSourceLoc &location,
                                                 const TString *name,
                                                 const TSymbol *symbol)
{
    const TVariable *variable = NULL;

    if (!symbol)
    {
        error(location, "undeclared identifier", name->c_str());
        recover();
    }
    else if (!symbol->isVariable())
    {
        error(location, "variable expected", name->c_str());
        recover();
    }
    else
    {
        variable = static_cast<const TVariable*>(symbol);

        if (symbolTable.findBuiltIn(variable->getName(), shaderVersion) &&
            !variable->getExtension().empty() &&
            extensionErrorCheck(location, variable->getExtension()))
        {
            recover();
        }
    }

    if (!variable)
    {
        TType type(EbtFloat, EbpUndefined);
        TVariable *fakeVariable = new TVariable(name, type);
        symbolTable.declare(fakeVariable);
        variable = fakeVariable;
    }

    return variable;
}

//
// Look up a function name in the symbol table, and make sure it is a function.
//
// Return the function symbol if found, otherwise 0.
//
const TFunction* TParseContext::findFunction(const TSourceLoc& line, TFunction* call, int shaderVersion, bool *builtIn)
{
    // First find by unmangled name to check whether the function name has been
    // hidden by a variable name or struct typename.
    // If a function is found, check for one with a matching argument list.
    const TSymbol* symbol = symbolTable.find(call->getName(), shaderVersion, builtIn);
    if (symbol == 0 || symbol->isFunction()) {
        symbol = symbolTable.find(call->getMangledName(), shaderVersion, builtIn);
    }

    if (symbol == 0) {
        error(line, "no matching overloaded function found", call->getName().c_str());
        return 0;
    }

    if (!symbol->isFunction()) {
        error(line, "function name expected", call->getName().c_str());
        return 0;
    }

    return static_cast<const TFunction*>(symbol);
}

//
// Initializers show up in several places in the grammar.  Have one set of
// code to handle them here.
//
// Returns true on error, false if no error
//
bool TParseContext::executeInitializer(const TSourceLoc& line, const TString& identifier, TPublicType& pType, 
                                       TIntermTyped* initializer, TIntermNode*& intermNode, TVariable* variable)
{
    TType type = TType(pType);

    if (variable == 0) {
        if (reservedErrorCheck(line, identifier))
            return true;

        if (voidErrorCheck(line, identifier, pType))
            return true;

        //
        // add variable to symbol table
        //
        variable = new TVariable(&identifier, type);
        if (! symbolTable.declare(variable)) {
            error(line, "redefinition", variable->getName().c_str());
            return true;
            // don't delete variable, it's used by error recovery, and the pool 
            // pop will take care of the memory
        }
    }

    //
    // identifier must be of type constant, a global, or a temporary
    //
    TQualifier qualifier = variable->getType().getQualifier();
    if ((qualifier != EvqTemporary) && (qualifier != EvqGlobal) && (qualifier != EvqConst)) {
        error(line, " cannot initialize this type of qualifier ", variable->getType().getQualifierString());
        return true;
    }
    //
    // test for and propagate constant
    //

    if (qualifier == EvqConst) {
        if (qualifier != initializer->getType().getQualifier()) {
            std::stringstream extraInfoStream;
            extraInfoStream << "'" << variable->getType().getCompleteString() << "'";
            std::string extraInfo = extraInfoStream.str();
            error(line, " assigning non-constant to", "=", extraInfo.c_str());
            variable->getType().setQualifier(EvqTemporary);
            return true;
        }
        if (type != initializer->getType()) {
            error(line, " non-matching types for const initializer ", 
                variable->getType().getQualifierString());
            variable->getType().setQualifier(EvqTemporary);
            return true;
        }
        if (initializer->getAsConstantUnion()) { 
            variable->shareConstPointer(initializer->getAsConstantUnion()->getUnionArrayPointer());
        } else if (initializer->getAsSymbolNode()) {
            const TSymbol* symbol = symbolTable.find(initializer->getAsSymbolNode()->getSymbol(), 0);
            const TVariable* tVar = static_cast<const TVariable*>(symbol);

            ConstantUnion* constArray = tVar->getConstPointer();
            variable->shareConstPointer(constArray);
        } else {
            std::stringstream extraInfoStream;
            extraInfoStream << "'" << variable->getType().getCompleteString() << "'";
            std::string extraInfo = extraInfoStream.str();
            error(line, " cannot assign to", "=", extraInfo.c_str());
            variable->getType().setQualifier(EvqTemporary);
            return true;
        }
    }
 
    if (qualifier != EvqConst) {
        TIntermSymbol* intermSymbol = intermediate.addSymbol(variable->getUniqueId(), variable->getName(), variable->getType(), line);
        intermNode = intermediate.addAssign(EOpInitialize, intermSymbol, initializer, line);
        if (intermNode == 0) {
            assignError(line, "=", intermSymbol->getCompleteString(), initializer->getCompleteString());
            return true;
        }
    } else 
        intermNode = 0;

    return false;
}

bool TParseContext::areAllChildConst(TIntermAggregate* aggrNode)
{
    ASSERT(aggrNode != NULL);
    if (!aggrNode->isConstructor())
        return false;

    bool allConstant = true;

    // check if all the child nodes are constants so that they can be inserted into 
    // the parent node
    TIntermSequence *sequence = aggrNode->getSequence() ;
    for (TIntermSequence::iterator p = sequence->begin(); p != sequence->end(); ++p) {
        if (!(*p)->getAsTyped()->getAsConstantUnion())
            return false;
    }

    return allConstant;
}

TPublicType TParseContext::addFullySpecifiedType(TQualifier qualifier, TLayoutQualifier layoutQualifier, const TPublicType& typeSpecifier)
{
    TPublicType returnType = typeSpecifier;
    returnType.qualifier = qualifier;
    returnType.layoutQualifier = layoutQualifier;

    if (typeSpecifier.array)
    {
        error(typeSpecifier.line, "not supported", "first-class array");
        recover();
        returnType.setArray(false);
    }

    if (shaderVersion < 300)
    {
        if (qualifier == EvqAttribute && (typeSpecifier.type == EbtBool || typeSpecifier.type == EbtInt))
        {
            error(typeSpecifier.line, "cannot be bool or int", getQualifierString(qualifier));
            recover();
        }

        if ((qualifier == EvqVaryingIn || qualifier == EvqVaryingOut) &&
            (typeSpecifier.type == EbtBool || typeSpecifier.type == EbtInt))
        {
            error(typeSpecifier.line, "cannot be bool or int", getQualifierString(qualifier));
            recover();
        }
    }
    else
    {
        switch (qualifier)
        {
          case EvqSmoothIn:
          case EvqSmoothOut:
          case EvqVertexOut:
          case EvqFragmentIn:
          case EvqCentroidOut:
          case EvqCentroidIn:
            if (typeSpecifier.type == EbtBool)
            {
                error(typeSpecifier.line, "cannot be bool", getQualifierString(qualifier));
                recover();
            }
            if (typeSpecifier.type == EbtInt || typeSpecifier.type == EbtUInt)
            {
                error(typeSpecifier.line, "must use 'flat' interpolation here", getQualifierString(qualifier));
                recover();
            }
            break;

          case EvqVertexIn:
          case EvqFragmentOut:
          case EvqFlatIn:
          case EvqFlatOut:
            if (typeSpecifier.type == EbtBool)
            {
                error(typeSpecifier.line, "cannot be bool", getQualifierString(qualifier));
                recover();
            }
            break;

          default: break;
        }
    }

    return returnType;
}

TIntermAggregate* TParseContext::parseSingleDeclaration(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier)
{
    TIntermSymbol* symbol = intermediate.addSymbol(0, identifier, TType(publicType), identifierLocation);
    TIntermAggregate* aggregate = intermediate.makeAggregate(symbol, identifierLocation);

    if (identifier != "")
    {
        if (singleDeclarationErrorCheck(publicType, identifierLocation, identifier))
            recover();

        // this error check can mutate the type
        if (nonInitConstErrorCheck(identifierLocation, identifier, publicType, false))
            recover();

        TVariable* variable = 0;

        if (nonInitErrorCheck(identifierLocation, identifier, publicType, variable))
            recover();

        if (variable && symbol)
        {
            symbol->setId(variable->getUniqueId());
        }
    }

    return aggregate;
}

TIntermAggregate* TParseContext::parseSingleArrayDeclaration(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& indexLocation, TIntermTyped *indexExpression)
{
    if (singleDeclarationErrorCheck(publicType, identifierLocation, identifier))
        recover();

    // this error check can mutate the type
    if (nonInitConstErrorCheck(identifierLocation, identifier, publicType, true))
        recover();

    if (arrayTypeErrorCheck(indexLocation, publicType) || arrayQualifierErrorCheck(indexLocation, publicType))
    {
        recover();
    }

    TPublicType arrayType = publicType;

    int size;
    if (arraySizeErrorCheck(identifierLocation, indexExpression, size))
    {
        recover();
    }
    else
    {
        arrayType.setArray(true, size);
    }

    TIntermSymbol* symbol = intermediate.addSymbol(0, identifier, TType(arrayType), identifierLocation);
    TIntermAggregate* aggregate = intermediate.makeAggregate(symbol, identifierLocation);
    TVariable* variable = 0;

    if (arrayErrorCheck(identifierLocation, identifier, arrayType, variable))
        recover();

    if (variable && symbol)
    {
        symbol->setId(variable->getUniqueId());
    }

    return aggregate;
}

TIntermAggregate* TParseContext::parseSingleInitDeclaration(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& initLocation, TIntermTyped *initializer)
{
    if (singleDeclarationErrorCheck(publicType, identifierLocation, identifier))
        recover();

    TIntermNode* intermNode;
    if (!executeInitializer(identifierLocation, identifier, publicType, initializer, intermNode))
    {
        //
        // Build intermediate representation
        //
        return intermNode ? intermediate.makeAggregate(intermNode, initLocation) : NULL;
    }
    else
    {
        recover();
        return NULL;
    }
}

TIntermAggregate* TParseContext::parseInvariantDeclaration(const TSourceLoc &invariantLoc,
                                                           const TSourceLoc &identifierLoc,
                                                           const TString *identifier,
                                                           const TSymbol *symbol)
{
    // invariant declaration
    if (globalErrorCheck(invariantLoc, symbolTable.atGlobalLevel(), "invariant varying"))
    {
        recover();
    }

    if (!symbol)
    {
        error(identifierLoc, "undeclared identifier declared as invariant", identifier->c_str());
        recover();
        return NULL;
    }
    else
    {
        const TString kGlFrontFacing("gl_FrontFacing");
        if (*identifier == kGlFrontFacing)
        {
            error(identifierLoc, "identifier should not be declared as invariant", identifier->c_str());
            recover();
            return NULL;
        }
        symbolTable.addInvariantVarying(*identifier);
        const TVariable *variable = getNamedVariable(identifierLoc, identifier, symbol);
        ASSERT(variable);
        const TType &type = variable->getType();
        TIntermSymbol *intermSymbol = intermediate.addSymbol(variable->getUniqueId(),
                                                             *identifier, type, identifierLoc);

        TIntermAggregate *aggregate = intermediate.makeAggregate(intermSymbol, identifierLoc);
        aggregate->setOp(EOpInvariantDeclaration);
        return aggregate;
    }
}

TIntermAggregate* TParseContext::parseDeclarator(TPublicType &publicType, TIntermAggregate *aggregateDeclaration, TSymbol *identifierSymbol, const TSourceLoc& identifierLocation, const TString &identifier)
{
    TIntermSymbol* symbol = intermediate.addSymbol(0, identifier, TType(publicType), identifierLocation);
    TIntermAggregate* intermAggregate = intermediate.growAggregate(aggregateDeclaration, symbol, identifierLocation);

    if (structQualifierErrorCheck(identifierLocation, publicType))
        recover();

    if (locationDeclaratorListCheck(identifierLocation, publicType))
        recover();

    if (nonInitConstErrorCheck(identifierLocation, identifier, publicType, false))
        recover();

    TVariable* variable = 0;
    if (nonInitErrorCheck(identifierLocation, identifier, publicType, variable))
        recover();
    if (symbol && variable)
        symbol->setId(variable->getUniqueId());

    return intermAggregate;
}

TIntermAggregate* TParseContext::parseArrayDeclarator(TPublicType &publicType, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& arrayLocation, TIntermNode *declaratorList, TIntermTyped *indexExpression)
{
    if (structQualifierErrorCheck(identifierLocation, publicType))
        recover();

    if (locationDeclaratorListCheck(identifierLocation, publicType))
        recover();

    if (nonInitConstErrorCheck(identifierLocation, identifier, publicType, true))
        recover();

    if (arrayTypeErrorCheck(arrayLocation, publicType) || arrayQualifierErrorCheck(arrayLocation, publicType))
    {
        recover();
    }
    else if (indexExpression)
    {
        int size;
        if (arraySizeErrorCheck(arrayLocation, indexExpression, size))
            recover();
        TPublicType arrayType(publicType);
        arrayType.setArray(true, size);
        TVariable* variable = NULL;
        if (arrayErrorCheck(arrayLocation, identifier, arrayType, variable))
            recover();
        TType type = TType(arrayType);
        type.setArraySize(size);

        return intermediate.growAggregate(declaratorList, intermediate.addSymbol(variable ? variable->getUniqueId() : 0, identifier, type, identifierLocation), identifierLocation);
    }
    else
    {
        TPublicType arrayType(publicType);
        arrayType.setArray(true);
        TVariable* variable = NULL;
        if (arrayErrorCheck(arrayLocation, identifier, arrayType, variable))
            recover();
    }

    return NULL;
}

TIntermAggregate* TParseContext::parseInitDeclarator(TPublicType &publicType, TIntermAggregate *declaratorList, const TSourceLoc& identifierLocation, const TString &identifier, const TSourceLoc& initLocation, TIntermTyped *initializer)
{
    if (structQualifierErrorCheck(identifierLocation, publicType))
        recover();

    if (locationDeclaratorListCheck(identifierLocation, publicType))
        recover();

    TIntermNode* intermNode;
    if (!executeInitializer(identifierLocation, identifier, publicType, initializer, intermNode))
    {
        //
        // build the intermediate representation
        //
        if (intermNode)
        {
            return intermediate.growAggregate(declaratorList, intermNode, initLocation);
        }
        else
        {
            return declaratorList;
        }
    }
    else
    {
        recover();
        return NULL;
    }
}

void TParseContext::parseGlobalLayoutQualifier(const TPublicType &typeQualifier)
{
    if (typeQualifier.qualifier != EvqUniform)
    {
        error(typeQualifier.line, "invalid qualifier:", getQualifierString(typeQualifier.qualifier), "global layout must be uniform");
        recover();
        return;
    }

    const TLayoutQualifier layoutQualifier = typeQualifier.layoutQualifier;
    ASSERT(!layoutQualifier.isEmpty());

    if (shaderVersion < 300)
    {
        error(typeQualifier.line, "layout qualifiers supported in GLSL ES 3.00 only", "layout");
        recover();
        return;
    }

    if (layoutLocationErrorCheck(typeQualifier.line, typeQualifier.layoutQualifier))
    {
        recover();
        return;
    }

    if (layoutQualifier.matrixPacking != EmpUnspecified)
    {
        defaultMatrixPacking = layoutQualifier.matrixPacking;
    }

    if (layoutQualifier.blockStorage != EbsUnspecified)
    {
        defaultBlockStorage = layoutQualifier.blockStorage;
    }
}

TFunction *TParseContext::addConstructorFunc(TPublicType publicType)
{
    TOperator op = EOpNull;
    if (publicType.userDef)
    {
        op = EOpConstructStruct;
    }
    else
    {
        switch (publicType.type)
        {
          case EbtFloat:
            if (publicType.isMatrix())
            {
                // TODO: non-square matrices
                switch(publicType.getCols())
                {
                  case 2: op = EOpConstructMat2;  break;
                  case 3: op = EOpConstructMat3;  break;
                  case 4: op = EOpConstructMat4;  break;
                }
            }
            else
            {
                switch(publicType.getNominalSize())
                {
                  case 1: op = EOpConstructFloat; break;
                  case 2: op = EOpConstructVec2;  break;
                  case 3: op = EOpConstructVec3;  break;
                  case 4: op = EOpConstructVec4;  break;
                }
            }
            break;

          case EbtInt:
            switch(publicType.getNominalSize())
            {
              case 1: op = EOpConstructInt;   break;
              case 2: op = EOpConstructIVec2; break;
              case 3: op = EOpConstructIVec3; break;
              case 4: op = EOpConstructIVec4; break;
            }
            break;

          case EbtUInt:
            switch(publicType.getNominalSize())
            {
              case 1: op = EOpConstructUInt;  break;
              case 2: op = EOpConstructUVec2; break;
              case 3: op = EOpConstructUVec3; break;
              case 4: op = EOpConstructUVec4; break;
            }
            break;

          case EbtBool:
            switch(publicType.getNominalSize())
            {
                case 1: op = EOpConstructBool;  break;
                case 2: op = EOpConstructBVec2; break;
                case 3: op = EOpConstructBVec3; break;
                case 4: op = EOpConstructBVec4; break;
            }
            break;

          default: break;
        }

        if (op == EOpNull)
        {
            error(publicType.line, "cannot construct this type", getBasicString(publicType.type));
            recover();
            publicType.type = EbtFloat;
            op = EOpConstructFloat;
        }
    }

    TString tempString;
    TType type(publicType);
    return new TFunction(&tempString, type, op);
}

// This function is used to test for the correctness of the parameters passed to various constructor functions
// and also convert them to the right datatype if it is allowed and required. 
//
// Returns 0 for an error or the constructed node (aggregate or typed) for no error.
//
TIntermTyped *TParseContext::addConstructor(TIntermNode *arguments, const TType *type, TOperator op, TFunction *fnCall, const TSourceLoc &line)
{
    TIntermAggregate *aggregateArguments = arguments->getAsAggregate();

    if (!aggregateArguments)
    {
        aggregateArguments = new TIntermAggregate;
        aggregateArguments->getSequence()->push_back(arguments);
    }

    if (op == EOpConstructStruct)
    {
        const TFieldList &fields = type->getStruct()->fields();
        TIntermSequence *args = aggregateArguments->getSequence();

        for (size_t i = 0; i < fields.size(); i++)
        {
            if (i >= args->size() || (*args)[i]->getAsTyped()->getType() != *fields[i]->type())
            {
                error(line, "Structure constructor arguments do not match structure fields", "Error");
                recover();

                return 0;
            }
        }
    }

    // Turn the argument list itself into a constructor
    TIntermTyped *constructor = intermediate.setAggregateOperator(aggregateArguments, op, line);
    TIntermTyped *constConstructor = foldConstConstructor(constructor->getAsAggregate(), *type);
    if (constConstructor)
    {
        return constConstructor;
    }

    return constructor;
}

TIntermTyped* TParseContext::foldConstConstructor(TIntermAggregate* aggrNode, const TType& type)
{
    bool canBeFolded = areAllChildConst(aggrNode);
    aggrNode->setType(type);
    if (canBeFolded) {
        bool returnVal = false;
        ConstantUnion* unionArray = new ConstantUnion[type.getObjectSize()];
        if (aggrNode->getSequence()->size() == 1)  {
            returnVal = intermediate.parseConstTree(aggrNode->getLine(), aggrNode, unionArray, aggrNode->getOp(), type, true);
        }
        else {
            returnVal = intermediate.parseConstTree(aggrNode->getLine(), aggrNode, unionArray, aggrNode->getOp(), type);
        }
        if (returnVal)
            return 0;

        return intermediate.addConstantUnion(unionArray, type, aggrNode->getLine());
    }

    return 0;
}

//
// This function returns the tree representation for the vector field(s) being accessed from contant vector.
// If only one component of vector is accessed (v.x or v[0] where v is a contant vector), then a contant node is
// returned, else an aggregate node is returned (for v.xy). The input to this function could either be the symbol
// node or it could be the intermediate tree representation of accessing fields in a constant structure or column of 
// a constant matrix.
//
TIntermTyped* TParseContext::addConstVectorNode(TVectorFields& fields, TIntermTyped* node, const TSourceLoc& line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();

    ConstantUnion *unionArray;
    if (tempConstantNode) {
        unionArray = tempConstantNode->getUnionArrayPointer();

        if (!unionArray) {
            return node;
        }
    } else { // The node has to be either a symbol node or an aggregate node or a tempConstant node, else, its an error
        error(line, "Cannot offset into the vector", "Error");
        recover();

        return 0;
    }

    ConstantUnion* constArray = new ConstantUnion[fields.num];

    for (int i = 0; i < fields.num; i++) {
        if (fields.offsets[i] >= node->getType().getNominalSize()) {
            std::stringstream extraInfoStream;
            extraInfoStream << "vector field selection out of range '" << fields.offsets[i] << "'";
            std::string extraInfo = extraInfoStream.str();
            error(line, "", "[", extraInfo.c_str());
            recover();
            fields.offsets[i] = 0;
        }
        
        constArray[i] = unionArray[fields.offsets[i]];

    } 
    typedNode = intermediate.addConstantUnion(constArray, node->getType(), line);
    return typedNode;
}

//
// This function returns the column being accessed from a constant matrix. The values are retrieved from
// the symbol table and parse-tree is built for a vector (each column of a matrix is a vector). The input 
// to the function could either be a symbol node (m[0] where m is a constant matrix)that represents a 
// constant matrix or it could be the tree representation of the constant matrix (s.m1[0] where s is a constant structure)
//
TIntermTyped* TParseContext::addConstMatrixNode(int index, TIntermTyped* node, const TSourceLoc& line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();

    if (index >= node->getType().getCols()) {
        std::stringstream extraInfoStream;
        extraInfoStream << "matrix field selection out of range '" << index << "'";
        std::string extraInfo = extraInfoStream.str();
        error(line, "", "[", extraInfo.c_str());
        recover();
        index = 0;
    }

    if (tempConstantNode) {
         ConstantUnion* unionArray = tempConstantNode->getUnionArrayPointer();
         int size = tempConstantNode->getType().getCols();
         typedNode = intermediate.addConstantUnion(&unionArray[size*index], tempConstantNode->getType(), line);
    } else {
        error(line, "Cannot offset into the matrix", "Error");
        recover();

        return 0;
    }

    return typedNode;
}


//
// This function returns an element of an array accessed from a constant array. The values are retrieved from
// the symbol table and parse-tree is built for the type of the element. The input 
// to the function could either be a symbol node (a[0] where a is a constant array)that represents a 
// constant array or it could be the tree representation of the constant array (s.a1[0] where s is a constant structure)
//
TIntermTyped* TParseContext::addConstArrayNode(int index, TIntermTyped* node, const TSourceLoc& line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();
    TType arrayElementType = node->getType();
    arrayElementType.clearArrayness();

    if (index >= node->getType().getArraySize()) {
        std::stringstream extraInfoStream;
        extraInfoStream << "array field selection out of range '" << index << "'";
        std::string extraInfo = extraInfoStream.str();
        error(line, "", "[", extraInfo.c_str());
        recover();
        index = 0;
    }

    if (tempConstantNode) {
        size_t arrayElementSize = arrayElementType.getObjectSize();
        ConstantUnion* unionArray = tempConstantNode->getUnionArrayPointer();
        typedNode = intermediate.addConstantUnion(&unionArray[arrayElementSize * index], tempConstantNode->getType(), line);
    } else {
        error(line, "Cannot offset into the array", "Error");
        recover();

        return 0;
    }

    return typedNode;
}


//
// This function returns the value of a particular field inside a constant structure from the symbol table. 
// If there is an embedded/nested struct, it appropriately calls addConstStructNested or addConstStructFromAggr
// function and returns the parse-tree with the values of the embedded/nested struct.
//
TIntermTyped* TParseContext::addConstStruct(const TString &identifier, TIntermTyped *node, const TSourceLoc& line)
{
    const TFieldList& fields = node->getType().getStruct()->fields();
    size_t instanceSize = 0;

    for (size_t index = 0; index < fields.size(); ++index) {
        if (fields[index]->name() == identifier) {
            break;
        } else {
            instanceSize += fields[index]->type()->getObjectSize();
        }
    }

    TIntermTyped *typedNode;
    TIntermConstantUnion *tempConstantNode = node->getAsConstantUnion();
    if (tempConstantNode) {
         ConstantUnion* constArray = tempConstantNode->getUnionArrayPointer();

         typedNode = intermediate.addConstantUnion(constArray+instanceSize, tempConstantNode->getType(), line); // type will be changed in the calling function
    } else {
        error(line, "Cannot offset into the structure", "Error");
        recover();

        return 0;
    }

    return typedNode;
}

//
// Interface/uniform blocks
//
TIntermAggregate* TParseContext::addInterfaceBlock(const TPublicType& typeQualifier, const TSourceLoc& nameLine, const TString& blockName, TFieldList* fieldList, 
                                                   const TString* instanceName, const TSourceLoc& instanceLine, TIntermTyped* arrayIndex, const TSourceLoc& arrayIndexLine)
{
    if (reservedErrorCheck(nameLine, blockName))
        recover();

    if (typeQualifier.qualifier != EvqUniform)
    {
        error(typeQualifier.line, "invalid qualifier:", getQualifierString(typeQualifier.qualifier), "interface blocks must be uniform");
        recover();
    }

    TLayoutQualifier blockLayoutQualifier = typeQualifier.layoutQualifier;
    if (layoutLocationErrorCheck(typeQualifier.line, blockLayoutQualifier))
    {
        recover();
    }

    if (blockLayoutQualifier.matrixPacking == EmpUnspecified)
    {
        blockLayoutQualifier.matrixPacking = defaultMatrixPacking;
    }

    if (blockLayoutQualifier.blockStorage == EbsUnspecified)
    {
        blockLayoutQualifier.blockStorage = defaultBlockStorage;
    }

    TSymbol* blockNameSymbol = new TInterfaceBlockName(&blockName);
    if (!symbolTable.declare(blockNameSymbol)) {
        error(nameLine, "redefinition", blockName.c_str(), "interface block name");
        recover();
    }

    // check for sampler types and apply layout qualifiers
    for (size_t memberIndex = 0; memberIndex < fieldList->size(); ++memberIndex) {
        TField* field = (*fieldList)[memberIndex];
        TType* fieldType = field->type();
        if (IsSampler(fieldType->getBasicType())) {
            error(field->line(), "unsupported type", fieldType->getBasicString(), "sampler types are not allowed in interface blocks");
            recover();
        }

        const TQualifier qualifier = fieldType->getQualifier();
        switch (qualifier)
        {
          case EvqGlobal:
          case EvqUniform:
            break;
          default:
            error(field->line(), "invalid qualifier on interface block member", getQualifierString(qualifier));
            recover();
            break;
        }

        // check layout qualifiers
        TLayoutQualifier fieldLayoutQualifier = fieldType->getLayoutQualifier();
        if (layoutLocationErrorCheck(field->line(), fieldLayoutQualifier))
        {
            recover();
        }

        if (fieldLayoutQualifier.blockStorage != EbsUnspecified)
        {
            error(field->line(), "invalid layout qualifier:", getBlockStorageString(fieldLayoutQualifier.blockStorage), "cannot be used here");
            recover();
        }

        if (fieldLayoutQualifier.matrixPacking == EmpUnspecified)
        {
            fieldLayoutQualifier.matrixPacking = blockLayoutQualifier.matrixPacking;
        }
        else if (!fieldType->isMatrix())
        {
            error(field->line(), "invalid layout qualifier:", getMatrixPackingString(fieldLayoutQualifier.matrixPacking), "can only be used on matrix types");
            recover();
        }

        fieldType->setLayoutQualifier(fieldLayoutQualifier);
    }

    // add array index
    int arraySize = 0;
    if (arrayIndex != NULL)
    {
        if (arraySizeErrorCheck(arrayIndexLine, arrayIndex, arraySize))
            recover();
    }

    TInterfaceBlock* interfaceBlock = new TInterfaceBlock(&blockName, fieldList, instanceName, arraySize, blockLayoutQualifier);
    TType interfaceBlockType(interfaceBlock, typeQualifier.qualifier, blockLayoutQualifier, arraySize);

    TString symbolName = "";
    int symbolId = 0;

    if (!instanceName)
    {
        // define symbols for the members of the interface block
        for (size_t memberIndex = 0; memberIndex < fieldList->size(); ++memberIndex)
        {
            TField* field = (*fieldList)[memberIndex];
            TType* fieldType = field->type();

            // set parent pointer of the field variable
            fieldType->setInterfaceBlock(interfaceBlock);

            TVariable* fieldVariable = new TVariable(&field->name(), *fieldType);
            fieldVariable->setQualifier(typeQualifier.qualifier);

            if (!symbolTable.declare(fieldVariable)) {
                error(field->line(), "redefinition", field->name().c_str(), "interface block member name");
                recover();
            }
        }
    }
    else
    {
        // add a symbol for this interface block
        TVariable* instanceTypeDef = new TVariable(instanceName, interfaceBlockType, false);
        instanceTypeDef->setQualifier(typeQualifier.qualifier);

        if (!symbolTable.declare(instanceTypeDef)) {
            error(instanceLine, "redefinition", instanceName->c_str(), "interface block instance name");
            recover();
        }

        symbolId = instanceTypeDef->getUniqueId();
        symbolName = instanceTypeDef->getName();
    }

    TIntermAggregate *aggregate = intermediate.makeAggregate(intermediate.addSymbol(symbolId, symbolName, interfaceBlockType, typeQualifier.line), nameLine);
    aggregate->setOp(EOpDeclaration);

    exitStructDeclaration();
    return aggregate;
}

bool TParseContext::enterStructDeclaration(const TSourceLoc& line, const TString& identifier)
{
    ++structNestingLevel;

    // Embedded structure definitions are not supported per GLSL ES spec.
    // They aren't allowed in GLSL either, but we need to detect this here
    // so we don't rely on the GLSL compiler to catch it.
    if (structNestingLevel > 1) {
        error(line, "", "Embedded struct definitions are not allowed");
        return true;
    }

    return false;
}

void TParseContext::exitStructDeclaration()
{
    --structNestingLevel;
}

namespace {

const int kWebGLMaxStructNesting = 4;

}  // namespace

bool TParseContext::structNestingErrorCheck(const TSourceLoc& line, const TField& field)
{
    if (!IsWebGLBasedSpec(shaderSpec)) {
        return false;
    }

    if (field.type()->getBasicType() != EbtStruct) {
        return false;
    }

    // We're already inside a structure definition at this point, so add
    // one to the field's struct nesting.
    if (1 + field.type()->getDeepestStructNesting() > kWebGLMaxStructNesting) {
        std::stringstream reasonStream;
        reasonStream << "Reference of struct type "
                     << field.type()->getStruct()->name().c_str()
                     << " exceeds maximum allowed nesting level of "
                     << kWebGLMaxStructNesting;
        std::string reason = reasonStream.str();
        error(line, reason.c_str(), field.name().c_str(), "");
        return true;
    }

    return false;
}

//
// Parse an array index expression
//
TIntermTyped* TParseContext::addIndexExpression(TIntermTyped *baseExpression, const TSourceLoc& location, TIntermTyped *indexExpression)
{
    TIntermTyped *indexedExpression = NULL;

    if (!baseExpression->isArray() && !baseExpression->isMatrix() && !baseExpression->isVector())
    {
        if (baseExpression->getAsSymbolNode())
        {
            error(location, " left of '[' is not of type array, matrix, or vector ", baseExpression->getAsSymbolNode()->getSymbol().c_str());
        }
        else
        {
            error(location, " left of '[' is not of type array, matrix, or vector ", "expression");
        }
        recover();
    }

    if (indexExpression->getQualifier() == EvqConst)
    {
        int index = indexExpression->getAsConstantUnion()->getIConst(0);
        if (index < 0)
        {
            std::stringstream infoStream;
            infoStream << index;
            std::string info = infoStream.str();
            error(location, "negative index", info.c_str());
            recover();
            index = 0;
        }
        if (baseExpression->getType().getQualifier() == EvqConst)
        {
            if (baseExpression->isArray())
            {
                // constant folding for arrays
                indexedExpression = addConstArrayNode(index, baseExpression, location);
            }
            else if (baseExpression->isVector())
            {
                // constant folding for vectors
                TVectorFields fields;
                fields.num = 1;
                fields.offsets[0] = index; // need to do it this way because v.xy sends fields integer array
                indexedExpression = addConstVectorNode(fields, baseExpression, location);
            }
            else if (baseExpression->isMatrix())
            {
                // constant folding for matrices
                indexedExpression = addConstMatrixNode(index, baseExpression, location);
            }
        }
        else
        {
            if (baseExpression->isArray())
            {
                if (index >= baseExpression->getType().getArraySize())
                {
                    std::stringstream extraInfoStream;
                    extraInfoStream << "array index out of range '" << index << "'";
                    std::string extraInfo = extraInfoStream.str();
                    error(location, "", "[", extraInfo.c_str());
                    recover();
                    index = baseExpression->getType().getArraySize() - 1;
                }
                else if (baseExpression->getQualifier() == EvqFragData && index > 0 && !isExtensionEnabled("GL_EXT_draw_buffers"))
                {
                    error(location, "", "[", "array indexes for gl_FragData must be zero when GL_EXT_draw_buffers is disabled");
                    recover();
                    index = 0;
                }
            }
            else if ((baseExpression->isVector() || baseExpression->isMatrix()) && baseExpression->getType().getNominalSize() <= index)
            {
                std::stringstream extraInfoStream;
                extraInfoStream << "field selection out of range '" << index << "'";
                std::string extraInfo = extraInfoStream.str();
                error(location, "", "[", extraInfo.c_str());
                recover();
                index = baseExpression->getType().getNominalSize() - 1;
            }

            indexExpression->getAsConstantUnion()->getUnionArrayPointer()->setIConst(index);
            indexedExpression = intermediate.addIndex(EOpIndexDirect, baseExpression, indexExpression, location);
        }
    }
    else
    {
        if (baseExpression->isInterfaceBlock())
        {
            error(location, "", "[", "array indexes for interface blocks arrays must be constant integral expressions");
            recover();
        }
        else if (baseExpression->getQualifier() == EvqFragmentOut)
        {
            error(location, "", "[", "array indexes for fragment outputs must be constant integral expressions");
            recover();
        }

        indexedExpression = intermediate.addIndex(EOpIndexIndirect, baseExpression, indexExpression, location);
    }

    if (indexedExpression == 0)
    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setFConst(0.0f);
        indexedExpression = intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpHigh, EvqConst), location);
    }
    else if (baseExpression->isArray())
    {
        const TType &baseType = baseExpression->getType();
        if (baseType.getStruct())
        {
            TType copyOfType(baseType.getStruct());
            indexedExpression->setType(copyOfType);
        }
        else if (baseType.isInterfaceBlock())
        {
            TType copyOfType(baseType.getInterfaceBlock(), baseType.getQualifier(), baseType.getLayoutQualifier(), 0);
            indexedExpression->setType(copyOfType);
        }
        else
        {
            indexedExpression->setType(TType(baseExpression->getBasicType(), baseExpression->getPrecision(), EvqTemporary, baseExpression->getNominalSize(), baseExpression->getSecondarySize()));
        }

        if (baseExpression->getType().getQualifier() == EvqConst)
        {
            indexedExpression->getTypePointer()->setQualifier(EvqConst);
        }
    }
    else if (baseExpression->isMatrix())
    {
        TQualifier qualifier = baseExpression->getType().getQualifier() == EvqConst ? EvqConst : EvqTemporary;
        indexedExpression->setType(TType(baseExpression->getBasicType(), baseExpression->getPrecision(), qualifier, baseExpression->getRows()));
    }
    else if (baseExpression->isVector())
    {
        TQualifier qualifier = baseExpression->getType().getQualifier() == EvqConst ? EvqConst : EvqTemporary;
        indexedExpression->setType(TType(baseExpression->getBasicType(), baseExpression->getPrecision(), qualifier));
    }
    else
    {
        indexedExpression->setType(baseExpression->getType());
    }

    return indexedExpression;
}

TIntermTyped* TParseContext::addFieldSelectionExpression(TIntermTyped *baseExpression, const TSourceLoc& dotLocation, const TString &fieldString, const TSourceLoc& fieldLocation)
{
    TIntermTyped *indexedExpression = NULL;

    if (baseExpression->isArray())
    {
        error(fieldLocation, "cannot apply dot operator to an array", ".");
        recover();
    }

    if (baseExpression->isVector())
    {
        TVectorFields fields;
        if (!parseVectorFields(fieldString, baseExpression->getNominalSize(), fields, fieldLocation))
        {
            fields.num = 1;
            fields.offsets[0] = 0;
            recover();
        }

        if (baseExpression->getType().getQualifier() == EvqConst)
        {
            // constant folding for vector fields
            indexedExpression = addConstVectorNode(fields, baseExpression, fieldLocation);
            if (indexedExpression == 0)
            {
                recover();
                indexedExpression = baseExpression;
            }
            else
            {
                indexedExpression->setType(TType(baseExpression->getBasicType(), baseExpression->getPrecision(), EvqConst, (int) (fieldString).size()));
            }
        }
        else
        {
            TString vectorString = fieldString;
            TIntermTyped* index = intermediate.addSwizzle(fields, fieldLocation);
            indexedExpression = intermediate.addIndex(EOpVectorSwizzle, baseExpression, index, dotLocation);
            indexedExpression->setType(TType(baseExpression->getBasicType(), baseExpression->getPrecision(), EvqTemporary, (int) vectorString.size()));
        }
    }
    else if (baseExpression->isMatrix())
    {
        TMatrixFields fields;
        if (!parseMatrixFields(fieldString, baseExpression->getCols(), baseExpression->getRows(), fields, fieldLocation))
        {
            fields.wholeRow = false;
            fields.wholeCol = false;
            fields.row = 0;
            fields.col = 0;
            recover();
        }

        if (fields.wholeRow || fields.wholeCol)
        {
            error(dotLocation, " non-scalar fields not implemented yet", ".");
            recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setIConst(0);
            TIntermTyped* index = intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), fieldLocation);
            indexedExpression = intermediate.addIndex(EOpIndexDirect, baseExpression, index, dotLocation);
            indexedExpression->setType(TType(baseExpression->getBasicType(), baseExpression->getPrecision(),EvqTemporary, baseExpression->getCols(), baseExpression->getRows()));
        }
        else
        {
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setIConst(fields.col * baseExpression->getRows() + fields.row);
            TIntermTyped* index = intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), fieldLocation);
            indexedExpression = intermediate.addIndex(EOpIndexDirect, baseExpression, index, dotLocation);
            indexedExpression->setType(TType(baseExpression->getBasicType(), baseExpression->getPrecision()));
        }
    }
    else if (baseExpression->getBasicType() == EbtStruct)
    {
        bool fieldFound = false;
        const TFieldList& fields = baseExpression->getType().getStruct()->fields();
        if (fields.empty())
        {
            error(dotLocation, "structure has no fields", "Internal Error");
            recover();
            indexedExpression = baseExpression;
        }
        else
        {
            unsigned int i;
            for (i = 0; i < fields.size(); ++i)
            {
                if (fields[i]->name() == fieldString)
                {
                    fieldFound = true;
                    break;
                }
            }
            if (fieldFound)
            {
                if (baseExpression->getType().getQualifier() == EvqConst)
                {
                    indexedExpression = addConstStruct(fieldString, baseExpression, dotLocation);
                    if (indexedExpression == 0)
                    {
                        recover();
                        indexedExpression = baseExpression;
                    }
                    else
                    {
                        indexedExpression->setType(*fields[i]->type());
                        // change the qualifier of the return type, not of the structure field
                        // as the structure definition is shared between various structures.
                        indexedExpression->getTypePointer()->setQualifier(EvqConst);
                    }
                }
                else
                {
                    ConstantUnion *unionArray = new ConstantUnion[1];
                    unionArray->setIConst(i);
                    TIntermTyped* index = intermediate.addConstantUnion(unionArray, *fields[i]->type(), fieldLocation);
                    indexedExpression = intermediate.addIndex(EOpIndexDirectStruct, baseExpression, index, dotLocation);
                    indexedExpression->setType(*fields[i]->type());
                }
            }
            else
            {
                error(dotLocation, " no such field in structure", fieldString.c_str());
                recover();
                indexedExpression = baseExpression;
            }
        }
    }
    else if (baseExpression->isInterfaceBlock())
    {
        bool fieldFound = false;
        const TFieldList& fields = baseExpression->getType().getInterfaceBlock()->fields();
        if (fields.empty())
        {
            error(dotLocation, "interface block has no fields", "Internal Error");
            recover();
            indexedExpression = baseExpression;
        }
        else
        {
            unsigned int i;
            for (i = 0; i < fields.size(); ++i)
            {
                if (fields[i]->name() == fieldString)
                {
                    fieldFound = true;
                    break;
                }
            }
            if (fieldFound)
            {
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setIConst(i);
                TIntermTyped* index = intermediate.addConstantUnion(unionArray, *fields[i]->type(), fieldLocation);
                indexedExpression = intermediate.addIndex(EOpIndexDirectInterfaceBlock, baseExpression, index, dotLocation);
                indexedExpression->setType(*fields[i]->type());
            }
            else
            {
                error(dotLocation, " no such field in interface block", fieldString.c_str());
                recover();
                indexedExpression = baseExpression;
            }
        }
    }
    else
    {
        if (shaderVersion < 300)
        {
            error(dotLocation, " field selection requires structure, vector, or matrix on left hand side", fieldString.c_str());
        }
        else
        {
            error(dotLocation, " field selection requires structure, vector, matrix, or interface block on left hand side", fieldString.c_str());
        }
        recover();
        indexedExpression = baseExpression;
    }

    return indexedExpression;
}

TLayoutQualifier TParseContext::parseLayoutQualifier(const TString &qualifierType, const TSourceLoc& qualifierTypeLine)
{
    TLayoutQualifier qualifier;

    qualifier.location = -1;
    qualifier.matrixPacking = EmpUnspecified;
    qualifier.blockStorage = EbsUnspecified;

    if (qualifierType == "shared")
    {
        qualifier.blockStorage = EbsShared;
    }
    else if (qualifierType == "packed")
    {
        qualifier.blockStorage = EbsPacked;
    }
    else if (qualifierType == "std140")
    {
        qualifier.blockStorage = EbsStd140;
    }
    else if (qualifierType == "row_major")
    {
        qualifier.matrixPacking = EmpRowMajor;
    }
    else if (qualifierType == "column_major")
    {
        qualifier.matrixPacking = EmpColumnMajor;
    }
    else if (qualifierType == "location")
    {
        error(qualifierTypeLine, "invalid layout qualifier", qualifierType.c_str(), "location requires an argument");
        recover();
    }
    else
    {
        error(qualifierTypeLine, "invalid layout qualifier", qualifierType.c_str());
        recover();
    }

    return qualifier;
}

TLayoutQualifier TParseContext::parseLayoutQualifier(const TString &qualifierType, const TSourceLoc& qualifierTypeLine, const TString &intValueString, int intValue, const TSourceLoc& intValueLine)
{
    TLayoutQualifier qualifier;

    qualifier.location = -1;
    qualifier.matrixPacking = EmpUnspecified;
    qualifier.blockStorage = EbsUnspecified;

    if (qualifierType != "location")
    {
        error(qualifierTypeLine, "invalid layout qualifier", qualifierType.c_str(), "only location may have arguments");
        recover();
    }
    else
    {
        // must check that location is non-negative
        if (intValue < 0)
        {
            error(intValueLine, "out of range:", intValueString.c_str(), "location must be non-negative");
            recover();
        }
        else
        {
            qualifier.location = intValue;
        }
    }

    return qualifier;
}

TLayoutQualifier TParseContext::joinLayoutQualifiers(TLayoutQualifier leftQualifier, TLayoutQualifier rightQualifier)
{
    TLayoutQualifier joinedQualifier = leftQualifier;

    if (rightQualifier.location != -1)
    {
        joinedQualifier.location = rightQualifier.location;
    }
    if (rightQualifier.matrixPacking != EmpUnspecified)
    {
        joinedQualifier.matrixPacking = rightQualifier.matrixPacking;
    }
    if (rightQualifier.blockStorage != EbsUnspecified)
    {
        joinedQualifier.blockStorage = rightQualifier.blockStorage;
    }

    return joinedQualifier;
}

TPublicType TParseContext::joinInterpolationQualifiers(const TSourceLoc &interpolationLoc, TQualifier interpolationQualifier,
                                                       const TSourceLoc &storageLoc, TQualifier storageQualifier)
{
    TQualifier mergedQualifier = EvqSmoothIn;

    if (storageQualifier == EvqFragmentIn) {
        if (interpolationQualifier == EvqSmooth)
            mergedQualifier = EvqSmoothIn;
        else if (interpolationQualifier == EvqFlat)
            mergedQualifier = EvqFlatIn;
        else UNREACHABLE();
    }
    else if (storageQualifier == EvqCentroidIn) {
        if (interpolationQualifier == EvqSmooth)
            mergedQualifier = EvqCentroidIn;
        else if (interpolationQualifier == EvqFlat)
            mergedQualifier = EvqFlatIn;
        else UNREACHABLE();
    }
    else if (storageQualifier == EvqVertexOut) {
        if (interpolationQualifier == EvqSmooth)
            mergedQualifier = EvqSmoothOut;
        else if (interpolationQualifier == EvqFlat)
            mergedQualifier = EvqFlatOut;
        else UNREACHABLE();
    }
    else if (storageQualifier == EvqCentroidOut) {
        if (interpolationQualifier == EvqSmooth)
            mergedQualifier = EvqCentroidOut;
        else if (interpolationQualifier == EvqFlat)
            mergedQualifier = EvqFlatOut;
        else UNREACHABLE();
    }
    else {
        error(interpolationLoc, "interpolation qualifier requires a fragment 'in' or vertex 'out' storage qualifier", getInterpolationString(interpolationQualifier));
        recover();

        mergedQualifier = storageQualifier;
    }

    TPublicType type;
    type.setBasic(EbtVoid, mergedQualifier, storageLoc);
    return type;
}

TFieldList *TParseContext::addStructDeclaratorList(const TPublicType& typeSpecifier, TFieldList *fieldList)
{
    if (voidErrorCheck(typeSpecifier.line, (*fieldList)[0]->name(), typeSpecifier)) {
        recover();
    }

    for (unsigned int i = 0; i < fieldList->size(); ++i) {
        //
        // Careful not to replace already known aspects of type, like array-ness
        //
        TType* type = (*fieldList)[i]->type();
        type->setBasicType(typeSpecifier.type);
        type->setPrimarySize(typeSpecifier.primarySize);
        type->setSecondarySize(typeSpecifier.secondarySize);
        type->setPrecision(typeSpecifier.precision);
        type->setQualifier(typeSpecifier.qualifier);
        type->setLayoutQualifier(typeSpecifier.layoutQualifier);

        // don't allow arrays of arrays
        if (type->isArray()) {
            if (arrayTypeErrorCheck(typeSpecifier.line, typeSpecifier))
                recover();
        }
        if (typeSpecifier.array)
            type->setArraySize(typeSpecifier.arraySize);
        if (typeSpecifier.userDef) {
            type->setStruct(typeSpecifier.userDef->getStruct());
        }

        if (structNestingErrorCheck(typeSpecifier.line, *(*fieldList)[i])) {
            recover();
        }
    }

    return fieldList;
}

TPublicType TParseContext::addStructure(const TSourceLoc& structLine, const TSourceLoc& nameLine, const TString *structName, TFieldList* fieldList)
{
    TStructure* structure = new TStructure(structName, fieldList);
    TType* structureType = new TType(structure);

    structure->setUniqueId(TSymbolTable::nextUniqueId());

    if (!structName->empty())
    {
        if (reservedErrorCheck(nameLine, *structName))
        {
            recover();
        }
        TVariable* userTypeDef = new TVariable(structName, *structureType, true);
        if (!symbolTable.declare(userTypeDef)) {
            error(nameLine, "redefinition", structName->c_str(), "struct");
            recover();
        }
    }

    // ensure we do not specify any storage qualifiers on the struct members
    for (unsigned int typeListIndex = 0; typeListIndex < fieldList->size(); typeListIndex++)
    {
        const TField &field = *(*fieldList)[typeListIndex];
        const TQualifier qualifier = field.type()->getQualifier();
        switch (qualifier)
        {
          case EvqGlobal:
          case EvqTemporary:
            break;
          default:
            error(field.line(), "invalid qualifier on struct member", getQualifierString(qualifier));
            recover();
            break;
        }
    }

    TPublicType publicType;
    publicType.setBasic(EbtStruct, EvqTemporary, structLine);
    publicType.userDef = structureType;
    exitStructDeclaration();

    return publicType;
}

//
// Parse an array of strings using yyparse.
//
// Returns 0 for success.
//
int PaParseStrings(size_t count, const char* const string[], const int length[],
                   TParseContext* context) {
    if ((count == 0) || (string == NULL))
        return 1;

    if (glslang_initialize(context))
        return 1;

    int error = glslang_scan(count, string, length, context);
    if (!error)
        error = glslang_parse(context);

    glslang_finalize(context);

    return (error == 0) && (context->numErrors() == 0) ? 0 : 1;
}



