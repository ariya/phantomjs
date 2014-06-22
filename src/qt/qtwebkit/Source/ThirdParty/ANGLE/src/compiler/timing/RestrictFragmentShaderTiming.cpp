//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/InfoSink.h"
#include "compiler/ParseHelper.h"
#include "compiler/depgraph/DependencyGraphOutput.h"
#include "compiler/timing/RestrictFragmentShaderTiming.h"

RestrictFragmentShaderTiming::RestrictFragmentShaderTiming(TInfoSinkBase& sink)
    : mSink(sink)
    , mNumErrors(0)
{
    // Sampling ops found only in fragment shaders.
    mSamplingOps.insert("texture2D(s21;vf2;f1;");
    mSamplingOps.insert("texture2DProj(s21;vf3;f1;");
    mSamplingOps.insert("texture2DProj(s21;vf4;f1;");
    mSamplingOps.insert("textureCube(sC1;vf3;f1;");
    // Sampling ops found in both vertex and fragment shaders.
    mSamplingOps.insert("texture2D(s21;vf2;");
    mSamplingOps.insert("texture2DProj(s21;vf3;");
    mSamplingOps.insert("texture2DProj(s21;vf4;");
    mSamplingOps.insert("textureCube(sC1;vf3;");
    // Sampling ops provided by OES_EGL_image_external.
    mSamplingOps.insert("texture2D(1;vf2;");
    mSamplingOps.insert("texture2DProj(1;vf3;");
    mSamplingOps.insert("texture2DProj(1;vf4;");
    // Sampling ops provided by ARB_texture_rectangle.
    mSamplingOps.insert("texture2DRect(1;vf2;");
    mSamplingOps.insert("texture2DRectProj(1;vf3;");
    mSamplingOps.insert("texture2DRectProj(1;vf4;");
}

// FIXME(mvujovic): We do not know if the execution time of built-in operations like sin, pow, etc.
// can vary based on the value of the input arguments. If so, we should restrict those as well.
void RestrictFragmentShaderTiming::enforceRestrictions(const TDependencyGraph& graph)
{
    mNumErrors = 0;

    // FIXME(mvujovic): The dependency graph does not support user defined function calls right now,
    // so we generate errors for them.
    validateUserDefinedFunctionCallUsage(graph);

    // Starting from each sampler, traverse the dependency graph and generate an error each time we
    // hit a node where sampler dependent values are not allowed.
    for (TGraphSymbolVector::const_iterator iter = graph.beginSamplerSymbols();
         iter != graph.endSamplerSymbols();
         ++iter)
    {
        TGraphSymbol* samplerSymbol = *iter;
        clearVisited();
        samplerSymbol->traverse(this);
    }
}

void RestrictFragmentShaderTiming::validateUserDefinedFunctionCallUsage(const TDependencyGraph& graph)
{
    for (TFunctionCallVector::const_iterator iter = graph.beginUserDefinedFunctionCalls();
         iter != graph.endUserDefinedFunctionCalls();
         ++iter)
    {
        TGraphFunctionCall* functionCall = *iter;
        beginError(functionCall->getIntermFunctionCall());
        mSink << "A call to a user defined function is not permitted.\n";
    }
}

void RestrictFragmentShaderTiming::beginError(const TIntermNode* node)
{
    ++mNumErrors;
    mSink.prefix(EPrefixError);
    mSink.location(node->getLine());
}

bool RestrictFragmentShaderTiming::isSamplingOp(const TIntermAggregate* intermFunctionCall) const
{
    return !intermFunctionCall->isUserDefined() &&
           mSamplingOps.find(intermFunctionCall->getName()) != mSamplingOps.end();
}

void RestrictFragmentShaderTiming::visitArgument(TGraphArgument* parameter)
{
    // Texture cache access time might leak sensitive information.
    // Thus, we restrict sampler dependent values from affecting the coordinate or LOD bias of a
    // sampling operation.
    if (isSamplingOp(parameter->getIntermFunctionCall())) {
        switch (parameter->getArgumentNumber()) {
            case 1:
                // Second argument (coord)
                beginError(parameter->getIntermFunctionCall());
                mSink << "An expression dependent on a sampler is not permitted to be the"
                      << " coordinate argument of a sampling operation.\n";
                break;
            case 2:
                // Third argument (bias)
                beginError(parameter->getIntermFunctionCall());
                mSink << "An expression dependent on a sampler is not permitted to be the"
                      << " bias argument of a sampling operation.\n";
                break;
            default:
                // First argument (sampler)
                break;
        }
    }
}

void RestrictFragmentShaderTiming::visitSelection(TGraphSelection* selection)
{
    beginError(selection->getIntermSelection());
    mSink << "An expression dependent on a sampler is not permitted in a conditional statement.\n";
}

void RestrictFragmentShaderTiming::visitLoop(TGraphLoop* loop)
{
    beginError(loop->getIntermLoop());
    mSink << "An expression dependent on a sampler is not permitted in a loop condition.\n";
}

void RestrictFragmentShaderTiming::visitLogicalOp(TGraphLogicalOp* logicalOp)
{
    beginError(logicalOp->getIntermLogicalOp());
    mSink << "An expression dependent on a sampler is not permitted on the left hand side of a logical "
          << logicalOp->getOpString()
          << " operator.\n";
}
