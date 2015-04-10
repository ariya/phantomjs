//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// NodeSearch.h: Utilities for searching translator node graphs
//

#ifndef TRANSLATOR_NODESEARCH_H_
#define TRANSLATOR_NODESEARCH_H_

#include "compiler/translator/intermediate.h"

namespace sh
{

template <class Parent>
class NodeSearchTraverser : public TIntermTraverser
{
  public:
    NodeSearchTraverser()
        : mFound(false)
    {}

    bool found() const { return mFound; }

    static bool search(TIntermNode *node)
    {
        Parent searchTraverser;
        node->traverse(&searchTraverser);
        return searchTraverser.found();
    }

  protected:
    bool mFound;
};

class FindDiscard : public NodeSearchTraverser<FindDiscard>
{
  public:
    virtual bool visitBranch(Visit visit, TIntermBranch *node)
    {
        switch (node->getFlowOp())
        {
          case EOpKill:
            mFound = true;
            break;

          default: break;
        }

        return !mFound;
    }
};

class FindSideEffectRewriting : public NodeSearchTraverser<FindSideEffectRewriting>
{
  public:
    virtual bool visitBinary(Visit visit, TIntermBinary *node)
    {
        switch (node->getOp())
        {
          case EOpLogicalOr:
          case EOpLogicalAnd:
            if (node->getRight()->hasSideEffects())
            {
                mFound = true;
            }
            break;

          default: break;
        }

        return !mFound;
    }
};

}

#endif // TRANSLATOR_NODESEARCH_H_
