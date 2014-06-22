//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/DetectCallDepth.h"
#include "compiler/translator/InfoSink.h"

DetectCallDepth::FunctionNode::FunctionNode(const TString& fname)
    : name(fname),
      visit(PreVisit)
{
}

const TString& DetectCallDepth::FunctionNode::getName() const
{
    return name;
}

void DetectCallDepth::FunctionNode::addCallee(
    DetectCallDepth::FunctionNode* callee)
{
    for (size_t i = 0; i < callees.size(); ++i) {
        if (callees[i] == callee)
            return;
    }
    callees.push_back(callee);
}

int DetectCallDepth::FunctionNode::detectCallDepth(DetectCallDepth* detectCallDepth, int depth)
{
    ASSERT(visit == PreVisit);
    ASSERT(detectCallDepth);

    int maxDepth = depth;
    visit = InVisit;
    for (size_t i = 0; i < callees.size(); ++i) {
        switch (callees[i]->visit) {
            case InVisit:
                // cycle detected, i.e., recursion detected.
                return kInfiniteCallDepth;
            case PostVisit:
                break;
            case PreVisit: {
                // Check before we recurse so we don't go too depth
                if (detectCallDepth->checkExceedsMaxDepth(depth))
                    return depth;
                int callDepth = callees[i]->detectCallDepth(detectCallDepth, depth + 1);
                // Check after we recurse so we can exit immediately and provide info.
                if (detectCallDepth->checkExceedsMaxDepth(callDepth)) {
                    detectCallDepth->getInfoSink().info << "<-" << callees[i]->getName();
                    return callDepth;
                }
                maxDepth = std::max(callDepth, maxDepth);
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
    }
    visit = PostVisit;
    return maxDepth;
}

void DetectCallDepth::FunctionNode::reset()
{
    visit = PreVisit;
}

DetectCallDepth::DetectCallDepth(TInfoSink& infoSink, bool limitCallStackDepth, int maxCallStackDepth)
    : TIntermTraverser(true, false, true, false),
      currentFunction(NULL),
      infoSink(infoSink),
      maxDepth(limitCallStackDepth ? maxCallStackDepth : FunctionNode::kInfiniteCallDepth)
{
}

DetectCallDepth::~DetectCallDepth()
{
    for (size_t i = 0; i < functions.size(); ++i)
        delete functions[i];
}

bool DetectCallDepth::visitAggregate(Visit visit, TIntermAggregate* node)
{
    switch (node->getOp())
    {
        case EOpPrototype:
            // Function declaration.
            // Don't add FunctionNode here because node->getName() is the
            // unmangled function name.
            break;
        case EOpFunction: {
            // Function definition.
            if (visit == PreVisit) {
                currentFunction = findFunctionByName(node->getName());
                if (currentFunction == NULL) {
                    currentFunction = new FunctionNode(node->getName());
                    functions.push_back(currentFunction);
                }
            } else if (visit == PostVisit) {
                currentFunction = NULL;
            }
            break;
        }
        case EOpFunctionCall: {
            // Function call.
            if (visit == PreVisit) {
                FunctionNode* func = findFunctionByName(node->getName());
                if (func == NULL) {
                    func = new FunctionNode(node->getName());
                    functions.push_back(func);
                }
                if (currentFunction)
                    currentFunction->addCallee(func);
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool DetectCallDepth::checkExceedsMaxDepth(int depth)
{
    return depth >= maxDepth;
}

void DetectCallDepth::resetFunctionNodes()
{
    for (size_t i = 0; i < functions.size(); ++i) {
        functions[i]->reset();
    }
}

DetectCallDepth::ErrorCode DetectCallDepth::detectCallDepthForFunction(FunctionNode* func)
{
    currentFunction = NULL;
    resetFunctionNodes();

    int maxCallDepth = func->detectCallDepth(this, 1);

    if (maxCallDepth == FunctionNode::kInfiniteCallDepth)
        return kErrorRecursion;

    if (maxCallDepth >= maxDepth)
        return kErrorMaxDepthExceeded;

    return kErrorNone;
}

DetectCallDepth::ErrorCode DetectCallDepth::detectCallDepth()
{
    if (maxDepth != FunctionNode::kInfiniteCallDepth) {
        // Check all functions because the driver may fail on them
        // TODO: Before detectingRecursion, strip unused functions.
        for (size_t i = 0; i < functions.size(); ++i) {
            ErrorCode error = detectCallDepthForFunction(functions[i]);
            if (error != kErrorNone)
                return error;
        }
    } else {
        FunctionNode* main = findFunctionByName("main(");
        if (main == NULL)
            return kErrorMissingMain;

        return detectCallDepthForFunction(main);
    }

    return kErrorNone;
}

DetectCallDepth::FunctionNode* DetectCallDepth::findFunctionByName(
    const TString& name)
{
    for (size_t i = 0; i < functions.size(); ++i) {
        if (functions[i]->getName() == name)
            return functions[i];
    }
    return NULL;
}

