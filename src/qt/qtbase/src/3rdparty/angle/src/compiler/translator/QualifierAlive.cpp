//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/intermediate.h"

class TAliveTraverser : public TIntermTraverser {
public:
    TAliveTraverser(TQualifier q) : TIntermTraverser(true, false, false, true), found(false), qualifier(q)
    {
    }

	bool wasFound() { return found; }

protected:
    bool found;
    TQualifier qualifier;

    void visitSymbol(TIntermSymbol*);
    bool visitSelection(Visit, TIntermSelection*);
};

//
// Report whether or not a variable of the given qualifier type
// is guaranteed written.  Not always possible to determine if
// it is written conditionally.
//
// ?? It does not do this well yet, this is just a place holder
// that simply determines if it was reference at all, anywhere.
//
bool QualifierWritten(TIntermNode* node, TQualifier qualifier)
{
    TAliveTraverser it(qualifier);

    if (node)
        node->traverse(&it);

    return it.wasFound();
}

void TAliveTraverser::visitSymbol(TIntermSymbol* node)
{
    //
    // If it's what we're looking for, record it.
    //
    if (node->getQualifier() == qualifier)
        found = true;
}

bool TAliveTraverser::visitSelection(Visit preVisit, TIntermSelection* node)
{
    if (wasFound())
        return false;

    return true;
}
