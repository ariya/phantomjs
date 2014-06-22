//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "DirectiveParser.h"

#include <cassert>
#include <cstdlib>
#include <sstream>

#include "DiagnosticsBase.h"
#include "DirectiveHandlerBase.h"
#include "ExpressionParser.h"
#include "MacroExpander.h"
#include "Token.h"
#include "Tokenizer.h"

namespace {
enum DirectiveType
{
    DIRECTIVE_NONE,
    DIRECTIVE_DEFINE,
    DIRECTIVE_UNDEF,
    DIRECTIVE_IF,
    DIRECTIVE_IFDEF,
    DIRECTIVE_IFNDEF,
    DIRECTIVE_ELSE,
    DIRECTIVE_ELIF,
    DIRECTIVE_ENDIF,
    DIRECTIVE_ERROR,
    DIRECTIVE_PRAGMA,
    DIRECTIVE_EXTENSION,
    DIRECTIVE_VERSION,
    DIRECTIVE_LINE
};
}  // namespace

static DirectiveType getDirective(const pp::Token* token)
{
    static const std::string kDirectiveDefine("define");
    static const std::string kDirectiveUndef("undef");
    static const std::string kDirectiveIf("if");
    static const std::string kDirectiveIfdef("ifdef");
    static const std::string kDirectiveIfndef("ifndef");
    static const std::string kDirectiveElse("else");
    static const std::string kDirectiveElif("elif");
    static const std::string kDirectiveEndif("endif");
    static const std::string kDirectiveError("error");
    static const std::string kDirectivePragma("pragma");
    static const std::string kDirectiveExtension("extension");
    static const std::string kDirectiveVersion("version");
    static const std::string kDirectiveLine("line");

    if (token->type != pp::Token::IDENTIFIER)
        return DIRECTIVE_NONE;

    if (token->text == kDirectiveDefine)
        return DIRECTIVE_DEFINE;
    else if (token->text == kDirectiveUndef)
        return DIRECTIVE_UNDEF;
    else if (token->text == kDirectiveIf)
        return DIRECTIVE_IF;
    else if (token->text == kDirectiveIfdef)
        return DIRECTIVE_IFDEF;
    else if (token->text == kDirectiveIfndef)
        return DIRECTIVE_IFNDEF;
    else if (token->text == kDirectiveElse)
        return DIRECTIVE_ELSE;
    else if (token->text == kDirectiveElif)
        return DIRECTIVE_ELIF;
    else if (token->text == kDirectiveEndif)
        return DIRECTIVE_ENDIF;
    else if (token->text == kDirectiveError)
        return DIRECTIVE_ERROR;
    else if (token->text == kDirectivePragma)
        return DIRECTIVE_PRAGMA;
    else if (token->text == kDirectiveExtension)
        return DIRECTIVE_EXTENSION;
    else if (token->text == kDirectiveVersion)
        return DIRECTIVE_VERSION;
    else if (token->text == kDirectiveLine)
        return DIRECTIVE_LINE;

    return DIRECTIVE_NONE;
}

static bool isConditionalDirective(DirectiveType directive)
{
    switch (directive)
    {
      case DIRECTIVE_IF:
      case DIRECTIVE_IFDEF:
      case DIRECTIVE_IFNDEF:
      case DIRECTIVE_ELSE:
      case DIRECTIVE_ELIF:
      case DIRECTIVE_ENDIF:
        return true;
      default:
        return false;
    }
}

// Returns true if the token represents End Of Directive.
static bool isEOD(const pp::Token* token)
{
    return (token->type == '\n') || (token->type == pp::Token::LAST);
}

static void skipUntilEOD(pp::Lexer* lexer, pp::Token* token)
{
    while(!isEOD(token))
    {
        lexer->lex(token);
    }
}

static bool isMacroNameReserved(const std::string& name)
{
    // Names prefixed with "GL_" are reserved.
    if (name.substr(0, 3) == "GL_")
        return true;

    // Names containing two consecutive underscores are reserved.
    if (name.find("__") != std::string::npos)
        return true;

    return false;
}

static bool isMacroPredefined(const std::string& name,
                              const pp::MacroSet& macroSet)
{
    pp::MacroSet::const_iterator iter = macroSet.find(name);
    return iter != macroSet.end() ? iter->second.predefined : false;
}

namespace pp
{

class DefinedParser : public Lexer
{
  public:
    DefinedParser(Lexer* lexer,
                  const MacroSet* macroSet,
                  Diagnostics* diagnostics) :
        mLexer(lexer),
        mMacroSet(macroSet),
        mDiagnostics(diagnostics)
    {
    }

  protected:
    virtual void lex(Token* token)
    {
        static const std::string kDefined("defined");

        mLexer->lex(token);
        if (token->type != Token::IDENTIFIER)
            return;
        if (token->text != kDefined)
            return;

        bool paren = false;
        mLexer->lex(token);
        if (token->type == '(')
        {
            paren = true;
            mLexer->lex(token);
        }

        if (token->type != Token::IDENTIFIER)
        {
            mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                                 token->location, token->text);
            skipUntilEOD(mLexer, token);
            return;
        }
        MacroSet::const_iterator iter = mMacroSet->find(token->text);
        std::string expression = iter != mMacroSet->end() ? "1" : "0";

        if (paren)
        {
            mLexer->lex(token);
            if (token->type != ')')
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                                     token->location, token->text);
                skipUntilEOD(mLexer, token);
                return;
            }
        }

        // We have a valid defined operator.
        // Convert the current token into a CONST_INT token.
        token->type = Token::CONST_INT;
        token->text = expression;
    }

  private:
    Lexer* mLexer;
    const MacroSet* mMacroSet;
    Diagnostics* mDiagnostics;
};

DirectiveParser::DirectiveParser(Tokenizer* tokenizer,
                                 MacroSet* macroSet,
                                 Diagnostics* diagnostics,
                                 DirectiveHandler* directiveHandler) :
    mPastFirstStatement(false),
    mTokenizer(tokenizer),
    mMacroSet(macroSet),
    mDiagnostics(diagnostics),
    mDirectiveHandler(directiveHandler)
{
}

void DirectiveParser::lex(Token* token)
{
    do
    {
        mTokenizer->lex(token);

        if (token->type == Token::PP_HASH)
        {
            parseDirective(token);
            mPastFirstStatement = true;
        }

        if (token->type == Token::LAST)
        {
            if (!mConditionalStack.empty())
            {
                const ConditionalBlock& block = mConditionalStack.back();
                mDiagnostics->report(Diagnostics::CONDITIONAL_UNTERMINATED,
                                     block.location, block.type);
            }
            break;
        }

    } while (skipping() || (token->type == '\n'));

    mPastFirstStatement = true;
}

void DirectiveParser::parseDirective(Token* token)
{
    assert(token->type == Token::PP_HASH);

    mTokenizer->lex(token);
    if (isEOD(token))
    {
        // Empty Directive.
        return;
    }

    DirectiveType directive = getDirective(token);

    // While in an excluded conditional block/group,
    // we only parse conditional directives.
    if (skipping() && !isConditionalDirective(directive))
    {
        skipUntilEOD(mTokenizer, token);
        return;
    }

    switch(directive)
    {
      case DIRECTIVE_NONE:
        mDiagnostics->report(Diagnostics::DIRECTIVE_INVALID_NAME,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        break;
      case DIRECTIVE_DEFINE:
        parseDefine(token);
        break;
      case DIRECTIVE_UNDEF:
        parseUndef(token);
        break;
      case DIRECTIVE_IF:
        parseIf(token);
        break;
      case DIRECTIVE_IFDEF:
        parseIfdef(token);
        break;
      case DIRECTIVE_IFNDEF:
        parseIfndef(token);
        break;
      case DIRECTIVE_ELSE:
        parseElse(token);
        break;
      case DIRECTIVE_ELIF:
        parseElif(token);
        break;
      case DIRECTIVE_ENDIF:
        parseEndif(token);
        break;
      case DIRECTIVE_ERROR:
        parseError(token);
        break;
      case DIRECTIVE_PRAGMA:
        parsePragma(token);
        break;
      case DIRECTIVE_EXTENSION:
        parseExtension(token);
        break;
      case DIRECTIVE_VERSION:
        parseVersion(token);
        break;
      case DIRECTIVE_LINE:
        parseLine(token);
        break;
      default:
        assert(false);
        break;
    }

    skipUntilEOD(mTokenizer, token);
    if (token->type == Token::LAST)
    {
        mDiagnostics->report(Diagnostics::EOF_IN_DIRECTIVE,
                             token->location, token->text);
    }
}

void DirectiveParser::parseDefine(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_DEFINE);

    mTokenizer->lex(token);
    if (token->type != Token::IDENTIFIER)
    {
        mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                             token->location, token->text);
        return;
    }
    if (isMacroPredefined(token->text, *mMacroSet))
    {
        mDiagnostics->report(Diagnostics::MACRO_PREDEFINED_REDEFINED,
                             token->location, token->text);
        return;
    }
    if (isMacroNameReserved(token->text))
    {
        mDiagnostics->report(Diagnostics::MACRO_NAME_RESERVED,
                             token->location, token->text);
        return;
    }

    Macro macro;
    macro.type = Macro::kTypeObj;
    macro.name = token->text;

    mTokenizer->lex(token);
    if (token->type == '(' && !token->hasLeadingSpace())
    {
        // Function-like macro. Collect arguments.
        macro.type = Macro::kTypeFunc;
        do {
            mTokenizer->lex(token);
            if (token->type != Token::IDENTIFIER)
                break;
            macro.parameters.push_back(token->text);

            mTokenizer->lex(token);  // Get ','.
        } while (token->type == ',');

        if (token->type != ')')
        {
            mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                                 token->location,
                                 token->text);
            return;
        }
        mTokenizer->lex(token);  // Get ')'.
    }

    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        // Reset the token location because it is unnecessary in replacement
        // list. Resetting it also allows us to reuse Token::equals() to
        // compare macros.
        token->location = SourceLocation();
        macro.replacements.push_back(*token);
        mTokenizer->lex(token);
    }
    if (!macro.replacements.empty())
    {
        // Whitespace preceding the replacement list is not considered part of
        // the replacement list for either form of macro.
        macro.replacements.front().setHasLeadingSpace(false);
    }

    // Check for macro redefinition.
    MacroSet::const_iterator iter = mMacroSet->find(macro.name);
    if (iter != mMacroSet->end() && !macro.equals(iter->second))
    {
        mDiagnostics->report(Diagnostics::MACRO_REDEFINED,
                             token->location,
                             macro.name);
        return;
    }
    mMacroSet->insert(std::make_pair(macro.name, macro));
}

void DirectiveParser::parseUndef(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_UNDEF);

    mTokenizer->lex(token);
    if (token->type != Token::IDENTIFIER)
    {
        mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                             token->location, token->text);
        return;
    }

    MacroSet::iterator iter = mMacroSet->find(token->text);
    if (iter != mMacroSet->end())
    {
        if (iter->second.predefined)
        {
            mDiagnostics->report(Diagnostics::MACRO_PREDEFINED_UNDEFINED,
                                 token->location, token->text);
        }
        else
        {
            mMacroSet->erase(iter);
        }
    }

    mTokenizer->lex(token);
}

void DirectiveParser::parseIf(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_IF);
    parseConditionalIf(token);
}

void DirectiveParser::parseIfdef(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_IFDEF);
    parseConditionalIf(token);
}

void DirectiveParser::parseIfndef(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_IFNDEF);
    parseConditionalIf(token);
}

void DirectiveParser::parseElse(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_ELSE);

    if (mConditionalStack.empty())
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_ELSE_WITHOUT_IF,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        return;
    }

    ConditionalBlock& block = mConditionalStack.back();
    if (block.skipBlock)
    {
        // No diagnostics. Just skip the whole line.
        skipUntilEOD(mTokenizer, token);
        return;
    }
    if (block.foundElseGroup)
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_ELSE_AFTER_ELSE,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        return;
    }

    block.foundElseGroup = true;
    block.skipGroup = block.foundValidGroup;
    block.foundValidGroup = true;

    // Warn if there are extra tokens after #else.
    mTokenizer->lex(token);
    if (!isEOD(token))
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_UNEXPECTED_TOKEN,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
    }
}

void DirectiveParser::parseElif(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_ELIF);

    if (mConditionalStack.empty())
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_ELIF_WITHOUT_IF,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        return;
    }

    ConditionalBlock& block = mConditionalStack.back();
    if (block.skipBlock)
    {
        // No diagnostics. Just skip the whole line.
        skipUntilEOD(mTokenizer, token);
        return;
    }
    if (block.foundElseGroup)
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_ELIF_AFTER_ELSE,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        return;
    }
    if (block.foundValidGroup)
    {
        // Do not parse the expression.
        // Also be careful not to emit a diagnostic.
        block.skipGroup = true;
        skipUntilEOD(mTokenizer, token);
        return;
    }

    int expression = parseExpressionIf(token);
    block.skipGroup = expression == 0;
    block.foundValidGroup = expression != 0;
}

void DirectiveParser::parseEndif(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_ENDIF);

    if (mConditionalStack.empty())
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_ENDIF_WITHOUT_IF,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        return;
    }

    mConditionalStack.pop_back();

    // Warn if there are tokens after #endif.
    mTokenizer->lex(token);
    if (!isEOD(token))
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_UNEXPECTED_TOKEN,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
    }
}

void DirectiveParser::parseError(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_ERROR);

    std::ostringstream stream;
    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        stream << *token;
        mTokenizer->lex(token);
    }
    mDirectiveHandler->handleError(token->location, stream.str());
}

// Parses pragma of form: #pragma name[(value)].
void DirectiveParser::parsePragma(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_PRAGMA);

    enum State
    {
        PRAGMA_NAME,
        LEFT_PAREN,
        PRAGMA_VALUE,
        RIGHT_PAREN
    };

    bool valid = true;
    std::string name, value;
    int state = PRAGMA_NAME;

    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        switch(state++)
        {
          case PRAGMA_NAME:
            name = token->text;
            valid = valid && (token->type == Token::IDENTIFIER);
            break;
          case LEFT_PAREN:
            valid = valid && (token->type == '(');
            break;
          case PRAGMA_VALUE:
            value = token->text;
            valid = valid && (token->type == Token::IDENTIFIER);
            break;
          case RIGHT_PAREN:
            valid = valid && (token->type == ')');
            break;
          default:
            valid = false;
            break;
        }
        mTokenizer->lex(token);
    }

    valid = valid && ((state == PRAGMA_NAME) ||     // Empty pragma.
                      (state == LEFT_PAREN) ||      // Without value.
                      (state == RIGHT_PAREN + 1));  // With value.
    if (!valid)
    {
        mDiagnostics->report(Diagnostics::UNRECOGNIZED_PRAGMA,
                             token->location, name);
    }
    else if (state > PRAGMA_NAME)  // Do not notify for empty pragma.
    {
        mDirectiveHandler->handlePragma(token->location, name, value);
    }
}

void DirectiveParser::parseExtension(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_EXTENSION);

    enum State
    {
        EXT_NAME,
        COLON,
        EXT_BEHAVIOR
    };

    bool valid = true;
    std::string name, behavior;
    int state = EXT_NAME;

    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        switch (state++)
        {
          case EXT_NAME:
            if (valid && (token->type != Token::IDENTIFIER))
            {
                mDiagnostics->report(Diagnostics::INVALID_EXTENSION_NAME,
                                     token->location, token->text);
                valid = false;
            }
            if (valid) name = token->text;
            break;
          case COLON:
            if (valid && (token->type != ':'))
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                                     token->location, token->text);
                valid = false;
            }
            break;
          case EXT_BEHAVIOR:
            if (valid && (token->type != Token::IDENTIFIER))
            {
                mDiagnostics->report(Diagnostics::INVALID_EXTENSION_BEHAVIOR,
                                     token->location, token->text);
                valid = false;
            }
            if (valid) behavior = token->text;
            break;
          default:
            if (valid)
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                                     token->location, token->text);
                valid = false;
            }
            break;
        }
        mTokenizer->lex(token);
    }
    if (valid && (state != EXT_BEHAVIOR + 1))
    {
        mDiagnostics->report(Diagnostics::INVALID_EXTENSION_DIRECTIVE,
                             token->location, token->text);
        valid = false;
    }
    if (valid)
        mDirectiveHandler->handleExtension(token->location, name, behavior);
}

void DirectiveParser::parseVersion(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_VERSION);

    if (mPastFirstStatement)
    {
        mDiagnostics->report(Diagnostics::VERSION_NOT_FIRST_STATEMENT,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        return;
    }

    enum State
    {
        VERSION_NUMBER
    };

    bool valid = true;
    int version = 0;
    int state = VERSION_NUMBER;

    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        switch (state++)
        {
          case VERSION_NUMBER:
            if (valid && (token->type != Token::CONST_INT))
            {
                mDiagnostics->report(Diagnostics::INVALID_VERSION_NUMBER,
                                     token->location, token->text);
                valid = false;
            }
            if (valid && !token->iValue(&version))
            {
                mDiagnostics->report(Diagnostics::INTEGER_OVERFLOW,
                                     token->location, token->text);
                valid = false;
            }
            break;
          default:
            if (valid)
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                                     token->location, token->text);
                valid = false;
            }
            break;
        }
        mTokenizer->lex(token);
    }
    if (valid && (state != VERSION_NUMBER + 1))
    {
        mDiagnostics->report(Diagnostics::INVALID_VERSION_DIRECTIVE,
                             token->location, token->text);
        valid = false;
    }
    if (valid)
        mDirectiveHandler->handleVersion(token->location, version);
}

void DirectiveParser::parseLine(Token* token)
{
    assert(getDirective(token) == DIRECTIVE_LINE);

    enum State
    {
        LINE_NUMBER,
        FILE_NUMBER
    };

    bool valid = true;
    int line = 0, file = 0;
    int state = LINE_NUMBER;

    MacroExpander macroExpander(mTokenizer, mMacroSet, mDiagnostics);
    macroExpander.lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        switch (state++)
        {
          case LINE_NUMBER:
            if (valid && (token->type != Token::CONST_INT))
            {
                mDiagnostics->report(Diagnostics::INVALID_LINE_NUMBER,
                                     token->location, token->text);
                valid = false;
            }
            if (valid && !token->iValue(&line))
            {
                mDiagnostics->report(Diagnostics::INTEGER_OVERFLOW,
                                     token->location, token->text);
                valid = false;
            }
            break;
          case FILE_NUMBER:
            if (valid && (token->type != Token::CONST_INT))
            {
                mDiagnostics->report(Diagnostics::INVALID_FILE_NUMBER,
                                     token->location, token->text);
                valid = false;
            }
            if (valid && !token->iValue(&file))
            {
                mDiagnostics->report(Diagnostics::INTEGER_OVERFLOW,
                                     token->location, token->text);
                valid = false;
            }
            break;
          default:
            if (valid)
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                                     token->location, token->text);
                valid = false;
            }
            break;
        }
        macroExpander.lex(token);
    }

    if (valid && (state != FILE_NUMBER) && (state != FILE_NUMBER + 1))
    {
        mDiagnostics->report(Diagnostics::INVALID_LINE_DIRECTIVE,
                             token->location, token->text);
        valid = false;
    }
    if (valid)
    {
        mTokenizer->setLineNumber(line);
        if (state == FILE_NUMBER + 1) mTokenizer->setFileNumber(file);
    }
}

bool DirectiveParser::skipping() const
{
    if (mConditionalStack.empty()) return false;

    const ConditionalBlock& block = mConditionalStack.back();
    return block.skipBlock || block.skipGroup;
}

void DirectiveParser::parseConditionalIf(Token* token)
{
    ConditionalBlock block;
    block.type = token->text;
    block.location = token->location;

    if (skipping())
    {
        // This conditional block is inside another conditional group
        // which is skipped. As a consequence this whole block is skipped.
        // Be careful not to parse the conditional expression that might
        // emit a diagnostic.
        skipUntilEOD(mTokenizer, token);
        block.skipBlock = true;
    }
    else
    {
        DirectiveType directive = getDirective(token);

        int expression = 0;
        switch (directive)
        {
          case DIRECTIVE_IF:
            expression = parseExpressionIf(token);
            break;
          case DIRECTIVE_IFDEF:
            expression = parseExpressionIfdef(token);
            break;
          case DIRECTIVE_IFNDEF:
            expression = parseExpressionIfdef(token) == 0 ? 1 : 0;
            break;
          default:
            assert(false);
            break;
        }
        block.skipGroup = expression == 0;
        block.foundValidGroup = expression != 0;
    }
    mConditionalStack.push_back(block);
}

int DirectiveParser::parseExpressionIf(Token* token)
{
    assert((getDirective(token) == DIRECTIVE_IF) ||
           (getDirective(token) == DIRECTIVE_ELIF));

    DefinedParser definedParser(mTokenizer, mMacroSet, mDiagnostics);
    MacroExpander macroExpander(&definedParser, mMacroSet, mDiagnostics);
    ExpressionParser expressionParser(&macroExpander, mDiagnostics);

    int expression = 0;
    macroExpander.lex(token);
    expressionParser.parse(token, &expression);

    // Warn if there are tokens after #if expression.
    if (!isEOD(token))
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_UNEXPECTED_TOKEN,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
    }

    return expression;
}

int DirectiveParser::parseExpressionIfdef(Token* token)
{
    assert((getDirective(token) == DIRECTIVE_IFDEF) ||
           (getDirective(token) == DIRECTIVE_IFNDEF));

    mTokenizer->lex(token);
    if (token->type != Token::IDENTIFIER)
    {
        mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
        return 0;
    }

    MacroSet::const_iterator iter = mMacroSet->find(token->text);
    int expression = iter != mMacroSet->end() ? 1 : 0;

    // Warn if there are tokens after #ifdef expression.
    mTokenizer->lex(token);
    if (!isEOD(token))
    {
        mDiagnostics->report(Diagnostics::CONDITIONAL_UNEXPECTED_TOKEN,
                             token->location, token->text);
        skipUntilEOD(mTokenizer, token);
    }
    return expression;
}

}  // namespace pp
