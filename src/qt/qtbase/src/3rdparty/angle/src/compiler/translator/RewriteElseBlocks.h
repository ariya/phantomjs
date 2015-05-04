//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RewriteElseBlocks.h: Prototype for tree transform to change
//   all if-else blocks to if-if blocks.
//

#ifndef COMPILER_REWRITE_ELSE_BLOCKS_H_
#define COMPILER_REWRITE_ELSE_BLOCKS_H_

#include "compiler/translator/IntermNode.h"

namespace sh
{

void RewriteElseBlocks(TIntermNode *node);

}

#endif // COMPILER_REWRITE_ELSE_BLOCKS_H_
