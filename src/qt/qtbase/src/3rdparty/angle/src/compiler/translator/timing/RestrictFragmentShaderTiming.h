//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TIMING_RESTRICT_FRAGMENT_SHADER_TIMING_H_
#define COMPILER_TIMING_RESTRICT_FRAGMENT_SHADER_TIMING_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/translator/intermediate.h"
#include "compiler/translator/depgraph/DependencyGraph.h"

class TInfoSinkBase;

class RestrictFragmentShaderTiming : TDependencyGraphTraverser {
public:
    RestrictFragmentShaderTiming(TInfoSinkBase& sink);
    void enforceRestrictions(const TDependencyGraph& graph);
    int numErrors() const { return mNumErrors; }

    virtual void visitArgument(TGraphArgument* parameter);
    virtual void visitSelection(TGraphSelection* selection);
    virtual void visitLoop(TGraphLoop* loop);
    virtual void visitLogicalOp(TGraphLogicalOp* logicalOp);

private:
    void beginError(const TIntermNode* node);
    void validateUserDefinedFunctionCallUsage(const TDependencyGraph& graph);
    bool isSamplingOp(const TIntermAggregate* intermFunctionCall) const;

    TInfoSinkBase& mSink;
    int mNumErrors;

    typedef std::set<TString> StringSet;
    StringSet mSamplingOps;
};

#endif  // COMPILER_TIMING_RESTRICT_FRAGMENT_SHADER_TIMING_H_
