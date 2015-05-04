//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/ParseContext.h"

//
// Use this class to carry along data from node to node in
// the traversal
//
class TConstTraverser : public TIntermTraverser
{
  public:
    TConstTraverser(ConstantUnion *cUnion, bool singleConstParam,
                    TOperator constructType, TInfoSink &sink, TType &t)
        : error(false),
          mIndex(0),
          mUnionArray(cUnion),
          mType(t),
          mConstructorType(constructType),
          mSingleConstantParam(singleConstParam),
          mInfoSink(sink),
          mSize(0),
          mIsDiagonalMatrixInit(false),
          mMatrixCols(0),
          mMatrixRows(0)
    {
    }

    bool error;

  protected:
    void visitSymbol(TIntermSymbol *);
    void visitConstantUnion(TIntermConstantUnion *);
    bool visitBinary(Visit visit, TIntermBinary *);
    bool visitUnary(Visit visit, TIntermUnary *);
    bool visitSelection(Visit visit, TIntermSelection *);
    bool visitAggregate(Visit visit, TIntermAggregate *);
    bool visitLoop(Visit visit, TIntermLoop *);
    bool visitBranch(Visit visit, TIntermBranch *);

    size_t mIndex;
    ConstantUnion *mUnionArray;
    TType mType;
    TOperator mConstructorType;
    bool mSingleConstantParam;
    TInfoSink &mInfoSink;
    size_t mSize; // size of the constructor ( 4 for vec4)
    bool mIsDiagonalMatrixInit;
    int mMatrixCols; // columns of the matrix
    int mMatrixRows; // rows of the matrix
};

//
// The rest of the file are the traversal functions.  The last one
// is the one that starts the traversal.
//
// Return true from interior nodes to have the external traversal
// continue on to children.  If you process children yourself,
// return false.
//
void TConstTraverser::visitSymbol(TIntermSymbol *node)
{
    mInfoSink.info.message(EPrefixInternalError, node->getLine(),
                           "Symbol Node found in constant constructor");
    return;
}

bool TConstTraverser::visitBinary(Visit visit, TIntermBinary *node)
{
    TQualifier qualifier = node->getType().getQualifier();

    if (qualifier != EvqConst)
    {
        TString buf;
        buf.append("'constructor' : assigning non-constant to ");
        buf.append(mType.getCompleteString());
        mInfoSink.info.message(EPrefixError, node->getLine(), buf.c_str());
        error = true;
        return false;
    }

    mInfoSink.info.message(EPrefixInternalError, node->getLine(),
                           "Binary Node found in constant constructor");
    return false;
}

bool TConstTraverser::visitUnary(Visit visit, TIntermUnary *node)
{
    TString buf;
    buf.append("'constructor' : assigning non-constant to ");
    buf.append(mType.getCompleteString());
    mInfoSink.info.message(EPrefixError, node->getLine(), buf.c_str());
    error = true;
    return false;
}

bool TConstTraverser::visitAggregate(Visit visit, TIntermAggregate *node)
{
    if (!node->isConstructor() && node->getOp() != EOpComma)
    {
        TString buf;
        buf.append("'constructor' : assigning non-constant to ");
        buf.append(mType.getCompleteString());
        mInfoSink.info.message(EPrefixError, node->getLine(), buf.c_str());
        error = true;
        return false;
    }

    if (node->getSequence()->size() == 0)
    {
        error = true;
        return false;
    }

    bool flag = node->getSequence()->size() == 1 &&
                (*node->getSequence())[0]->getAsTyped()->getAsConstantUnion();
    if (flag)
    {
        mSingleConstantParam = true;
        mConstructorType = node->getOp();
        mSize = node->getType().getObjectSize();

        if (node->getType().isMatrix())
        {
            mIsDiagonalMatrixInit = true;
            mMatrixCols = node->getType().getCols();
            mMatrixRows = node->getType().getRows();
        }
    }

    for (TIntermSequence::iterator p = node->getSequence()->begin();
         p != node->getSequence()->end(); p++)
    {
        if (node->getOp() == EOpComma)
            mIndex = 0;
        (*p)->traverse(this);
    }
    if (flag)
    {
        mSingleConstantParam = false;
        mConstructorType = EOpNull;
        mSize = 0;
        mIsDiagonalMatrixInit = false;
        mMatrixCols = 0;
        mMatrixRows = 0;
    }
    return false;
}

bool TConstTraverser::visitSelection(Visit visit, TIntermSelection *node)
{
    mInfoSink.info.message(EPrefixInternalError, node->getLine(),
                           "Selection Node found in constant constructor");
    error = true;
    return false;
}

void TConstTraverser::visitConstantUnion(TIntermConstantUnion *node)
{
    if (!node->getUnionArrayPointer())
    {
        // The constant was not initialized, this should already have been logged
        ASSERT(mInfoSink.info.size() != 0);
        return;
    }

    ConstantUnion *leftUnionArray = mUnionArray;
    size_t instanceSize = mType.getObjectSize();
    TBasicType basicType = mType.getBasicType();

    if (mIndex >= instanceSize)
        return;

    if (!mSingleConstantParam)
    {
        size_t objectSize = node->getType().getObjectSize();
        ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
        for (size_t i=0; i < objectSize; i++)
        {
            if (mIndex >= instanceSize)
                return;
            leftUnionArray[mIndex].cast(basicType, rightUnionArray[i]);
            mIndex++;
        }
    }
    else
    {
        size_t totalSize = mIndex + mSize;
        ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
        if (!mIsDiagonalMatrixInit)
        {
            int count = 0;
            for (size_t i = mIndex; i < totalSize; i++)
            {
                if (i >= instanceSize)
                    return;
                leftUnionArray[i].cast(basicType, rightUnionArray[count]);
                mIndex++;
                if (node->getType().getObjectSize() > 1)
                    count++;
            }
        }
        else
        {
            // for matrix diagonal constructors from a single scalar
            for (int i = 0, col = 0; col < mMatrixCols; col++)
            {
                for (int row = 0; row < mMatrixRows; row++, i++)
                {
                    if (col == row)
                    {
                        leftUnionArray[i].cast(basicType, rightUnionArray[0]);
                    }
                    else
                    {
                        leftUnionArray[i].setFConst(0.0f);
                    }
                    mIndex++;
                }
            }
        }
    }
}

bool TConstTraverser::visitLoop(Visit visit, TIntermLoop *node)
{
    mInfoSink.info.message(EPrefixInternalError, node->getLine(),
                           "Loop Node found in constant constructor");
    error = true;
    return false;
}

bool TConstTraverser::visitBranch(Visit visit, TIntermBranch *node)
{
    mInfoSink.info.message(EPrefixInternalError, node->getLine(),
                           "Branch Node found in constant constructor");
    error = true;
    return false;
}

//
// This function is the one to call externally to start the traversal.
// Individual functions can be initialized to 0 to skip processing of that
// type of node.  It's children will still be processed.
//
bool TIntermediate::parseConstTree(
    const TSourceLoc &line, TIntermNode *root, ConstantUnion *unionArray,
    TOperator constructorType, TType t, bool singleConstantParam)
{
    if (root == 0)
        return false;

    TConstTraverser it(unionArray, singleConstantParam, constructorType,
                       mInfoSink, t);

    root->traverse(&it);
    if (it.error)
        return true;
    else
        return false;
}
