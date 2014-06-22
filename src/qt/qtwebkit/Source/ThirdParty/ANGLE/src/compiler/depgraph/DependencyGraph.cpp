//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#if defined(_MSC_VER)
#pragma warning(disable: 4718)
#endif

#include "compiler/depgraph/DependencyGraph.h"
#include "compiler/depgraph/DependencyGraphBuilder.h"

TDependencyGraph::TDependencyGraph(TIntermNode* intermNode)
{
    TDependencyGraphBuilder::build(intermNode, this);
}

TDependencyGraph::~TDependencyGraph()
{
    for (TGraphNodeVector::const_iterator iter = mAllNodes.begin(); iter != mAllNodes.end(); ++iter)
    {
        TGraphNode* node = *iter;
        delete node;
    }
}

TGraphArgument* TDependencyGraph::createArgument(TIntermAggregate* intermFunctionCall,
                                                 int argumentNumber)
{
    TGraphArgument* argument = new TGraphArgument(intermFunctionCall, argumentNumber);
    mAllNodes.push_back(argument);
    return argument;
}

TGraphFunctionCall* TDependencyGraph::createFunctionCall(TIntermAggregate* intermFunctionCall)
{
    TGraphFunctionCall* functionCall = new TGraphFunctionCall(intermFunctionCall);
    mAllNodes.push_back(functionCall);
    if (functionCall->getIntermFunctionCall()->isUserDefined())
        mUserDefinedFunctionCalls.push_back(functionCall);
    return functionCall;
}

TGraphSymbol* TDependencyGraph::getOrCreateSymbol(TIntermSymbol* intermSymbol)
{
    TSymbolIdMap::const_iterator iter = mSymbolIdMap.find(intermSymbol->getId());

    TGraphSymbol* symbol = NULL;

    if (iter != mSymbolIdMap.end()) {
        TSymbolIdPair pair = *iter;
        symbol = pair.second;
    } else {
        symbol = new TGraphSymbol(intermSymbol);
        mAllNodes.push_back(symbol);

        TSymbolIdPair pair(intermSymbol->getId(), symbol);
        mSymbolIdMap.insert(pair);

        // We save all sampler symbols in a collection, so we can start graph traversals from them quickly.
        if (IsSampler(intermSymbol->getBasicType()))
            mSamplerSymbols.push_back(symbol);
    }

    return symbol;
}

TGraphSelection* TDependencyGraph::createSelection(TIntermSelection* intermSelection)
{
    TGraphSelection* selection = new TGraphSelection(intermSelection);
    mAllNodes.push_back(selection);
    return selection;
}

TGraphLoop* TDependencyGraph::createLoop(TIntermLoop* intermLoop)
{
    TGraphLoop* loop = new TGraphLoop(intermLoop);
    mAllNodes.push_back(loop);
    return loop;
}

TGraphLogicalOp* TDependencyGraph::createLogicalOp(TIntermBinary* intermLogicalOp)
{
    TGraphLogicalOp* logicalOp = new TGraphLogicalOp(intermLogicalOp);
    mAllNodes.push_back(logicalOp);
    return logicalOp;
}

const char* TGraphLogicalOp::getOpString() const
{
    const char* opString = NULL;
    switch (getIntermLogicalOp()->getOp()) {
        case EOpLogicalAnd: opString = "and"; break;
        case EOpLogicalOr: opString = "or"; break;
        default: opString = "unknown"; break;
    }
    return opString;
}
