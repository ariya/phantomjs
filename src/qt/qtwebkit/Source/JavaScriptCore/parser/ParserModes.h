/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef ParserModes_h
#define ParserModes_h

namespace JSC {

enum JSParserStrictness { JSParseNormal, JSParseStrict };
enum JSParserMode { JSParseProgramCode, JSParseFunctionCode };

enum ProfilerMode { ProfilerOff, ProfilerOn };
enum DebuggerMode { DebuggerOff, DebuggerOn };

enum FunctionNameIsInScopeToggle { FunctionNameIsNotInScope, FunctionNameIsInScope };

typedef unsigned CodeFeatures;

const CodeFeatures NoFeatures = 0;
const CodeFeatures EvalFeature = 1 << 0;
const CodeFeatures ArgumentsFeature = 1 << 1;
const CodeFeatures WithFeature = 1 << 2;
const CodeFeatures CatchFeature = 1 << 3;
const CodeFeatures ThisFeature = 1 << 4;
const CodeFeatures StrictModeFeature = 1 << 5;
const CodeFeatures ShadowsArgumentsFeature = 1 << 6;

const CodeFeatures AllFeatures = EvalFeature | ArgumentsFeature | WithFeature | CatchFeature | ThisFeature | StrictModeFeature | ShadowsArgumentsFeature;

} // namespace JSC

#endif // ParserModes_h
