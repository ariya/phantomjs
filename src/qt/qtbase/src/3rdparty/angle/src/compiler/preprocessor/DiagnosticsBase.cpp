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
                         const SourceLocation &loc,
                         const std::string &text)
{
    // TODO(alokp): Keep a count of errors and warnings.
    print(id, loc, text);
}

Diagnostics::Severity Diagnostics::severity(ID id)
{
    if ((id > PP_ERROR_BEGIN) && (id < PP_ERROR_END))
        return PP_ERROR;

    if ((id > PP_WARNING_BEGIN) && (id < PP_WARNING_END))
        return PP_WARNING;

    assert(false);
    return PP_ERROR;
}

std::string Diagnostics::message(ID id)
{
    switch (id)
    {
      // Errors begin.
      case PP_INTERNAL_ERROR:
        return "internal error";
      case PP_OUT_OF_MEMORY:
        return "out of memory";
      case PP_INVALID_CHARACTER:
        return "invalid character";
      case PP_INVALID_NUMBER:
        return "invalid number";
      case PP_INTEGER_OVERFLOW:
        return "integer overflow";
      case PP_FLOAT_OVERFLOW:
        return "float overflow";
      case PP_TOKEN_TOO_LONG:
        return "token too long";
      case PP_INVALID_EXPRESSION:
        return "invalid expression";
      case PP_DIVISION_BY_ZERO:
        return "division by zero";
      case PP_EOF_IN_COMMENT:
        return "unexpected end of file found in comment";
      case PP_UNEXPECTED_TOKEN:
        return "unexpected token";
      case PP_DIRECTIVE_INVALID_NAME:
        return "invalid directive name";
      case PP_MACRO_NAME_RESERVED:
        return "macro name is reserved";
      case PP_MACRO_REDEFINED:
        return "macro redefined";
      case PP_MACRO_PREDEFINED_REDEFINED:
        return "predefined macro redefined";
      case PP_MACRO_PREDEFINED_UNDEFINED:
        return "predefined macro undefined";
      case PP_MACRO_UNTERMINATED_INVOCATION:
        return "unterminated macro invocation";
      case PP_MACRO_TOO_FEW_ARGS:
        return "Not enough arguments for macro";
      case PP_MACRO_TOO_MANY_ARGS:
        return "Too many arguments for macro";
      case PP_CONDITIONAL_ENDIF_WITHOUT_IF:
        return "unexpected #endif found without a matching #if";
      case PP_CONDITIONAL_ELSE_WITHOUT_IF:
        return "unexpected #else found without a matching #if";
      case PP_CONDITIONAL_ELSE_AFTER_ELSE:
        return "unexpected #else found after another #else";
      case PP_CONDITIONAL_ELIF_WITHOUT_IF:
        return "unexpected #elif found without a matching #if";
      case PP_CONDITIONAL_ELIF_AFTER_ELSE:
        return "unexpected #elif found after #else";
      case PP_CONDITIONAL_UNTERMINATED:
        return "unexpected end of file found in conditional block";
      case PP_INVALID_EXTENSION_NAME:
        return "invalid extension name";
      case PP_INVALID_EXTENSION_BEHAVIOR:
        return "invalid extension behavior";
      case PP_INVALID_EXTENSION_DIRECTIVE:
        return "invalid extension directive";
      case PP_INVALID_VERSION_NUMBER:
        return "invalid version number";
      case PP_INVALID_VERSION_DIRECTIVE:
        return "invalid version directive";
      case PP_VERSION_NOT_FIRST_STATEMENT:
        return "#version directive must occur before anything else, "
               "except for comments and white space";
      case PP_INVALID_LINE_NUMBER:
        return "invalid line number";
      case PP_INVALID_FILE_NUMBER:
        return "invalid file number";
      case PP_INVALID_LINE_DIRECTIVE:
        return "invalid line directive";
      // Errors end.
      // Warnings begin.
      case PP_EOF_IN_DIRECTIVE:
        return "unexpected end of file found in directive";
      case PP_CONDITIONAL_UNEXPECTED_TOKEN:
        return "unexpected token after conditional expression";
      case PP_UNRECOGNIZED_PRAGMA:
        return "unrecognized pragma";
      // Warnings end.
      default:
        assert(false);
        return "";
    }
}

}  // namespace pp
