//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_EXPRESSION_PARSER_H_
#define COMPILER_PREPROCESSOR_EXPRESSION_PARSER_H_

#include "pp_utils.h"

namespace pp
{

class Diagnostics;
class Lexer;
struct Token;

class ExpressionParser
{
  public:
    ExpressionParser(Lexer *lexer, Diagnostics *diagnostics);

    bool parse(Token *token, int *result);

  private:
    PP_DISALLOW_COPY_AND_ASSIGN(ExpressionParser);

    Lexer *mLexer;
    Diagnostics *mDiagnostics;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_EXPRESSION_PARSER_H_
