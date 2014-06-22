//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/depgraph/DependencyGraph.h"

// These methods do a breadth-first traversal through the graph and mark visited nodes.

void TGraphNode::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->markVisited(this);
}

void TGraphParentNode::traverse(TDependencyGraphTraverser* graphTraverser)
{
    TGraphNode::traverse(graphTraverser);

    graphTraverser->incrementDepth();

    // Visit the parent node's children.
    for (TGraphNodeSet::const_iterator iter = mDependentNodes.begin();
         iter != mDependentNodes.end();
         ++iter)
    {
        TGraphNode* node = *iter;
        if (!graphTraverser->isVisited(node))
            node->traverse(graphTraverser);
    }

    graphTraverser->decrementDepth();
}

void TGraphArgument::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitArgument(this);
    TGraphParentNode::traverse(graphTraverser);
}

void TGraphFunctionCall::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitFunctionCall(this);
    TGraphParentNode::traverse(graphTraverser);
}

void TGraphSymbol::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitSymbol(this);
    TGraphParentNode::traverse(graphTraverser);
}

void TGraphSelection::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitSelection(this);
    TGraphNode::traverse(graphTraverser);
}

void TGraphLoop::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitLoop(this);
    TGraphNode::traverse(graphTraverser);
}

void TGraphLogicalOp::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitLogicalOp(this);
    TGraphNode::traverse(graphTraverser);
}
