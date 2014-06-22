//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/intermediate.h"

//
// Traverse the intermediate representation tree, and
// call a node type specific function for each node.
// Done recursively through the member function Traverse().
// Node types can be skipped if their function to call is 0,
// but their subtree will still be traversed.
// Nodes with children can have their whole subtree skipped
// if preVisit is turned on and the type specific function
// returns false.
//
// preVisit, postVisit, and rightToLeft control what order
// nodes are visited in.
//

//
// Traversal functions for terminals are straighforward....
//
void TIntermSymbol::traverse(TIntermTraverser* it)
{
	it->visitSymbol(this);
}

void TIntermConstantUnion::traverse(TIntermTraverser* it)
{
	it->visitConstantUnion(this);
}

//
// Traverse a binary node.
//
void TIntermBinary::traverse(TIntermTraverser* it)
{
	bool visit = true;

	//
	// visit the node before children if pre-visiting.
	//
	if(it->preVisit)
	{
		visit = it->visitBinary(PreVisit, this);
	}
	
	//
	// Visit the children, in the right order.
	//
	if(visit)
	{
		it->incrementDepth();

		if(it->rightToLeft) 
		{
			if(right)
			{
				right->traverse(it);
			}
			
			if(it->inVisit)
			{
				visit = it->visitBinary(InVisit, this);
			}

			if(visit && left)
			{
				left->traverse(it);
			}
		}
		else
		{
			if(left)
			{
				left->traverse(it);
			}
			
			if(it->inVisit)
			{
				visit = it->visitBinary(InVisit, this);
			}

			if(visit && right)
			{
				right->traverse(it);
			}
		}

		it->decrementDepth();
	}

	//
	// Visit the node after the children, if requested and the traversal
	// hasn't been cancelled yet.
	//
	if(visit && it->postVisit)
	{
		it->visitBinary(PostVisit, this);
	}
}

//
// Traverse a unary node.  Same comments in binary node apply here.
//
void TIntermUnary::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitUnary(PreVisit, this);

	if (visit) {
		it->incrementDepth();
		operand->traverse(it);
		it->decrementDepth();
	}
	
	if (visit && it->postVisit)
		it->visitUnary(PostVisit, this);
}

//
// Traverse an aggregate node.  Same comments in binary node apply here.
//
void TIntermAggregate::traverse(TIntermTraverser* it)
{
	bool visit = true;
	
	if(it->preVisit)
	{
		visit = it->visitAggregate(PreVisit, this);
	}
	
	if(visit)
	{
		it->incrementDepth();

		if(it->rightToLeft)
		{
			for(TIntermSequence::reverse_iterator sit = sequence.rbegin(); sit != sequence.rend(); sit++)
			{
				(*sit)->traverse(it);

				if(visit && it->inVisit)
				{
					if(*sit != sequence.front())
					{
						visit = it->visitAggregate(InVisit, this);
					}
				}
			}
		}
		else
		{
			for(TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
			{
				(*sit)->traverse(it);

				if(visit && it->inVisit)
				{
					if(*sit != sequence.back())
					{
						visit = it->visitAggregate(InVisit, this);
					}
				}
			}
		}
		
		it->decrementDepth();
	}

	if(visit && it->postVisit)
	{
		it->visitAggregate(PostVisit, this);
	}
}

//
// Traverse a selection node.  Same comments in binary node apply here.
//
void TIntermSelection::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitSelection(PreVisit, this);
	
	if (visit) {
		it->incrementDepth();
		if (it->rightToLeft) {
			if (falseBlock)
				falseBlock->traverse(it);
			if (trueBlock)
				trueBlock->traverse(it);
			condition->traverse(it);
		} else {
			condition->traverse(it);
			if (trueBlock)
				trueBlock->traverse(it);
			if (falseBlock)
				falseBlock->traverse(it);
		}
		it->decrementDepth();
	}

	if (visit && it->postVisit)
		it->visitSelection(PostVisit, this);
}

//
// Traverse a loop node.  Same comments in binary node apply here.
//
void TIntermLoop::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if(it->preVisit)
	{
		visit = it->visitLoop(PreVisit, this);
	}
	
	if(visit)
	{
		it->incrementDepth();

		if(it->rightToLeft)
		{
			if(expr)
			{
				expr->traverse(it);
			}

			if(body)
			{
				body->traverse(it);
			}

			if(cond)
			{
				cond->traverse(it);
			}
		}
		else
		{
			if(cond)
			{
				cond->traverse(it);
			}

			if(body)
			{
				body->traverse(it);
			}

			if(expr)
			{
				expr->traverse(it);
			}
		}

		it->decrementDepth();
	}

	if(visit && it->postVisit)
	{
		it->visitLoop(PostVisit, this);
	}
}

//
// Traverse a branch node.  Same comments in binary node apply here.
//
void TIntermBranch::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitBranch(PreVisit, this);
	
	if (visit && expression) {
		it->incrementDepth();
		expression->traverse(it);
		it->decrementDepth();
	}

	if (visit && it->postVisit)
		it->visitBranch(PostVisit, this);
}

