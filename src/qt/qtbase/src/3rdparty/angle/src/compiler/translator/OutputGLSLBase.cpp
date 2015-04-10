//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/OutputGLSLBase.h"
#include "compiler/translator/compilerdebug.h"

#include <cfloat>

namespace
{
TString arrayBrackets(const TType& type)
{
    ASSERT(type.isArray());
    TInfoSinkBase out;
    out << "[" << type.getArraySize() << "]";
    return TString(out.c_str());
}

bool isSingleStatement(TIntermNode* node) {
    if (const TIntermAggregate* aggregate = node->getAsAggregate())
    {
        return (aggregate->getOp() != EOpFunction) &&
               (aggregate->getOp() != EOpSequence);
    }
    else if (const TIntermSelection* selection = node->getAsSelectionNode())
    {
        // Ternary operators are usually part of an assignment operator.
        // This handles those rare cases in which they are all by themselves.
        return selection->usesTernaryOperator();
    }
    else if (node->getAsLoopNode())
    {
        return false;
    }
    return true;
}
}  // namespace

TOutputGLSLBase::TOutputGLSLBase(TInfoSinkBase& objSink,
                                 ShArrayIndexClampingStrategy clampingStrategy,
                                 ShHashFunction64 hashFunction,
                                 NameMap& nameMap,
                                 TSymbolTable& symbolTable)
    : TIntermTraverser(true, true, true),
      mObjSink(objSink),
      mDeclaringVariables(false),
      mClampingStrategy(clampingStrategy),
      mHashFunction(hashFunction),
      mNameMap(nameMap),
      mSymbolTable(symbolTable)
{
}

void TOutputGLSLBase::writeTriplet(Visit visit, const char* preStr, const char* inStr, const char* postStr)
{
    TInfoSinkBase& out = objSink();
    if (visit == PreVisit && preStr)
    {
        out << preStr;
    }
    else if (visit == InVisit && inStr)
    {
        out << inStr;
    }
    else if (visit == PostVisit && postStr)
    {
        out << postStr;
    }
}

void TOutputGLSLBase::writeVariableType(const TType& type)
{
    TInfoSinkBase& out = objSink();
    TQualifier qualifier = type.getQualifier();
    // TODO(alokp): Validate qualifier for variable declarations.
    if ((qualifier != EvqTemporary) && (qualifier != EvqGlobal))
        out << type.getQualifierString() << " ";
    // Declare the struct if we have not done so already.
    if ((type.getBasicType() == EbtStruct) && !structDeclared(type.getStruct()))
    {
        declareStruct(type.getStruct());
    }
    else
    {
        if (writeVariablePrecision(type.getPrecision()))
            out << " ";
        out << getTypeName(type);
    }
}

void TOutputGLSLBase::writeFunctionParameters(const TIntermSequence& args)
{
    TInfoSinkBase& out = objSink();
    for (TIntermSequence::const_iterator iter = args.begin();
         iter != args.end(); ++iter)
    {
        const TIntermSymbol* arg = (*iter)->getAsSymbolNode();
        ASSERT(arg != NULL);

        const TType& type = arg->getType();
        writeVariableType(type);

        const TString& name = arg->getSymbol();
        if (!name.empty())
            out << " " << hashName(name);
        if (type.isArray())
            out << arrayBrackets(type);

        // Put a comma if this is not the last argument.
        if (iter != args.end() - 1)
            out << ", ";
    }
}

const ConstantUnion* TOutputGLSLBase::writeConstantUnion(const TType& type,
                                                         const ConstantUnion* pConstUnion)
{
    TInfoSinkBase& out = objSink();

    if (type.getBasicType() == EbtStruct)
    {
        const TStructure* structure = type.getStruct();
        out << hashName(structure->name()) << "(";

        const TFieldList& fields = structure->fields();
        for (size_t i = 0; i < fields.size(); ++i)
        {
            const TType* fieldType = fields[i]->type();
            ASSERT(fieldType != NULL);
            pConstUnion = writeConstantUnion(*fieldType, pConstUnion);
            if (i != fields.size() - 1) out << ", ";
        }
        out << ")";
    }
    else
    {
        size_t size = type.getObjectSize();
        bool writeType = size > 1;
        if (writeType) out << getTypeName(type) << "(";
        for (size_t i = 0; i < size; ++i, ++pConstUnion)
        {
            switch (pConstUnion->getType())
            {
                case EbtFloat: out << std::min(FLT_MAX, std::max(-FLT_MAX, pConstUnion->getFConst())); break;
                case EbtInt: out << pConstUnion->getIConst(); break;
                case EbtBool: out << pConstUnion->getBConst(); break;
                default: UNREACHABLE();
            }
            if (i != size - 1) out << ", ";
        }
        if (writeType) out << ")";
    }
    return pConstUnion;
}

void TOutputGLSLBase::visitSymbol(TIntermSymbol* node)
{
    TInfoSinkBase& out = objSink();
    if (mLoopUnroll.NeedsToReplaceSymbolWithValue(node))
        out << mLoopUnroll.GetLoopIndexValue(node);
    else
        out << hashVariableName(node->getSymbol());

    if (mDeclaringVariables && node->getType().isArray())
        out << arrayBrackets(node->getType());
}

void TOutputGLSLBase::visitConstantUnion(TIntermConstantUnion* node)
{
    writeConstantUnion(node->getType(), node->getUnionArrayPointer());
}

bool TOutputGLSLBase::visitBinary(Visit visit, TIntermBinary* node)
{
    bool visitChildren = true;
    TInfoSinkBase& out = objSink();
    switch (node->getOp())
    {
        case EOpInitialize:
            if (visit == InVisit)
            {
                out << " = ";
                // RHS of initialize is not being declared.
                mDeclaringVariables = false;
            }
            break;
        case EOpAssign: writeTriplet(visit, "(", " = ", ")"); break;
        case EOpAddAssign: writeTriplet(visit, "(", " += ", ")"); break;
        case EOpSubAssign: writeTriplet(visit, "(", " -= ", ")"); break;
        case EOpDivAssign: writeTriplet(visit, "(", " /= ", ")"); break;
        // Notice the fall-through.
        case EOpMulAssign: 
        case EOpVectorTimesMatrixAssign:
        case EOpVectorTimesScalarAssign:
        case EOpMatrixTimesScalarAssign:
        case EOpMatrixTimesMatrixAssign:
            writeTriplet(visit, "(", " *= ", ")");
            break;

        case EOpIndexDirect:
            writeTriplet(visit, NULL, "[", "]");
            break;
        case EOpIndexIndirect:
            if (node->getAddIndexClamp())
            {
                if (visit == InVisit)
                {
                    if (mClampingStrategy == SH_CLAMP_WITH_CLAMP_INTRINSIC) {
                        out << "[int(clamp(float(";
                    } else {
                        out << "[webgl_int_clamp(";
                    }
                }
                else if (visit == PostVisit)
                {
                    int maxSize;
                    TIntermTyped *left = node->getLeft();
                    TType leftType = left->getType();

                    if (left->isArray())
                    {
                        // The shader will fail validation if the array length is not > 0.
                        maxSize = leftType.getArraySize() - 1;
                    }
                    else
                    {
                        maxSize = leftType.getNominalSize() - 1;
                    }

                    if (mClampingStrategy == SH_CLAMP_WITH_CLAMP_INTRINSIC) {
                        out << "), 0.0, float(" << maxSize << ")))]";
                    } else {
                        out << ", 0, " << maxSize << ")]";
                    }
                }
            }
            else
            {
                writeTriplet(visit, NULL, "[", "]");
            }
            break;
        case EOpIndexDirectStruct:
            if (visit == InVisit)
            {
                // Here we are writing out "foo.bar", where "foo" is struct
                // and "bar" is field. In AST, it is represented as a binary
                // node, where left child represents "foo" and right child "bar".
                // The node itself represents ".". The struct field "bar" is
                // actually stored as an index into TStructure::fields.
                out << ".";
                const TStructure* structure = node->getLeft()->getType().getStruct();
                const TIntermConstantUnion* index = node->getRight()->getAsConstantUnion();
                const TField* field = structure->fields()[index->getIConst(0)];

                TString fieldName = field->name();
                if (!mSymbolTable.findBuiltIn(structure->name()))
                    fieldName = hashName(fieldName);

                out << fieldName;
                visitChildren = false;
            }
            break;
        case EOpVectorSwizzle:
            if (visit == InVisit)
            {
                out << ".";
                TIntermAggregate* rightChild = node->getRight()->getAsAggregate();
                TIntermSequence& sequence = rightChild->getSequence();
                for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); ++sit)
                {
                    TIntermConstantUnion* element = (*sit)->getAsConstantUnion();
                    ASSERT(element->getBasicType() == EbtInt);
                    ASSERT(element->getNominalSize() == 1);
                    const ConstantUnion& data = element->getUnionArrayPointer()[0];
                    ASSERT(data.getType() == EbtInt);
                    switch (data.getIConst())
                    {
                        case 0: out << "x"; break;
                        case 1: out << "y"; break;
                        case 2: out << "z"; break;
                        case 3: out << "w"; break;
                        default: UNREACHABLE(); break;
                    }
                }
                visitChildren = false;
            }
            break;

        case EOpAdd: writeTriplet(visit, "(", " + ", ")"); break;
        case EOpSub: writeTriplet(visit, "(", " - ", ")"); break;
        case EOpMul: writeTriplet(visit, "(", " * ", ")"); break;
        case EOpDiv: writeTriplet(visit, "(", " / ", ")"); break;
        case EOpMod: UNIMPLEMENTED(); break;
        case EOpEqual: writeTriplet(visit, "(", " == ", ")"); break;
        case EOpNotEqual: writeTriplet(visit, "(", " != ", ")"); break;
        case EOpLessThan: writeTriplet(visit, "(", " < ", ")"); break;
        case EOpGreaterThan: writeTriplet(visit, "(", " > ", ")"); break;
        case EOpLessThanEqual: writeTriplet(visit, "(", " <= ", ")"); break;
        case EOpGreaterThanEqual: writeTriplet(visit, "(", " >= ", ")"); break;

        // Notice the fall-through.
        case EOpVectorTimesScalar:
        case EOpVectorTimesMatrix:
        case EOpMatrixTimesVector:
        case EOpMatrixTimesScalar:
        case EOpMatrixTimesMatrix:
            writeTriplet(visit, "(", " * ", ")");
            break;

        case EOpLogicalOr: writeTriplet(visit, "(", " || ", ")"); break;
        case EOpLogicalXor: writeTriplet(visit, "(", " ^^ ", ")"); break;
        case EOpLogicalAnd: writeTriplet(visit, "(", " && ", ")"); break;
        default: UNREACHABLE(); break;
    }

    return visitChildren;
}

bool TOutputGLSLBase::visitUnary(Visit visit, TIntermUnary* node)
{
    TString preString;
    TString postString = ")";

    switch (node->getOp())
    {
        case EOpNegative: preString = "(-"; break;
        case EOpVectorLogicalNot: preString = "not("; break;
        case EOpLogicalNot: preString = "(!"; break;

        case EOpPostIncrement: preString = "("; postString = "++)"; break;
        case EOpPostDecrement: preString = "("; postString = "--)"; break;
        case EOpPreIncrement: preString = "(++"; break;
        case EOpPreDecrement: preString = "(--"; break;

        case EOpConvIntToBool:
        case EOpConvFloatToBool:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: preString =  "bool(";  break;
                case 2: preString = "bvec2("; break;
                case 3: preString = "bvec3("; break;
                case 4: preString = "bvec4("; break;
                default: UNREACHABLE();
            }
            break;
        case EOpConvBoolToFloat:
        case EOpConvIntToFloat:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: preString = "float(";  break;
                case 2: preString = "vec2("; break;
                case 3: preString = "vec3("; break;
                case 4: preString = "vec4("; break;
                default: UNREACHABLE();
            }
            break;
        case EOpConvFloatToInt:
        case EOpConvBoolToInt:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: preString = "int(";  break;
                case 2: preString = "ivec2("; break;
                case 3: preString = "ivec3("; break;
                case 4: preString = "ivec4("; break;
                default: UNREACHABLE();
            }
            break;

        case EOpRadians: preString = "radians("; break;
        case EOpDegrees: preString = "degrees("; break;
        case EOpSin: preString = "sin("; break;
        case EOpCos: preString = "cos("; break;
        case EOpTan: preString = "tan("; break;
        case EOpAsin: preString = "asin("; break;
        case EOpAcos: preString = "acos("; break;
        case EOpAtan: preString = "atan("; break;

        case EOpExp: preString = "exp("; break;
        case EOpLog: preString = "log("; break;
        case EOpExp2: preString = "exp2("; break;
        case EOpLog2: preString = "log2("; break;
        case EOpSqrt: preString = "sqrt("; break;
        case EOpInverseSqrt: preString = "inversesqrt("; break;

        case EOpAbs: preString = "abs("; break;
        case EOpSign: preString = "sign("; break;
        case EOpFloor: preString = "floor("; break;
        case EOpCeil: preString = "ceil("; break;
        case EOpFract: preString = "fract("; break;

        case EOpLength: preString = "length("; break;
        case EOpNormalize: preString = "normalize("; break;

        case EOpDFdx: preString = "dFdx("; break;
        case EOpDFdy: preString = "dFdy("; break;
        case EOpFwidth: preString = "fwidth("; break;

        case EOpAny: preString = "any("; break;
        case EOpAll: preString = "all("; break;

        default: UNREACHABLE(); break;
    }

    if (visit == PreVisit && node->getUseEmulatedFunction())
        preString = BuiltInFunctionEmulator::GetEmulatedFunctionName(preString);
    writeTriplet(visit, preString.c_str(), NULL, postString.c_str());

    return true;
}

bool TOutputGLSLBase::visitSelection(Visit visit, TIntermSelection* node)
{
    TInfoSinkBase& out = objSink();

    if (node->usesTernaryOperator())
    {
        // Notice two brackets at the beginning and end. The outer ones
        // encapsulate the whole ternary expression. This preserves the
        // order of precedence when ternary expressions are used in a
        // compound expression, i.e., c = 2 * (a < b ? 1 : 2).
        out << "((";
        node->getCondition()->traverse(this);
        out << ") ? (";
        node->getTrueBlock()->traverse(this);
        out << ") : (";
        node->getFalseBlock()->traverse(this);
        out << "))";
    }
    else
    {
        out << "if (";
        node->getCondition()->traverse(this);
        out << ")\n";

        incrementDepth(node);
        visitCodeBlock(node->getTrueBlock());

        if (node->getFalseBlock())
        {
            out << "else\n";
            visitCodeBlock(node->getFalseBlock());
        }
        decrementDepth();
    }
    return false;
}

bool TOutputGLSLBase::visitAggregate(Visit visit, TIntermAggregate* node)
{
    bool visitChildren = true;
    TInfoSinkBase& out = objSink();
    TString preString;
    bool delayedWrite = false;
    switch (node->getOp())
    {
        case EOpSequence: {
            // Scope the sequences except when at the global scope.
            if (depth > 0) out << "{\n";

            incrementDepth(node);
            const TIntermSequence& sequence = node->getSequence();
            for (TIntermSequence::const_iterator iter = sequence.begin();
                 iter != sequence.end(); ++iter)
            {
                TIntermNode* node = *iter;
                ASSERT(node != NULL);
                node->traverse(this);

                if (isSingleStatement(node))
                    out << ";\n";
            }
            decrementDepth();

            // Scope the sequences except when at the global scope.
            if (depth > 0) out << "}\n";
            visitChildren = false;
            break;
        }
        case EOpPrototype: {
            // Function declaration.
            ASSERT(visit == PreVisit);
            writeVariableType(node->getType());
            out << " " << hashName(node->getName());

            out << "(";
            writeFunctionParameters(node->getSequence());
            out << ")";

            visitChildren = false;
            break;
        }
        case EOpFunction: {
            // Function definition.
            ASSERT(visit == PreVisit);
            writeVariableType(node->getType());
            out << " " << hashFunctionName(node->getName());

            incrementDepth(node);
            // Function definition node contains one or two children nodes
            // representing function parameters and function body. The latter
            // is not present in case of empty function bodies.
            const TIntermSequence& sequence = node->getSequence();
            ASSERT((sequence.size() == 1) || (sequence.size() == 2));
            TIntermSequence::const_iterator seqIter = sequence.begin();

            // Traverse function parameters.
            TIntermAggregate* params = (*seqIter)->getAsAggregate();
            ASSERT(params != NULL);
            ASSERT(params->getOp() == EOpParameters);
            params->traverse(this);

            // Traverse function body.
            TIntermAggregate* body = ++seqIter != sequence.end() ?
                (*seqIter)->getAsAggregate() : NULL;
            visitCodeBlock(body);
            decrementDepth();
 
            // Fully processed; no need to visit children.
            visitChildren = false;
            break;
        }
        case EOpFunctionCall:
            // Function call.
            if (visit == PreVisit)
            {
                out << hashFunctionName(node->getName()) << "(";
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << ")";
            }
            break;
        case EOpParameters: {
            // Function parameters.
            ASSERT(visit == PreVisit);
            out << "(";
            writeFunctionParameters(node->getSequence());
            out << ")";
            visitChildren = false;
            break;
        }
        case EOpDeclaration: {
            // Variable declaration.
            if (visit == PreVisit)
            {
                const TIntermSequence& sequence = node->getSequence();
                const TIntermTyped* variable = sequence.front()->getAsTyped();
                writeVariableType(variable->getType());
                out << " ";
                mDeclaringVariables = true;
            }
            else if (visit == InVisit)
            {
                out << ", ";
                mDeclaringVariables = true;
            }
            else
            {
                mDeclaringVariables = false;
            }
            break;
        }
        case EOpConstructFloat: writeTriplet(visit, "float(", NULL, ")"); break;
        case EOpConstructVec2: writeTriplet(visit, "vec2(", ", ", ")"); break;
        case EOpConstructVec3: writeTriplet(visit, "vec3(", ", ", ")"); break;
        case EOpConstructVec4: writeTriplet(visit, "vec4(", ", ", ")"); break;
        case EOpConstructBool: writeTriplet(visit, "bool(", NULL, ")"); break;
        case EOpConstructBVec2: writeTriplet(visit, "bvec2(", ", ", ")"); break;
        case EOpConstructBVec3: writeTriplet(visit, "bvec3(", ", ", ")"); break;
        case EOpConstructBVec4: writeTriplet(visit, "bvec4(", ", ", ")"); break;
        case EOpConstructInt: writeTriplet(visit, "int(", NULL, ")"); break;
        case EOpConstructIVec2: writeTriplet(visit, "ivec2(", ", ", ")"); break;
        case EOpConstructIVec3: writeTriplet(visit, "ivec3(", ", ", ")"); break;
        case EOpConstructIVec4: writeTriplet(visit, "ivec4(", ", ", ")"); break;
        case EOpConstructMat2: writeTriplet(visit, "mat2(", ", ", ")"); break;
        case EOpConstructMat3: writeTriplet(visit, "mat3(", ", ", ")"); break;
        case EOpConstructMat4: writeTriplet(visit, "mat4(", ", ", ")"); break;
        case EOpConstructStruct:
            if (visit == PreVisit)
            {
                const TType& type = node->getType();
                ASSERT(type.getBasicType() == EbtStruct);
                out << hashName(type.getStruct()->name()) << "(";
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << ")";
            }
            break;

        case EOpLessThan: preString = "lessThan("; delayedWrite = true; break;
        case EOpGreaterThan: preString = "greaterThan("; delayedWrite = true; break;
        case EOpLessThanEqual: preString = "lessThanEqual("; delayedWrite = true; break;
        case EOpGreaterThanEqual: preString = "greaterThanEqual("; delayedWrite = true; break;
        case EOpVectorEqual: preString = "equal("; delayedWrite = true; break;
        case EOpVectorNotEqual: preString = "notEqual("; delayedWrite = true; break;
        case EOpComma: writeTriplet(visit, NULL, ", ", NULL); break;

        case EOpMod: preString = "mod("; delayedWrite = true; break;
        case EOpPow: preString = "pow("; delayedWrite = true; break;
        case EOpAtan: preString = "atan("; delayedWrite = true; break;
        case EOpMin: preString = "min("; delayedWrite = true; break;
        case EOpMax: preString = "max("; delayedWrite = true; break;
        case EOpClamp: preString = "clamp("; delayedWrite = true; break;
        case EOpMix: preString = "mix("; delayedWrite = true; break;
        case EOpStep: preString = "step("; delayedWrite = true; break;
        case EOpSmoothStep: preString = "smoothstep("; delayedWrite = true; break;

        case EOpDistance: preString = "distance("; delayedWrite = true; break;
        case EOpDot: preString = "dot("; delayedWrite = true; break;
        case EOpCross: preString = "cross("; delayedWrite = true; break;
        case EOpFaceForward: preString = "faceforward("; delayedWrite = true; break;
        case EOpReflect: preString = "reflect("; delayedWrite = true; break;
        case EOpRefract: preString = "refract("; delayedWrite = true; break;
        case EOpMul: preString = "matrixCompMult("; delayedWrite = true; break;

        default: UNREACHABLE(); break;
    }
    if (delayedWrite && visit == PreVisit && node->getUseEmulatedFunction())
        preString = BuiltInFunctionEmulator::GetEmulatedFunctionName(preString);
    if (delayedWrite)
        writeTriplet(visit, preString.c_str(), ", ", ")");
    return visitChildren;
}

bool TOutputGLSLBase::visitLoop(Visit visit, TIntermLoop* node)
{
    TInfoSinkBase& out = objSink();

    incrementDepth(node);
    // Loop header.
    TLoopType loopType = node->getType();
    if (loopType == ELoopFor)  // for loop
    {
        if (!node->getUnrollFlag()) {
            out << "for (";
            if (node->getInit())
                node->getInit()->traverse(this);
            out << "; ";

            if (node->getCondition())
                node->getCondition()->traverse(this);
            out << "; ";

            if (node->getExpression())
                node->getExpression()->traverse(this);
            out << ")\n";
        }
    }
    else if (loopType == ELoopWhile)  // while loop
    {
        out << "while (";
        ASSERT(node->getCondition() != NULL);
        node->getCondition()->traverse(this);
        out << ")\n";
    }
    else  // do-while loop
    {
        ASSERT(loopType == ELoopDoWhile);
        out << "do\n";
    }

    // Loop body.
    if (node->getUnrollFlag())
    {
        TLoopIndexInfo indexInfo;
        mLoopUnroll.FillLoopIndexInfo(node, indexInfo);
        mLoopUnroll.Push(indexInfo);
        while (mLoopUnroll.SatisfiesLoopCondition())
        {
            visitCodeBlock(node->getBody());
            mLoopUnroll.Step();
        }
        mLoopUnroll.Pop();
    }
    else
    {
        visitCodeBlock(node->getBody());
    }

    // Loop footer.
    if (loopType == ELoopDoWhile)  // do-while loop
    {
        out << "while (";
        ASSERT(node->getCondition() != NULL);
        node->getCondition()->traverse(this);
        out << ");\n";
    }
    decrementDepth();

    // No need to visit children. They have been already processed in
    // this function.
    return false;
}

bool TOutputGLSLBase::visitBranch(Visit visit, TIntermBranch* node)
{
    switch (node->getFlowOp())
    {
        case EOpKill: writeTriplet(visit, "discard", NULL, NULL); break;
        case EOpBreak: writeTriplet(visit, "break", NULL, NULL); break;
        case EOpContinue: writeTriplet(visit, "continue", NULL, NULL); break;
        case EOpReturn: writeTriplet(visit, "return ", NULL, NULL); break;
        default: UNREACHABLE(); break;
    }

    return true;
}

void TOutputGLSLBase::visitCodeBlock(TIntermNode* node) {
    TInfoSinkBase &out = objSink();
    if (node != NULL)
    {
        node->traverse(this);
        // Single statements not part of a sequence need to be terminated
        // with semi-colon.
        if (isSingleStatement(node))
            out << ";\n";
    }
    else
    {
        out << "{\n}\n";  // Empty code block.
    }
}

TString TOutputGLSLBase::getTypeName(const TType& type)
{
    TInfoSinkBase out;
    if (type.isMatrix())
    {
        out << "mat";
        out << type.getNominalSize();
    }
    else if (type.isVector())
    {
        switch (type.getBasicType())
        {
            case EbtFloat: out << "vec"; break;
            case EbtInt: out << "ivec"; break;
            case EbtBool: out << "bvec"; break;
            default: UNREACHABLE(); break;
        }
        out << type.getNominalSize();
    }
    else
    {
        if (type.getBasicType() == EbtStruct)
            out << hashName(type.getStruct()->name());
        else
            out << type.getBasicString();
    }
    return TString(out.c_str());
}

TString TOutputGLSLBase::hashName(const TString& name)
{
    if (mHashFunction == NULL || name.empty())
        return name;
    NameMap::const_iterator it = mNameMap.find(name.c_str());
    if (it != mNameMap.end())
        return it->second.c_str();
    TString hashedName = TIntermTraverser::hash(name, mHashFunction);
    mNameMap[name.c_str()] = hashedName.c_str();
    return hashedName;
}

TString TOutputGLSLBase::hashVariableName(const TString& name)
{
    if (mSymbolTable.findBuiltIn(name) != NULL)
        return name;
    return hashName(name);
}

TString TOutputGLSLBase::hashFunctionName(const TString& mangled_name)
{
    TString name = TFunction::unmangleName(mangled_name);
    if (mSymbolTable.findBuiltIn(mangled_name) != NULL || name == "main")
        return name;
    return hashName(name);
}

bool TOutputGLSLBase::structDeclared(const TStructure* structure) const
{
    return mDeclaredStructs.find(structure->name()) != mDeclaredStructs.end();
}

void TOutputGLSLBase::declareStruct(const TStructure* structure)
{
    TInfoSinkBase& out = objSink();

    out << "struct " << hashName(structure->name()) << "{\n";
    const TFieldList& fields = structure->fields();
    for (size_t i = 0; i < fields.size(); ++i)
    {
        const TField* field = fields[i];
        if (writeVariablePrecision(field->type()->getPrecision()))
            out << " ";
        out << getTypeName(*field->type()) << " " << hashName(field->name());
        if (field->type()->isArray())
            out << arrayBrackets(*field->type());
        out << ";\n";
    }
    out << "}";

    mDeclaredStructs.insert(structure->name());
}
