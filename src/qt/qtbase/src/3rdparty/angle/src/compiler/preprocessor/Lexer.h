//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_LEXER_H_
#define COMPILER_PREPROCESSOR_LEXER_H_

namespace pp
{

struct Token;

class Lexer
{
  public:
    virtual ~Lexer();

    virtual void lex(Token *token) = 0;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_LEXER_H_

