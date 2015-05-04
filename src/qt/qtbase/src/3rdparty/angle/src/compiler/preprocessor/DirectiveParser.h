//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_DIRECTIVE_PARSER_H_
#define COMPILER_PREPROCESSOR_DIRECTIVE_PARSER_H_

#include "Lexer.h"
#include "Macro.h"
#include "pp_utils.h"
#include "SourceLocation.h"

namespace pp
{

class Diagnostics;
class DirectiveHandler;
class Tokenizer;

class DirectiveParser : public Lexer
{
  public:
    DirectiveParser(Tokenizer *tokenizer,
                    MacroSet *macroSet,
                    Diagnostics *diagnostics,
                    DirectiveHandler *directiveHandler);

    virtual void lex(Token *token);

  private:
    PP_DISALLOW_COPY_AND_ASSIGN(DirectiveParser);

    void parseDirective(Token *token);
    void parseDefine(Token *token);
    void parseUndef(Token *token);
    void parseIf(Token *token);
    void parseIfdef(Token *token);
    void parseIfndef(Token *token);
    void parseElse(Token *token);
    void parseElif(Token *token);
    void parseEndif(Token *token);
    void parseError(Token *token);
    void parsePragma(Token *token);
    void parseExtension(Token *token);
    void parseVersion(Token *token);
    void parseLine(Token *token);

    bool skipping() const;
    void parseConditionalIf(Token *token);
    int parseExpressionIf(Token *token);
    int parseExpressionIfdef(Token *token);

    struct ConditionalBlock
    {
        std::string type;
        SourceLocation location;
        bool skipBlock;
        bool skipGroup;
        bool foundValidGroup;
        bool foundElseGroup;

        ConditionalBlock()
            : skipBlock(false),
              skipGroup(false),
              foundValidGroup(false),
              foundElseGroup(false)
        {
        }
    };
    bool mPastFirstStatement;
    std::vector<ConditionalBlock> mConditionalStack;
    Tokenizer *mTokenizer;
    MacroSet *mMacroSet;
    Diagnostics *mDiagnostics;
    DirectiveHandler *mDirectiveHandler;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_DIRECTIVE_PARSER_H_

