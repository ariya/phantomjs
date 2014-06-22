//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Build the intermediate representation.
//

#include <float.h>
#include <limits.h>
#include <algorithm>

#include "compiler/HashNames.h"
#include "compiler/localintermediate.h"
#include "compiler/QualifierAlive.h"
#include "compiler/RemoveTree.h"

bool CompareStructure(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray);

static TPrecision GetHigherPrecision( TPrecision left, TPrecision right ){
    return left > right ? left : right;
}

const char* getOperatorString(TOperator op) {
    switch (op) {
      case EOpInitialize: return "=";
      case EOpAssign: return "=";
      case EOpAddAssign: return "+=";
      case EOpSubAssign: return "-=";
      case EOpDivAssign: return "/=";

      // Fall-through.
      case EOpMulAssign: 
      case EOpVectorTimesMatrixAssign:
      case EOpVectorTimesScalarAssign:
      case EOpMatrixTimesScalarAssign:
      case EOpMatrixTimesMatrixAssign: return "*=";

      // Fall-through.
      case EOpIndexDirect:
      case EOpIndexIndirect: return "[]";

      case EOpIndexDirectStruct: return ".";
      case EOpVectorSwizzle: return ".";
      case EOpAdd: return "+";
      case EOpSub: return "-";
      case EOpMul: return "*";
      case EOpDiv: return "/";
      case EOpMod: UNIMPLEMENTED(); break;
      case EOpEqual: return "==";
      case EOpNotEqual: return "!=";
      case EOpLessThan: return "<";
      case EOpGreaterThan: return ">";
      case EOpLessThanEqual: return "<=";
      case EOpGreaterThanEqual: return ">=";

      // Fall-through.
      case EOpVectorTimesScalar:
      case EOpVectorTimesMatrix:
      case EOpMatrixTimesVector:
      case EOpMatrixTimesScalar:
      case EOpMatrixTimesMatrix: return "*";

      case EOpLogicalOr: return "||";
      case EOpLogicalXor: return "^^";
      case EOpLogicalAnd: return "&&";
      case EOpNegative: return "-";
      case EOpVectorLogicalNot: return "not";
      case EOpLogicalNot: return "!";
      case EOpPostIncrement: return "++";
      case EOpPostDecrement: return "--";
      case EOpPreIncrement: return "++";
      case EOpPreDecrement: return "--";

      // Fall-through.
      case EOpConvIntToBool:
      case EOpConvFloatToBool: return "bool";
 
      // Fall-through.
      case EOpConvBoolToFloat:
      case EOpConvIntToFloat: return "float";
 
      // Fall-through.
      case EOpConvFloatToInt:
      case EOpConvBoolToInt: return "int";

      case EOpRadians: return "radians";
      case EOpDegrees: return "degrees";
      case EOpSin: return "sin";
      case EOpCos: return "cos";
      case EOpTan: return "tan";
      case EOpAsin: return "asin";
      case EOpAcos: return "acos";
      case EOpAtan: return "atan";
      case EOpExp: return "exp";
      case EOpLog: return "log";
      case EOpExp2: return "exp2";
      case EOpLog2: return "log2";
      case EOpSqrt: return "sqrt";
      case EOpInverseSqrt: return "inversesqrt";
      case EOpAbs: return "abs";
      case EOpSign: return "sign";
      case EOpFloor: return "floor";
      case EOpCeil: return "ceil";
      case EOpFract: return "fract";
      case EOpLength: return "length";
      case EOpNormalize: return "normalize";
      case EOpDFdx: return "dFdx";
      case EOpDFdy: return "dFdy";
      case EOpFwidth: return "fwidth";
      case EOpAny: return "any";
      case EOpAll: return "all";

      default: break;
    }
    return "";
}

////////////////////////////////////////////////////////////////////////////
//
// First set of functions are to help build the intermediate representation.
// These functions are not member functions of the nodes.
// They are called from parser productions.
//
/////////////////////////////////////////////////////////////////////////////

//
// Add a terminal node for an identifier in an expression.
//
// Returns the added node.
//
TIntermSymbol* TIntermediate::addSymbol(int id, const TString& name, const TType& type, const TSourceLoc& line)
{
    TIntermSymbol* node = new TIntermSymbol(id, name, type);
    node->setLine(line);

    return node;
}

//
// Connect two nodes with a new parent that does a binary operation on the nodes.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addBinaryMath(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc& line, TSymbolTable& symbolTable)
{
    switch (op) {
        case EOpEqual:
        case EOpNotEqual:
            if (left->isArray())
                return 0;
            break;
        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
            if (left->isMatrix() || left->isArray() || left->isVector() || left->getBasicType() == EbtStruct) {
                return 0;
            }
            break;
        case EOpLogicalOr:
        case EOpLogicalXor:
        case EOpLogicalAnd:
            if (left->getBasicType() != EbtBool || left->isMatrix() || left->isArray() || left->isVector()) {
                return 0;
            }
            break;
        case EOpAdd:
        case EOpSub:
        case EOpDiv:
        case EOpMul:
            if (left->getBasicType() == EbtStruct || left->getBasicType() == EbtBool)
                return 0;
        default: break;
    }

    //
    // First try converting the children to compatible types.
    //
    if (left->getType().getStruct() && right->getType().getStruct()) {
        if (left->getType() != right->getType())
            return 0;
    } else {
        TIntermTyped* child = addConversion(op, left->getType(), right);
        if (child)
            right = child;
        else {
            child = addConversion(op, right->getType(), left);
            if (child)
                left = child;
            else
                return 0;
        }
    }

    //
    // Need a new node holding things together then.  Make
    // one and promote it to the right type.
    //
    TIntermBinary* node = new TIntermBinary(op);
    node->setLine(line);

    node->setLeft(left);
    node->setRight(right);
    if (!node->promote(infoSink))
        return 0;

    //
    // See if we can fold constants.
    //
    TIntermTyped* typedReturnNode = 0;
    TIntermConstantUnion *leftTempConstant = left->getAsConstantUnion();
    TIntermConstantUnion *rightTempConstant = right->getAsConstantUnion();
    if (leftTempConstant && rightTempConstant) {
        typedReturnNode = leftTempConstant->fold(node->getOp(), rightTempConstant, infoSink);

        if (typedReturnNode)
            return typedReturnNode;
    }

    return node;
}

//
// Connect two nodes through an assignment.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addAssign(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc& line)
{
    //
    // Like adding binary math, except the conversion can only go
    // from right to left.
    //
    TIntermBinary* node = new TIntermBinary(op);
    node->setLine(line);

    TIntermTyped* child = addConversion(op, left->getType(), right);
    if (child == 0)
        return 0;

    node->setLeft(left);
    node->setRight(child);
    if (! node->promote(infoSink))
        return 0;

    return node;
}

//
// Connect two nodes through an index operator, where the left node is the base
// of an array or struct, and the right node is a direct or indirect offset.
//
// Returns the added node.
// The caller should set the type of the returned node.
//
TIntermTyped* TIntermediate::addIndex(TOperator op, TIntermTyped* base, TIntermTyped* index, const TSourceLoc& line)
{
    TIntermBinary* node = new TIntermBinary(op);
    node->setLine(line);
    node->setLeft(base);
    node->setRight(index);

    // caller should set the type

    return node;
}

//
// Add one node as the parent of another that it operates on.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addUnaryMath(TOperator op, TIntermNode* childNode, const TSourceLoc& line, TSymbolTable& symbolTable)
{
    TIntermUnary* node;
    TIntermTyped* child = childNode->getAsTyped();

    if (child == 0) {
        infoSink.info.message(EPrefixInternalError, line, "Bad type in AddUnaryMath");
        return 0;
    }

    switch (op) {
        case EOpLogicalNot:
            if (child->getType().getBasicType() != EbtBool || child->getType().isMatrix() || child->getType().isArray() || child->getType().isVector()) {
                return 0;
            }
            break;

        case EOpPostIncrement:
        case EOpPreIncrement:
        case EOpPostDecrement:
        case EOpPreDecrement:
        case EOpNegative:
            if (child->getType().getBasicType() == EbtStruct || child->getType().isArray())
                return 0;
        default: break;
    }

    //
    // Do we need to promote the operand?
    //
    // Note: Implicit promotions were removed from the language.
    //
    TBasicType newType = EbtVoid;
    switch (op) {
        case EOpConstructInt:   newType = EbtInt;   break;
        case EOpConstructBool:  newType = EbtBool;  break;
        case EOpConstructFloat: newType = EbtFloat; break;
        default: break;
    }

    if (newType != EbtVoid) {
        child = addConversion(op, TType(newType, child->getPrecision(), EvqTemporary,
            child->getNominalSize(),
            child->isMatrix(),
            child->isArray()),
            child);
        if (child == 0)
            return 0;
    }

    //
    // For constructors, we are now done, it's all in the conversion.
    //
    switch (op) {
        case EOpConstructInt:
        case EOpConstructBool:
        case EOpConstructFloat:
            return child;
        default: break;
    }

    TIntermConstantUnion *childTempConstant = 0;
    if (child->getAsConstantUnion())
        childTempConstant = child->getAsConstantUnion();

    //
    // Make a new node for the operator.
    //
    node = new TIntermUnary(op);
    node->setLine(line);
    node->setOperand(child);

    if (! node->promote(infoSink))
        return 0;

    if (childTempConstant)  {
        TIntermTyped* newChild = childTempConstant->fold(op, 0, infoSink);

        if (newChild)
            return newChild;
    }

    return node;
}

//
// This is the safe way to change the operator on an aggregate, as it
// does lots of error checking and fixing.  Especially for establishing
// a function call's operation on it's set of parameters.  Sequences
// of instructions are also aggregates, but they just direnctly set
// their operator to EOpSequence.
//
// Returns an aggregate node, which could be the one passed in if
// it was already an aggregate but no operator was set.
//
TIntermAggregate* TIntermediate::setAggregateOperator(TIntermNode* node, TOperator op, const TSourceLoc& line)
{
    TIntermAggregate* aggNode;

    //
    // Make sure we have an aggregate.  If not turn it into one.
    //
    if (node) {
        aggNode = node->getAsAggregate();
        if (aggNode == 0 || aggNode->getOp() != EOpNull) {
            //
            // Make an aggregate containing this node.
            //
            aggNode = new TIntermAggregate();
            aggNode->getSequence().push_back(node);
        }
    } else
        aggNode = new TIntermAggregate();

    //
    // Set the operator.
    //
    aggNode->setOp(op);
    aggNode->setLine(line);

    return aggNode;
}

//
// Convert one type to another.
//
// Returns the node representing the conversion, which could be the same
// node passed in if no conversion was needed.
//
// Return 0 if a conversion can't be done.
//
TIntermTyped* TIntermediate::addConversion(TOperator op, const TType& type, TIntermTyped* node)
{
    //
    // Does the base type allow operation?
    //
    switch (node->getBasicType()) {
        case EbtVoid:
        case EbtSampler2D:
        case EbtSamplerCube:
            return 0;
        default: break;
    }

    //
    // Otherwise, if types are identical, no problem
    //
    if (type == node->getType())
        return node;

    //
    // If one's a structure, then no conversions.
    //
    if (type.getStruct() || node->getType().getStruct())
        return 0;

    //
    // If one's an array, then no conversions.
    //
    if (type.isArray() || node->getType().isArray())
        return 0;

    TBasicType promoteTo;

    switch (op) {
        //
        // Explicit conversions
        //
        case EOpConstructBool:
            promoteTo = EbtBool;
            break;
        case EOpConstructFloat:
            promoteTo = EbtFloat;
            break;
        case EOpConstructInt:
            promoteTo = EbtInt;
            break;
        default:
            //
            // implicit conversions were removed from the language.
            //
            if (type.getBasicType() != node->getType().getBasicType())
                return 0;
            //
            // Size and structure could still differ, but that's
            // handled by operator promotion.
            //
            return node;
    }

    if (node->getAsConstantUnion()) {

        return (promoteConstantUnion(promoteTo, node->getAsConstantUnion()));
    } else {

        //
        // Add a new newNode for the conversion.
        //
        TIntermUnary* newNode = 0;

        TOperator newOp = EOpNull;
        switch (promoteTo) {
            case EbtFloat:
                switch (node->getBasicType()) {
                    case EbtInt:   newOp = EOpConvIntToFloat;  break;
                    case EbtBool:  newOp = EOpConvBoolToFloat; break;
                    default:
                        infoSink.info.message(EPrefixInternalError, node->getLine(), "Bad promotion node");
                        return 0;
                }
                break;
            case EbtBool:
                switch (node->getBasicType()) {
                    case EbtInt:   newOp = EOpConvIntToBool;   break;
                    case EbtFloat: newOp = EOpConvFloatToBool; break;
                    default:
                        infoSink.info.message(EPrefixInternalError, node->getLine(), "Bad promotion node");
                        return 0;
                }
                break;
            case EbtInt:
                switch (node->getBasicType()) {
                    case EbtBool:   newOp = EOpConvBoolToInt;  break;
                    case EbtFloat:  newOp = EOpConvFloatToInt; break;
                    default:
                        infoSink.info.message(EPrefixInternalError, node->getLine(), "Bad promotion node");
                        return 0;
                }
                break;
            default:
                infoSink.info.message(EPrefixInternalError, node->getLine(), "Bad promotion type");
                return 0;
        }

        TType type(promoteTo, node->getPrecision(), EvqTemporary, node->getNominalSize(), node->isMatrix(), node->isArray());
        newNode = new TIntermUnary(newOp, type);
        newNode->setLine(node->getLine());
        newNode->setOperand(node);

        return newNode;
    }
}

//
// Safe way to combine two nodes into an aggregate.  Works with null pointers,
// a node that's not a aggregate yet, etc.
//
// Returns the resulting aggregate, unless 0 was passed in for
// both existing nodes.
//
TIntermAggregate* TIntermediate::growAggregate(TIntermNode* left, TIntermNode* right, const TSourceLoc& line)
{
    if (left == 0 && right == 0)
        return 0;

    TIntermAggregate* aggNode = 0;
    if (left)
        aggNode = left->getAsAggregate();
    if (!aggNode || aggNode->getOp() != EOpNull) {
        aggNode = new TIntermAggregate;
        if (left)
            aggNode->getSequence().push_back(left);
    }

    if (right)
        aggNode->getSequence().push_back(right);

    aggNode->setLine(line);

    return aggNode;
}

//
// Turn an existing node into an aggregate.
//
// Returns an aggregate, unless 0 was passed in for the existing node.
//
TIntermAggregate* TIntermediate::makeAggregate(TIntermNode* node, const TSourceLoc& line)
{
    if (node == 0)
        return 0;

    TIntermAggregate* aggNode = new TIntermAggregate;
    aggNode->getSequence().push_back(node);
    aggNode->setLine(line);

    return aggNode;
}

//
// For "if" test nodes.  There are three children; a condition,
// a true path, and a false path.  The two paths are in the
// nodePair.
//
// Returns the selection node created.
//
TIntermNode* TIntermediate::addSelection(TIntermTyped* cond, TIntermNodePair nodePair, const TSourceLoc& line)
{
    //
    // For compile time constant selections, prune the code and
    // test now.
    //

    if (cond->getAsTyped() && cond->getAsTyped()->getAsConstantUnion()) {
        if (cond->getAsConstantUnion()->getBConst(0) == true)
            return nodePair.node1 ? setAggregateOperator(nodePair.node1, EOpSequence, nodePair.node1->getLine()) : NULL;
        else
            return nodePair.node2 ? setAggregateOperator(nodePair.node2, EOpSequence, nodePair.node2->getLine()) : NULL;
    }

    TIntermSelection* node = new TIntermSelection(cond, nodePair.node1, nodePair.node2);
    node->setLine(line);

    return node;
}


TIntermTyped* TIntermediate::addComma(TIntermTyped* left, TIntermTyped* right, const TSourceLoc& line)
{
    if (left->getType().getQualifier() == EvqConst && right->getType().getQualifier() == EvqConst) {
        return right;
    } else {
        TIntermTyped *commaAggregate = growAggregate(left, right, line);
        commaAggregate->getAsAggregate()->setOp(EOpComma);
        commaAggregate->setType(right->getType());
        commaAggregate->getTypePointer()->setQualifier(EvqTemporary);
        return commaAggregate;
    }
}

//
// For "?:" test nodes.  There are three children; a condition,
// a true path, and a false path.  The two paths are specified
// as separate parameters.
//
// Returns the selection node created, or 0 if one could not be.
//
TIntermTyped* TIntermediate::addSelection(TIntermTyped* cond, TIntermTyped* trueBlock, TIntermTyped* falseBlock, const TSourceLoc& line)
{
    //
    // Get compatible types.
    //
    TIntermTyped* child = addConversion(EOpSequence, trueBlock->getType(), falseBlock);
    if (child)
        falseBlock = child;
    else {
        child = addConversion(EOpSequence, falseBlock->getType(), trueBlock);
        if (child)
            trueBlock = child;
        else
            return 0;
    }

    //
    // See if all the operands are constant, then fold it otherwise not.
    //

    if (cond->getAsConstantUnion() && trueBlock->getAsConstantUnion() && falseBlock->getAsConstantUnion()) {
        if (cond->getAsConstantUnion()->getBConst(0))
            return trueBlock;
        else
            return falseBlock;
    }

    //
    // Make a selection node.
    //
    TIntermSelection* node = new TIntermSelection(cond, trueBlock, falseBlock, trueBlock->getType());
    node->getTypePointer()->setQualifier(EvqTemporary);
    node->setLine(line);

    return node;
}

//
// Constant terminal nodes.  Has a union that contains bool, float or int constants
//
// Returns the constant union node created.
//

TIntermConstantUnion* TIntermediate::addConstantUnion(ConstantUnion* unionArrayPointer, const TType& t, const TSourceLoc& line)
{
    TIntermConstantUnion* node = new TIntermConstantUnion(unionArrayPointer, t);
    node->setLine(line);

    return node;
}

TIntermTyped* TIntermediate::addSwizzle(TVectorFields& fields, const TSourceLoc& line)
{

    TIntermAggregate* node = new TIntermAggregate(EOpSequence);

    node->setLine(line);
    TIntermConstantUnion* constIntNode;
    TIntermSequence &sequenceVector = node->getSequence();
    ConstantUnion* unionArray;

    for (int i = 0; i < fields.num; i++) {
        unionArray = new ConstantUnion[1];
        unionArray->setIConst(fields.offsets[i]);
        constIntNode = addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), line);
        sequenceVector.push_back(constIntNode);
    }

    return node;
}

//
// Create loop nodes.
//
TIntermNode* TIntermediate::addLoop(TLoopType type, TIntermNode* init, TIntermTyped* cond, TIntermTyped* expr, TIntermNode* body, const TSourceLoc& line)
{
    TIntermNode* node = new TIntermLoop(type, init, cond, expr, body);
    node->setLine(line);

    return node;
}

//
// Add branches.
//
TIntermBranch* TIntermediate::addBranch(TOperator branchOp, const TSourceLoc& line)
{
    return addBranch(branchOp, 0, line);
}

TIntermBranch* TIntermediate::addBranch(TOperator branchOp, TIntermTyped* expression, const TSourceLoc& line)
{
    TIntermBranch* node = new TIntermBranch(branchOp, expression);
    node->setLine(line);

    return node;
}

//
// This is to be executed once the final root is put on top by the parsing
// process.
//
bool TIntermediate::postProcess(TIntermNode* root)
{
    if (root == 0)
        return true;

    //
    // First, finish off the top level sequence, if any
    //
    TIntermAggregate* aggRoot = root->getAsAggregate();
    if (aggRoot && aggRoot->getOp() == EOpNull)
        aggRoot->setOp(EOpSequence);

    return true;
}

//
// This deletes the tree.
//
void TIntermediate::remove(TIntermNode* root)
{
    if (root)
        RemoveAllTreeNodes(root);
}

////////////////////////////////////////////////////////////////
//
// Member functions of the nodes used for building the tree.
//
////////////////////////////////////////////////////////////////

//
// Say whether or not an operation node changes the value of a variable.
//
// Returns true if state is modified.
//
bool TIntermOperator::modifiesState() const
{
    switch (op) {
        case EOpPostIncrement:
        case EOpPostDecrement:
        case EOpPreIncrement:
        case EOpPreDecrement:
        case EOpAssign:
        case EOpAddAssign:
        case EOpSubAssign:
        case EOpMulAssign:
        case EOpVectorTimesMatrixAssign:
        case EOpVectorTimesScalarAssign:
        case EOpMatrixTimesScalarAssign:
        case EOpMatrixTimesMatrixAssign:
        case EOpDivAssign:
            return true;
        default:
            return false;
    }
}

//
// returns true if the operator is for one of the constructors
//
bool TIntermOperator::isConstructor() const
{
    switch (op) {
        case EOpConstructVec2:
        case EOpConstructVec3:
        case EOpConstructVec4:
        case EOpConstructMat2:
        case EOpConstructMat3:
        case EOpConstructMat4:
        case EOpConstructFloat:
        case EOpConstructIVec2:
        case EOpConstructIVec3:
        case EOpConstructIVec4:
        case EOpConstructInt:
        case EOpConstructBVec2:
        case EOpConstructBVec3:
        case EOpConstructBVec4:
        case EOpConstructBool:
        case EOpConstructStruct:
            return true;
        default:
            return false;
    }
}
//
// Make sure the type of a unary operator is appropriate for its
// combination of operation and operand type.
//
// Returns false in nothing makes sense.
//
bool TIntermUnary::promote(TInfoSink&)
{
    switch (op) {
        case EOpLogicalNot:
            if (operand->getBasicType() != EbtBool)
                return false;
            break;
        case EOpNegative:
        case EOpPostIncrement:
        case EOpPostDecrement:
        case EOpPreIncrement:
        case EOpPreDecrement:
            if (operand->getBasicType() == EbtBool)
                return false;
            break;

            // operators for built-ins are already type checked against their prototype
        case EOpAny:
        case EOpAll:
        case EOpVectorLogicalNot:
            return true;

        default:
            if (operand->getBasicType() != EbtFloat)
                return false;
    }

    setType(operand->getType());
    type.setQualifier(EvqTemporary);

    return true;
}

//
// Establishes the type of the resultant operation, as well as
// makes the operator the correct one for the operands.
//
// Returns false if operator can't work on operands.
//
bool TIntermBinary::promote(TInfoSink& infoSink)
{
    // This function only handles scalars, vectors, and matrices.
    if (left->isArray() || right->isArray()) {
        infoSink.info.message(EPrefixInternalError, getLine(), "Invalid operation for arrays");
        return false;
    }

    // GLSL ES 2.0 does not support implicit type casting.
    // So the basic type should always match.
    if (left->getBasicType() != right->getBasicType())
        return false;

    //
    // Base assumption:  just make the type the same as the left
    // operand.  Then only deviations from this need be coded.
    //
    setType(left->getType());

    // The result gets promoted to the highest precision.
    TPrecision higherPrecision = GetHigherPrecision(left->getPrecision(), right->getPrecision());
    getTypePointer()->setPrecision(higherPrecision);

    // Binary operations results in temporary variables unless both
    // operands are const.
    if (left->getQualifier() != EvqConst || right->getQualifier() != EvqConst) {
        getTypePointer()->setQualifier(EvqTemporary);
    }

    int size = std::max(left->getNominalSize(), right->getNominalSize());

    //
    // All scalars. Code after this test assumes this case is removed!
    //
    if (size == 1) {
        switch (op) {
            //
            // Promote to conditional
            //
            case EOpEqual:
            case EOpNotEqual:
            case EOpLessThan:
            case EOpGreaterThan:
            case EOpLessThanEqual:
            case EOpGreaterThanEqual:
                setType(TType(EbtBool, EbpUndefined));
                break;

            //
            // And and Or operate on conditionals
            //
            case EOpLogicalAnd:
            case EOpLogicalOr:
                // Both operands must be of type bool.
                if (left->getBasicType() != EbtBool || right->getBasicType() != EbtBool)
                    return false;
                setType(TType(EbtBool, EbpUndefined));
                break;

            default:
                break;
        }
        return true;
    }

    // If we reach here, at least one of the operands is vector or matrix.
    // The other operand could be a scalar, vector, or matrix.
    // Are the sizes compatible?
    //
    if (left->getNominalSize() != right->getNominalSize()) {
        // If the nominal size of operands do not match:
        // One of them must be scalar.
        if (left->getNominalSize() != 1 && right->getNominalSize() != 1)
            return false;
        // Operator cannot be of type pure assignment.
        if (op == EOpAssign || op == EOpInitialize)
            return false;
    }

    //
    // Can these two operands be combined?
    //
    TBasicType basicType = left->getBasicType();
    switch (op) {
        case EOpMul:
            if (!left->isMatrix() && right->isMatrix()) {
                if (left->isVector())
                    op = EOpVectorTimesMatrix;
                else {
                    op = EOpMatrixTimesScalar;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, true));
                }
            } else if (left->isMatrix() && !right->isMatrix()) {
                if (right->isVector()) {
                    op = EOpMatrixTimesVector;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, false));
                } else {
                    op = EOpMatrixTimesScalar;
                }
            } else if (left->isMatrix() && right->isMatrix()) {
                op = EOpMatrixTimesMatrix;
            } else if (!left->isMatrix() && !right->isMatrix()) {
                if (left->isVector() && right->isVector()) {
                    // leave as component product
                } else if (left->isVector() || right->isVector()) {
                    op = EOpVectorTimesScalar;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, false));
                }
            } else {
                infoSink.info.message(EPrefixInternalError, getLine(), "Missing elses");
                return false;
            }
            break;
        case EOpMulAssign:
            if (!left->isMatrix() && right->isMatrix()) {
                if (left->isVector())
                    op = EOpVectorTimesMatrixAssign;
                else {
                    return false;
                }
            } else if (left->isMatrix() && !right->isMatrix()) {
                if (right->isVector()) {
                    return false;
                } else {
                    op = EOpMatrixTimesScalarAssign;
                }
            } else if (left->isMatrix() && right->isMatrix()) {
                op = EOpMatrixTimesMatrixAssign;
            } else if (!left->isMatrix() && !right->isMatrix()) {
                if (left->isVector() && right->isVector()) {
                    // leave as component product
                } else if (left->isVector() || right->isVector()) {
                    if (! left->isVector())
                        return false;
                    op = EOpVectorTimesScalarAssign;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, false));
                }
            } else {
                infoSink.info.message(EPrefixInternalError, getLine(), "Missing elses");
                return false;
            }
            break;

        case EOpAssign:
        case EOpInitialize:
        case EOpAdd:
        case EOpSub:
        case EOpDiv:
        case EOpAddAssign:
        case EOpSubAssign:
        case EOpDivAssign:
            if ((left->isMatrix() && right->isVector()) ||
                (left->isVector() && right->isMatrix()))
                return false;
            setType(TType(basicType, higherPrecision, EvqTemporary, size, left->isMatrix() || right->isMatrix()));
            break;

        case EOpEqual:
        case EOpNotEqual:
        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
            if ((left->isMatrix() && right->isVector()) ||
                (left->isVector() && right->isMatrix()))
                return false;
            setType(TType(EbtBool, EbpUndefined));
            break;

        default:
            return false;
    }
    
    return true;
}

bool CompareStruct(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray)
{
    const TFieldList& fields = leftNodeType.getStruct()->fields();

    size_t structSize = fields.size();
    size_t index = 0;

    for (size_t j = 0; j < structSize; j++) {
        size_t size = fields[j]->type()->getObjectSize();
        for (size_t i = 0; i < size; i++) {
            if (fields[j]->type()->getBasicType() == EbtStruct) {
                if (!CompareStructure(*(fields[j]->type()), &rightUnionArray[index], &leftUnionArray[index]))
                    return false;
            } else {
                if (leftUnionArray[index] != rightUnionArray[index])
                    return false;
                index++;
            }
        }
    }
    return true;
}

bool CompareStructure(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray)
{
    if (leftNodeType.isArray()) {
        TType typeWithoutArrayness = leftNodeType;
        typeWithoutArrayness.clearArrayness();

        size_t arraySize = leftNodeType.getArraySize();

        for (size_t i = 0; i < arraySize; ++i) {
            size_t offset = typeWithoutArrayness.getObjectSize() * i;
            if (!CompareStruct(typeWithoutArrayness, &rightUnionArray[offset], &leftUnionArray[offset]))
                return false;
        }
    } else
        return CompareStruct(leftNodeType, rightUnionArray, leftUnionArray);

    return true;
}

//
// The fold functions see if an operation on a constant can be done in place,
// without generating run-time code.
//
// Returns the node to keep using, which may or may not be the node passed in.
//

TIntermTyped* TIntermConstantUnion::fold(TOperator op, TIntermTyped* constantNode, TInfoSink& infoSink)
{
    ConstantUnion *unionArray = getUnionArrayPointer();
    size_t objectSize = getType().getObjectSize();

    if (constantNode) {  // binary operations
        TIntermConstantUnion *node = constantNode->getAsConstantUnion();
        ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
        TType returnType = getType();

        // for a case like float f = 1.2 + vec4(2,3,4,5);
        if (constantNode->getType().getObjectSize() == 1 && objectSize > 1) {
            rightUnionArray = new ConstantUnion[objectSize];
            for (size_t i = 0; i < objectSize; ++i)
                rightUnionArray[i] = *node->getUnionArrayPointer();
            returnType = getType();
        } else if (constantNode->getType().getObjectSize() > 1 && objectSize == 1) {
            // for a case like float f = vec4(2,3,4,5) + 1.2;
            unionArray = new ConstantUnion[constantNode->getType().getObjectSize()];
            for (size_t i = 0; i < constantNode->getType().getObjectSize(); ++i)
                unionArray[i] = *getUnionArrayPointer();
            returnType = node->getType();
            objectSize = constantNode->getType().getObjectSize();
        }

        ConstantUnion* tempConstArray = 0;
        TIntermConstantUnion *tempNode;

        bool boolNodeFlag = false;
        switch(op) {
            case EOpAdd:
                tempConstArray = new ConstantUnion[objectSize];
                {// support MSVC++6.0
                    for (size_t i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] + rightUnionArray[i];
                }
                break;
            case EOpSub:
                tempConstArray = new ConstantUnion[objectSize];
                {// support MSVC++6.0
                    for (size_t i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] - rightUnionArray[i];
                }
                break;

            case EOpMul:
            case EOpVectorTimesScalar:
            case EOpMatrixTimesScalar:
                tempConstArray = new ConstantUnion[objectSize];
                {// support MSVC++6.0
                    for (size_t i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] * rightUnionArray[i];
                }
                break;
            case EOpMatrixTimesMatrix:
                if (getType().getBasicType() != EbtFloat || node->getBasicType() != EbtFloat) {
                    infoSink.info.message(EPrefixInternalError, getLine(), "Constant Folding cannot be done for matrix multiply");
                    return 0;
                }
                {// support MSVC++6.0
                    int size = getNominalSize();
                    tempConstArray = new ConstantUnion[size*size];
                    for (int row = 0; row < size; row++) {
                        for (int column = 0; column < size; column++) {
                            tempConstArray[size * column + row].setFConst(0.0f);
                            for (int i = 0; i < size; i++) {
                                tempConstArray[size * column + row].setFConst(tempConstArray[size * column + row].getFConst() + unionArray[i * size + row].getFConst() * (rightUnionArray[column * size + i].getFConst()));
                            }
                        }
                    }
                }
                break;
            case EOpDiv:
                tempConstArray = new ConstantUnion[objectSize];
                {// support MSVC++6.0
                    for (size_t i = 0; i < objectSize; i++) {
                        switch (getType().getBasicType()) {
            case EbtFloat:
                if (rightUnionArray[i] == 0.0f) {
                    infoSink.info.message(EPrefixWarning, getLine(), "Divide by zero error during constant folding");
                    tempConstArray[i].setFConst(unionArray[i].getFConst() < 0 ? -FLT_MAX : FLT_MAX);
                } else
                    tempConstArray[i].setFConst(unionArray[i].getFConst() / rightUnionArray[i].getFConst());
                break;

            case EbtInt:
                if (rightUnionArray[i] == 0) {
                    infoSink.info.message(EPrefixWarning, getLine(), "Divide by zero error during constant folding");
                    tempConstArray[i].setIConst(INT_MAX);
                } else
                    tempConstArray[i].setIConst(unionArray[i].getIConst() / rightUnionArray[i].getIConst());
                break;
            default:
                infoSink.info.message(EPrefixInternalError, getLine(), "Constant folding cannot be done for \"/\"");
                return 0;
                        }
                    }
                }
                break;

            case EOpMatrixTimesVector:
                if (node->getBasicType() != EbtFloat) {
                    infoSink.info.message(EPrefixInternalError, getLine(), "Constant Folding cannot be done for matrix times vector");
                    return 0;
                }
                tempConstArray = new ConstantUnion[getNominalSize()];

                {// support MSVC++6.0
                    for (int size = getNominalSize(), i = 0; i < size; i++) {
                        tempConstArray[i].setFConst(0.0f);
                        for (int j = 0; j < size; j++) {
                            tempConstArray[i].setFConst(tempConstArray[i].getFConst() + ((unionArray[j*size + i].getFConst()) * rightUnionArray[j].getFConst()));
                        }
                    }
                }

                tempNode = new TIntermConstantUnion(tempConstArray, node->getType());
                tempNode->setLine(getLine());

                return tempNode;

            case EOpVectorTimesMatrix:
                if (getType().getBasicType() != EbtFloat) {
                    infoSink.info.message(EPrefixInternalError, getLine(), "Constant Folding cannot be done for vector times matrix");
                    return 0;
                }

                tempConstArray = new ConstantUnion[getNominalSize()];
                {// support MSVC++6.0
                    for (int size = getNominalSize(), i = 0; i < size; i++) {
                        tempConstArray[i].setFConst(0.0f);
                        for (int j = 0; j < size; j++) {
                            tempConstArray[i].setFConst(tempConstArray[i].getFConst() + ((unionArray[j].getFConst()) * rightUnionArray[i*size + j].getFConst()));
                        }
                    }
                }
                break;

            case EOpLogicalAnd: // this code is written for possible future use, will not get executed currently
                tempConstArray = new ConstantUnion[objectSize];
                {// support MSVC++6.0
                    for (size_t i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] && rightUnionArray[i];
                }
                break;

            case EOpLogicalOr: // this code is written for possible future use, will not get executed currently
                tempConstArray = new ConstantUnion[objectSize];
                {// support MSVC++6.0
                    for (size_t i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] || rightUnionArray[i];
                }
                break;

            case EOpLogicalXor:
                tempConstArray = new ConstantUnion[objectSize];
                {// support MSVC++6.0
                    for (size_t i = 0; i < objectSize; i++)
                        switch (getType().getBasicType()) {
            case EbtBool: tempConstArray[i].setBConst((unionArray[i] == rightUnionArray[i]) ? false : true); break;
            default: assert(false && "Default missing");
                    }
                }
                break;

            case EOpLessThan:
                assert(objectSize == 1);
                tempConstArray = new ConstantUnion[1];
                tempConstArray->setBConst(*unionArray < *rightUnionArray);
                returnType = TType(EbtBool, EbpUndefined, EvqConst);
                break;
            case EOpGreaterThan:
                assert(objectSize == 1);
                tempConstArray = new ConstantUnion[1];
                tempConstArray->setBConst(*unionArray > *rightUnionArray);
                returnType = TType(EbtBool, EbpUndefined, EvqConst);
                break;
            case EOpLessThanEqual:
                {
                    assert(objectSize == 1);
                    ConstantUnion constant;
                    constant.setBConst(*unionArray > *rightUnionArray);
                    tempConstArray = new ConstantUnion[1];
                    tempConstArray->setBConst(!constant.getBConst());
                    returnType = TType(EbtBool, EbpUndefined, EvqConst);
                    break;
                }
            case EOpGreaterThanEqual:
                {
                    assert(objectSize == 1);
                    ConstantUnion constant;
                    constant.setBConst(*unionArray < *rightUnionArray);
                    tempConstArray = new ConstantUnion[1];
                    tempConstArray->setBConst(!constant.getBConst());
                    returnType = TType(EbtBool, EbpUndefined, EvqConst);
                    break;
                }

            case EOpEqual:
                if (getType().getBasicType() == EbtStruct) {
                    if (!CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray))
                        boolNodeFlag = true;
                } else {
                    for (size_t i = 0; i < objectSize; i++) {
                        if (unionArray[i] != rightUnionArray[i]) {
                            boolNodeFlag = true;
                            break;  // break out of for loop
                        }
                    }
                }

                tempConstArray = new ConstantUnion[1];
                if (!boolNodeFlag) {
                    tempConstArray->setBConst(true);
                }
                else {
                    tempConstArray->setBConst(false);
                }

                tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
                tempNode->setLine(getLine());

                return tempNode;

            case EOpNotEqual:
                if (getType().getBasicType() == EbtStruct) {
                    if (CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray))
                        boolNodeFlag = true;
                } else {
                    for (size_t i = 0; i < objectSize; i++) {
                        if (unionArray[i] == rightUnionArray[i]) {
                            boolNodeFlag = true;
                            break;  // break out of for loop
                        }
                    }
                }

                tempConstArray = new ConstantUnion[1];
                if (!boolNodeFlag) {
                    tempConstArray->setBConst(true);
                }
                else {
                    tempConstArray->setBConst(false);
                }

                tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
                tempNode->setLine(getLine());

                return tempNode;

            default:
                infoSink.info.message(EPrefixInternalError, getLine(), "Invalid operator for constant folding");
                return 0;
        }
        tempNode = new TIntermConstantUnion(tempConstArray, returnType);
        tempNode->setLine(getLine());

        return tempNode;
    } else {
        //
        // Do unary operations
        //
        TIntermConstantUnion *newNode = 0;
        ConstantUnion* tempConstArray = new ConstantUnion[objectSize];
        for (size_t i = 0; i < objectSize; i++) {
            switch(op) {
                case EOpNegative:
                    switch (getType().getBasicType()) {
                        case EbtFloat: tempConstArray[i].setFConst(-unionArray[i].getFConst()); break;
                        case EbtInt:   tempConstArray[i].setIConst(-unionArray[i].getIConst()); break;
                        default:
                            infoSink.info.message(EPrefixInternalError, getLine(), "Unary operation not folded into constant");
                            return 0;
                    }
                    break;
                case EOpLogicalNot: // this code is written for possible future use, will not get executed currently
                    switch (getType().getBasicType()) {
                        case EbtBool:  tempConstArray[i].setBConst(!unionArray[i].getBConst()); break;
                        default:
                            infoSink.info.message(EPrefixInternalError, getLine(), "Unary operation not folded into constant");
                            return 0;
                    }
                    break;
                default:
                    return 0;
            }
        }
        newNode = new TIntermConstantUnion(tempConstArray, getType());
        newNode->setLine(getLine());
        return newNode;
    }
}

TIntermTyped* TIntermediate::promoteConstantUnion(TBasicType promoteTo, TIntermConstantUnion* node)
{
    size_t size = node->getType().getObjectSize();

    ConstantUnion *leftUnionArray = new ConstantUnion[size];

    for (size_t i = 0; i < size; i++) {

        switch (promoteTo) {
            case EbtFloat:
                switch (node->getType().getBasicType()) {
                    case EbtInt:
                        leftUnionArray[i].setFConst(static_cast<float>(node->getIConst(i)));
                        break;
                    case EbtBool:
                        leftUnionArray[i].setFConst(static_cast<float>(node->getBConst(i)));
                        break;
                    case EbtFloat:
                        leftUnionArray[i].setFConst(static_cast<float>(node->getFConst(i)));
                        break;
                    default:
                        infoSink.info.message(EPrefixInternalError, node->getLine(), "Cannot promote");
                        return 0;
                }
                break;
            case EbtInt:
                switch (node->getType().getBasicType()) {
                    case EbtInt:
                        leftUnionArray[i].setIConst(static_cast<int>(node->getIConst(i)));
                        break;
                    case EbtBool:
                        leftUnionArray[i].setIConst(static_cast<int>(node->getBConst(i)));
                        break;
                    case EbtFloat:
                        leftUnionArray[i].setIConst(static_cast<int>(node->getFConst(i)));
                        break;
                    default:
                        infoSink.info.message(EPrefixInternalError, node->getLine(), "Cannot promote");
                        return 0;
                }
                break;
            case EbtBool:
                switch (node->getType().getBasicType()) {
                    case EbtInt:
                        leftUnionArray[i].setBConst(node->getIConst(i) != 0);
                        break;
                    case EbtBool:
                        leftUnionArray[i].setBConst(node->getBConst(i));
                        break;
                    case EbtFloat:
                        leftUnionArray[i].setBConst(node->getFConst(i) != 0.0f);
                        break;
                    default:
                        infoSink.info.message(EPrefixInternalError, node->getLine(), "Cannot promote");
                        return 0;
                }

                break;
            default:
                infoSink.info.message(EPrefixInternalError, node->getLine(), "Incorrect data type found");
                return 0;
        }

    }

    const TType& t = node->getType();

    return addConstantUnion(leftUnionArray, TType(promoteTo, t.getPrecision(), t.getQualifier(), t.getNominalSize(), t.isMatrix(), t.isArray()), node->getLine());
}

// static
TString TIntermTraverser::hash(const TString& name, ShHashFunction64 hashFunction)
{
    if (hashFunction == NULL || name.empty())
        return name;
    khronos_uint64_t number = (*hashFunction)(name.c_str(), name.length());
    TStringStream stream;
    stream << HASHED_NAME_PREFIX << std::hex << number;
    TString hashedName = stream.str();
    return hashedName;
}
