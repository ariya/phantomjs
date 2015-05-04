//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Preprocessor.h"

#include <cassert>
#include <sstream>

#include "DiagnosticsBase.h"
#include "DirectiveParser.h"
#include "Macro.h"
#include "MacroExpander.h"
#include "Token.h"
#include "Tokenizer.h"

namespace pp
{

struct PreprocessorImpl
{
    Diagnostics *diagnostics;
    MacroSet macroSet;
    Tokenizer tokenizer;
    DirectiveParser directiveParser;
    MacroExpander macroExpander;

    PreprocessorImpl(Diagnostics *diag,
                     DirectiveHandler *directiveHandler)
        : diagnostics(diag),
          tokenizer(diag),
          directiveParser(&tokenizer, &macroSet, diag, directiveHandler),
          macroExpander(&directiveParser, &macroSet, diag)
    {
    }
};

Preprocessor::Preprocessor(Diagnostics *diagnostics,
                           DirectiveHandler *directiveHandler)
{
    mImpl = new PreprocessorImpl(diagnostics, directiveHandler);
}

Preprocessor::~Preprocessor()
{
    delete mImpl;
}

bool Preprocessor::init(size_t count,
                        const char * const string[],
                        const int length[])
{
    static const int kGLSLVersion = 100;

    // Add standard pre-defined macros.
    predefineMacro("__LINE__", 0);
    predefineMacro("__FILE__", 0);
    predefineMacro("__VERSION__", kGLSLVersion);
    predefineMacro("GL_ES", 1);

    return mImpl->tokenizer.init(count, string, length);
}

void Preprocessor::predefineMacro(const char *name, int value)
{
    std::ostringstream stream;
    stream << value;

    Token token;
    token.type = Token::CONST_INT;
    token.text = stream.str();

    Macro macro;
    macro.predefined = true;
    macro.type = Macro::kTypeObj;
    macro.name = name;
    macro.replacements.push_back(token);

    mImpl->macroSet[name] = macro;
}

void Preprocessor::lex(Token *token)
{
    bool validToken = false;
    while (!validToken)
    {
        mImpl->macroExpander.lex(token);
        switch (token->type)
        {
          // We should not be returning internal preprocessing tokens.
          // Convert preprocessing tokens to compiler tokens or report
          // diagnostics.
          case Token::PP_HASH:
            assert(false);
            break;
          case Token::PP_NUMBER:
            mImpl->diagnostics->report(Diagnostics::PP_INVALID_NUMBER,
                                       token->location, token->text);
            break;
          case Token::PP_OTHER:
            mImpl->diagnostics->report(Diagnostics::PP_INVALID_CHARACTER,
                                       token->location, token->text);
            break;
          default:
            validToken = true;
            break;
        }
    }
}

void Preprocessor::setMaxTokenSize(size_t maxTokenSize)
{
    mImpl->tokenizer.setMaxTokenSize(maxTokenSize);
}

}  // namespace pp
