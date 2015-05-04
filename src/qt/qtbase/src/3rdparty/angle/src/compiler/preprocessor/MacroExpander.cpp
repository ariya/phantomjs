//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "MacroExpander.h"

#include <algorithm>
#include <sstream>

#include "DiagnosticsBase.h"
#include "Token.h"

namespace pp
{

class TokenLexer : public Lexer
{
 public:
    typedef std::vector<Token> TokenVector;

    TokenLexer(TokenVector *tokens)
    {
        tokens->swap(mTokens);
        mIter = mTokens.begin();
    }

    virtual void lex(Token *token)
    {
        if (mIter == mTokens.end())
        {
            token->reset();
            token->type = Token::LAST;
        }
        else
        {
            *token = *mIter++;
        }
    }

 private:
    PP_DISALLOW_COPY_AND_ASSIGN(TokenLexer);

    TokenVector mTokens;
    TokenVector::const_iterator mIter;
};

MacroExpander::MacroExpander(Lexer *lexer,
                             MacroSet *macroSet,
                             Diagnostics *diagnostics)
    : mLexer(lexer),
      mMacroSet(macroSet),
      mDiagnostics(diagnostics)
{
}

MacroExpander::~MacroExpander()
{
    for (std::size_t i = 0; i < mContextStack.size(); ++i)
    {
        delete mContextStack[i];
    }
}

void MacroExpander::lex(Token *token)
{
    while (true)
    {
        getToken(token);

        if (token->type != Token::IDENTIFIER)
            break;

        if (token->expansionDisabled())
            break;

        MacroSet::const_iterator iter = mMacroSet->find(token->text);
        if (iter == mMacroSet->end())
            break;

        const Macro& macro = iter->second;
        if (macro.disabled)
        {
            // If a particular token is not expanded, it is never expanded.
            token->setExpansionDisabled(true);
            break;
        }
        if ((macro.type == Macro::kTypeFunc) && !isNextTokenLeftParen())
        {
            // If the token immediately after the macro name is not a '(',
            // this macro should not be expanded.
            break;
        }

        pushMacro(macro, *token);
    }
}

void MacroExpander::getToken(Token *token)
{
    if (mReserveToken.get())
    {
        *token = *mReserveToken;
        mReserveToken.reset();
        return;
    }

    // First pop all empty macro contexts.
    while (!mContextStack.empty() && mContextStack.back()->empty())
    {
        popMacro();
    }

    if (!mContextStack.empty())
    {
        *token = mContextStack.back()->get();
    }
    else
    {
        mLexer->lex(token);
    }
}

void MacroExpander::ungetToken(const Token &token)
{
    if (!mContextStack.empty())
    {
        MacroContext *context = mContextStack.back();
        context->unget();
        assert(context->replacements[context->index] == token);
    }
    else
    {
        assert(!mReserveToken.get());
        mReserveToken.reset(new Token(token));
    }
}

bool MacroExpander::isNextTokenLeftParen()
{
    Token token;
    getToken(&token);

    bool lparen = token.type == '(';
    ungetToken(token);

    return lparen;
}

bool MacroExpander::pushMacro(const Macro &macro, const Token &identifier)
{
    assert(!macro.disabled);
    assert(!identifier.expansionDisabled());
    assert(identifier.type == Token::IDENTIFIER);
    assert(identifier.text == macro.name);

    std::vector<Token> replacements;
    if (!expandMacro(macro, identifier, &replacements))
        return false;

    // Macro is disabled for expansion until it is popped off the stack.
    macro.disabled = true;

    MacroContext *context = new MacroContext;
    context->macro = &macro;
    context->replacements.swap(replacements);
    mContextStack.push_back(context);
    return true;
}

void MacroExpander::popMacro()
{
    assert(!mContextStack.empty());

    MacroContext *context = mContextStack.back();
    mContextStack.pop_back();

    assert(context->empty());
    assert(context->macro->disabled);
    context->macro->disabled = false;
    delete context;
}

bool MacroExpander::expandMacro(const Macro &macro,
                                const Token &identifier,
                                std::vector<Token> *replacements)
{
    replacements->clear();
    if (macro.type == Macro::kTypeObj)
    {
        replacements->assign(macro.replacements.begin(),
                             macro.replacements.end());

        if (macro.predefined)
        {
            const char kLine[] = "__LINE__";
            const char kFile[] = "__FILE__";

            assert(replacements->size() == 1);
            Token& repl = replacements->front();
            if (macro.name == kLine)
            {
                std::ostringstream stream;
                stream << identifier.location.line;
                repl.text = stream.str();
            }
            else if (macro.name == kFile)
            {
                std::ostringstream stream;
                stream << identifier.location.file;
                repl.text = stream.str();
            }
        }
    }
    else
    {
        assert(macro.type == Macro::kTypeFunc);
        std::vector<MacroArg> args;
        args.reserve(macro.parameters.size());
        if (!collectMacroArgs(macro, identifier, &args))
            return false;

        replaceMacroParams(macro, args, replacements);
    }

    for (std::size_t i = 0; i < replacements->size(); ++i)
    {
        Token& repl = replacements->at(i);
        if (i == 0)
        {
            // The first token in the replacement list inherits the padding
            // properties of the identifier token.
            repl.setAtStartOfLine(identifier.atStartOfLine());
            repl.setHasLeadingSpace(identifier.hasLeadingSpace());
        }
        repl.location = identifier.location;
    }
    return true;
}

bool MacroExpander::collectMacroArgs(const Macro &macro,
                                     const Token &identifier,
                                     std::vector<MacroArg> *args)
{
    Token token;
    getToken(&token);
    assert(token.type == '(');

    args->push_back(MacroArg());
    for (int openParens = 1; openParens != 0; )
    {
        getToken(&token);

        if (token.type == Token::LAST)
        {
            mDiagnostics->report(Diagnostics::PP_MACRO_UNTERMINATED_INVOCATION,
                                 identifier.location, identifier.text);
            // Do not lose EOF token.
            ungetToken(token);
            return false;
        }

        bool isArg = false; // True if token is part of the current argument.
        switch (token.type)
        {
          case '(':
            ++openParens;
            isArg = true;
            break;
          case ')':
            --openParens;
            isArg = openParens != 0;
            break;
          case ',':
            // The individual arguments are separated by comma tokens, but
            // the comma tokens between matching inner parentheses do not
            // seperate arguments.
            if (openParens == 1)
                args->push_back(MacroArg());
            isArg = openParens != 1;
            break;
          default:
            isArg = true;
            break;
        }
        if (isArg)
        {
            MacroArg &arg = args->back();
            // Initial whitespace is not part of the argument.
            if (arg.empty())
                token.setHasLeadingSpace(false);
            arg.push_back(token);
        }
    }

    const Macro::Parameters &params = macro.parameters;
    // If there is only one empty argument, it is equivalent to no argument.
    if (params.empty() && (args->size() == 1) && args->front().empty())
    {
        args->clear();
    }
    // Validate the number of arguments.
    if (args->size() != params.size())
    {
        Diagnostics::ID id = args->size() < macro.parameters.size() ?
            Diagnostics::PP_MACRO_TOO_FEW_ARGS :
            Diagnostics::PP_MACRO_TOO_MANY_ARGS;
        mDiagnostics->report(id, identifier.location, identifier.text);
        return false;
    }

    // Pre-expand each argument before substitution.
    // This step expands each argument individually before they are
    // inserted into the macro body.
    for (std::size_t i = 0; i < args->size(); ++i)
    {
        MacroArg &arg = args->at(i);
        TokenLexer lexer(&arg);
        MacroExpander expander(&lexer, mMacroSet, mDiagnostics);

        arg.clear();
        expander.lex(&token);
        while (token.type != Token::LAST)
        {
            arg.push_back(token);
            expander.lex(&token);
        }
    }
    return true;
}

void MacroExpander::replaceMacroParams(const Macro &macro,
                                       const std::vector<MacroArg> &args,
                                       std::vector<Token> *replacements)
{
    for (std::size_t i = 0; i < macro.replacements.size(); ++i)
    {
        const Token &repl = macro.replacements[i];
        if (repl.type != Token::IDENTIFIER)
        {
            replacements->push_back(repl);
            continue;
        }

        // TODO(alokp): Optimize this.
        // There is no need to search for macro params every time.
        // The param index can be cached with the replacement token.
        Macro::Parameters::const_iterator iter = std::find(
            macro.parameters.begin(), macro.parameters.end(), repl.text);
        if (iter == macro.parameters.end())
        {
            replacements->push_back(repl);
            continue;
        }

        std::size_t iArg = std::distance(macro.parameters.begin(), iter);
        const MacroArg &arg = args[iArg];
        if (arg.empty())
        {
            continue;
        }
        std::size_t iRepl = replacements->size();
        replacements->insert(replacements->end(), arg.begin(), arg.end());
        // The replacement token inherits padding properties from
        // macro replacement token.
        replacements->at(iRepl).setHasLeadingSpace(repl.hasLeadingSpace());
    }
}

}  // namespace pp

