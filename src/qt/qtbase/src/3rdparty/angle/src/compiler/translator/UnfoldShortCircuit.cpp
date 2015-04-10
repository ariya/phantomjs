//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UnfoldShortCircuit is an AST traverser to output short-circuiting operators as if-else statements.
// The results are assigned to s# temporaries, which are used by the main translator instead of
// the original expression.
//

#include "compiler/translator/UnfoldShortCircuit.h"

#include "compiler/translator/InfoSink.h"
#include "compiler/translator/OutputHLSL.h"

namespace sh
{
UnfoldShortCircuit::UnfoldShortCircuit(TParseContext &context, OutputHLSL *outputHLSL) : mContext(context), mOutputHLSL(outputHLSL)
{
    mTemporaryIndex = 0;
}

void UnfoldShortCircuit::traverse(TIntermNode *node)
{
    int rewindIndex = mTemporaryIndex;
    node->traverse(this);
    mTemporaryIndex = rewindIndex;
}

bool UnfoldShortCircuit::visitBinary(Visit visit, TIntermBinary *node)
{
    TInfoSinkBase &out = mOutputHLSL->getBodyStream();

    // If our right node doesn't have side effects, we know we don't need to unfold this
    // expression: there will be no short-circuiting side effects to avoid
    // (note: unfolding doesn't depend on the left node -- it will always be evaluated)
    if (!node->getRight()->hasSideEffects())
    {
        return true;
    }

    switch (node->getOp())
    {
      case EOpLogicalOr:
        // "x || y" is equivalent to "x ? true : y", which unfolds to "bool s; if(x) s = true; else s = y;",
        // and then further simplifies down to "bool s = x; if(!s) s = y;".
        {
            int i = mTemporaryIndex;

            out << "bool s" << i << ";\n";

            out << "{\n";

            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(this);
            out << "s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(mOutputHLSL);
            out << ";\n";
            out << "if (!s" << i << ")\n"
                   "{\n";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(this);
            out << "    s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(mOutputHLSL);
            out << ";\n"
                   "}\n";

            out << "}\n";

            mTemporaryIndex = i + 1;
        }
        return false;
      case EOpLogicalAnd:
        // "x && y" is equivalent to "x ? y : false", which unfolds to "bool s; if(x) s = y; else s = false;",
        // and then further simplifies down to "bool s = x; if(s) s = y;".
        {
            int i = mTemporaryIndex;

            out << "bool s" << i << ";\n";

            out << "{\n";

            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(this);
            out << "s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(mOutputHLSL);
            out << ";\n";
            out << "if (s" << i << ")\n"
                   "{\n";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(this);
            out << "    s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(mOutputHLSL);
            out << ";\n"
                   "}\n";

            out << "}\n";

            mTemporaryIndex = i + 1;
        }
        return false;
      default:
        return true;
    }
}

bool UnfoldShortCircuit::visitSelection(Visit visit, TIntermSelection *node)
{
    TInfoSinkBase &out = mOutputHLSL->getBodyStream();

    // Unfold "b ? x : y" into "type s; if(b) s = x; else s = y;"
    if (node->usesTernaryOperator())
    {
        int i = mTemporaryIndex;

        out << mOutputHLSL->typeString(node->getType()) << " s" << i << ";\n";

        out << "{\n";

        mTemporaryIndex = i + 1;
        node->getCondition()->traverse(this);
        out << "if (";
        mTemporaryIndex = i + 1;
        node->getCondition()->traverse(mOutputHLSL);
        out << ")\n"
               "{\n";
        mTemporaryIndex = i + 1;
        node->getTrueBlock()->traverse(this);
        out << "    s" << i << " = ";
        mTemporaryIndex = i + 1;
        node->getTrueBlock()->traverse(mOutputHLSL);
        out << ";\n"
               "}\n"
               "else\n"
               "{\n";
        mTemporaryIndex = i + 1;
        node->getFalseBlock()->traverse(this);
        out << "    s" << i << " = ";
        mTemporaryIndex = i + 1;
        node->getFalseBlock()->traverse(mOutputHLSL);
        out << ";\n"
               "}\n";

        out << "}\n";

        mTemporaryIndex = i + 1;
    }

    return false;
}

bool UnfoldShortCircuit::visitLoop(Visit visit, TIntermLoop *node)
{
    int rewindIndex = mTemporaryIndex;

    if (node->getInit())
    {
        node->getInit()->traverse(this);
    }
    
    if (node->getCondition())
    {
        node->getCondition()->traverse(this);
    }

    if (node->getExpression())
    {
        node->getExpression()->traverse(this);
    }

    mTemporaryIndex = rewindIndex;

    return false;
}

int UnfoldShortCircuit::getNextTemporaryIndex()
{
    return mTemporaryIndex++;
}
}
