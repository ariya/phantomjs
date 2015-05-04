//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/IntermNode.h"
#include "compiler/translator/RemoveTree.h"

//
// Code to delete the intermediate tree.
//
void RemoveAllTreeNodes(TIntermNode* root)
{
    std::queue<TIntermNode*> nodeQueue;

    nodeQueue.push(root);

    while (!nodeQueue.empty())
    {
        TIntermNode *node = nodeQueue.front();
        nodeQueue.pop();

        node->enqueueChildren(&nodeQueue);

        delete node;
    }
}

