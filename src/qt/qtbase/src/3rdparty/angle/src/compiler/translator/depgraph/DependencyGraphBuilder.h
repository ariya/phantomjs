//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_BUILDER_H
#define COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_BUILDER_H

#include "compiler/translator/depgraph/DependencyGraph.h"

//
// Creates a dependency graph of symbols, function calls, conditions etc. by traversing a
// intermediate tree.
//
class TDependencyGraphBuilder : public TIntermTraverser {
public:
    static void build(TIntermNode* node, TDependencyGraph* graph);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitBinary(Visit visit, TIntermBinary*);
    virtual bool visitSelection(Visit visit, TIntermSelection*);
    virtual bool visitAggregate(Visit visit, TIntermAggregate*);
    virtual bool visitLoop(Visit visit, TIntermLoop*);

private:
    typedef std::stack<TGraphSymbol*> TSymbolStack;
    typedef std::set<TGraphParentNode*> TParentNodeSet;

    //
    // For collecting the dependent nodes of assignments, conditions, etc.
    // while traversing the intermediate tree.
    //
    // This data structure is stack of sets. Each set contains dependency graph parent nodes.
    //
    class TNodeSetStack {
    public:
        TNodeSetStack() {};
        ~TNodeSetStack() { clear(); }

        // This should only be called after a pushSet.
        // Returns NULL if the top set is empty.
        TParentNodeSet* getTopSet() const
        {
            ASSERT(!nodeSets.empty());
            TParentNodeSet* topSet = nodeSets.top();
            return !topSet->empty() ? topSet : NULL;
        }

        void pushSet() { nodeSets.push(new TParentNodeSet()); }
        void popSet()
        {
            ASSERT(!nodeSets.empty());
            delete nodeSets.top();
            nodeSets.pop();
        }

        // Pops the top set and adds its contents to the new top set.
        // This should only be called after a pushSet.
        // If there is no set below the top set, the top set is just deleted.
        void popSetIntoNext()
        {
            ASSERT(!nodeSets.empty());
            TParentNodeSet* oldTopSet = nodeSets.top();
            nodeSets.pop();

            if (!nodeSets.empty()) {
                TParentNodeSet* newTopSet = nodeSets.top();
                newTopSet->insert(oldTopSet->begin(), oldTopSet->end());
            }

            delete oldTopSet;
        }

        // Does nothing if there is no top set.
        // This can be called when there is no top set if we are visiting
        // symbols that are not under an assignment or condition.
        // We don't need to track those symbols.
        void insertIntoTopSet(TGraphParentNode* node)
        {
            if (nodeSets.empty())
                return;

            nodeSets.top()->insert(node);
        }

        void clear()
        {
            while (!nodeSets.empty())
                popSet();
        }

    private:
        typedef std::stack<TParentNodeSet*> TParentNodeSetStack;

        TParentNodeSetStack nodeSets;
    };

    //
    // An instance of this class pushes a new node set when instantiated.
    // When the instance goes out of scope, it and pops the node set.
    //
    class TNodeSetMaintainer {
    public:
        TNodeSetMaintainer(TDependencyGraphBuilder* factory)
            : sets(factory->mNodeSets) { sets.pushSet(); }
        ~TNodeSetMaintainer() { sets.popSet(); }
    protected:
        TNodeSetStack& sets;
    };

    //
    // An instance of this class pushes a new node set when instantiated.
    // When the instance goes out of scope, it and pops the top node set and adds its contents to
    // the new top node set.
    //
    class TNodeSetPropagatingMaintainer {
    public:
        TNodeSetPropagatingMaintainer(TDependencyGraphBuilder* factory)
            : sets(factory->mNodeSets) { sets.pushSet(); }
        ~TNodeSetPropagatingMaintainer() { sets.popSetIntoNext(); }
    protected:
        TNodeSetStack& sets;
    };

    //
    // An instance of this class keeps track of the leftmost symbol while we're exploring an
    // assignment.
    // It will push the placeholder symbol kLeftSubtree when instantiated under a left subtree,
    // and kRightSubtree under a right subtree.
    // When it goes out of scope, it will pop the leftmost symbol at the top of the scope.
    // During traversal, the TDependencyGraphBuilder will replace kLeftSubtree with a real symbol.
    // kRightSubtree will never be replaced by a real symbol because we are tracking the leftmost
    // symbol.
    //
    class TLeftmostSymbolMaintainer {
    public:
        TLeftmostSymbolMaintainer(TDependencyGraphBuilder* factory, TGraphSymbol& subtree)
            : leftmostSymbols(factory->mLeftmostSymbols)
        {
            needsPlaceholderSymbol = leftmostSymbols.empty() || leftmostSymbols.top() != &subtree;
            if (needsPlaceholderSymbol)
                leftmostSymbols.push(&subtree);
        }

        ~TLeftmostSymbolMaintainer()
        {
            if (needsPlaceholderSymbol)
                leftmostSymbols.pop();
        }

    protected:
        TSymbolStack& leftmostSymbols;
        bool needsPlaceholderSymbol;
    };

    TDependencyGraphBuilder(TDependencyGraph* graph)
        : TIntermTraverser(true, false, false)
        , mLeftSubtree(NULL)
        , mRightSubtree(NULL)
        , mGraph(graph) {}
    void build(TIntermNode* intermNode) { intermNode->traverse(this); }

    void connectMultipleNodesToSingleNode(TParentNodeSet* nodes, TGraphNode* node) const;

    void visitAssignment(TIntermBinary*);
    void visitLogicalOp(TIntermBinary*);
    void visitBinaryChildren(TIntermBinary*);
    void visitFunctionDefinition(TIntermAggregate*);
    void visitFunctionCall(TIntermAggregate* intermFunctionCall);
    void visitAggregateChildren(TIntermAggregate*);

    TGraphSymbol mLeftSubtree;
    TGraphSymbol mRightSubtree;

    TDependencyGraph* mGraph;
    TNodeSetStack mNodeSets;
    TSymbolStack mLeftmostSymbols;
};

#endif  // COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_BUILDER_H
