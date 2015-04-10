//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/depgraph/DependencyGraphBuilder.h"

void TDependencyGraphBuilder::build(TIntermNode* node, TDependencyGraph* graph)
{
    TDependencyGraphBuilder builder(graph);
    builder.build(node);
}

bool TDependencyGraphBuilder::visitAggregate(Visit visit, TIntermAggregate* intermAggregate)
{
    switch (intermAggregate->getOp()) {
        case EOpFunction: visitFunctionDefinition(intermAggregate); break;
        case EOpFunctionCall: visitFunctionCall(intermAggregate); break;
        default: visitAggregateChildren(intermAggregate); break;
    }

    return false;
}

void TDependencyGraphBuilder::visitFunctionDefinition(TIntermAggregate* intermAggregate)
{
    // Currently, we do not support user defined functions.
    if (intermAggregate->getName() != "main(")
        return;

    visitAggregateChildren(intermAggregate);
}

// Takes an expression like "f(x)" and creates a dependency graph like
// "x -> argument 0 -> function call".
void TDependencyGraphBuilder::visitFunctionCall(TIntermAggregate* intermFunctionCall)
{
    TGraphFunctionCall* functionCall = mGraph->createFunctionCall(intermFunctionCall);

    // Run through the function call arguments.
    int argumentNumber = 0;
    TIntermSequence& intermArguments = intermFunctionCall->getSequence();
    for (TIntermSequence::const_iterator iter = intermArguments.begin();
         iter != intermArguments.end();
         ++iter, ++argumentNumber)
    {
        TNodeSetMaintainer nodeSetMaintainer(this);

        TIntermNode* intermArgument = *iter;
        intermArgument->traverse(this);

        if (TParentNodeSet* argumentNodes = mNodeSets.getTopSet()) {
            TGraphArgument* argument = mGraph->createArgument(intermFunctionCall, argumentNumber);
            connectMultipleNodesToSingleNode(argumentNodes, argument);
            argument->addDependentNode(functionCall);
        }
    }

    // Push the leftmost symbol of this function call into the current set of dependent symbols to
    // represent the result of this function call.
    // Thus, an expression like "y = f(x)" will yield a dependency graph like
    // "x -> argument 0 -> function call -> y".
    // This line essentially passes the function call node back up to an earlier visitAssignment
    // call, which will create the connection "function call -> y".
    mNodeSets.insertIntoTopSet(functionCall);
}

void TDependencyGraphBuilder::visitAggregateChildren(TIntermAggregate* intermAggregate)
{
    TIntermSequence& sequence = intermAggregate->getSequence();
    for(TIntermSequence::const_iterator iter = sequence.begin(); iter != sequence.end(); ++iter)
    {
        TIntermNode* intermChild = *iter;
        intermChild->traverse(this);
    }
}

void TDependencyGraphBuilder::visitSymbol(TIntermSymbol* intermSymbol)
{
    // Push this symbol into the set of dependent symbols for the current assignment or condition
    // that we are traversing.
    TGraphSymbol* symbol = mGraph->getOrCreateSymbol(intermSymbol);
    mNodeSets.insertIntoTopSet(symbol);

    // If this symbol is the current leftmost symbol under an assignment, replace the previous
    // leftmost symbol with this symbol.
    if (!mLeftmostSymbols.empty() && mLeftmostSymbols.top() != &mRightSubtree) {
        mLeftmostSymbols.pop();
        mLeftmostSymbols.push(symbol);
    }
}

bool TDependencyGraphBuilder::visitBinary(Visit visit, TIntermBinary* intermBinary)
{
    TOperator op = intermBinary->getOp();
    if (op == EOpInitialize || intermBinary->isAssignment())
        visitAssignment(intermBinary);
    else if (op == EOpLogicalAnd || op == EOpLogicalOr)
        visitLogicalOp(intermBinary);
    else
        visitBinaryChildren(intermBinary);

    return false;
}

void TDependencyGraphBuilder::visitAssignment(TIntermBinary* intermAssignment)
{
    TIntermTyped* intermLeft = intermAssignment->getLeft();
    if (!intermLeft)
        return;

    TGraphSymbol* leftmostSymbol = NULL;

    {
        TNodeSetMaintainer nodeSetMaintainer(this);

        {
            TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mLeftSubtree);
            intermLeft->traverse(this);
            leftmostSymbol = mLeftmostSymbols.top();

            // After traversing the left subtree of this assignment, we should have found a real
            // leftmost symbol, and the leftmost symbol should not be a placeholder.
            ASSERT(leftmostSymbol != &mLeftSubtree);
            ASSERT(leftmostSymbol != &mRightSubtree);
        }

        if (TIntermTyped* intermRight = intermAssignment->getRight()) {
            TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mRightSubtree);
            intermRight->traverse(this);
        }

        if (TParentNodeSet* assignmentNodes = mNodeSets.getTopSet())
            connectMultipleNodesToSingleNode(assignmentNodes, leftmostSymbol);
    }

    // Push the leftmost symbol of this assignment into the current set of dependent symbols to
    // represent the result of this assignment.
    // An expression like "a = (b = c)" will yield a dependency graph like "c -> b -> a".
    // This line essentially passes the leftmost symbol of the nested assignment ("b" in this
    // example) back up to the earlier visitAssignment call for the outer assignment, which will
    // create the connection "b -> a".
    mNodeSets.insertIntoTopSet(leftmostSymbol);
}

void TDependencyGraphBuilder::visitLogicalOp(TIntermBinary* intermLogicalOp)
{
    if (TIntermTyped* intermLeft = intermLogicalOp->getLeft()) {
        TNodeSetPropagatingMaintainer nodeSetMaintainer(this);

        intermLeft->traverse(this);
        if (TParentNodeSet* leftNodes = mNodeSets.getTopSet()) {
            TGraphLogicalOp* logicalOp = mGraph->createLogicalOp(intermLogicalOp);
            connectMultipleNodesToSingleNode(leftNodes, logicalOp);
        }
    }

    if (TIntermTyped* intermRight = intermLogicalOp->getRight()) {
        TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mRightSubtree);
        intermRight->traverse(this);
    }
}

void TDependencyGraphBuilder::visitBinaryChildren(TIntermBinary* intermBinary)
{
    if (TIntermTyped* intermLeft = intermBinary->getLeft())
        intermLeft->traverse(this);

    if (TIntermTyped* intermRight = intermBinary->getRight()) {
        TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mRightSubtree);
        intermRight->traverse(this);
    }
}

bool TDependencyGraphBuilder::visitSelection(Visit visit, TIntermSelection* intermSelection)
{
    if (TIntermNode* intermCondition = intermSelection->getCondition()) {
        TNodeSetMaintainer nodeSetMaintainer(this);

        intermCondition->traverse(this);
        if (TParentNodeSet* conditionNodes = mNodeSets.getTopSet()) {
            TGraphSelection* selection = mGraph->createSelection(intermSelection);
            connectMultipleNodesToSingleNode(conditionNodes, selection);
        }
    }

    if (TIntermNode* intermTrueBlock = intermSelection->getTrueBlock())
        intermTrueBlock->traverse(this);

    if (TIntermNode* intermFalseBlock = intermSelection->getFalseBlock())
        intermFalseBlock->traverse(this);

    return false;
}

bool TDependencyGraphBuilder::visitLoop(Visit visit, TIntermLoop* intermLoop)
{
    if (TIntermTyped* intermCondition = intermLoop->getCondition()) {
        TNodeSetMaintainer nodeSetMaintainer(this);

        intermCondition->traverse(this);
        if (TParentNodeSet* conditionNodes = mNodeSets.getTopSet()) {
            TGraphLoop* loop = mGraph->createLoop(intermLoop);
            connectMultipleNodesToSingleNode(conditionNodes, loop);
        }
    }

    if (TIntermNode* intermBody = intermLoop->getBody())
        intermBody->traverse(this);

    if (TIntermTyped* intermExpression = intermLoop->getExpression())
        intermExpression->traverse(this);

    return false;
}


void TDependencyGraphBuilder::connectMultipleNodesToSingleNode(TParentNodeSet* nodes,
                                                               TGraphNode* node) const
{
    for (TParentNodeSet::const_iterator iter = nodes->begin(); iter != nodes->end(); ++iter)
    {
        TGraphParentNode* currentNode = *iter;
        currentNode->addDependentNode(node);
    }
}
