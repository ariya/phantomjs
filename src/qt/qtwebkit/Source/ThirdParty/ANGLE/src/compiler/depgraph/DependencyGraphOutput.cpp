//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/depgraph/DependencyGraphOutput.h"

void TDependencyGraphOutput::outputIndentation()
{
    for (int i = 0; i < getDepth(); ++i)
        mSink << "  ";
}

void TDependencyGraphOutput::visitArgument(TGraphArgument* parameter)
{
    outputIndentation();
    mSink << "argument " << parameter->getArgumentNumber() << " of call to "
          << parameter->getIntermFunctionCall()->getName() << "\n";
}

void TDependencyGraphOutput::visitFunctionCall(TGraphFunctionCall* functionCall)
{
    outputIndentation();
    mSink << "function call " <<  functionCall->getIntermFunctionCall()->getName() << "\n";
}

void TDependencyGraphOutput::visitSymbol(TGraphSymbol* symbol)
{
    outputIndentation();
    mSink << symbol->getIntermSymbol()->getSymbol() << " (symbol id: "
          << symbol->getIntermSymbol()->getId() << ")\n";
}

void TDependencyGraphOutput::visitSelection(TGraphSelection* selection)
{
    outputIndentation();
    mSink << "selection\n";
}

void TDependencyGraphOutput::visitLoop(TGraphLoop* loop)
{
    outputIndentation();
    mSink << "loop condition\n";
}

void TDependencyGraphOutput::visitLogicalOp(TGraphLogicalOp* logicalOp)
{
    outputIndentation();
    mSink << "logical " << logicalOp->getOpString() << "\n";
}

void TDependencyGraphOutput::outputAllSpanningTrees(TDependencyGraph& graph)
{
    mSink << "\n";

    for (TGraphNodeVector::const_iterator iter = graph.begin(); iter != graph.end(); ++iter)
    {
        TGraphNode* symbol = *iter;
        mSink << "--- Dependency graph spanning tree ---\n";
        clearVisited();
        symbol->traverse(this);
        mSink << "\n";
    }
}
