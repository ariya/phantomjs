//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/UnfoldShortCircuitAST.h"

namespace
{

// "x || y" is equivalent to "x ? true : y".
TIntermSelection *UnfoldOR(TIntermTyped *x, TIntermTyped *y)
{
    const TType boolType(EbtBool, EbpUndefined);
    ConstantUnion *u = new ConstantUnion;
    u->setBConst(true);
    TIntermConstantUnion *trueNode = new TIntermConstantUnion(
        u, TType(EbtBool, EbpUndefined, EvqConst, 1));
    return new TIntermSelection(x, trueNode, y, boolType);
}

// "x && y" is equivalent to "x ? y : false".
TIntermSelection *UnfoldAND(TIntermTyped *x, TIntermTyped *y)
{
    const TType boolType(EbtBool, EbpUndefined);
    ConstantUnion *u = new ConstantUnion;
    u->setBConst(false);
    TIntermConstantUnion *falseNode = new TIntermConstantUnion(
        u, TType(EbtBool, EbpUndefined, EvqConst, 1));
    return new TIntermSelection(x, y, falseNode, boolType);
}

}  // namespace anonymous

bool UnfoldShortCircuitAST::visitBinary(Visit visit, TIntermBinary *node)
{
    TIntermSelection *replacement = NULL;

    switch (node->getOp())
    {
      case EOpLogicalOr:
        replacement = UnfoldOR(node->getLeft(), node->getRight());
        break;
      case EOpLogicalAnd:
        replacement = UnfoldAND(node->getLeft(), node->getRight());
        break;
      default:
        break;
    }
    if (replacement)
    {
        replacements.push_back(
            NodeUpdateEntry(getParentNode(), node, replacement));
    }
    return true;
}

void UnfoldShortCircuitAST::updateTree()
{
    for (size_t ii = 0; ii < replacements.size(); ++ii)
    {
        const NodeUpdateEntry& entry = replacements[ii];
        ASSERT(entry.parent);
        bool replaced = entry.parent->replaceChildNode(
            entry.original, entry.replacement);
        ASSERT(replaced);

        // In AST traversing, a parent is visited before its children.
        // After we replace a node, if an immediate child is to
        // be replaced, we need to make sure we don't update the replaced
	// node; instead, we update the replacement node.
        for (size_t jj = ii + 1; jj < replacements.size(); ++jj)
        {
            NodeUpdateEntry& entry2 = replacements[jj];
            if (entry2.parent == entry.original)
                entry2.parent = entry.replacement;
        }
    }
}

