//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_MACRO_EXPANDER_H_
#define COMPILER_PREPROCESSOR_MACRO_EXPANDER_H_

#include <cassert>
#include <memory>
#include <vector>

#include "Lexer.h"
#include "Macro.h"
#include "pp_utils.h"

namespace pp
{

class Diagnostics;

class MacroExpander : public Lexer
{
  public:
    MacroExpander(Lexer *lexer, MacroSet *macroSet, Diagnostics *diagnostics);
    virtual ~MacroExpander();

    virtual void lex(Token *token);

  private:
    PP_DISALLOW_COPY_AND_ASSIGN(MacroExpander);

    void getToken(Token *token);
    void ungetToken(const Token &token);
    bool isNextTokenLeftParen();

    bool pushMacro(const Macro &macro, const Token &identifier);
    void popMacro();

    bool expandMacro(const Macro &macro,
                     const Token &identifier,
                     std::vector<Token> *replacements);

    typedef std::vector<Token> MacroArg;
    bool collectMacroArgs(const Macro &macro,
                          const Token &identifier,
                          std::vector<MacroArg> *args);
    void replaceMacroParams(const Macro &macro,
                            const std::vector<MacroArg> &args,
                            std::vector<Token> *replacements);

    struct MacroContext
    {
        const Macro *macro;
        std::size_t index;
        std::vector<Token> replacements;

        MacroContext()
            : macro(0),
              index(0)
        {
        }
        bool empty() const
        {
            return index == replacements.size();
        }
        const Token &get()
        {
            return replacements[index++];
        }
        void unget()
        {
            assert(index > 0);
            --index;
        }
    };

    Lexer *mLexer;
    MacroSet *mMacroSet;
    Diagnostics *mDiagnostics;

    std::auto_ptr<Token> mReserveToken;
    std::vector<MacroContext *> mContextStack;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_MACRO_EXPANDER_H_

