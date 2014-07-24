/*
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "CodeCache.h"

#include "BytecodeGenerator.h"
#include "CodeSpecializationKind.h"
#include "Operations.h"
#include "Parser.h"
#include "StrongInlines.h"
#include "UnlinkedCodeBlock.h"

namespace JSC {

const double CodeCacheMap::workingSetTime = 10.0;
const int64_t CodeCacheMap::globalWorkingSetMaxBytes = 16000000;
const size_t CodeCacheMap::globalWorkingSetMaxEntries = 2000;
const unsigned CodeCacheMap::nonGlobalWorkingSetScale = 20;
const int64_t CodeCacheMap::nonGlobalWorkingSetMaxBytes = CodeCacheMap::globalWorkingSetMaxBytes / CodeCacheMap::nonGlobalWorkingSetScale;
const size_t CodeCacheMap::nonGlobalWorkingSetMaxEntries = CodeCacheMap::globalWorkingSetMaxEntries / CodeCacheMap::nonGlobalWorkingSetScale;

void CodeCacheMap::pruneSlowCase()
{
    m_minCapacity = std::max(m_size - m_sizeAtLastPrune, static_cast<int64_t>(0));
    m_sizeAtLastPrune = m_size;
    m_timeAtLastPrune = monotonicallyIncreasingTime();

    if (m_capacity < m_minCapacity)
        m_capacity = m_minCapacity;

    while (m_size > m_capacity || !canPruneQuickly()) {
        MapType::iterator it = m_map.begin();
        m_size -= it->key.length();
        m_map.remove(it);
    }
}

CodeCache::CodeCache(CodeCacheKind kind)
: m_sourceCode(kind == GlobalCodeCache ? CodeCacheMap::globalWorkingSetMaxBytes : CodeCacheMap::nonGlobalWorkingSetMaxBytes,
    kind == GlobalCodeCache ? CodeCacheMap::globalWorkingSetMaxEntries : CodeCacheMap::nonGlobalWorkingSetMaxEntries)
{
}

CodeCache::~CodeCache()
{
}

template <typename T> struct CacheTypes { };

template <> struct CacheTypes<UnlinkedProgramCodeBlock> {
    typedef JSC::ProgramNode RootNode;
    static const SourceCodeKey::CodeType codeType = SourceCodeKey::ProgramType;
};

template <> struct CacheTypes<UnlinkedEvalCodeBlock> {
    typedef JSC::EvalNode RootNode;
    static const SourceCodeKey::CodeType codeType = SourceCodeKey::EvalType;
};

template <class UnlinkedCodeBlockType, class ExecutableType>
UnlinkedCodeBlockType* CodeCache::generateBytecode(VM& vm, JSScope* scope, ExecutableType* executable, const SourceCode& source, JSParserStrictness strictness, DebuggerMode debuggerMode, ProfilerMode profilerMode, ParserError& error)
{
    typedef typename CacheTypes<UnlinkedCodeBlockType>::RootNode RootNode;
    RefPtr<RootNode> rootNode = parse<RootNode>(&vm, source, 0, Identifier(), strictness, JSParseProgramCode, error);
    if (!rootNode)
        return 0;
    executable->recordParse(rootNode->features(), rootNode->hasCapturedVariables(), rootNode->lineNo(), rootNode->lastLine(), rootNode->startColumn());

    UnlinkedCodeBlockType* unlinkedCode = UnlinkedCodeBlockType::create(&vm, executable->executableInfo());
    unlinkedCode->recordParse(rootNode->features(), rootNode->hasCapturedVariables(), rootNode->lineNo() - source.firstLine(), rootNode->lastLine() - rootNode->lineNo());
    OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(vm, scope, rootNode.get(), unlinkedCode, debuggerMode, profilerMode)));
    error = generator->generate();
    rootNode->destroyData();
    if (error.m_type != ParserError::ErrorNone)
        return 0;
    return unlinkedCode;
}

template <class UnlinkedCodeBlockType, class ExecutableType>
UnlinkedCodeBlockType* CodeCache::getCodeBlock(VM& vm, JSScope* scope, ExecutableType* executable, const SourceCode& source, JSParserStrictness strictness, DebuggerMode debuggerMode, ProfilerMode profilerMode, ParserError& error)
{
    // We completely skip the cache if we're an eval that isn't at the top of the scope chain.
    if (CacheTypes<UnlinkedCodeBlockType>::codeType == SourceCodeKey::EvalType) {
        if (scope->next() && !scope->isActivationObject())
            return generateBytecode<UnlinkedCodeBlockType, ExecutableType>(vm, scope, executable, source, strictness, debuggerMode, profilerMode, error);
    }

    SourceCodeKey key = SourceCodeKey(source, String(), CacheTypes<UnlinkedCodeBlockType>::codeType, strictness);
    CodeCacheMap::AddResult addResult = m_sourceCode.add(key, SourceCodeValue());
    bool canCache = debuggerMode == DebuggerOff && profilerMode == ProfilerOff;

    if (!addResult.isNewEntry && canCache) {
        UnlinkedCodeBlockType* unlinkedCode = jsCast<UnlinkedCodeBlockType*>(addResult.iterator->value.cell.get());
        unsigned firstLine = source.firstLine() + unlinkedCode->firstLine();
        unsigned startColumn = source.firstLine() ? source.startColumn() : 0;
        executable->recordParse(unlinkedCode->codeFeatures(), unlinkedCode->hasCapturedVariables(), firstLine, firstLine + unlinkedCode->lineCount(), startColumn);
        return unlinkedCode;
    }
    UnlinkedCodeBlockType* unlinkedCode = generateBytecode<UnlinkedCodeBlockType, ExecutableType>(vm, scope, executable, source, strictness, debuggerMode, profilerMode, error);

    if (!canCache || !unlinkedCode) {
        m_sourceCode.remove(addResult.iterator);
        return unlinkedCode;
    }

    addResult.iterator->value = SourceCodeValue(vm, unlinkedCode, m_sourceCode.age());
    return unlinkedCode;
}

UnlinkedProgramCodeBlock* CodeCache::getProgramCodeBlock(VM& vm, ProgramExecutable* executable, const SourceCode& source, JSParserStrictness strictness, DebuggerMode debuggerMode, ProfilerMode profilerMode, ParserError& error)
{
    return getCodeBlock<UnlinkedProgramCodeBlock>(vm, 0, executable, source, strictness, debuggerMode, profilerMode, error);
}

UnlinkedEvalCodeBlock* CodeCache::getEvalCodeBlock(VM& vm, JSScope* scope, EvalExecutable* executable, const SourceCode& source, JSParserStrictness strictness, DebuggerMode debuggerMode, ProfilerMode profilerMode, ParserError& error)
{
    return getCodeBlock<UnlinkedEvalCodeBlock>(vm, scope, executable, source, strictness, debuggerMode, profilerMode, error);
}

UnlinkedFunctionExecutable* CodeCache::getFunctionExecutableFromGlobalCode(VM& vm, const Identifier& name, const SourceCode& source, ParserError& error)
{
    SourceCodeKey key = SourceCodeKey(source, name.string(), SourceCodeKey::FunctionType, JSParseNormal);
    CodeCacheMap::AddResult addResult = m_sourceCode.add(key, SourceCodeValue());
    if (!addResult.isNewEntry)
        return jsCast<UnlinkedFunctionExecutable*>(addResult.iterator->value.cell.get());

    RefPtr<ProgramNode> program = parse<ProgramNode>(&vm, source, 0, Identifier(), JSParseNormal, JSParseProgramCode, error);
    if (!program) {
        ASSERT(error.m_type != ParserError::ErrorNone);
        m_sourceCode.remove(addResult.iterator);
        return 0;
    }

    // This function assumes an input string that would result in a single anonymous function expression.
    StatementNode* exprStatement = program->singleStatement();
    ASSERT(exprStatement);
    ASSERT(exprStatement->isExprStatement());
    ExpressionNode* funcExpr = static_cast<ExprStatementNode*>(exprStatement)->expr();
    ASSERT(funcExpr);
    RELEASE_ASSERT(funcExpr->isFuncExprNode());
    FunctionBodyNode* body = static_cast<FuncExprNode*>(funcExpr)->body();
    ASSERT(body);
    ASSERT(body->ident().isNull());

    UnlinkedFunctionExecutable* functionExecutable = UnlinkedFunctionExecutable::create(&vm, source, body);
    functionExecutable->m_nameValue.set(vm, functionExecutable, jsString(&vm, name.string()));

    addResult.iterator->value = SourceCodeValue(vm, functionExecutable, m_sourceCode.age());
    return functionExecutable;
}

}
