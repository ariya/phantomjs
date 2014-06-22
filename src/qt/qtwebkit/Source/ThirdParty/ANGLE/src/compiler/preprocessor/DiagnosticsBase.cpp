//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "DiagnosticsBase.h"

#include <cassert>

namespace pp
{

Diagnostics::~Diagnostics()
{
}

void Diagnostics::report(ID id,
                         const SourceLocation& loc,
                         const std::string& text)
{
    // TODO(alokp): Keep a count of errors and warnings.
    print(id, loc, text);
}

Diagnostics::Severity Diagnostics::severity(ID id)
{
    if ((id > ERROR_BEGIN) && (id < ERROR_END))
        return ERROR;

    if ((id > WARNING_BEGIN) && (id < WARNING_END))
        return WARNING;

    assert(false);
    return ERROR;
}

std::string Diagnostics::message(ID id)
{
    switch (id)
    {
      // Errors begin.
      case INTERNAL_ERROR:
          return "internal error";
      case OUT_OF_MEMORY:
          return "out of memory";
      case INVALID_CHARACTER:
          return "invalid character";
      case INVALID_NUMBER:
          return "invalid number";
      case INTEGER_OVERFLOW:
          return "integer overflow";
      case FLOAT_OVERFLOW:
          return "float overflow";
      case TOKEN_TOO_LONG:
          return "token too long";
      case INVALID_EXPRESSION:
          return "invalid expression";
      case DIVISION_BY_ZERO:
          return "division by zero";
      case EOF_IN_COMMENT:
          return "unexpected end of file found in comment";
      case UNEXPECTED_TOKEN:
          return "unexpected token";
      case DIRECTIVE_INVALID_NAME:
          return "invalid directive name";
      case MACRO_NAME_RESERVED:
          return "macro name is reserved";
      case MACRO_REDEFINED:
          return "macro redefined";
      case MACRO_PREDEFINED_REDEFINED:
          return "predefined macro redefined";
      case MACRO_PREDEFINED_UNDEFINED:
          return "predefined macro undefined";
      case MACRO_UNTERMINATED_INVOCATION:
          return "unterminated macro invocation";
      case MACRO_TOO_FEW_ARGS:
          return "Not enough arguments for macro";
      case MACRO_TOO_MANY_ARGS:
          return "Too many arguments for macro";
      case CONDITIONAL_ENDIF_WITHOUT_IF:
          return "unexpected #endif found without a matching #if";
      case CONDITIONAL_ELSE_WITHOUT_IF:
          return "unexpected #else found without a matching #if";
      case CONDITIONAL_ELSE_AFTER_ELSE:
          return "unexpected #else found after another #else";
      case CONDITIONAL_ELIF_WITHOUT_IF:
          return "unexpected #elif found without a matching #if";
      case CONDITIONAL_ELIF_AFTER_ELSE:
          return "unexpected #elif found after #else";
      case CONDITIONAL_UNTERMINATED:
          return "unexpected end of file found in conditional block";
      case INVALID_EXTENSION_NAME:
          return "invalid extension name";
      case INVALID_EXTENSION_BEHAVIOR:
          return "invalid extension behavior";
      case INVALID_EXTENSION_DIRECTIVE:
          return "invalid extension directive";
      case INVALID_VERSION_NUMBER:
          return "invalid version number";
      case INVALID_VERSION_DIRECTIVE:
          return "invalid version directive";
      case VERSION_NOT_FIRST_STATEMENT:
        return "#version directive must occur before anything else, "
               "except for comments and white space";
      case INVALID_LINE_NUMBER:
          return "invalid line number";
      case INVALID_FILE_NUMBER:
          return "invalid file number";
      case INVALID_LINE_DIRECTIVE:
          return "invalid line directive";
      // Errors end.
      // Warnings begin.
      case EOF_IN_DIRECTIVE:
          return "unexpected end of file found in directive";
      case CONDITIONAL_UNEXPECTED_TOKEN:
          return "unexpected token after conditional expression";
      case UNRECOGNIZED_PRAGMA:
          return "unrecognized pragma";
      // Warnings end.
      default:
          assert(false);
          return "";
    }
}

}  // namespace pp
