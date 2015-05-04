//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Build the intermediate representation.
//

#include <float.h>
#include <limits.h>
#include <algorithm>

#include "compiler/translator/HashNames.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/SymbolTable.h"

namespace
{

TPrecision GetHigherPrecision(TPrecision left, TPrecision right)
{
    return left > right ? left : right;
}

bool ValidateMultiplication(TOperator op, const TType &left, const TType &right)
{
    switch (op)
    {
      case EOpMul:
      case EOpMulAssign:
        return left.getNominalSize() == right.getNominalSize() &&
               left.getSecondarySize() == right.getSecondarySize();
      case EOpVectorTimesScalar:
      case EOpVectorTimesScalarAssign:
        return true;
      case EOpVectorTimesMatrix:
        return left.getNominalSize() == right.getRows();
      case EOpVectorTimesMatrixAssign:
        return left.getNominalSize() == right.getRows() &&
               left.getNominalSize() == right.getCols();
      case EOpMatrixTimesVector:
        return left.getCols() == right.getNominalSize();
      case EOpMatrixTimesScalar:
      case EOpMatrixTimesScalarAssign:
        return true;
      case EOpMatrixTimesMatrix:
        return left.getCols() == right.getRows();
      case EOpMatrixTimesMatrixAssign:
        return left.getCols() == right.getCols() &&
               left.getRows() == right.getRows();

      default:
        UNREACHABLE();
        return false;
    }
}

bool CompareStructure(const TType& leftNodeType,
                      ConstantUnion *rightUnionArray,
                      ConstantUnion *leftUnionArray);

bool CompareStruct(const TType &leftNodeType,
                   ConstantUnion *rightUnionArray,
                   ConstantUnion *leftUnionArray)
{
    const TFieldList &fields = leftNodeType.getStruct()->fields();

    size_t structSize = fields.size();
    size_t index = 0;

    for (size_t j = 0; j < structSize; j++)
    {
        size_t size = fields[j]->type()->getObjectSize();
        for (size_t i = 0; i < size; i++)
        {
            if (fields[j]->type()->getBasicType() == EbtStruct)
            {
                if (!CompareStructure(*fields[j]->type(),
                                      &rightUnionArray[index],
                                      &leftUnionArray[index]))
                {
                    return false;
                }
            }
            else
            {
                if (leftUnionArray[index] != rightUnionArray[index])
                    return false;
                index++;
            }
        }
    }
    return true;
}

bool CompareStructure(const TType &leftNodeType,
                      ConstantUnion *rightUnionArray,
                      ConstantUnion *leftUnionArray)
{
    if (leftNodeType.isArray())
    {
        TType typeWithoutArrayness = leftNodeType;
        typeWithoutArrayness.clearArrayness();

        size_t arraySize = leftNodeType.getArraySize();

        for (size_t i = 0; i < arraySize; ++i)
        {
            size_t offset = typeWithoutArrayness.getObjectSize() * i;
            if (!CompareStruct(typeWithoutArrayness,
                               &rightUnionArray[offset],
                               &leftUnionArray[offset]))
            {
                return false;
            }
        }
    }
    else
    {
        return CompareStruct(leftNodeType, rightUnionArray, leftUnionArray);
    }
    return true;
}

}  // namespace anonymous


////////////////////////////////////////////////////////////////
//
// Member functions of the nodes used for building the tree.
//
////////////////////////////////////////////////////////////////

void TIntermTyped::setTypePreservePrecision(const TType &t)
{
    TPrecision precision = getPrecision();
    mType = t;
    ASSERT(mType.getBasicType() != EbtBool || precision == EbpUndefined);
    mType.setPrecision(precision);
}

#define REPLACE_IF_IS(node, type, original, replacement) \
    if (node == original) { \
        node = static_cast<type *>(replacement); \
        return true; \
    }

bool TIntermLoop::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(mInit, TIntermNode, original, replacement);
    REPLACE_IF_IS(mCond, TIntermTyped, original, replacement);
    REPLACE_IF_IS(mExpr, TIntermTyped, original, replacement);
    REPLACE_IF_IS(mBody, TIntermNode, original, replacement);
    return false;
}

void TIntermLoop::enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const
{
    if (mInit)
    {
        nodeQueue->push(mInit);
    }
    if (mCond)
    {
        nodeQueue->push(mCond);
    }
    if (mExpr)
    {
        nodeQueue->push(mExpr);
    }
    if (mBody)
    {
        nodeQueue->push(mBody);
    }
}

bool TIntermBranch::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(mExpression, TIntermTyped, original, replacement);
    return false;
}

void TIntermBranch::enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const
{
    if (mExpression)
    {
        nodeQueue->push(mExpression);
    }
}

bool TIntermBinary::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(mLeft, TIntermTyped, original, replacement);
    REPLACE_IF_IS(mRight, TIntermTyped, original, replacement);
    return false;
}

void TIntermBinary::enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const
{
    if (mLeft)
    {
        nodeQueue->push(mLeft);
    }
    if (mRight)
    {
        nodeQueue->push(mRight);
    }
}

bool TIntermUnary::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(mOperand, TIntermTyped, original, replacement);
    return false;
}

void TIntermUnary::enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const
{
    if (mOperand)
    {
        nodeQueue->push(mOperand);
    }
}

bool TIntermAggregate::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    for (size_t ii = 0; ii < mSequence.size(); ++ii)
    {
        REPLACE_IF_IS(mSequence[ii], TIntermNode, original, replacement);
    }
    return false;
}

void TIntermAggregate::enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const
{
    for (size_t childIndex = 0; childIndex < mSequence.size(); childIndex++)
    {
        nodeQueue->push(mSequence[childIndex]);
    }
}

void TIntermAggregate::setPrecisionFromChildren()
{
    if (getBasicType() == EbtBool)
    {
        mType.setPrecision(EbpUndefined);
        return;
    }

    TPrecision precision = EbpUndefined;
    TIntermSequence::iterator childIter = mSequence.begin();
    while (childIter != mSequence.end())
    {
        TIntermTyped *typed = (*childIter)->getAsTyped();
        if (typed)
            precision = GetHigherPrecision(typed->getPrecision(), precision);
        ++childIter;
    }
    mType.setPrecision(precision);
}

void TIntermAggregate::setBuiltInFunctionPrecision()
{
    // All built-ins returning bool should be handled as ops, not functions.
    ASSERT(getBasicType() != EbtBool);

    TPrecision precision = EbpUndefined;
    TIntermSequence::iterator childIter = mSequence.begin();
    while (childIter != mSequence.end())
    {
        TIntermTyped *typed = (*childIter)->getAsTyped();
        // ESSL spec section 8: texture functions get their precision from the sampler.
        if (typed && IsSampler(typed->getBasicType()))
        {
            precision = typed->getPrecision();
            break;
        }
        ++childIter;
    }
    // ESSL 3.0 spec section 8: textureSize always gets highp precision.
    // All other functions that take a sampler are assumed to be texture functions.
    if (mName.find("textureSize") == 0)
        mType.setPrecision(EbpHigh);
    else
        mType.setPrecision(precision);
}

bool TIntermSelection::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(mCondition, TIntermTyped, original, replacement);
    REPLACE_IF_IS(mTrueBlock, TIntermNode, original, replacement);
    REPLACE_IF_IS(mFalseBlock, TIntermNode, original, replacement);
    return false;
}

void TIntermSelection::enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const
{
    if (mCondition)
    {
        nodeQueue->push(mCondition);
    }
    if (mTrueBlock)
    {
        nodeQueue->push(mTrueBlock);
    }
    if (mFalseBlock)
    {
        nodeQueue->push(mFalseBlock);
    }
}

//
// Say whether or not an operation node changes the value of a variable.
//
bool TIntermOperator::isAssignment() const
{
    switch (mOp)
    {
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
    switch (mOp)
    {
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
      case EOpConstructUVec2:
      case EOpConstructUVec3:
      case EOpConstructUVec4:
      case EOpConstructUInt:
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
bool TIntermUnary::promote(TInfoSink &)
{
    switch (mOp)
    {
      case EOpLogicalNot:
        if (mOperand->getBasicType() != EbtBool)
            return false;
        break;
      case EOpNegative:
      case EOpPositive:
      case EOpPostIncrement:
      case EOpPostDecrement:
      case EOpPreIncrement:
      case EOpPreDecrement:
        if (mOperand->getBasicType() == EbtBool)
            return false;
        break;

      // operators for built-ins are already type checked against their prototype
      case EOpAny:
      case EOpAll:
      case EOpVectorLogicalNot:
        return true;

      default:
        if (mOperand->getBasicType() != EbtFloat)
            return false;
    }

    setType(mOperand->getType());
    mType.setQualifier(EvqTemporary);

    return true;
}

//
// Establishes the type of the resultant operation, as well as
// makes the operator the correct one for the operands.
//
// Returns false if operator can't work on operands.
//
bool TIntermBinary::promote(TInfoSink &infoSink)
{
    // This function only handles scalars, vectors, and matrices.
    if (mLeft->isArray() || mRight->isArray())
    {
        infoSink.info.message(EPrefixInternalError, getLine(),
                              "Invalid operation for arrays");
        return false;
    }

    // GLSL ES 2.0 does not support implicit type casting.
    // So the basic type should always match.
    if (mLeft->getBasicType() != mRight->getBasicType())
    {
        return false;
    }

    //
    // Base assumption:  just make the type the same as the left
    // operand.  Then only deviations from this need be coded.
    //
    setType(mLeft->getType());

    // The result gets promoted to the highest precision.
    TPrecision higherPrecision = GetHigherPrecision(
        mLeft->getPrecision(), mRight->getPrecision());
    getTypePointer()->setPrecision(higherPrecision);

    // Binary operations results in temporary variables unless both
    // operands are const.
    if (mLeft->getQualifier() != EvqConst || mRight->getQualifier() != EvqConst)
    {
        getTypePointer()->setQualifier(EvqTemporary);
    }

    const int nominalSize =
        std::max(mLeft->getNominalSize(), mRight->getNominalSize());

    //
    // All scalars or structs. Code after this test assumes this case is removed!
    //
    if (nominalSize == 1)
    {
        switch (mOp)
        {
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
            if (mLeft->getBasicType() != EbtBool || mRight->getBasicType() != EbtBool)
            {
                return false;
            }
            setType(TType(EbtBool, EbpUndefined));
            break;

          default:
            break;
        }
        return true;
    }

    // If we reach here, at least one of the operands is vector or matrix.
    // The other operand could be a scalar, vector, or matrix.
    // Can these two operands be combined?
    //
    TBasicType basicType = mLeft->getBasicType();
    switch (mOp)
    {
      case EOpMul:
        if (!mLeft->isMatrix() && mRight->isMatrix())
        {
            if (mLeft->isVector())
            {
                mOp = EOpVectorTimesMatrix;
                setType(TType(basicType, higherPrecision, EvqTemporary,
                              mRight->getCols(), 1));
            }
            else
            {
                mOp = EOpMatrixTimesScalar;
                setType(TType(basicType, higherPrecision, EvqTemporary,
                              mRight->getCols(), mRight->getRows()));
            }
        }
        else if (mLeft->isMatrix() && !mRight->isMatrix())
        {
            if (mRight->isVector())
            {
                mOp = EOpMatrixTimesVector;
                setType(TType(basicType, higherPrecision, EvqTemporary,
                              mLeft->getRows(), 1));
            }
            else
            {
                mOp = EOpMatrixTimesScalar;
            }
        }
        else if (mLeft->isMatrix() && mRight->isMatrix())
        {
            mOp = EOpMatrixTimesMatrix;
            setType(TType(basicType, higherPrecision, EvqTemporary,
                          mRight->getCols(), mLeft->getRows()));
        }
        else if (!mLeft->isMatrix() && !mRight->isMatrix())
        {
            if (mLeft->isVector() && mRight->isVector())
            {
                // leave as component product
            }
            else if (mLeft->isVector() || mRight->isVector())
            {
                mOp = EOpVectorTimesScalar;
                setType(TType(basicType, higherPrecision, EvqTemporary,
                              nominalSize, 1));
            }
        }
        else
        {
            infoSink.info.message(EPrefixInternalError, getLine(),
                                  "Missing elses");
            return false;
        }

        if (!ValidateMultiplication(mOp, mLeft->getType(), mRight->getType()))
        {
            return false;
        }
        break;

      case EOpMulAssign:
        if (!mLeft->isMatrix() && mRight->isMatrix())
        {
            if (mLeft->isVector())
            {
                mOp = EOpVectorTimesMatrixAssign;
            }
            else
            {
                return false;
            }
        }
        else if (mLeft->isMatrix() && !mRight->isMatrix())
        {
            if (mRight->isVector())
            {
                return false;
            }
            else
            {
                mOp = EOpMatrixTimesScalarAssign;
            }
        }
        else if (mLeft->isMatrix() && mRight->isMatrix())
        {
            mOp = EOpMatrixTimesMatrixAssign;
            setType(TType(basicType, higherPrecision, EvqTemporary,
                          mRight->getCols(), mLeft->getRows()));
        }
        else if (!mLeft->isMatrix() && !mRight->isMatrix())
        {
            if (mLeft->isVector() && mRight->isVector())
            {
                // leave as component product
            }
            else if (mLeft->isVector() || mRight->isVector())
            {
                if (!mLeft->isVector())
                    return false;
                mOp = EOpVectorTimesScalarAssign;
                setType(TType(basicType, higherPrecision, EvqTemporary,
                              mLeft->getNominalSize(), 1));
            }
        }
        else
        {
            infoSink.info.message(EPrefixInternalError, getLine(),
                                  "Missing elses");
            return false;
        }

        if (!ValidateMultiplication(mOp, mLeft->getType(), mRight->getType()))
        {
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
        if ((mLeft->isMatrix() && mRight->isVector()) ||
            (mLeft->isVector() && mRight->isMatrix()))
        {
            return false;
        }

        // Are the sizes compatible?
        if (mLeft->getNominalSize() != mRight->getNominalSize() ||
            mLeft->getSecondarySize() != mRight->getSecondarySize())
        {
            // If the nominal size of operands do not match:
            // One of them must be scalar.
            if (!mLeft->isScalar() && !mRight->isScalar())
                return false;

            // Operator cannot be of type pure assignment.
            if (mOp == EOpAssign || mOp == EOpInitialize)
                return false;
        }

        {
            const int secondarySize = std::max(
                mLeft->getSecondarySize(), mRight->getSecondarySize());
            setType(TType(basicType, higherPrecision, EvqTemporary,
                          nominalSize, secondarySize));
        }
        break;

      case EOpEqual:
      case EOpNotEqual:
      case EOpLessThan:
      case EOpGreaterThan:
      case EOpLessThanEqual:
      case EOpGreaterThanEqual:
        if ((mLeft->getNominalSize() != mRight->getNominalSize()) ||
            (mLeft->getSecondarySize() != mRight->getSecondarySize()))
        {
            return false;
        }
        setType(TType(EbtBool, EbpUndefined));
        break;

      default:
        return false;
    }
    return true;
}

//
// The fold functions see if an operation on a constant can be done in place,
// without generating run-time code.
//
// Returns the node to keep using, which may or may not be the node passed in.
//
TIntermTyped *TIntermConstantUnion::fold(
    TOperator op, TIntermTyped *constantNode, TInfoSink &infoSink)
{
    ConstantUnion *unionArray = getUnionArrayPointer();

    if (!unionArray)
        return NULL;

    size_t objectSize = getType().getObjectSize();

    if (constantNode)
    {
        // binary operations
        TIntermConstantUnion *node = constantNode->getAsConstantUnion();
        ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
        TType returnType = getType();

        if (!rightUnionArray)
            return NULL;

        // for a case like float f = 1.2 + vec4(2,3,4,5);
        if (constantNode->getType().getObjectSize() == 1 && objectSize > 1)
        {
            rightUnionArray = new ConstantUnion[objectSize];
            for (size_t i = 0; i < objectSize; ++i)
            {
                rightUnionArray[i] = *node->getUnionArrayPointer();
            }
            returnType = getType();
        }
        else if (constantNode->getType().getObjectSize() > 1 && objectSize == 1)
        {
            // for a case like float f = vec4(2,3,4,5) + 1.2;
            unionArray = new ConstantUnion[constantNode->getType().getObjectSize()];
            for (size_t i = 0; i < constantNode->getType().getObjectSize(); ++i)
            {
                unionArray[i] = *getUnionArrayPointer();
            }
            returnType = node->getType();
            objectSize = constantNode->getType().getObjectSize();
        }

        ConstantUnion *tempConstArray = NULL;
        TIntermConstantUnion *tempNode;

        bool boolNodeFlag = false;
        switch(op)
        {
          case EOpAdd:
            tempConstArray = new ConstantUnion[objectSize];
            for (size_t i = 0; i < objectSize; i++)
                tempConstArray[i] = unionArray[i] + rightUnionArray[i];
            break;
          case EOpSub:
            tempConstArray = new ConstantUnion[objectSize];
            for (size_t i = 0; i < objectSize; i++)
                tempConstArray[i] = unionArray[i] - rightUnionArray[i];
            break;

          case EOpMul:
          case EOpVectorTimesScalar:
          case EOpMatrixTimesScalar:
            tempConstArray = new ConstantUnion[objectSize];
            for (size_t i = 0; i < objectSize; i++)
                tempConstArray[i] = unionArray[i] * rightUnionArray[i];
            break;

          case EOpMatrixTimesMatrix:
            {
                if (getType().getBasicType() != EbtFloat ||
                    node->getBasicType() != EbtFloat)
                {
                    infoSink.info.message(
                        EPrefixInternalError, getLine(),
                        "Constant Folding cannot be done for matrix multiply");
                    return NULL;
                }

                const int leftCols = getCols();
                const int leftRows = getRows();
                const int rightCols = constantNode->getType().getCols();
                const int rightRows = constantNode->getType().getRows();
                const int resultCols = rightCols;
                const int resultRows = leftRows;

                tempConstArray = new ConstantUnion[resultCols*resultRows];
                for (int row = 0; row < resultRows; row++)
                {
                    for (int column = 0; column < resultCols; column++)
                    {
                        tempConstArray[resultRows * column + row].setFConst(0.0f);
                        for (int i = 0; i < leftCols; i++)
                        {
                            tempConstArray[resultRows * column + row].setFConst(
                                tempConstArray[resultRows * column + row].getFConst() +
                                unionArray[i * leftRows + row].getFConst() *
                                rightUnionArray[column * rightRows + i].getFConst());
                        }
                    }
                }

                // update return type for matrix product
                returnType.setPrimarySize(resultCols);
                returnType.setSecondarySize(resultRows);
            }
            break;

          case EOpDiv:
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    switch (getType().getBasicType())
                    {
                      case EbtFloat:
                        if (rightUnionArray[i] == 0.0f)
                        {
                            infoSink.info.message(
                                EPrefixWarning, getLine(),
                                "Divide by zero error during constant folding");
                            tempConstArray[i].setFConst(
                                unionArray[i].getFConst() < 0 ? -FLT_MAX : FLT_MAX);
                        }
                        else
                        {
                            tempConstArray[i].setFConst(
                                unionArray[i].getFConst() /
                                rightUnionArray[i].getFConst());
                        }
                        break;

                      case EbtInt:
                        if (rightUnionArray[i] == 0)
                        {
                            infoSink.info.message(
                                EPrefixWarning, getLine(),
                                "Divide by zero error during constant folding");
                            tempConstArray[i].setIConst(INT_MAX);
                        }
                        else
                        {
                            tempConstArray[i].setIConst(
                                unionArray[i].getIConst() /
                                rightUnionArray[i].getIConst());
                        }
                        break;

                      case EbtUInt:
                        if (rightUnionArray[i] == 0)
                        {
                            infoSink.info.message(
                                EPrefixWarning, getLine(),
                                "Divide by zero error during constant folding");
                            tempConstArray[i].setUConst(UINT_MAX);
                        }
                        else
                        {
                            tempConstArray[i].setUConst(
                                unionArray[i].getUConst() /
                                rightUnionArray[i].getUConst());
                        }
                        break;

                      default:
                        infoSink.info.message(
                            EPrefixInternalError, getLine(),
                            "Constant folding cannot be done for \"/\"");
                        return NULL;
                    }
                }
            }
            break;

          case EOpMatrixTimesVector:
            {
                if (node->getBasicType() != EbtFloat)
                {
                    infoSink.info.message(
                        EPrefixInternalError, getLine(),
                        "Constant Folding cannot be done for matrix times vector");
                    return NULL;
                }

                const int matrixCols = getCols();
                const int matrixRows = getRows();

                tempConstArray = new ConstantUnion[matrixRows];

                for (int matrixRow = 0; matrixRow < matrixRows; matrixRow++)
                {
                    tempConstArray[matrixRow].setFConst(0.0f);
                    for (int col = 0; col < matrixCols; col++)
                    {
                        tempConstArray[matrixRow].setFConst(
                            tempConstArray[matrixRow].getFConst() +
                            unionArray[col * matrixRows + matrixRow].getFConst() *
                            rightUnionArray[col].getFConst());
                    }
                }

                returnType = node->getType();
                returnType.setPrimarySize(matrixRows);

                tempNode = new TIntermConstantUnion(tempConstArray, returnType);
                tempNode->setLine(getLine());

                return tempNode;
            }

          case EOpVectorTimesMatrix:
            {
                if (getType().getBasicType() != EbtFloat)
                {
                    infoSink.info.message(
                        EPrefixInternalError, getLine(),
                        "Constant Folding cannot be done for vector times matrix");
                    return NULL;
                }

                const int matrixCols = constantNode->getType().getCols();
                const int matrixRows = constantNode->getType().getRows();

                tempConstArray = new ConstantUnion[matrixCols];

                for (int matrixCol = 0; matrixCol < matrixCols; matrixCol++)
                {
                    tempConstArray[matrixCol].setFConst(0.0f);
                    for (int matrixRow = 0; matrixRow < matrixRows; matrixRow++)
                    {
                        tempConstArray[matrixCol].setFConst(
                            tempConstArray[matrixCol].getFConst() +
                            unionArray[matrixRow].getFConst() *
                            rightUnionArray[matrixCol * matrixRows + matrixRow].getFConst());
                    }
                }

                returnType.setPrimarySize(matrixCols);
            }
            break;

          case EOpLogicalAnd:
            // this code is written for possible future use,
            // will not get executed currently
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    tempConstArray[i] = unionArray[i] && rightUnionArray[i];
                }
            }
            break;

          case EOpLogicalOr:
            // this code is written for possible future use,
            // will not get executed currently
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    tempConstArray[i] = unionArray[i] || rightUnionArray[i];
                }
            }
            break;

          case EOpLogicalXor:
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    switch (getType().getBasicType())
                    {
                      case EbtBool:
                        tempConstArray[i].setBConst(
                            unionArray[i] == rightUnionArray[i] ? false : true);
                        break;
                      default:
                        UNREACHABLE();
                        break;
                    }
                }
            }
            break;

          case EOpLessThan:
            ASSERT(objectSize == 1);
            tempConstArray = new ConstantUnion[1];
            tempConstArray->setBConst(*unionArray < *rightUnionArray);
            returnType = TType(EbtBool, EbpUndefined, EvqConst);
            break;

          case EOpGreaterThan:
            ASSERT(objectSize == 1);
            tempConstArray = new ConstantUnion[1];
            tempConstArray->setBConst(*unionArray > *rightUnionArray);
            returnType = TType(EbtBool, EbpUndefined, EvqConst);
            break;

          case EOpLessThanEqual:
            {
                ASSERT(objectSize == 1);
                ConstantUnion constant;
                constant.setBConst(*unionArray > *rightUnionArray);
                tempConstArray = new ConstantUnion[1];
                tempConstArray->setBConst(!constant.getBConst());
                returnType = TType(EbtBool, EbpUndefined, EvqConst);
                break;
            }

          case EOpGreaterThanEqual:
            {
                ASSERT(objectSize == 1);
                ConstantUnion constant;
                constant.setBConst(*unionArray < *rightUnionArray);
                tempConstArray = new ConstantUnion[1];
                tempConstArray->setBConst(!constant.getBConst());
                returnType = TType(EbtBool, EbpUndefined, EvqConst);
                break;
            }

          case EOpEqual:
            if (getType().getBasicType() == EbtStruct)
            {
                if (!CompareStructure(node->getType(),
                                      node->getUnionArrayPointer(),
                                      unionArray))
                {
                    boolNodeFlag = true;
                }
            }
            else
            {
                for (size_t i = 0; i < objectSize; i++)
                {
                    if (unionArray[i] != rightUnionArray[i])
                    {
                        boolNodeFlag = true;
                        break;  // break out of for loop
                    }
                }
            }

            tempConstArray = new ConstantUnion[1];
            if (!boolNodeFlag)
            {
                tempConstArray->setBConst(true);
            }
            else
            {
                tempConstArray->setBConst(false);
            }

            tempNode = new TIntermConstantUnion(
                tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
            tempNode->setLine(getLine());

            return tempNode;

          case EOpNotEqual:
            if (getType().getBasicType() == EbtStruct)
            {
                if (CompareStructure(node->getType(),
                                     node->getUnionArrayPointer(),
                                     unionArray))
                {
                    boolNodeFlag = true;
                }
            }
            else
            {
                for (size_t i = 0; i < objectSize; i++)
                {
                    if (unionArray[i] == rightUnionArray[i])
                    {
                        boolNodeFlag = true;
                        break;  // break out of for loop
                    }
                }
            }

            tempConstArray = new ConstantUnion[1];
            if (!boolNodeFlag)
            {
                tempConstArray->setBConst(true);
            }
            else
            {
                tempConstArray->setBConst(false);
            }

            tempNode = new TIntermConstantUnion(
                tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
            tempNode->setLine(getLine());

            return tempNode;

          default:
            infoSink.info.message(
                EPrefixInternalError, getLine(),
                "Invalid operator for constant folding");
            return NULL;
        }
        tempNode = new TIntermConstantUnion(tempConstArray, returnType);
        tempNode->setLine(getLine());

        return tempNode;
    }
    else
    {
        //
        // Do unary operations
        //
        TIntermConstantUnion *newNode = 0;
        ConstantUnion* tempConstArray = new ConstantUnion[objectSize];
        for (size_t i = 0; i < objectSize; i++)
        {
            switch(op)
            {
              case EOpNegative:
                switch (getType().getBasicType())
                {
                  case EbtFloat:
                    tempConstArray[i].setFConst(-unionArray[i].getFConst());
                    break;
                  case EbtInt:
                    tempConstArray[i].setIConst(-unionArray[i].getIConst());
                    break;
                  case EbtUInt:
                    tempConstArray[i].setUConst(static_cast<unsigned int>(
                        -static_cast<int>(unionArray[i].getUConst())));
                    break;
                  default:
                    infoSink.info.message(
                        EPrefixInternalError, getLine(),
                        "Unary operation not folded into constant");
                    return NULL;
                }
                break;

              case EOpPositive:
                switch (getType().getBasicType())
                {
                  case EbtFloat:
                    tempConstArray[i].setFConst(unionArray[i].getFConst());
                    break;
                  case EbtInt:
                    tempConstArray[i].setIConst(unionArray[i].getIConst());
                    break;
                  case EbtUInt:
                    tempConstArray[i].setUConst(static_cast<unsigned int>(
                        static_cast<int>(unionArray[i].getUConst())));
                    break;
                  default:
                    infoSink.info.message(
                        EPrefixInternalError, getLine(),
                        "Unary operation not folded into constant");
                    return NULL;
                }
                break;

              case EOpLogicalNot:
                // this code is written for possible future use,
                // will not get executed currently
                switch (getType().getBasicType())
                {
                  case EbtBool:
                    tempConstArray[i].setBConst(!unionArray[i].getBConst());
                    break;
                  default:
                    infoSink.info.message(
                        EPrefixInternalError, getLine(),
                        "Unary operation not folded into constant");
                    return NULL;
                }
                break;

              default:
                return NULL;
            }
        }
        newNode = new TIntermConstantUnion(tempConstArray, getType());
        newNode->setLine(getLine());
        return newNode;
    }
}

// static
TString TIntermTraverser::hash(const TString &name, ShHashFunction64 hashFunction)
{
    if (hashFunction == NULL || name.empty())
        return name;
    khronos_uint64_t number = (*hashFunction)(name.c_str(), name.length());
    TStringStream stream;
    stream << HASHED_NAME_PREFIX << std::hex << number;
    TString hashedName = stream.str();
    return hashedName;
}
