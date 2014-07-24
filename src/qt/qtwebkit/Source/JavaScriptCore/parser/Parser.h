/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010, 2011, 2013 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef Parser_h
#define Parser_h

#include "Debugger.h"
#include "ExceptionHelpers.h"
#include "Executable.h"
#include "JSGlobalObject.h"
#include "Lexer.h"
#include "Nodes.h"
#include "ParserArena.h"
#include "ParserError.h"
#include "ParserTokens.h"
#include "SourceProvider.h"
#include "SourceProviderCache.h"
#include "SourceProviderCacheItem.h"
#include "VMStackBounds.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
namespace JSC {
struct Scope;
}

namespace WTF {
template <> struct VectorTraits<JSC::Scope> : SimpleClassVectorTraits {
    static const bool canInitializeWithMemset = false; // Not all Scope data members initialize to 0.
};
}

namespace JSC {

class ExecState;
class FunctionBodyNode;
class FunctionParameters;
class Identifier;
class VM;
class ProgramNode;
class SourceCode;

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

enum SourceElementsMode { CheckForStrictMode, DontCheckForStrictMode };
enum FunctionRequirements { FunctionNoRequirements, FunctionNeedsName };

template <typename T> inline bool isEvalNode() { return false; }
template <> inline bool isEvalNode<EvalNode>() { return true; }

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

struct ScopeLabelInfo {
    ScopeLabelInfo(StringImpl* ident, bool isLoop)
        : m_ident(ident)
        , m_isLoop(isLoop)
    {
    }

    StringImpl* m_ident;
    bool m_isLoop;
};

struct Scope {
    Scope(const VM* vm, bool isFunction, bool strictMode)
        : m_vm(vm)
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
        : m_vm(rhs.m_vm)
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

    void declareCallee(const Identifier* ident)
    {
        m_declaredVariables.add(ident->string().impl());
    }

    bool declareVariable(const Identifier* ident)
    {
        bool isValidStrictMode = m_vm->propertyNames->eval != *ident && m_vm->propertyNames->arguments != *ident;
        m_isValidStrictMode = m_isValidStrictMode && isValidStrictMode;
        m_declaredVariables.add(ident->string().impl());
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
        bool isArguments = m_vm->propertyNames->arguments == *ident;
        bool isValidStrictMode = m_declaredVariables.add(ident->string().impl()).isNewEntry && m_vm->propertyNames->eval != *ident && !isArguments;
        m_isValidStrictMode = m_isValidStrictMode && isValidStrictMode;
        if (isArguments)
            m_shadowsArguments = true;
        return isValidStrictMode;
    }

    void useVariable(const Identifier* ident, bool isEval)
    {
        m_usesEval |= isEval;
        m_usedVariables.add(ident->string().impl());
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
    }

    void fillParametersForSourceProviderCache(SourceProviderCacheItemCreationParameters& parameters)
    {
        ASSERT(m_isFunction);
        parameters.usesEval = m_usesEval;
        parameters.strictMode = m_strictMode;
        parameters.needsFullActivation = m_needsFullActivation;
        copyCapturedVariablesToVector(m_writtenVariables, parameters.writtenVariables);
        copyCapturedVariablesToVector(m_usedVariables, parameters.usedVariables);
    }

    void restoreFromSourceProviderCache(const SourceProviderCacheItem* info)
    {
        ASSERT(m_isFunction);
        m_usesEval = info->usesEval;
        m_strictMode = info->strictMode;
        m_needsFullActivation = info->needsFullActivation;
        for (unsigned i = 0; i < info->usedVariablesCount; ++i)
            m_usedVariables.add(info->usedVariables()[i]);
        for (unsigned i = 0; i < info->writtenVariablesCount; ++i)
            m_writtenVariables.add(info->writtenVariables()[i]);
    }

private:
    const VM* m_vm;
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

template <typename LexerType>
class Parser {
    WTF_MAKE_NONCOPYABLE(Parser);
    WTF_MAKE_FAST_ALLOCATED;

public:
    Parser(VM*, const SourceCode&, FunctionParameters*, const Identifier&, JSParserStrictness, JSParserMode);
    ~Parser();

    template <class ParsedNode>
    PassRefPtr<ParsedNode> parse(ParserError&);

private:
    struct AllowInOverride {
        AllowInOverride(Parser* parser)
            : m_parser(parser)
            , m_oldAllowsIn(parser->m_allowsIn)
        {
            parser->m_allowsIn = true;
        }
        ~AllowInOverride()
        {
            m_parser->m_allowsIn = m_oldAllowsIn;
        }
        Parser* m_parser;
        bool m_oldAllowsIn;
    };

    struct AutoPopScopeRef : public ScopeRef {
        AutoPopScopeRef(Parser* parser, ScopeRef scope)
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
        Parser* m_parser;
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
        m_scopeStack.append(Scope(m_vm, isFunction, isStrict));
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

    Parser();
    String parseInner();

    void didFinishParsing(SourceElements*, ParserArenaData<DeclarationStacks::VarStack>*, 
        ParserArenaData<DeclarationStacks::FunctionStack>*, CodeFeatures, int, IdentifierSet&);

    // Used to determine type of error to report.
    bool isFunctionBodyNode(ScopeNode*) { return false; }
    bool isFunctionBodyNode(FunctionBodyNode*) { return true; }

    ALWAYS_INLINE void next(unsigned lexerFlags = 0)
    {
        int lastLine = m_token.m_location.line;
        int lastTokenEnd = m_token.m_location.endOffset;
        int lastTokenLineStart = m_token.m_location.lineStartOffset;
        m_lastTokenEndPosition = JSTextPosition(lastLine, lastTokenEnd, lastTokenLineStart);
        m_lexer->setLastLineNumber(lastLine);
        m_token.m_type = m_lexer->lex(&m_token, lexerFlags, strictMode());
    }

    ALWAYS_INLINE void nextExpectIdentifier(unsigned lexerFlags = 0)
    {
        int lastLine = m_token.m_location.line;
        int lastTokenEnd = m_token.m_location.endOffset;
        int lastTokenLineStart = m_token.m_location.lineStartOffset;
        m_lastTokenEndPosition = JSTextPosition(lastLine, lastTokenEnd, lastTokenLineStart);
        m_lexer->setLastLineNumber(lastLine);
        m_token.m_type = m_lexer->lexExpectIdentifier(&m_token, lexerFlags, strictMode());
    }

    ALWAYS_INLINE bool nextTokenIsColon()
    {
        return m_lexer->nextTokenIsColon();
    }

    ALWAYS_INLINE bool consume(JSTokenType expected, unsigned flags = 0)
    {
        bool result = m_token.m_type == expected;
        if (result)
            next(flags);
        return result;
    }
    
    ALWAYS_INLINE String getToken() {
        SourceProvider* sourceProvider = m_source->provider();
        return sourceProvider->getRange(tokenStart(), tokenEndPosition().offset);
    }
    
    ALWAYS_INLINE bool match(JSTokenType expected)
    {
        return m_token.m_type == expected;
    }
    
    ALWAYS_INLINE unsigned tokenStart()
    {
        return m_token.m_location.startOffset;
    }
    
    ALWAYS_INLINE const JSTextPosition& tokenStartPosition()
    {
        return m_token.m_startPosition;
    }

    ALWAYS_INLINE int tokenLine()
    {
        return m_token.m_location.line;
    }
    
    ALWAYS_INLINE int tokenColumn()
    {
        return tokenStart() - tokenLineStart();
    }

    ALWAYS_INLINE const JSTextPosition& tokenEndPosition()
    {
        return m_token.m_endPosition;
    }
    
    ALWAYS_INLINE unsigned tokenLineStart()
    {
        return m_token.m_location.lineStartOffset;
    }
    
    ALWAYS_INLINE const JSTokenLocation& tokenLocation()
    {
        return m_token.m_location;
    }

    const char* getTokenName(JSTokenType tok)
    {
        switch (tok) {
        case NULLTOKEN: 
            return "null";
        case TRUETOKEN:
            return "true";
        case FALSETOKEN: 
            return "false";
        case BREAK: 
            return "break";
        case CASE: 
            return "case";
        case DEFAULT: 
            return "default";
        case FOR: 
            return "for";
        case NEW: 
            return "new";
        case VAR: 
            return "var";
        case CONSTTOKEN: 
            return "const";
        case CONTINUE: 
            return "continue";
        case FUNCTION: 
            return "function";
        case IF: 
            return "if";
        case THISTOKEN: 
            return "this";
        case DO: 
            return "do";
        case WHILE: 
            return "while";
        case SWITCH: 
            return "switch";
        case WITH: 
            return "with";
        case THROW: 
            return "throw";
        case TRY: 
            return "try";
        case CATCH: 
            return "catch";
        case FINALLY: 
            return "finally";
        case DEBUGGER: 
            return "debugger";
        case ELSE: 
            return "else";
        case OPENBRACE: 
            return "{";
        case CLOSEBRACE: 
            return "}";
        case OPENPAREN: 
            return "(";
        case CLOSEPAREN: 
            return ")";
        case OPENBRACKET: 
            return "[";
        case CLOSEBRACKET: 
            return "]";
        case COMMA: 
            return ",";
        case QUESTION: 
            return "?";
        case SEMICOLON: 
            return ";";
        case COLON: 
            return ":";
        case DOT: 
            return ".";
        case EQUAL: 
            return "=";
        case PLUSEQUAL: 
            return "+=";
        case MINUSEQUAL: 
            return "-=";
        case MULTEQUAL: 
            return "*=";
        case DIVEQUAL: 
            return "/=";
        case LSHIFTEQUAL: 
            return "<<=";
        case RSHIFTEQUAL: 
            return ">>=";
        case URSHIFTEQUAL: 
            return ">>>=";
        case ANDEQUAL: 
            return "&=";
        case MODEQUAL: 
            return "%=";
        case XOREQUAL: 
            return "^=";
        case OREQUAL: 
            return "|=";
        case AUTOPLUSPLUS: 
        case PLUSPLUS: 
            return "++";
        case AUTOMINUSMINUS: 
        case MINUSMINUS: 
            return "--";
        case EXCLAMATION: 
            return "!";
        case TILDE: 
            return "~";
        case TYPEOF: 
            return "typeof";
        case VOIDTOKEN: 
            return "void";
        case DELETETOKEN: 
            return "delete";
        case OR: 
            return "||";
        case AND: 
            return "&&";
        case BITOR: 
            return "|";
        case BITXOR: 
            return "^";
        case BITAND: 
            return "&";
        case EQEQ: 
            return "==";
        case NE: 
            return "!=";
        case STREQ: 
            return "===";
        case STRNEQ: 
            return "!==";
        case LT: 
            return "<";
        case GT: 
            return ">";
        case LE: 
            return "<=";
        case GE: 
            return ">=";
        case INSTANCEOF: 
            return "instanceof";
        case INTOKEN: 
            return "in";
        case LSHIFT: 
            return "<<";
        case RSHIFT: 
            return ">>";
        case URSHIFT: 
            return ">>>";
        case PLUS: 
            return "+";
        case MINUS: 
            return "-";
        case TIMES: 
            return "*";
        case DIVIDE: 
            return "/";
        case MOD: 
            return "%";
        case RETURN: 
        case RESERVED_IF_STRICT:
        case RESERVED: 
        case NUMBER:
        case IDENT: 
        case STRING:
        case UNTERMINATED_IDENTIFIER_ESCAPE_ERRORTOK:
        case UNTERMINATED_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK:
        case UNTERMINATED_MULTILINE_COMMENT_ERRORTOK:
        case UNTERMINATED_NUMERIC_LITERAL_ERRORTOK:
        case UNTERMINATED_STRING_LITERAL_ERRORTOK:
        case INVALID_IDENTIFIER_ESCAPE_ERRORTOK:
        case INVALID_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK:
        case INVALID_NUMERIC_LITERAL_ERRORTOK:
        case INVALID_OCTAL_NUMBER_ERRORTOK:
        case INVALID_STRING_LITERAL_ERRORTOK:
        case ERRORTOK:
        case EOFTOK:
            return 0;
        case LastUntaggedToken: 
            break;
        }
        RELEASE_ASSERT_NOT_REACHED();
        return "internal error";
    }
    
    ALWAYS_INLINE void updateErrorMessageSpecialCase(JSTokenType expectedToken) 
    {
        switch (expectedToken) {
        case RESERVED_IF_STRICT:
            m_errorMessage = "Use of reserved word '" + getToken() + "' in strict mode";
            return;
        case RESERVED:
            m_errorMessage = "Use of reserved word '" + getToken() + '\'';
            return;
        case NUMBER: 
            m_errorMessage = "Unexpected number '" + getToken() + '\'';
            return;
        case IDENT: 
            m_errorMessage = "Expected an identifier but found '" + getToken() + "' instead";
            return;
        case STRING: 
            m_errorMessage = "Unexpected string " + getToken();
            return;
            
        case UNTERMINATED_IDENTIFIER_ESCAPE_ERRORTOK:
        case UNTERMINATED_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK:
            m_errorMessage = "Incomplete unicode escape in identifier: '" + getToken() + '\'';
            return;
        case UNTERMINATED_MULTILINE_COMMENT_ERRORTOK:
            m_errorMessage = "Unterminated multiline comment";
            return;
        case UNTERMINATED_NUMERIC_LITERAL_ERRORTOK:
            m_errorMessage = "Unterminated numeric literal '" + getToken() + '\'';
            return;
        case UNTERMINATED_STRING_LITERAL_ERRORTOK:
            m_errorMessage = "Unterminated string literal '" + getToken() + '\'';
            return;
        case INVALID_IDENTIFIER_ESCAPE_ERRORTOK:
            m_errorMessage = "Invalid escape in identifier: '" + getToken() + '\'';
            return;
        case INVALID_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK:
            m_errorMessage = "Invalid unicode escape in identifier: '" + getToken() + '\'';
            return;
        case INVALID_NUMERIC_LITERAL_ERRORTOK:
            m_errorMessage = "Invalid numeric literal: '" + getToken() + '\'';
            return;
        case INVALID_OCTAL_NUMBER_ERRORTOK:
            m_errorMessage = "Invalid use of octal: '" + getToken() + '\'';
                return;
        case INVALID_STRING_LITERAL_ERRORTOK:
            m_errorMessage = "Invalid string literal: '" + getToken() + '\'';
            return;
        case ERRORTOK:
            m_errorMessage = "Unrecognized token '" + getToken() + '\'';
            return;
        case EOFTOK:  
            m_errorMessage = ASCIILiteral("Unexpected EOF");
            return;
        case RETURN:
            m_errorMessage = ASCIILiteral("Return statements are only valid inside functions");
            return;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            m_errorMessage = ASCIILiteral("internal error");
            return;
        }
    }
    
    NEVER_INLINE void updateErrorMessage() 
    {
        const char* name = getTokenName(m_token.m_type);
        if (!name) 
            updateErrorMessageSpecialCase(m_token.m_type);
        else 
            m_errorMessage = String::format("Unexpected token '%s'", name);
        ASSERT(!m_errorMessage.isNull());
    }
    
    NEVER_INLINE void updateErrorMessage(JSTokenType expectedToken) 
    {
        const char* name = getTokenName(expectedToken);
        if (name)
            m_errorMessage = String::format("Expected token '%s'", name);
        else {
            if (!getTokenName(m_token.m_type))
                updateErrorMessageSpecialCase(m_token.m_type);
            else
                updateErrorMessageSpecialCase(expectedToken);
        }
        ASSERT(!m_errorMessage.isNull());
    }
    
    NEVER_INLINE void updateErrorWithNameAndMessage(const char* beforeMsg, String name, const char* afterMsg)
    {
        m_errorMessage = makeString(beforeMsg, " '", name, "' ", afterMsg);
    }
    
    NEVER_INLINE void updateErrorMessage(const char* msg)
    {
        ASSERT(msg);
        m_errorMessage = String(msg);
        ASSERT(!m_errorMessage.isNull());
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
    
    template <SourceElementsMode mode, class TreeBuilder> TreeSourceElements parseSourceElements(TreeBuilder&);
    template <class TreeBuilder> TreeStatement parseStatement(TreeBuilder&, const Identifier*& directive, unsigned* directiveLiteralLength = 0);
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
    template <bool strict, class TreeBuilder> ALWAYS_INLINE TreeProperty parseProperty(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeFunctionBody parseFunctionBody(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeFormalParameterList parseFormalParameters(TreeBuilder&);
    template <class TreeBuilder> ALWAYS_INLINE TreeExpression parseVarDeclarationList(TreeBuilder&, int& declarations, const Identifier*& lastIdent, TreeExpression& lastInitializer, JSTextPosition& identStart, JSTextPosition& initStart, JSTextPosition& initEnd);
    template <class TreeBuilder> ALWAYS_INLINE TreeConstDeclList parseConstDeclarationList(TreeBuilder& context);
    template <FunctionRequirements, bool nameIsInContainingScope, class TreeBuilder> bool parseFunctionInfo(TreeBuilder&, const Identifier*&, TreeFormalParameterList&, TreeFunctionBody&, unsigned& openBraceOffset, unsigned& closeBraceOffset, int& bodyStartLine, unsigned& bodyStartColumn);
    ALWAYS_INLINE int isBinaryOperator(JSTokenType);
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
        return m_stack.isSafeToRecurse();
    }
    
    const JSTextPosition& lastTokenEndPosition() const
    {
        return m_lastTokenEndPosition;
    }

    bool hasError() const
    {
        return !m_errorMessage.isNull();
    }

    VM* m_vm;
    const SourceCode* m_source;
    ParserArena* m_arena;
    OwnPtr<LexerType> m_lexer;
    
    VMStackBounds m_stack;
    bool m_hasStackOverflow;
    String m_errorMessage;
    JSToken m_token;
    bool m_allowsIn;
    JSTextPosition m_lastTokenEndPosition;
    int m_assignmentCount;
    int m_nonLHSCount;
    bool m_syntaxAlreadyValidated;
    int m_statementDepth;
    int m_nonTrivialExpressionCount;
    const Identifier* m_lastIdentifier;
    RefPtr<SourceProviderCache> m_functionCache;
    SourceElements* m_sourceElements;
    ParserArenaData<DeclarationStacks::VarStack>* m_varDeclarations;
    ParserArenaData<DeclarationStacks::FunctionStack>* m_funcDeclarations;
    IdentifierSet m_capturedVariables;
    CodeFeatures m_features;
    int m_numConstants;
    
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
};


template <typename LexerType>
template <class ParsedNode>
PassRefPtr<ParsedNode> Parser<LexerType>::parse(ParserError& error)
{
    int errLine;
    String errMsg;

    if (ParsedNode::scopeIsFunction)
        m_lexer->setIsReparsing();

    m_sourceElements = 0;

    errLine = -1;
    errMsg = String();

    JSTokenLocation startLocation(tokenLocation());
    unsigned startColumn = m_source->startColumn();

    String parseError = parseInner();

    int lineNumber = m_lexer->lineNumber();
    bool lexError = m_lexer->sawError();
    String lexErrorMessage = lexError ? m_lexer->getErrorMessage() : String();
    ASSERT(lexErrorMessage.isNull() != lexError);
    m_lexer->clear();

    if (!parseError.isNull() || lexError) {
        errLine = lineNumber;
        errMsg = !lexErrorMessage.isNull() ? lexErrorMessage : parseError;
        m_sourceElements = 0;
    }

    RefPtr<ParsedNode> result;
    if (m_sourceElements) {
        JSTokenLocation endLocation;
        endLocation.line = m_lexer->lastLineNumber();
        endLocation.lineStartOffset = m_lexer->currentLineStartOffset();
        endLocation.startOffset = m_lexer->currentOffset();
        result = ParsedNode::create(m_vm,
                                    startLocation,
                                    endLocation,
                                    startColumn,
                                    m_sourceElements,
                                    m_varDeclarations ? &m_varDeclarations->data : 0,
                                    m_funcDeclarations ? &m_funcDeclarations->data : 0,
                                    m_capturedVariables,
                                    *m_source,
                                    m_features,
                                    m_numConstants);
        result->setLoc(m_source->firstLine(), m_lastTokenEndPosition.line, m_lexer->currentOffset(), m_lexer->currentLineStartOffset());
    } else {
        // We can never see a syntax error when reparsing a function, since we should have
        // reported the error when parsing the containing program or eval code. So if we're
        // parsing a function body node, we assume that what actually happened here is that
        // we ran out of stack while parsing. If we see an error while parsing eval or program
        // code we assume that it was a syntax error since running out of stack is much less
        // likely, and we are currently unable to distinguish between the two cases.
        if (isFunctionBodyNode(static_cast<ParsedNode*>(0)) || m_hasStackOverflow)
            error = ParserError(ParserError::StackOverflow, ParserError::SyntaxErrorNone, m_token);
        else {
            ParserError::SyntaxErrorType errorType = ParserError::SyntaxErrorIrrecoverable;
            if (m_token.m_type == EOFTOK)
                errorType = ParserError::SyntaxErrorRecoverable;
            else if (m_token.m_type & UnterminatedErrorTokenFlag)
                errorType = ParserError::SyntaxErrorUnterminatedLiteral;
            
            if (isEvalNode<ParsedNode>())
                error = ParserError(ParserError::EvalError, errorType, m_token, errMsg, errLine);
            else
                error = ParserError(ParserError::SyntaxError, errorType, m_token, errMsg, errLine);
        }
    }

    m_arena->reset();

    return result.release();
}

template <class ParsedNode>
PassRefPtr<ParsedNode> parse(VM* vm, const SourceCode& source, FunctionParameters* parameters, const Identifier& name, JSParserStrictness strictness, JSParserMode parserMode, ParserError& error)
{
    SamplingRegion samplingRegion("Parsing");

    ASSERT(!source.provider()->source().isNull());
    if (source.provider()->source().is8Bit()) {
        Parser< Lexer<LChar> > parser(vm, source, parameters, name, strictness, parserMode);
        return parser.parse<ParsedNode>(error);
    }
    Parser< Lexer<UChar> > parser(vm, source, parameters, name, strictness, parserMode);
    return parser.parse<ParsedNode>(error);
}

} // namespace
#endif
