/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"

#include "JSParser.h"

using namespace JSC;

#include "CodeBlock.h"
#include "JSGlobalData.h"
#include "NodeInfo.h"
#include "ASTBuilder.h"
#include "SourceProvider.h"
#include "SourceProviderCacheItem.h"
#include <wtf/HashFunctions.h>
#include <wtf/OwnPtr.h>
#include <wtf/WTFThreadData.h>
#include <utility>

using namespace std;

namespace JSC {
#define fail() do { m_error = true; return 0; } while (0)
#define failIfFalse(cond) do { if (!(cond)) fail(); } while (0)
#define failIfTrue(cond) do { if ((cond)) fail(); } while (0)
#define failIfTrueIfStrict(cond) do { if ((cond) && strictMode()) fail(); } while (0)
#define failIfFalseIfStrict(cond) do { if ((!(cond)) && strictMode()) fail(); } while (0)
#define consumeOrFail(tokenType) do { if (!consume(tokenType)) fail(); } while (0)
#define matchOrFail(tokenType) do { if (!match(tokenType)) fail(); } while (0)
#define failIfStackOverflow() do { failIfFalse(canRecurse()); } while (0)

// Macros to make the more common TreeBuilder types a little less verbose
#define TreeStatement typename TreeBuilder::Statement
#define TreeExpression typename TreeBuilder::Expression
#define TreeFormalParameterList typename TreeBuilder::FormalParameterList
#define TreeSourceElements typename TreeBuilder::SourceElements
#define TreeClause typename TreeBuilder::Clause
#define TreeClauseList typename TreeBuilder::ClauseList
#define TreeConstDeclList typename TreeBuilder::ConstDeclList
#define TreeArguments typename TreeBuilder::Arguments
#define TreeArgumentsList typename TreeBuilder::ArgumentsList
#define TreeFunctionBody typename TreeBuilder::FunctionBody
#define TreeProperty typename TreeBuilder::Property
#define TreePropertyList typename TreeBuilder::PropertyList

COMPILE_ASSERT(LastUntaggedToken < 64, LessThan64UntaggedTokens);

class JSParser {
public:
    JSParser(Lexer*, JSGlobalData*, FunctionParameters*, bool isStrictContext, bool isFunction, SourceProvider*);
    const char* parseProgram();
private:
    struct AllowInOverride {
        AllowInOverride(JSParser* parser)
            : m_parser(parser)
            , m_oldAllowsIn(parser->m_allowsIn)
        {
            parser->m_allowsIn = true;
        }
        ~AllowInOverride()
        {
            m_parser->m_allowsIn = m_oldAllowsIn;
        }
        JSParser* m_parser;
        bool m_oldAllowsIn;
    };
    
    struct ScopeLabelInfo {
        ScopeLabelInfo(StringImpl* ident, bool isLoop)
        : m_ident(ident)
        , m_isLoop(isLoop)
        {
        }
        StringImpl* m_ident;
        bool m_isLoop;
    };
    
    void next(Lexer::LexType lexType = Lexer::IdentifyReservedWords)
    {
        m_lastLine = m_token.m_info.line;
        m_lastTokenEnd = m_token.m_info.endOffset;
        m_lexer->setLastLineNumber(m_lastLine);
        m_token.m_type = m_lexer->lex(&m_token.m_data, &m_token.m_info, lexType, strictMode());
    }
    
    bool nextTokenIsColon()
    {
        return m_lexer->nextTokenIsColon();
    }

    bool consume(JSTokenType expected)
    {
        bool result = m_token.m_type == expected;
        failIfFalse(result);
        next();
        return result;
    }

    bool match(JSTokenType expected)
    {
        return m_token.m_type == expected;
    }

    int tokenStart()
    {
        return m_token.m_info.startOffset;
    }

    int tokenLine()
    {
        return m_token.m_info.line;
    }

    int tokenEnd()
    {
        return m_token.m_info.endOffset;
    }
    
    void startLoop() { currentScope()->startLoop(); }
    void endLoop() { currentScope()->endLoop(); }
    void startSwitch() { currentScope()->startSwitch(); }
    void endSwitch() { currentScope()->endSwitch(); }
    void setStrictMode() { currentScope()->setStrictMode(); }
    bool strictMode() { return currentScope()->strictMode(); }
    bool isValidStrictMode() { return currentScope()->isValidStrictMode(); }
    bool declareParameter(const Identifier* ident) { return currentScope()->declareParameter(ident); }
    bool breakIsValid()
    {
        ScopeRef current = currentScope();
        while (!current->breakIsValid()) {
            if (!current.hasContainingScope())
                return false;
            current = current.containingScope();
        }
        return true;
    }
    bool continueIsValid()
    {
        ScopeRef current = currentScope();
        while (!current->continueIsValid()) {
            if (!current.hasContainingScope())
                return false;
            current = current.containingScope();
        }
        return true;
    }
    void pushLabel(const Identifier* label, bool isLoop) { currentScope()->pushLabel(label, isLoop); }
    void popLabel() { currentScope()->popLabel(); }
    ScopeLabelInfo* getLabel(const Identifier* label)
    {
        ScopeRef current = currentScope();
        ScopeLabelInfo* result = 0;
        while (!(result = current->getLabel(label))) {
            if (!current.hasContainingScope())
                return 0;
            current = current.containingScope();
        }
        return result;
    }

    enum SourceElementsMode { CheckForStrictMode, DontCheckForStrictMode };
    template <SourceElementsMode mode, class TreeBuilder> TreeSourceElements parseSourceElements(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseStatement(TreeBuilder&, const Identifier*& directive);
    template <class TreeBuilder> TreeStatement parseFunctionDeclaration(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseVarDeclaration(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseConstDeclaration(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseDoWhileStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseWhileStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseForStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseBreakStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseContinueStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseReturnStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseThrowStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseWithStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseSwitchStatement(TreeBuilder&);
    template <class TreeBuilder> TreeClauseList parseSwitchClauses(TreeBuilder&);
    template <class TreeBuilder> TreeClause parseSwitchDefaultClause(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseTryStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseDebuggerStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseExpressionStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseExpressionOrLabelStatement(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseIfStatement(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeStatement parseBlockStatement(TreeBuilder&);
    template <class TreeBuilder> TreeExpression parseExpression(TreeBuilder&);
    template <class TreeBuilder> TreeExpression parseAssignmentExpression(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseConditionalExpression(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseBinaryExpression(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseUnaryExpression(TreeBuilder&);
    template <class TreeBuilder> TreeExpression parseMemberExpression(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parsePrimaryExpression(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseArrayLiteral(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseObjectLiteral(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseStrictObjectLiteral(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeArguments parseArguments(TreeBuilder&);
#if COMPILER(WINSCW)
    template <bool strict, class TreeBuilder> ALWAYS_INLINE TreeProperty parseProperty(bool, TreeBuilder&);
#else
    template <bool strict, class TreeBuilder> ALWAYS_INLINE TreeProperty parseProperty(TreeBuilder&);
#endif
    template <class TreeBuilder> ALWAYS_INLINE TreeFunctionBody parseFunctionBody(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeFormalParameterList parseFormalParameters(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseVarDeclarationList(TreeBuilder&, int& declarations, const Identifier*& lastIdent, TreeExpression& lastInitializer, int& identStart, int& initStart, int& initEnd);
    template <class TreeBuilder> ALWAYS_INLINE TreeConstDeclList parseConstDeclarationList(TreeBuilder& context);
    enum FunctionRequirements { FunctionNoRequirements, FunctionNeedsName };
#if COMPILER(WINSCW)
    template <FunctionRequirements, bool nameIsInContainingScope, class TreeBuilder> bool parseFunctionInfo(FunctionRequirements, bool, TreeBuilder&, const Identifier*&, TreeFormalParameterList&, TreeFunctionBody&, int& openBrace, int& closeBrace, int& bodyStartLine);
#else
    template <FunctionRequirements, bool nameIsInContainingScope, class TreeBuilder> bool parseFunctionInfo(TreeBuilder&, const Identifier*&, TreeFormalParameterList&, TreeFunctionBody&, int& openBrace, int& closeBrace, int& bodyStartLine);
#endif
    ALWAYS_INLINE int isBinaryOperator(JSTokenType token);
    bool allowAutomaticSemicolon();

    bool autoSemiColon()
    {
        if (m_token.m_type == SEMICOLON) {
            next();
            return true;
        }
        return allowAutomaticSemicolon();
    }

    bool canRecurse()
    {
        return m_stack.recursionCheck();
    }
    
    int lastTokenEnd() const
    {
        return m_lastTokenEnd;
    }

    ParserArena m_arena;
    Lexer* m_lexer;
    StackBounds m_stack;
    bool m_error;
    const char* m_errorMessage;
    JSGlobalData* m_globalData;
    JSToken m_token;
    bool m_allowsIn;
    int m_lastLine;
    int m_lastTokenEnd;
    int m_assignmentCount;
    int m_nonLHSCount;
    bool m_syntaxAlreadyValidated;
    int m_statementDepth;
    int m_nonTrivialExpressionCount;
    const Identifier* m_lastIdentifier;

    struct DepthManager {
        DepthManager(int* depth)
            : m_originalDepth(*depth)
            , m_depth(depth)
        {
        }
        
        ~DepthManager()
        {
            *m_depth = m_originalDepth;
        }
        
    private:
        int m_originalDepth;
        int* m_depth;
    };
    
    struct Scope {
        Scope(JSGlobalData* globalData, bool isFunction, bool strictMode)
            : m_globalData(globalData)
            , m_shadowsArguments(false)
            , m_usesEval(false)
            , m_needsFullActivation(false)
            , m_allowsNewDecls(true)
            , m_strictMode(strictMode)
            , m_isFunction(isFunction)
            , m_isFunctionBoundary(false)
            , m_isValidStrictMode(true)
            , m_loopDepth(0)
            , m_switchDepth(0)
        {
        }

        Scope(const Scope& rhs)
            : m_globalData(rhs.m_globalData)
            , m_shadowsArguments(rhs.m_shadowsArguments)
            , m_usesEval(rhs.m_usesEval)
            , m_needsFullActivation(rhs.m_needsFullActivation)
            , m_allowsNewDecls(rhs.m_allowsNewDecls)
            , m_strictMode(rhs.m_strictMode)
            , m_isFunction(rhs.m_isFunction)
            , m_isFunctionBoundary(rhs.m_isFunctionBoundary)
            , m_isValidStrictMode(rhs.m_isValidStrictMode)
            , m_loopDepth(rhs.m_loopDepth)
            , m_switchDepth(rhs.m_switchDepth)
        {
            if (rhs.m_labels) {
                m_labels = adoptPtr(new LabelStack);
                
                typedef LabelStack::const_iterator iterator;
                iterator end = rhs.m_labels->end();
                for (iterator it = rhs.m_labels->begin(); it != end; ++it)
                    m_labels->append(ScopeLabelInfo(it->m_ident, it->m_isLoop));
            }
        }

        void startSwitch() { m_switchDepth++; }
        void endSwitch() { m_switchDepth--; }
        void startLoop() { m_loopDepth++; }
        void endLoop() { ASSERT(m_loopDepth); m_loopDepth--; }
        bool inLoop() { return !!m_loopDepth; }
        bool breakIsValid() { return m_loopDepth || m_switchDepth; }
        bool continueIsValid() { return m_loopDepth; }

        void pushLabel(const Identifier* label, bool isLoop)
        {
            if (!m_labels)
                m_labels = adoptPtr(new LabelStack);
            m_labels->append(ScopeLabelInfo(label->impl(), isLoop));
        }

        void popLabel()
        {
            ASSERT(m_labels);
            ASSERT(m_labels->size());
            m_labels->removeLast();
        }

        ScopeLabelInfo* getLabel(const Identifier* label)
        {
            if (!m_labels)
                return 0;
            for (int i = m_labels->size(); i > 0; i--) {
                if (m_labels->at(i - 1).m_ident == label->impl())
                    return &m_labels->at(i - 1);
            }
            return 0;
        }

        void setIsFunction()
        {
            m_isFunction = true;
            m_isFunctionBoundary = true;
        }
        bool isFunction() { return m_isFunction; }
        bool isFunctionBoundary() { return m_isFunctionBoundary; }
        
        bool declareVariable(const Identifier* ident)
        {
            bool isValidStrictMode = m_globalData->propertyNames->eval != *ident && m_globalData->propertyNames->arguments != *ident;
            m_isValidStrictMode = m_isValidStrictMode && isValidStrictMode;
            m_declaredVariables.add(ident->ustring().impl());
            return isValidStrictMode;
        }
        
        void declareWrite(const Identifier* ident)
        {
            ASSERT(m_strictMode);
            m_writtenVariables.add(ident->impl());
        }

        void preventNewDecls() { m_allowsNewDecls = false; }
        bool allowsNewDecls() const { return m_allowsNewDecls; }

        bool declareParameter(const Identifier* ident)
        {
            bool isArguments = m_globalData->propertyNames->arguments == *ident;
            bool isValidStrictMode = m_declaredVariables.add(ident->ustring().impl()).second && m_globalData->propertyNames->eval != *ident && !isArguments;
            m_isValidStrictMode = m_isValidStrictMode && isValidStrictMode;
            if (isArguments)
                m_shadowsArguments = true;
            return isValidStrictMode;
        }
        
        void useVariable(const Identifier* ident, bool isEval)
        {
            m_usesEval |= isEval;
            m_usedVariables.add(ident->ustring().impl());
        }
        
        void setNeedsFullActivation() { m_needsFullActivation = true; }
        
        bool collectFreeVariables(Scope* nestedScope, bool shouldTrackClosedVariables)
        {
            if (nestedScope->m_usesEval)
                m_usesEval = true;
            IdentifierSet::iterator end = nestedScope->m_usedVariables.end();
            for (IdentifierSet::iterator ptr = nestedScope->m_usedVariables.begin(); ptr != end; ++ptr) {
                if (nestedScope->m_declaredVariables.contains(*ptr))
                    continue;
                m_usedVariables.add(*ptr);
                if (shouldTrackClosedVariables)
                    m_closedVariables.add(*ptr);
            }
            if (nestedScope->m_writtenVariables.size()) {
                IdentifierSet::iterator end = nestedScope->m_writtenVariables.end();
                for (IdentifierSet::iterator ptr = nestedScope->m_writtenVariables.begin(); ptr != end; ++ptr) {
                    if (nestedScope->m_declaredVariables.contains(*ptr))
                        continue;
                    m_writtenVariables.add(*ptr);
                }
            }

            return true;
        }

        void getUncapturedWrittenVariables(IdentifierSet& writtenVariables)
        {
            IdentifierSet::iterator end = m_writtenVariables.end();
            for (IdentifierSet::iterator ptr = m_writtenVariables.begin(); ptr != end; ++ptr) {
                if (!m_declaredVariables.contains(*ptr))
                    writtenVariables.add(*ptr);
            }
        }

        void getCapturedVariables(IdentifierSet& capturedVariables)
        {
            if (m_needsFullActivation || m_usesEval) {
                capturedVariables.swap(m_declaredVariables);
                return;
            }
            for (IdentifierSet::iterator ptr = m_closedVariables.begin(); ptr != m_closedVariables.end(); ++ptr) {
                if (!m_declaredVariables.contains(*ptr))
                    continue;
                capturedVariables.add(*ptr);
            }
        }
        void setStrictMode() { m_strictMode = true; }
        bool strictMode() const { return m_strictMode; }
        bool isValidStrictMode() const { return m_isValidStrictMode; }
        bool shadowsArguments() const { return m_shadowsArguments; }
        
        void copyCapturedVariablesToVector(const IdentifierSet& capturedVariables, Vector<RefPtr<StringImpl> >& vector)
        {
            IdentifierSet::iterator end = capturedVariables.end();
            for (IdentifierSet::iterator it = capturedVariables.begin(); it != end; ++it) {
                if (m_declaredVariables.contains(*it))
                    continue;
                vector.append(*it);
            }
            vector.shrinkToFit();
        }

        void saveFunctionInfo(SourceProviderCacheItem* info)
        {
            ASSERT(m_isFunction);
            info->usesEval = m_usesEval;
            copyCapturedVariablesToVector(m_writtenVariables, info->writtenVariables);
            copyCapturedVariablesToVector(m_usedVariables, info->usedVariables);
        }

        void restoreFunctionInfo(const SourceProviderCacheItem* info)
        {
            ASSERT(m_isFunction);
            m_usesEval = info->usesEval;
            unsigned size = info->usedVariables.size();
            for (unsigned i = 0; i < size; ++i)
                m_usedVariables.add(info->usedVariables[i]);
            size = info->writtenVariables.size();
            for (unsigned i = 0; i < size; ++i)
                m_writtenVariables.add(info->writtenVariables[i]);
        }

    private:
        JSGlobalData* m_globalData;
        bool m_shadowsArguments : 1;
        bool m_usesEval : 1;
        bool m_needsFullActivation : 1;
        bool m_allowsNewDecls : 1;
        bool m_strictMode : 1;
        bool m_isFunction : 1;
        bool m_isFunctionBoundary : 1;
        bool m_isValidStrictMode : 1;
        int m_loopDepth;
        int m_switchDepth;

        typedef Vector<ScopeLabelInfo, 2> LabelStack;
        OwnPtr<LabelStack> m_labels;
        IdentifierSet m_declaredVariables;
        IdentifierSet m_usedVariables;
        IdentifierSet m_closedVariables;
        IdentifierSet m_writtenVariables;
    };

    typedef Vector<Scope, 10> ScopeStack;

    struct ScopeRef {
        ScopeRef(ScopeStack* scopeStack, unsigned index)
            : m_scopeStack(scopeStack)
            , m_index(index)
        {
        }
        Scope* operator->() { return &m_scopeStack->at(m_index); }
        unsigned index() const { return m_index; }

        bool hasContainingScope()
        {
            return m_index && !m_scopeStack->at(m_index).isFunctionBoundary();
        }

        ScopeRef containingScope()
        {
            ASSERT(hasContainingScope());
            return ScopeRef(m_scopeStack, m_index - 1);
        }

    private:
        ScopeStack* m_scopeStack;
        unsigned m_index;
    };

    struct AutoPopScopeRef : public ScopeRef {
        AutoPopScopeRef(JSParser* parser, ScopeRef scope)
            : ScopeRef(scope)
            , m_parser(parser)
        {
        }

        ~AutoPopScopeRef()
        {
            if (m_parser)
                m_parser->popScope(*this, false);
        }

        void setPopped()
        {
            m_parser = 0;
        }

    private:
        JSParser* m_parser;
    };

    ScopeRef currentScope()
    {
        return ScopeRef(&m_scopeStack, m_scopeStack.size() - 1);
    }
    
    ScopeRef pushScope()
    {
        bool isFunction = false;
        bool isStrict = false;
        if (!m_scopeStack.isEmpty()) {
            isStrict = m_scopeStack.last().strictMode();
            isFunction = m_scopeStack.last().isFunction();
        }
        m_scopeStack.append(Scope(m_globalData, isFunction, isStrict));
        return currentScope();
    }

    bool popScopeInternal(ScopeRef& scope, bool shouldTrackClosedVariables)
    {
        ASSERT_UNUSED(scope, scope.index() == m_scopeStack.size() - 1);
        ASSERT(m_scopeStack.size() > 1);
        bool result = m_scopeStack[m_scopeStack.size() - 2].collectFreeVariables(&m_scopeStack.last(), shouldTrackClosedVariables);
        m_scopeStack.removeLast();
        return result;
    }

    bool popScope(ScopeRef& scope, bool shouldTrackClosedVariables)
    {
        return popScopeInternal(scope, shouldTrackClosedVariables);
    }

    bool popScope(AutoPopScopeRef& scope, bool shouldTrackClosedVariables)
    {
        scope.setPopped();
        return popScopeInternal(scope, shouldTrackClosedVariables);
    }

    bool declareVariable(const Identifier* ident)
    {
        unsigned i = m_scopeStack.size() - 1;
        ASSERT(i < m_scopeStack.size());
        while (!m_scopeStack[i].allowsNewDecls()) {
            i--;
            ASSERT(i < m_scopeStack.size());
        }
        return m_scopeStack[i].declareVariable(ident);
    }
    
    void declareWrite(const Identifier* ident)
    {
        if (!m_syntaxAlreadyValidated)
            m_scopeStack.last().declareWrite(ident);
    }

    ScopeStack m_scopeStack;

    const SourceProviderCacheItem* findCachedFunctionInfo(int openBracePos) 
    {
        return m_functionCache ? m_functionCache->get(openBracePos) : 0;
    }

    SourceProviderCache* m_functionCache;
};

const char* jsParse(JSGlobalData* globalData, FunctionParameters* parameters, JSParserStrictness strictness, JSParserMode parserMode, const SourceCode* source)
{
    JSParser parser(globalData->lexer, globalData, parameters, strictness == JSParseStrict, parserMode == JSParseFunctionCode, source->provider());
    return parser.parseProgram();
}

JSParser::JSParser(Lexer* lexer, JSGlobalData* globalData, FunctionParameters* parameters, bool inStrictContext, bool isFunction, SourceProvider* provider)
    : m_lexer(lexer)
    , m_stack(globalData->stack())
    , m_error(false)
    , m_errorMessage("Parse error")
    , m_globalData(globalData)
    , m_allowsIn(true)
    , m_lastLine(0)
    , m_lastTokenEnd(0)
    , m_assignmentCount(0)
    , m_nonLHSCount(0)
    , m_syntaxAlreadyValidated(provider->isValid())
    , m_statementDepth(0)
    , m_nonTrivialExpressionCount(0)
    , m_lastIdentifier(0)
    , m_functionCache(m_lexer->sourceProvider()->cache())
{
    ScopeRef scope = pushScope();
    if (isFunction)
        scope->setIsFunction();
    if (inStrictContext)
        scope->setStrictMode();
    if (parameters) {
        for (unsigned i = 0; i < parameters->size(); i++)
            scope->declareParameter(&parameters->at(i));
    }
    next();
    m_lexer->setLastLineNumber(tokenLine());
}

const char* JSParser::parseProgram()
{
    unsigned oldFunctionCacheSize = m_functionCache ? m_functionCache->byteSize() : 0;
    ASTBuilder context(m_globalData, m_lexer);
    if (m_lexer->isReparsing())
        m_statementDepth--;
    ScopeRef scope = currentScope();
    SourceElements* sourceElements = parseSourceElements<CheckForStrictMode>(context);
    if (!sourceElements || !consume(EOFTOK))
        return m_errorMessage;
    IdentifierSet capturedVariables;
    scope->getCapturedVariables(capturedVariables);
    CodeFeatures features = context.features();
    if (scope->strictMode())
        features |= StrictModeFeature;
    if (scope->shadowsArguments())
        features |= ShadowsArgumentsFeature;
    
    unsigned functionCacheSize = m_functionCache ? m_functionCache->byteSize() : 0;
    if (functionCacheSize != oldFunctionCacheSize)
        m_lexer->sourceProvider()->notifyCacheSizeChanged(functionCacheSize - oldFunctionCacheSize);

    m_globalData->parser->didFinishParsing(sourceElements, context.varDeclarations(), context.funcDeclarations(), features,
                                           m_lastLine, context.numConstants(), capturedVariables);
    return 0;
}

bool JSParser::allowAutomaticSemicolon()
{
    return match(CLOSEBRACE) || match(EOFTOK) || m_lexer->prevTerminator();
}

template <JSParser::SourceElementsMode mode, class TreeBuilder> TreeSourceElements JSParser::parseSourceElements(TreeBuilder& context)
{
    TreeSourceElements sourceElements = context.createSourceElements();
    bool seenNonDirective = false;
    const Identifier* directive = 0;
    unsigned startOffset = m_token.m_info.startOffset;
    unsigned oldLastLineNumber = m_lexer->lastLineNumber();
    unsigned oldLineNumber = m_lexer->lineNumber();
    bool hasSetStrict = false;
    while (TreeStatement statement = parseStatement(context, directive)) {
        if (mode == CheckForStrictMode && !seenNonDirective) {
            if (directive) {
                if (!hasSetStrict && m_globalData->propertyNames->useStrictIdentifier == *directive) {
                    setStrictMode();
                    hasSetStrict = true;
                    failIfFalse(isValidStrictMode());
                    m_lexer->setOffset(startOffset);
                    next();
                    m_lexer->setLastLineNumber(oldLastLineNumber);
                    m_lexer->setLineNumber(oldLineNumber);
                    failIfTrue(m_error);
                    continue;
                }
            } else
                seenNonDirective = true;
        }
        context.appendStatement(sourceElements, statement);
    }

    if (m_error)
        fail();
    return sourceElements;
}

template <class TreeBuilder> TreeStatement JSParser::parseVarDeclaration(TreeBuilder& context)
{
    ASSERT(match(VAR));
    int start = tokenLine();
    int end = 0;
    int scratch;
    const Identifier* scratch1 = 0;
    TreeExpression scratch2 = 0;
    int scratch3 = 0;
    TreeExpression varDecls = parseVarDeclarationList(context, scratch, scratch1, scratch2, scratch3, scratch3, scratch3);
    failIfTrue(m_error);
    failIfFalse(autoSemiColon());

    return context.createVarStatement(varDecls, start, end);
}

template <class TreeBuilder> TreeStatement JSParser::parseConstDeclaration(TreeBuilder& context)
{
    ASSERT(match(CONSTTOKEN));
    int start = tokenLine();
    int end = 0;
    TreeConstDeclList constDecls = parseConstDeclarationList(context);
    failIfTrue(m_error);
    failIfFalse(autoSemiColon());
    
    return context.createConstStatement(constDecls, start, end);
}

template <class TreeBuilder> TreeStatement JSParser::parseDoWhileStatement(TreeBuilder& context)
{
    ASSERT(match(DO));
    int startLine = tokenLine();
    next();
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement);
    int endLine = tokenLine();
    consumeOrFail(WHILE);
    consumeOrFail(OPENPAREN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    consumeOrFail(CLOSEPAREN);
    if (match(SEMICOLON))
        next(); // Always performs automatic semicolon insertion.
    return context.createDoWhileStatement(statement, expr, startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseWhileStatement(TreeBuilder& context)
{
    ASSERT(match(WHILE));
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement);
    return context.createWhileStatement(expr, statement, startLine, endLine);
}

template <class TreeBuilder> TreeExpression JSParser::parseVarDeclarationList(TreeBuilder& context, int& declarations, const Identifier*& lastIdent, TreeExpression& lastInitializer, int& identStart, int& initStart, int& initEnd)
{
    TreeExpression varDecls = 0;
    do {
        declarations++;
        next();
        matchOrFail(IDENT);

        int varStart = tokenStart();
        identStart = varStart;
        const Identifier* name = m_token.m_data.ident;
        lastIdent = name;
        next();
        bool hasInitializer = match(EQUAL);
        failIfFalseIfStrict(declareVariable(name));
        context.addVar(name, (hasInitializer || (!m_allowsIn && match(INTOKEN))) ? DeclarationStacks::HasInitializer : 0);
        if (hasInitializer) {
            int varDivot = tokenStart() + 1;
            initStart = tokenStart();
            next(); // consume '='
            int initialAssignments = m_assignmentCount;
            TreeExpression initializer = parseAssignmentExpression(context);
            initEnd = lastTokenEnd();
            lastInitializer = initializer;
            failIfFalse(initializer);

            TreeExpression node = context.createAssignResolve(*name, initializer, initialAssignments != m_assignmentCount, varStart, varDivot, lastTokenEnd());
            if (!varDecls)
                varDecls = node;
            else
                varDecls = context.combineCommaNodes(varDecls, node);
        }
    } while (match(COMMA));
    return varDecls;
}

template <class TreeBuilder> TreeConstDeclList JSParser::parseConstDeclarationList(TreeBuilder& context)
{
    failIfTrue(strictMode());
    TreeConstDeclList constDecls = 0;
    TreeConstDeclList tail = 0;
    do {
        next();
        matchOrFail(IDENT);
        const Identifier* name = m_token.m_data.ident;
        next();
        bool hasInitializer = match(EQUAL);
        declareVariable(name);
        context.addVar(name, DeclarationStacks::IsConstant | (hasInitializer ? DeclarationStacks::HasInitializer : 0));
        TreeExpression initializer = 0;
        if (hasInitializer) {
            next(); // consume '='
            initializer = parseAssignmentExpression(context);
        }
        tail = context.appendConstDecl(tail, name, initializer);
        if (!constDecls)
            constDecls = tail;
    } while (match(COMMA));
    return constDecls;
}

template <class TreeBuilder> TreeStatement JSParser::parseForStatement(TreeBuilder& context)
{
    ASSERT(match(FOR));
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    int nonLHSCount = m_nonLHSCount;
    int declarations = 0;
    int declsStart = 0;
    int declsEnd = 0;
    TreeExpression decls = 0;
    bool hasDeclaration = false;
    if (match(VAR)) {
        /*
         for (var IDENT in expression) statement
         for (var IDENT = expression in expression) statement
         for (var varDeclarationList; expressionOpt; expressionOpt)
         */
        hasDeclaration = true;
        const Identifier* forInTarget = 0;
        TreeExpression forInInitializer = 0;
        m_allowsIn = false;
        int initStart = 0;
        int initEnd = 0;
        decls = parseVarDeclarationList(context, declarations, forInTarget, forInInitializer, declsStart, initStart, initEnd);
        m_allowsIn = true;
        if (m_error)
            fail();

        // Remainder of a standard for loop is handled identically
        if (match(SEMICOLON))
            goto standardForLoop;

        failIfFalse(declarations == 1);

        // Handle for-in with var declaration
        int inLocation = tokenStart();
        if (!consume(INTOKEN))
            fail();

        TreeExpression expr = parseExpression(context);
        failIfFalse(expr);
        int exprEnd = lastTokenEnd();

        int endLine = tokenLine();
        consumeOrFail(CLOSEPAREN);

        const Identifier* unused = 0;
        startLoop();
        TreeStatement statement = parseStatement(context, unused);
        endLoop();
        failIfFalse(statement);

        return context.createForInLoop(forInTarget, forInInitializer, expr, statement, declsStart, inLocation, exprEnd, initStart, initEnd, startLine, endLine);
    }

    if (!match(SEMICOLON)) {
        m_allowsIn = false;
        declsStart = tokenStart();
        decls = parseExpression(context);
        declsEnd = lastTokenEnd();
        m_allowsIn = true;
        failIfFalse(decls);
    }

    if (match(SEMICOLON)) {
    standardForLoop:
        // Standard for loop
        next();
        TreeExpression condition = 0;

        if (!match(SEMICOLON)) {
            condition = parseExpression(context);
            failIfFalse(condition);
        }
        consumeOrFail(SEMICOLON);

        TreeExpression increment = 0;
        if (!match(CLOSEPAREN)) {
            increment = parseExpression(context);
            failIfFalse(increment);
        }
        int endLine = tokenLine();
        consumeOrFail(CLOSEPAREN);
        const Identifier* unused = 0;
        startLoop();
        TreeStatement statement = parseStatement(context, unused);
        endLoop();
        failIfFalse(statement);
        return context.createForLoop(decls, condition, increment, statement, hasDeclaration, startLine, endLine);
    }

    // For-in loop
    failIfFalse(nonLHSCount == m_nonLHSCount);
    consumeOrFail(INTOKEN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    int exprEnd = lastTokenEnd();
    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement);
    
    return context.createForInLoop(decls, expr, statement, declsStart, declsEnd, exprEnd, startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseBreakStatement(TreeBuilder& context)
{
    ASSERT(match(BREAK));
    int startCol = tokenStart();
    int endCol = tokenEnd();
    int startLine = tokenLine();
    int endLine = tokenLine();
    next();

    if (autoSemiColon()) {
        failIfFalse(breakIsValid());
        return context.createBreakStatement(startCol, endCol, startLine, endLine);
    }
    matchOrFail(IDENT);
    const Identifier* ident = m_token.m_data.ident;
    failIfFalse(getLabel(ident));
    endCol = tokenEnd();
    endLine = tokenLine();
    next();
    failIfFalse(autoSemiColon());
    return context.createBreakStatement(ident, startCol, endCol, startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseContinueStatement(TreeBuilder& context)
{
    ASSERT(match(CONTINUE));
    int startCol = tokenStart();
    int endCol = tokenEnd();
    int startLine = tokenLine();
    int endLine = tokenLine();
    next();

    if (autoSemiColon()) {
        failIfFalse(continueIsValid());
        return context.createContinueStatement(startCol, endCol, startLine, endLine);
    }
    matchOrFail(IDENT);
    const Identifier* ident = m_token.m_data.ident;
    ScopeLabelInfo* label = getLabel(ident);
    failIfFalse(label);
    failIfFalse(label->m_isLoop);
    endCol = tokenEnd();
    endLine = tokenLine();
    next();
    failIfFalse(autoSemiColon());
    return context.createContinueStatement(ident, startCol, endCol, startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseReturnStatement(TreeBuilder& context)
{
    ASSERT(match(RETURN));
    failIfFalse(currentScope()->isFunction());
    int startLine = tokenLine();
    int endLine = startLine;
    int start = tokenStart();
    int end = tokenEnd();
    next();
    // We do the auto semicolon check before attempting to parse an expression
    // as we need to ensure the a line break after the return correctly terminates
    // the statement
    if (match(SEMICOLON))
        endLine  = tokenLine();
    if (autoSemiColon())
        return context.createReturnStatement(0, start, end, startLine, endLine);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    end = lastTokenEnd();
    if (match(SEMICOLON))
        endLine  = tokenLine();
    failIfFalse(autoSemiColon());
    return context.createReturnStatement(expr, start, end, startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseThrowStatement(TreeBuilder& context)
{
    ASSERT(match(THROW));
    int eStart = tokenStart();
    int startLine = tokenLine();
    next();
    
    failIfTrue(autoSemiColon());

    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    int eEnd = lastTokenEnd();
    int endLine = tokenLine();
    failIfFalse(autoSemiColon());

    return context.createThrowStatement(expr, eStart, eEnd, startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseWithStatement(TreeBuilder& context)
{
    ASSERT(match(WITH));
    failIfTrue(strictMode());
    currentScope()->setNeedsFullActivation();
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    int start = tokenStart();
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    int end = lastTokenEnd();

    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    const Identifier* unused = 0;
    TreeStatement statement = parseStatement(context, unused);
    failIfFalse(statement);

    return context.createWithStatement(expr, statement, start, end, startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseSwitchStatement(TreeBuilder& context)
{
    ASSERT(match(SWITCH));
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    consumeOrFail(OPENBRACE);
    startSwitch();
    TreeClauseList firstClauses = parseSwitchClauses(context);
    failIfTrue(m_error);

    TreeClause defaultClause = parseSwitchDefaultClause(context);
    failIfTrue(m_error);

    TreeClauseList secondClauses = parseSwitchClauses(context);
    failIfTrue(m_error);
    endSwitch();
    consumeOrFail(CLOSEBRACE);

    return context.createSwitchStatement(expr, firstClauses, defaultClause, secondClauses, startLine, endLine);

}

template <class TreeBuilder> TreeClauseList JSParser::parseSwitchClauses(TreeBuilder& context)
{
    if (!match(CASE))
        return 0;
    next();
    TreeExpression condition = parseExpression(context);
    failIfFalse(condition);
    consumeOrFail(COLON);
    TreeSourceElements statements = parseSourceElements<DontCheckForStrictMode>(context);
    failIfFalse(statements);
    TreeClause clause = context.createClause(condition, statements);
    TreeClauseList clauseList = context.createClauseList(clause);
    TreeClauseList tail = clauseList;

    while (match(CASE)) {
        next();
        TreeExpression condition = parseExpression(context);
        failIfFalse(condition);
        consumeOrFail(COLON);
        TreeSourceElements statements = parseSourceElements<DontCheckForStrictMode>(context);
        failIfFalse(statements);
        clause = context.createClause(condition, statements);
        tail = context.createClauseList(tail, clause);
    }
    return clauseList;
}

template <class TreeBuilder> TreeClause JSParser::parseSwitchDefaultClause(TreeBuilder& context)
{
    if (!match(DEFAULT))
        return 0;
    next();
    consumeOrFail(COLON);
    TreeSourceElements statements = parseSourceElements<DontCheckForStrictMode>(context);
    failIfFalse(statements);
    return context.createClause(0, statements);
}

template <class TreeBuilder> TreeStatement JSParser::parseTryStatement(TreeBuilder& context)
{
    ASSERT(match(TRY));
    TreeStatement tryBlock = 0;
    const Identifier* ident = &m_globalData->propertyNames->nullIdentifier;
    bool catchHasEval = false;
    TreeStatement catchBlock = 0;
    TreeStatement finallyBlock = 0;
    int firstLine = tokenLine();
    next();
    matchOrFail(OPENBRACE);

    tryBlock = parseBlockStatement(context);
    failIfFalse(tryBlock);
    int lastLine = m_lastLine;

    if (match(CATCH)) {
        currentScope()->setNeedsFullActivation();
        next();
        consumeOrFail(OPENPAREN);
        matchOrFail(IDENT);
        ident = m_token.m_data.ident;
        next();
        AutoPopScopeRef catchScope(this, pushScope());
        failIfFalseIfStrict(catchScope->declareVariable(ident));
        catchScope->preventNewDecls();
        consumeOrFail(CLOSEPAREN);
        matchOrFail(OPENBRACE);
        int initialEvalCount = context.evalCount();
        catchBlock = parseBlockStatement(context);
        failIfFalse(catchBlock);
        catchHasEval = initialEvalCount != context.evalCount();
        failIfFalse(popScope(catchScope, TreeBuilder::NeedsFreeVariableInfo));
    }

    if (match(FINALLY)) {
        next();
        matchOrFail(OPENBRACE);
        finallyBlock = parseBlockStatement(context);
        failIfFalse(finallyBlock);
    }
    failIfFalse(catchBlock || finallyBlock);
    return context.createTryStatement(tryBlock, ident, catchHasEval, catchBlock, finallyBlock, firstLine, lastLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseDebuggerStatement(TreeBuilder& context)
{
    ASSERT(match(DEBUGGER));
    int startLine = tokenLine();
    int endLine = startLine;
    next();
    if (match(SEMICOLON))
        startLine = tokenLine();
    failIfFalse(autoSemiColon());
    return context.createDebugger(startLine, endLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseBlockStatement(TreeBuilder& context)
{
    ASSERT(match(OPENBRACE));
    int start = tokenLine();
    next();
    if (match(CLOSEBRACE)) {
        next();
        return context.createBlockStatement(0, start, m_lastLine);
    }
    TreeSourceElements subtree = parseSourceElements<DontCheckForStrictMode>(context);
    failIfFalse(subtree);
    matchOrFail(CLOSEBRACE);
    next();
    return context.createBlockStatement(subtree, start, m_lastLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseStatement(TreeBuilder& context, const Identifier*& directive)
{
    DepthManager statementDepth(&m_statementDepth);
    m_statementDepth++;
    directive = 0;
    int nonTrivialExpressionCount = 0;
    failIfStackOverflow();
    switch (m_token.m_type) {
    case OPENBRACE:
        return parseBlockStatement(context);
    case VAR:
        return parseVarDeclaration(context);
    case CONSTTOKEN:
        return parseConstDeclaration(context);
    case FUNCTION:
        failIfFalseIfStrict(m_statementDepth == 1);
        return parseFunctionDeclaration(context);
    case SEMICOLON:
        next();
        return context.createEmptyStatement();
    case IF:
        return parseIfStatement(context);
    case DO:
        return parseDoWhileStatement(context);
    case WHILE:
        return parseWhileStatement(context);
    case FOR:
        return parseForStatement(context);
    case CONTINUE:
        return parseContinueStatement(context);
    case BREAK:
        return parseBreakStatement(context);
    case RETURN:
        return parseReturnStatement(context);
    case WITH:
        return parseWithStatement(context);
    case SWITCH:
        return parseSwitchStatement(context);
    case THROW:
        return parseThrowStatement(context);
    case TRY:
        return parseTryStatement(context);
    case DEBUGGER:
        return parseDebuggerStatement(context);
    case EOFTOK:
    case CASE:
    case CLOSEBRACE:
    case DEFAULT:
        // These tokens imply the end of a set of source elements
        return 0;
    case IDENT:
        return parseExpressionOrLabelStatement(context);
    case STRING:
        directive = m_token.m_data.ident;
        nonTrivialExpressionCount = m_nonTrivialExpressionCount;
    default:
        TreeStatement exprStatement = parseExpressionStatement(context);
        if (directive && nonTrivialExpressionCount != m_nonTrivialExpressionCount)
            directive = 0;
        return exprStatement;
    }
}

template <class TreeBuilder> TreeFormalParameterList JSParser::parseFormalParameters(TreeBuilder& context)
{
    matchOrFail(IDENT);
    failIfFalseIfStrict(declareParameter(m_token.m_data.ident));
    TreeFormalParameterList list = context.createFormalParameterList(*m_token.m_data.ident);
    TreeFormalParameterList tail = list;
    next();
    while (match(COMMA)) {
        next();
        matchOrFail(IDENT);
        const Identifier* ident = m_token.m_data.ident;
        failIfFalseIfStrict(declareParameter(ident));
        next();
        tail = context.createFormalParameterList(tail, *ident);
    }
    return list;
}

template <class TreeBuilder> TreeFunctionBody JSParser::parseFunctionBody(TreeBuilder& context)
{
    if (match(CLOSEBRACE))
        return context.createFunctionBody(strictMode());
    DepthManager statementDepth(&m_statementDepth);
    m_statementDepth = 0;
    typename TreeBuilder::FunctionBodyBuilder bodyBuilder(m_globalData, m_lexer);
    failIfFalse(parseSourceElements<CheckForStrictMode>(bodyBuilder));
    return context.createFunctionBody(strictMode());
}

#if COMPILER(WINSCW)
template <JSParser::FunctionRequirements, bool, class TreeBuilder> bool JSParser::parseFunctionInfo(JSParser::FunctionRequirements requirements, bool nameIsInContainingScope, TreeBuilder& context, const Identifier*& name, TreeFormalParameterList& parameters, TreeFunctionBody& body, int& openBracePos, int& closeBracePos, int& bodyStartLine)
#else
template <JSParser::FunctionRequirements requirements, bool nameIsInContainingScope, class TreeBuilder> bool JSParser::parseFunctionInfo(TreeBuilder& context, const Identifier*& name, TreeFormalParameterList& parameters, TreeFunctionBody& body, int& openBracePos, int& closeBracePos, int& bodyStartLine)
#endif
{
    AutoPopScopeRef functionScope(this, pushScope());
    functionScope->setIsFunction();
    if (match(IDENT)) {
        name = m_token.m_data.ident;
        failIfTrue(*name == m_globalData->propertyNames->underscoreProto);
        next();
        if (!nameIsInContainingScope)
            failIfFalseIfStrict(functionScope->declareVariable(name));
    } else if (requirements == FunctionNeedsName)
        return false;
    consumeOrFail(OPENPAREN);
    if (!match(CLOSEPAREN)) {
        parameters = parseFormalParameters(context);
        failIfFalse(parameters);
    }
    consumeOrFail(CLOSEPAREN);
    matchOrFail(OPENBRACE);

    openBracePos = m_token.m_data.intValue;
    bodyStartLine = tokenLine();

    if (const SourceProviderCacheItem* cachedInfo = TreeBuilder::CanUseFunctionCache ? findCachedFunctionInfo(openBracePos) : 0) {
        // If we know about this function already, we can use the cached info and skip the parser to the end of the function.
        body = context.createFunctionBody(strictMode());

        functionScope->restoreFunctionInfo(cachedInfo);
        failIfFalse(popScope(functionScope, TreeBuilder::NeedsFreeVariableInfo));

        closeBracePos = cachedInfo->closeBracePos;
        m_token = cachedInfo->closeBraceToken();
        m_lexer->setOffset(m_token.m_info.endOffset);
        m_lexer->setLineNumber(m_token.m_info.line);

        next();
        return true;
    }

    next();

    body = parseFunctionBody(context);
    failIfFalse(body);
    if (functionScope->strictMode() && name) {
        failIfTrue(m_globalData->propertyNames->arguments == *name);
        failIfTrue(m_globalData->propertyNames->eval == *name);
    }
    closeBracePos = m_token.m_data.intValue;
    
    // Cache the tokenizer state and the function scope the first time the function is parsed.
    // Any future reparsing can then skip the function.
    static const int minimumFunctionLengthToCache = 64;
    OwnPtr<SourceProviderCacheItem> newInfo;
    int functionLength = closeBracePos - openBracePos;
    if (TreeBuilder::CanUseFunctionCache && m_functionCache && functionLength > minimumFunctionLengthToCache) {
        newInfo = adoptPtr(new SourceProviderCacheItem(m_token.m_info.line, closeBracePos));
        functionScope->saveFunctionInfo(newInfo.get());
    }
    
    failIfFalse(popScope(functionScope, TreeBuilder::NeedsFreeVariableInfo));
    matchOrFail(CLOSEBRACE);

    if (newInfo) {
        unsigned approximateByteSize = newInfo->approximateByteSize();
        m_functionCache->add(openBracePos, newInfo.release(), approximateByteSize);
    }

    next();
    return true;
}

template <class TreeBuilder> TreeStatement JSParser::parseFunctionDeclaration(TreeBuilder& context)
{
    ASSERT(match(FUNCTION));
    next();
    const Identifier* name = 0;
    TreeFormalParameterList parameters = 0;
    TreeFunctionBody body = 0;
    int openBracePos = 0;
    int closeBracePos = 0;
    int bodyStartLine = 0;
#if COMPILER(WINSCW)
    failIfFalse((parseFunctionInfo<FunctionNeedsName, true>(FunctionNeedsName, true, context, name, parameters, body, openBracePos, closeBracePos, bodyStartLine)));
#else
    failIfFalse((parseFunctionInfo<FunctionNeedsName, true>(context, name, parameters, body, openBracePos, closeBracePos, bodyStartLine)));
#endif
    failIfFalse(name);
    failIfFalseIfStrict(declareVariable(name));
    return context.createFuncDeclStatement(name, body, parameters, openBracePos, closeBracePos, bodyStartLine, m_lastLine);
}

struct LabelInfo {
    LabelInfo(const Identifier* ident, int start, int end)
        : m_ident(ident)
        , m_start(start)
        , m_end(end)
    {
    }

    const Identifier* m_ident;
    int m_start;
    int m_end;
};

template <class TreeBuilder> TreeStatement JSParser::parseExpressionOrLabelStatement(TreeBuilder& context)
{

    /* Expression and Label statements are ambiguous at LL(1), so we have a
     * special case that looks for a colon as the next character in the input.
     */
    Vector<LabelInfo> labels;

    do {
        int start = tokenStart();
        int startLine = tokenLine();
        if (!nextTokenIsColon()) {
            // If we hit this path we're making a expression statement, which
            // by definition can't make use of continue/break so we can just
            // ignore any labels we might have accumulated.
            TreeExpression expression = parseExpression(context);
            failIfFalse(expression);
            failIfFalse(autoSemiColon());
            return context.createExprStatement(expression, startLine, m_lastLine);
        }
        const Identifier* ident = m_token.m_data.ident;
        int end = tokenEnd();
        next();
        consumeOrFail(COLON);
        if (!m_syntaxAlreadyValidated) {
            // This is O(N^2) over the current list of consecutive labels, but I
            // have never seen more than one label in a row in the real world.
            for (size_t i = 0; i < labels.size(); i++)
                failIfTrue(ident->impl() == labels[i].m_ident->impl());
            failIfTrue(getLabel(ident));
            labels.append(LabelInfo(ident, start, end));
        }
    } while (match(IDENT));
    bool isLoop = false;
    switch (m_token.m_type) {
    case FOR:
    case WHILE:
    case DO:
        isLoop = true;
        break;

    default:
        break;
    }
    const Identifier* unused = 0;
    if (!m_syntaxAlreadyValidated) {
        for (size_t i = 0; i < labels.size(); i++)
            pushLabel(labels[i].m_ident, isLoop);
    }
    TreeStatement statement = parseStatement(context, unused);
    if (!m_syntaxAlreadyValidated) {
        for (size_t i = 0; i < labels.size(); i++)
            popLabel();
    }
    failIfFalse(statement);
    for (size_t i = 0; i < labels.size(); i++) {
        const LabelInfo& info = labels[labels.size() - i - 1];
        statement = context.createLabelStatement(info.m_ident, statement, info.m_start, info.m_end);
    }
    return statement;
}

template <class TreeBuilder> TreeStatement JSParser::parseExpressionStatement(TreeBuilder& context)
{
    int startLine = tokenLine();
    TreeExpression expression = parseExpression(context);
    failIfFalse(expression);
    failIfFalse(autoSemiColon());
    return context.createExprStatement(expression, startLine, m_lastLine);
}

template <class TreeBuilder> TreeStatement JSParser::parseIfStatement(TreeBuilder& context)
{
    ASSERT(match(IF));

    int start = tokenLine();
    next();

    consumeOrFail(OPENPAREN);

    TreeExpression condition = parseExpression(context);
    failIfFalse(condition);
    int end = tokenLine();
    consumeOrFail(CLOSEPAREN);

    const Identifier* unused = 0;
    TreeStatement trueBlock = parseStatement(context, unused);
    failIfFalse(trueBlock);

    if (!match(ELSE))
        return context.createIfStatement(condition, trueBlock, start, end);
    
    Vector<TreeExpression> exprStack;
    Vector<pair<int, int> > posStack;
    Vector<TreeStatement> statementStack;
    bool trailingElse = false;
    do {
        next();
        if (!match(IF)) {
            const Identifier* unused = 0;
            TreeStatement block = parseStatement(context, unused);
            failIfFalse(block);
            statementStack.append(block);
            trailingElse = true;
            break;
        }
        int innerStart = tokenLine();
        next();
        
        consumeOrFail(OPENPAREN);
        
        TreeExpression innerCondition = parseExpression(context);
        failIfFalse(innerCondition);
        int innerEnd = tokenLine();
        consumeOrFail(CLOSEPAREN);
        const Identifier* unused = 0;
        TreeStatement innerTrueBlock = parseStatement(context, unused);
        failIfFalse(innerTrueBlock);     
        exprStack.append(innerCondition);
        posStack.append(make_pair(innerStart, innerEnd));
        statementStack.append(innerTrueBlock);
    } while (match(ELSE));

    if (!trailingElse) {
        TreeExpression condition = exprStack.last();
        exprStack.removeLast();
        TreeStatement trueBlock = statementStack.last();
        statementStack.removeLast();
        pair<int, int> pos = posStack.last();
        posStack.removeLast();
        statementStack.append(context.createIfStatement(condition, trueBlock, pos.first, pos.second));
    }

    while (!exprStack.isEmpty()) {
        TreeExpression condition = exprStack.last();
        exprStack.removeLast();
        TreeStatement falseBlock = statementStack.last();
        statementStack.removeLast();
        TreeStatement trueBlock = statementStack.last();
        statementStack.removeLast();
        pair<int, int> pos = posStack.last();
        posStack.removeLast();
        statementStack.append(context.createIfStatement(condition, trueBlock, falseBlock, pos.first, pos.second));
    }
    
    return context.createIfStatement(condition, trueBlock, statementStack.last(), start, end);
}

template <class TreeBuilder> TreeExpression JSParser::parseExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    TreeExpression node = parseAssignmentExpression(context);
    failIfFalse(node);
    if (!match(COMMA))
        return node;
    next();
    m_nonTrivialExpressionCount++;
    m_nonLHSCount++;
    TreeExpression right = parseAssignmentExpression(context);
    failIfFalse(right);
    typename TreeBuilder::Comma commaNode = context.createCommaExpr(node, right);
    while (match(COMMA)) {
        next();
        right = parseAssignmentExpression(context);
        failIfFalse(right);
        context.appendToComma(commaNode, right);
    }
    return commaNode;
}


template <typename TreeBuilder> TreeExpression JSParser::parseAssignmentExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    int start = tokenStart();
    int initialAssignmentCount = m_assignmentCount;
    int initialNonLHSCount = m_nonLHSCount;
    TreeExpression lhs = parseConditionalExpression(context);
    failIfFalse(lhs);
    if (initialNonLHSCount != m_nonLHSCount)
        return lhs;

    int assignmentStack = 0;
    Operator op;
    bool hadAssignment = false;
    while (true) {
        switch (m_token.m_type) {
        case EQUAL: op = OpEqual; break;
        case PLUSEQUAL: op = OpPlusEq; break;
        case MINUSEQUAL: op = OpMinusEq; break;
        case MULTEQUAL: op = OpMultEq; break;
        case DIVEQUAL: op = OpDivEq; break;
        case LSHIFTEQUAL: op = OpLShift; break;
        case RSHIFTEQUAL: op = OpRShift; break;
        case URSHIFTEQUAL: op = OpURShift; break;
        case ANDEQUAL: op = OpAndEq; break;
        case XOREQUAL: op = OpXOrEq; break;
        case OREQUAL: op = OpOrEq; break;
        case MODEQUAL: op = OpModEq; break;
        default:
            goto end;
        }
        m_nonTrivialExpressionCount++;
        hadAssignment = true;
        context.assignmentStackAppend(assignmentStack, lhs, start, tokenStart(), m_assignmentCount, op);
        start = tokenStart();
        m_assignmentCount++;
        next();
        if (strictMode() && m_lastIdentifier && context.isResolve(lhs)) {
            failIfTrueIfStrict(m_globalData->propertyNames->eval == *m_lastIdentifier);
            failIfTrueIfStrict(m_globalData->propertyNames->arguments == *m_lastIdentifier);
            declareWrite(m_lastIdentifier);
            m_lastIdentifier = 0;
        }
        lhs = parseConditionalExpression(context);
        failIfFalse(lhs);
        if (initialNonLHSCount != m_nonLHSCount)
            break;
    }
end:
    if (hadAssignment)
        m_nonLHSCount++;

    if (!TreeBuilder::CreatesAST)
        return lhs;

    while (assignmentStack)
        lhs = context.createAssignment(assignmentStack, lhs, initialAssignmentCount, m_assignmentCount, lastTokenEnd());

    return lhs;
}

template <class TreeBuilder> TreeExpression JSParser::parseConditionalExpression(TreeBuilder& context)
{
    TreeExpression cond = parseBinaryExpression(context);
    failIfFalse(cond);
    if (!match(QUESTION))
        return cond;
    m_nonTrivialExpressionCount++;
    m_nonLHSCount++;
    next();
    TreeExpression lhs = parseAssignmentExpression(context);
    consumeOrFail(COLON);

    TreeExpression rhs = parseAssignmentExpression(context);
    failIfFalse(rhs);
    return context.createConditionalExpr(cond, lhs, rhs);
}

ALWAYS_INLINE static bool isUnaryOp(JSTokenType token)
{
    return token & UnaryOpTokenFlag;
}

int JSParser::isBinaryOperator(JSTokenType token)
{
    if (m_allowsIn)
        return token & (BinaryOpTokenPrecedenceMask << BinaryOpTokenAllowsInPrecedenceAdditionalShift);
    return token & BinaryOpTokenPrecedenceMask;
}

template <class TreeBuilder> TreeExpression JSParser::parseBinaryExpression(TreeBuilder& context)
{

    int operandStackDepth = 0;
    int operatorStackDepth = 0;
    typename TreeBuilder::BinaryExprContext binaryExprContext(context);
    while (true) {
        int exprStart = tokenStart();
        int initialAssignments = m_assignmentCount;
        TreeExpression current = parseUnaryExpression(context);
        failIfFalse(current);

        context.appendBinaryExpressionInfo(operandStackDepth, current, exprStart, lastTokenEnd(), lastTokenEnd(), initialAssignments != m_assignmentCount);
        int precedence = isBinaryOperator(m_token.m_type);
        if (!precedence)
            break;
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        int operatorToken = m_token.m_type;
        next();

        while (operatorStackDepth &&  context.operatorStackHasHigherPrecedence(operatorStackDepth, precedence)) {
            ASSERT(operandStackDepth > 1);

            typename TreeBuilder::BinaryOperand rhs = context.getFromOperandStack(-1);
            typename TreeBuilder::BinaryOperand lhs = context.getFromOperandStack(-2);
            context.shrinkOperandStackBy(operandStackDepth, 2);
            context.appendBinaryOperation(operandStackDepth, operatorStackDepth, lhs, rhs);
            context.operatorStackPop(operatorStackDepth);
        }
        context.operatorStackAppend(operatorStackDepth, operatorToken, precedence);
    }
    while (operatorStackDepth) {
        ASSERT(operandStackDepth > 1);

        typename TreeBuilder::BinaryOperand rhs = context.getFromOperandStack(-1);
        typename TreeBuilder::BinaryOperand lhs = context.getFromOperandStack(-2);
        context.shrinkOperandStackBy(operandStackDepth, 2);
        context.appendBinaryOperation(operandStackDepth, operatorStackDepth, lhs, rhs);
        context.operatorStackPop(operatorStackDepth);
    }
    return context.popOperandStack(operandStackDepth);
}

#if COMPILER(WINSCW)
template <bool complete, class TreeBuilder> TreeProperty JSParser::parseProperty(bool complete, TreeBuilder& context)
#else
template <bool complete, class TreeBuilder> TreeProperty JSParser::parseProperty(TreeBuilder& context)
#endif
{
    bool wasIdent = false;
    switch (m_token.m_type) {
    namedProperty:
    case IDENT:
        wasIdent = true;
    case STRING: {
        const Identifier* ident = m_token.m_data.ident;
        next(Lexer::IgnoreReservedWords);
        if (match(COLON)) {
            next();
            TreeExpression node = parseAssignmentExpression(context);
            failIfFalse(node);
#if COMPILER(WINSCW)
            if (complete)
                return context.template createProperty<true>(ident, node, PropertyNode::Constant);
            else
                return context.template createProperty<false>(ident, node, PropertyNode::Constant);
#else
            return context.template createProperty<complete>(ident, node, PropertyNode::Constant);
#endif
        }
        failIfFalse(wasIdent);
        matchOrFail(IDENT);
        const Identifier* accessorName = 0;
        TreeFormalParameterList parameters = 0;
        TreeFunctionBody body = 0;
        int openBracePos = 0;
        int closeBracePos = 0;
        int bodyStartLine = 0;
        PropertyNode::Type type;
        if (*ident == m_globalData->propertyNames->get)
            type = PropertyNode::Getter;
        else if (*ident == m_globalData->propertyNames->set)
            type = PropertyNode::Setter;
        else
            fail();
#if COMPILER(WINSCW)
        failIfFalse((parseFunctionInfo<FunctionNeedsName, false>(FunctionNeedsName, false, context, accessorName, parameters, body, openBracePos, closeBracePos, bodyStartLine)));
        if (complete)
            return context.template createGetterOrSetterProperty<true>(type, accessorName, parameters, body, openBracePos, closeBracePos, bodyStartLine, m_lastLine);
        else
            return context.template createGetterOrSetterProperty<false>(type, accessorName, parameters, body, openBracePos, closeBracePos, bodyStartLine, m_lastLine);
#else
        failIfFalse((parseFunctionInfo<FunctionNeedsName, false>(context, accessorName, parameters, body, openBracePos, closeBracePos, bodyStartLine)));
        return context.template createGetterOrSetterProperty<complete>(type, accessorName, parameters, body, openBracePos, closeBracePos, bodyStartLine, m_lastLine);
#endif
    }
    case NUMBER: {
        double propertyName = m_token.m_data.doubleValue;
        next();
        consumeOrFail(COLON);
        TreeExpression node = parseAssignmentExpression(context);
        failIfFalse(node);
#if COMPILER(WINSCW)
        if (complete)
            return context.template createProperty<true>(m_globalData, propertyName, node, PropertyNode::Constant);
        else
            return context.template createProperty<false>(m_globalData, propertyName, node, PropertyNode::Constant);
#else
        return context.template createProperty<complete>(m_globalData, propertyName, node, PropertyNode::Constant);
#endif
    }
    default:
        failIfFalse(m_token.m_type & KeywordTokenFlag);
        goto namedProperty;
    }
}

template <class TreeBuilder> TreeExpression JSParser::parseObjectLiteral(TreeBuilder& context)
{
    int startOffset = m_token.m_data.intValue;
    consumeOrFail(OPENBRACE);

    if (match(CLOSEBRACE)) {
        next();
        return context.createObjectLiteral();
    }

#if COMPILER(WINSCW)
    TreeProperty property = parseProperty<false>(false, context);
#else
    TreeProperty property = parseProperty<false>(context);
#endif
    failIfFalse(property);
    if (!m_syntaxAlreadyValidated && context.getType(property) != PropertyNode::Constant) {
        m_lexer->setOffset(startOffset);
        next();
        return parseStrictObjectLiteral(context);
    }
    TreePropertyList propertyList = context.createPropertyList(property);
    TreePropertyList tail = propertyList;
    while (match(COMMA)) {
        next();
        // allow extra comma, see http://bugs.webkit.org/show_bug.cgi?id=5939
        if (match(CLOSEBRACE))
            break;
#if COMPILER(WINSCW)
        property = parseProperty<false>(false, context);
#else
        property = parseProperty<false>(context);
#endif
        failIfFalse(property);
        if (!m_syntaxAlreadyValidated && context.getType(property) != PropertyNode::Constant) {
            m_lexer->setOffset(startOffset);
            next();
            return parseStrictObjectLiteral(context);
        }
        tail = context.createPropertyList(property, tail);
    }

    consumeOrFail(CLOSEBRACE);

    return context.createObjectLiteral(propertyList);
}

template <class TreeBuilder> TreeExpression JSParser::parseStrictObjectLiteral(TreeBuilder& context)
{
    consumeOrFail(OPENBRACE);
    
    if (match(CLOSEBRACE)) {
        next();
        return context.createObjectLiteral();
    }
    
#if COMPILER(WINSCW)
    TreeProperty property = parseProperty<true>(true, context);
#else
    TreeProperty property = parseProperty<true>(context);
#endif
    failIfFalse(property);
    
    typedef HashMap<RefPtr<StringImpl>, unsigned, IdentifierRepHash> ObjectValidationMap;
    ObjectValidationMap objectValidator;
    // Add the first property
    if (!m_syntaxAlreadyValidated)
        objectValidator.add(context.getName(property).impl(), context.getType(property));
    
    TreePropertyList propertyList = context.createPropertyList(property);
    TreePropertyList tail = propertyList;
    while (match(COMMA)) {
        next();
        // allow extra comma, see http://bugs.webkit.org/show_bug.cgi?id=5939
        if (match(CLOSEBRACE))
            break;
#if COMPILER(WINSCW)
        property = parseProperty<true>(true, context);
#else
        property = parseProperty<true>(context);
#endif
        failIfFalse(property);
        if (!m_syntaxAlreadyValidated) {
            std::pair<ObjectValidationMap::iterator, bool> propertyEntryIter = objectValidator.add(context.getName(property).impl(), context.getType(property));
            if (!propertyEntryIter.second) {
                failIfTrue(propertyEntryIter.first->second == PropertyNode::Constant);
                failIfTrue(context.getType(property) == PropertyNode::Constant);
                failIfTrue(context.getType(property) & propertyEntryIter.first->second);
                propertyEntryIter.first->second |= context.getType(property);
            }
        }
        tail = context.createPropertyList(property, tail);
    }
    
    consumeOrFail(CLOSEBRACE);
    
    return context.createObjectLiteral(propertyList);
}

template <class TreeBuilder> TreeExpression JSParser::parseArrayLiteral(TreeBuilder& context)
{
    consumeOrFail(OPENBRACKET);

    int elisions = 0;
    while (match(COMMA)) {
        next();
        elisions++;
    }
    if (match(CLOSEBRACKET)) {
        next();
        return context.createArray(elisions);
    }

    TreeExpression elem = parseAssignmentExpression(context);
    failIfFalse(elem);
    typename TreeBuilder::ElementList elementList = context.createElementList(elisions, elem);
    typename TreeBuilder::ElementList tail = elementList;
    elisions = 0;
    while (match(COMMA)) {
        next();
        elisions = 0;

        while (match(COMMA)) {
            next();
            elisions++;
        }

        if (match(CLOSEBRACKET)) {
            next();
            return context.createArray(elisions, elementList);
        }
        TreeExpression elem = parseAssignmentExpression(context);
        failIfFalse(elem);
        tail = context.createElementList(tail, elisions, elem);
    }

    consumeOrFail(CLOSEBRACKET);

    return context.createArray(elementList);
}

template <class TreeBuilder> TreeExpression JSParser::parsePrimaryExpression(TreeBuilder& context)
{
    switch (m_token.m_type) {
    case OPENBRACE:
        if (strictMode())
            return parseStrictObjectLiteral(context);
        return parseObjectLiteral(context);
    case OPENBRACKET:
        return parseArrayLiteral(context);
    case OPENPAREN: {
        next();
        int oldNonLHSCount = m_nonLHSCount;
        TreeExpression result = parseExpression(context);
        m_nonLHSCount = oldNonLHSCount;
        consumeOrFail(CLOSEPAREN);

        return result;
    }
    case THISTOKEN: {
        next();
        return context.thisExpr();
    }
    case IDENT: {
        int start = tokenStart();
        const Identifier* ident = m_token.m_data.ident;
        next();
        currentScope()->useVariable(ident, m_globalData->propertyNames->eval == *ident);
        m_lastIdentifier = ident;
        return context.createResolve(ident, start);
    }
    case STRING: {
        const Identifier* ident = m_token.m_data.ident;
        next();
        return context.createString(ident);
    }
    case NUMBER: {
        double d = m_token.m_data.doubleValue;
        next();
        return context.createNumberExpr(d);
    }
    case NULLTOKEN: {
        next();
        return context.createNull();
    }
    case TRUETOKEN: {
        next();
        return context.createBoolean(true);
    }
    case FALSETOKEN: {
        next();
        return context.createBoolean(false);
    }
    case DIVEQUAL:
    case DIVIDE: {
        /* regexp */
        const Identifier* pattern;
        const Identifier* flags;
        if (match(DIVEQUAL))
            failIfFalse(m_lexer->scanRegExp(pattern, flags, '='));
        else
            failIfFalse(m_lexer->scanRegExp(pattern, flags));

        int start = tokenStart();
        next();
        TreeExpression re = context.createRegExp(*pattern, *flags, start);
        if (!re) {
            m_errorMessage = Yarr::checkSyntax(pattern->ustring());
            ASSERT(m_errorMessage);
            fail();
        }
        return re;
    }
    default:
        fail();
    }
}

template <class TreeBuilder> TreeArguments JSParser::parseArguments(TreeBuilder& context)
{
    consumeOrFail(OPENPAREN);
    if (match(CLOSEPAREN)) {
        next();
        return context.createArguments();
    }
    TreeExpression firstArg = parseAssignmentExpression(context);
    failIfFalse(firstArg);

    TreeArgumentsList argList = context.createArgumentsList(firstArg);
    TreeArgumentsList tail = argList;
    while (match(COMMA)) {
        next();
        TreeExpression arg = parseAssignmentExpression(context);
        failIfFalse(arg);
        tail = context.createArgumentsList(tail, arg);
    }
    consumeOrFail(CLOSEPAREN);
    return context.createArguments(argList);
}

template <class TreeBuilder> TreeExpression JSParser::parseMemberExpression(TreeBuilder& context)
{
    TreeExpression base = 0;
    int start = tokenStart();
    int expressionStart = start;
    int newCount = 0;
    while (match(NEW)) {
        next();
        newCount++;
    }

    if (match(FUNCTION)) {
        const Identifier* name = &m_globalData->propertyNames->nullIdentifier;
        TreeFormalParameterList parameters = 0;
        TreeFunctionBody body = 0;
        int openBracePos = 0;
        int closeBracePos = 0;
        int bodyStartLine = 0;
        next();
#if COMPILER(WINSCW)
        failIfFalse((parseFunctionInfo<FunctionNoRequirements, false>(FunctionNoRequirements, false, context, name, parameters, body, openBracePos, closeBracePos, bodyStartLine)));
#else
        failIfFalse((parseFunctionInfo<FunctionNoRequirements, false>(context, name, parameters, body, openBracePos, closeBracePos, bodyStartLine)));
#endif
        base = context.createFunctionExpr(name, body, parameters, openBracePos, closeBracePos, bodyStartLine, m_lastLine);
    } else
        base = parsePrimaryExpression(context);

    failIfFalse(base);
    while (true) {
        switch (m_token.m_type) {
        case OPENBRACKET: {
            m_nonTrivialExpressionCount++;
            int expressionEnd = lastTokenEnd();
            next();
            int nonLHSCount = m_nonLHSCount;
            int initialAssignments = m_assignmentCount;
            TreeExpression property = parseExpression(context);
            failIfFalse(property);
            base = context.createBracketAccess(base, property, initialAssignments != m_assignmentCount, expressionStart, expressionEnd, tokenEnd());
            if (!consume(CLOSEBRACKET))
                fail();
            m_nonLHSCount = nonLHSCount;
            break;
        }
        case OPENPAREN: {
            m_nonTrivialExpressionCount++;
            if (newCount) {
                newCount--;
                if (match(OPENPAREN)) {
                    int exprEnd = lastTokenEnd();
                    TreeArguments arguments = parseArguments(context);
                    failIfFalse(arguments);
                    base = context.createNewExpr(base, arguments, start, exprEnd, lastTokenEnd());
                } else
                    base = context.createNewExpr(base, start, lastTokenEnd());               
            } else {
                int nonLHSCount = m_nonLHSCount;
                int expressionEnd = lastTokenEnd();
                TreeArguments arguments = parseArguments(context);
                failIfFalse(arguments);
                base = context.makeFunctionCallNode(base, arguments, expressionStart, expressionEnd, lastTokenEnd());
                m_nonLHSCount = nonLHSCount;
            }
            break;
        }
        case DOT: {
            m_nonTrivialExpressionCount++;
            int expressionEnd = lastTokenEnd();
            next(Lexer::IgnoreReservedWords);
            matchOrFail(IDENT);
            base = context.createDotAccess(base, *m_token.m_data.ident, expressionStart, expressionEnd, tokenEnd());
            next();
            break;
        }
        default:
            goto endMemberExpression;
        }
    }
endMemberExpression:
    while (newCount--)
        base = context.createNewExpr(base, start, lastTokenEnd());
    return base;
}

template <class TreeBuilder> TreeExpression JSParser::parseUnaryExpression(TreeBuilder& context)
{
    typename TreeBuilder::UnaryExprContext unaryExprContext(context);
    AllowInOverride allowInOverride(this);
    int tokenStackDepth = 0;
    bool modifiesExpr = false;
    bool requiresLExpr = false;
    while (isUnaryOp(m_token.m_type)) {
        if (strictMode()) {
            switch (m_token.m_type) {
            case PLUSPLUS:
            case MINUSMINUS:
            case AUTOPLUSPLUS:
            case AUTOMINUSMINUS:
                failIfTrue(requiresLExpr);
                modifiesExpr = true;
                requiresLExpr = true;
                break;
            case DELETETOKEN:
                failIfTrue(requiresLExpr);
                requiresLExpr = true;
                break;
            default:
                failIfTrue(requiresLExpr);
                break;
            }
        }
        m_nonLHSCount++;
        context.appendUnaryToken(tokenStackDepth, m_token.m_type, tokenStart());
        next();
        m_nonTrivialExpressionCount++;
    }
    int subExprStart = tokenStart();
    TreeExpression expr = parseMemberExpression(context);
    failIfFalse(expr);
    bool isEvalOrArguments = false;
    if (strictMode() && !m_syntaxAlreadyValidated) {
        if (context.isResolve(expr)) {
            isEvalOrArguments = *m_lastIdentifier == m_globalData->propertyNames->eval || *m_lastIdentifier == m_globalData->propertyNames->arguments;
        }
    }
    failIfTrueIfStrict(isEvalOrArguments && modifiesExpr);
    switch (m_token.m_type) {
    case PLUSPLUS:
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        expr = context.makePostfixNode(expr, OpPlusPlus, subExprStart, lastTokenEnd(), tokenEnd());
        m_assignmentCount++;
        failIfTrueIfStrict(isEvalOrArguments);
        failIfTrueIfStrict(requiresLExpr);
        next();
        break;
    case MINUSMINUS:
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        expr = context.makePostfixNode(expr, OpMinusMinus, subExprStart, lastTokenEnd(), tokenEnd());
        m_assignmentCount++;
        failIfTrueIfStrict(isEvalOrArguments);
        failIfTrueIfStrict(requiresLExpr);
        next();
        break;
    default:
        break;
    }

    int end = lastTokenEnd();

    if (!TreeBuilder::CreatesAST && (m_syntaxAlreadyValidated || !strictMode()))
        return expr;

    while (tokenStackDepth) {
        switch (context.unaryTokenStackLastType(tokenStackDepth)) {
        case EXCLAMATION:
            expr = context.createLogicalNot(expr);
            break;
        case TILDE:
            expr = context.makeBitwiseNotNode(expr);
            break;
        case MINUS:
            expr = context.makeNegateNode(expr);
            break;
        case PLUS:
            expr = context.createUnaryPlus(expr);
            break;
        case PLUSPLUS:
        case AUTOPLUSPLUS:
            expr = context.makePrefixNode(expr, OpPlusPlus, context.unaryTokenStackLastStart(tokenStackDepth), subExprStart + 1, end);
            m_assignmentCount++;
            break;
        case MINUSMINUS:
        case AUTOMINUSMINUS:
            expr = context.makePrefixNode(expr, OpMinusMinus, context.unaryTokenStackLastStart(tokenStackDepth), subExprStart + 1, end);
            m_assignmentCount++;
            break;
        case TYPEOF:
            expr = context.makeTypeOfNode(expr);
            break;
        case VOIDTOKEN:
            expr = context.createVoid(expr);
            break;
        case DELETETOKEN:
            failIfTrueIfStrict(context.isResolve(expr));
            expr = context.makeDeleteNode(expr, context.unaryTokenStackLastStart(tokenStackDepth), end, end);
            break;
        default:
            // If we get here something has gone horribly horribly wrong
            CRASH();
        }
        subExprStart = context.unaryTokenStackLastStart(tokenStackDepth);
        context.unaryTokenStackRemoveLast(tokenStackDepth);
    }
    return expr;
}

}

namespace WTF
{
    template <> struct VectorTraits<JSC::JSParser::Scope> : SimpleClassVectorTraits {
        static const bool canInitializeWithMemset = false; // Not all Scope data members initialize to 0.
    };
}
