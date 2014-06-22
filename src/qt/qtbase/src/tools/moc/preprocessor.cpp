/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "preprocessor.h"
#include "utils.h"
#include <qstringlist.h>
#include <qfile.h>
#include <qdir.h>
#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

#include "ppkeywords.cpp"
#include "keywords.cpp"

// transform \r\n into \n
// \r into \n (os9 style)
// backslash-newlines into newlines
static QByteArray cleaned(const QByteArray &input)
{
    QByteArray result;
    result.reserve(input.size());
    const char *data = input.constData();
    char *output = result.data();

    int newlines = 0;
    while (*data) {
        while (*data && is_space(*data))
            ++data;
        bool takeLine = (*data == '#');
        if (*data == '%' && *(data+1) == ':') {
            takeLine = true;
            ++data;
        }
        if (takeLine) {
            *output = '#';
            ++output;
            do ++data; while (*data && is_space(*data));
        }
        while (*data) {
            // handle \\\n, \\\r\n and \\\r
            if (*data == '\\') {
                if (*(data + 1) == '\r') {
                    ++data;
                }
                if (*data && (*(data + 1) == '\n' || (*data) == '\r')) {
                    ++newlines;
                    data += 1;
                    if (*data != '\r')
                        data += 1;
                    continue;
                }
            } else if (*data == '\r' && *(data + 1) == '\n') { // reduce \r\n to \n
                ++data;
            }

            char ch = *data;
            if (ch == '\r') // os9: replace \r with \n
                ch = '\n';
            *output = ch;
            ++output;

            if (*data == '\n') {
                // output additional newlines to keep the correct line-numbering
                // for the lines following the backslash-newline sequence(s)
                while (newlines) {
                    *output = '\n';
                    ++output;
                    --newlines;
                }
                ++data;
                break;
            }
            ++data;
        }
    }
    result.resize(output - result.constData());
    return result;
}

bool Preprocessor::preprocessOnly = false;
void Preprocessor::skipUntilEndif()
{
    while(index < symbols.size() - 1 && symbols.at(index).token != PP_ENDIF){
        switch (symbols.at(index).token) {
        case PP_IF:
        case PP_IFDEF:
        case PP_IFNDEF:
            ++index;
            skipUntilEndif();
            break;
        default:
            ;
        }
        ++index;
    }
}

bool Preprocessor::skipBranch()
{
    while (index < symbols.size() - 1
          && (symbols.at(index).token != PP_ENDIF
               && symbols.at(index).token != PP_ELIF
               && symbols.at(index).token != PP_ELSE)
       ){
        switch (symbols.at(index).token) {
        case PP_IF:
        case PP_IFDEF:
        case PP_IFNDEF:
            ++index;
            skipUntilEndif();
            break;
        default:
            ;
        }
        ++index;
    }
    return (index < symbols.size() - 1);
}


Symbols Preprocessor::tokenize(const QByteArray& input, int lineNum, Preprocessor::TokenizeMode mode)
{
    Symbols symbols;
    const char *begin = input.constData();
    const char *data = begin;
    while (*data) {
        if (mode == TokenizeCpp || mode == TokenizeDefine) {
            int column = 0;

            const char *lexem = data;
            int state = 0;
            Token token = NOTOKEN;
            for (;;) {
                if (static_cast<signed char>(*data) < 0) {
                    ++data;
                    continue;
                }
                int nextindex = keywords[state].next;
                int next = 0;
                if (*data == keywords[state].defchar)
                    next = keywords[state].defnext;
                else if (!state || nextindex)
                    next = keyword_trans[nextindex][(int)*data];
                if (!next)
                    break;
                state = next;
                token = keywords[state].token;
                ++data;
            }

            // suboptimal, is_ident_char  should use a table
            if (keywords[state].ident && is_ident_char(*data))
                token = keywords[state].ident;

            if (token == NOTOKEN) {
                ++data;
                // an error really, but let's ignore this input
                // to not confuse moc later. However in pre-processor
                // only mode let's continue.
                if (!Preprocessor::preprocessOnly)
                    continue;
            }

            ++column;

            if (token > SPECIAL_TREATMENT_MARK) {
                switch (token) {
                case QUOTE:
                    data = skipQuote(data);
                    token = STRING_LITERAL;
                    // concatenate multi-line strings for easier
                    // STRING_LITERAAL handling in moc
                    if (!Preprocessor::preprocessOnly
                        && !symbols.isEmpty()
                        && symbols.last().token == STRING_LITERAL) {

                        QByteArray newString = symbols.last().unquotedLexem();
                        newString += input.mid(lexem - begin + 1, data - lexem - 2);
                        newString.prepend('\"');
                        newString.append('\"');
                        symbols.last() = Symbol(symbols.last().lineNum,
                                                STRING_LITERAL,
                                                newString);
                        continue;
                    }
                    break;
                case SINGLEQUOTE:
                    while (*data && (*data != '\''
                                     || (*(data-1)=='\\'
                                         && *(data-2)!='\\')))
                        ++data;
                    if (*data)
                        ++data;
                    token = CHARACTER_LITERAL;
                    break;
                case LANGLE_SCOPE:
                    // split <:: into two tokens, < and ::
                    token = LANGLE;
                    data -= 2;
                    break;
                case DIGIT:
                    while (is_digit_char(*data))
                        ++data;
                    if (!*data || *data != '.') {
                        token = INTEGER_LITERAL;
                        if (data - lexem == 1 &&
                            (*data == 'x' || *data == 'X')
                            && *lexem == '0') {
                            ++data;
                            while (is_hex_char(*data))
                                ++data;
                        }
                        break;
                    }
                    token = FLOATING_LITERAL;
                    ++data;
                    // fall through
                case FLOATING_LITERAL:
                    while (is_digit_char(*data))
                        ++data;
                    if (*data == '+' || *data == '-')
                        ++data;
                    if (*data == 'e' || *data == 'E') {
                        ++data;
                        while (is_digit_char(*data))
                            ++data;
                    }
                    if (*data == 'f' || *data == 'F'
                        || *data == 'l' || *data == 'L')
                        ++data;
                    break;
                case HASH:
                    if (column == 1 && mode == TokenizeCpp) {
                        mode = PreparePreprocessorStatement;
                        while (*data && (*data == ' ' || *data == '\t'))
                            ++data;
                        if (is_ident_char(*data))
                            mode = TokenizePreprocessorStatement;
                        continue;
                    }
                    break;
                case PP_HASHHASH:
                    if (mode == TokenizeCpp)
                        continue;
                    break;
                case NEWLINE:
                    ++lineNum;
                    if (mode == TokenizeDefine) {
                        mode = TokenizeCpp;
                        // emit the newline token
                        break;
                    }
                    continue;
                case BACKSLASH:
                {
                    const char *rewind = data;
                    while (*data && (*data == ' ' || *data == '\t'))
                        ++data;
                    if (*data && *data == '\n') {
                        ++data;
                        continue;
                    }
                    data = rewind;
                } break;
                case CHARACTER:
                    while (is_ident_char(*data))
                        ++data;
                    token = IDENTIFIER;
                    break;
                case C_COMMENT:
                    if (*data) {
                        if (*data == '\n')
                            ++lineNum;
                        ++data;
                        if (*data) {
                            if (*data == '\n')
                                ++lineNum;
                            ++data;
                        }
                    }
                    while (*data && (*(data-1) != '/' || *(data-2) != '*')) {
                        if (*data == '\n')
                            ++lineNum;
                        ++data;
                    }
                    token = WHITESPACE; // one comment, one whitespace
                    // fall through;
                case WHITESPACE:
                    if (column == 1)
                        column = 0;
                    while (*data && (*data == ' ' || *data == '\t'))
                        ++data;
                    if (Preprocessor::preprocessOnly) // tokenize whitespace
                        break;
                    continue;
                case CPP_COMMENT:
                    while (*data && *data != '\n')
                        ++data;
                    continue; // ignore safely, the newline is a separator
                default:
                    continue; //ignore
                }
            }
#ifdef USE_LEXEM_STORE
            if (!Preprocessor::preprocessOnly
                && token != IDENTIFIER
                && token != STRING_LITERAL
                && token != FLOATING_LITERAL
                && token != INTEGER_LITERAL)
                symbols += Symbol(lineNum, token);
            else
#endif
                symbols += Symbol(lineNum, token, input, lexem-begin, data-lexem);

        } else { //   Preprocessor

            const char *lexem = data;
            int state = 0;
            Token token = NOTOKEN;
            if (mode == TokenizePreprocessorStatement) {
                state = pp_keyword_trans[0][(int)'#'];
                mode = TokenizePreprocessor;
            }
            for (;;) {
                if (static_cast<signed char>(*data) < 0) {
                    ++data;
                    continue;
                }

                int nextindex = pp_keywords[state].next;
                int next = 0;
                if (*data == pp_keywords[state].defchar)
                    next = pp_keywords[state].defnext;
                else if (!state || nextindex)
                    next = pp_keyword_trans[nextindex][(int)*data];
                if (!next)
                    break;
                state = next;
                token = pp_keywords[state].token;
                ++data;
            }
            // suboptimal, is_ident_char  should use a table
            if (pp_keywords[state].ident && is_ident_char(*data))
                token = pp_keywords[state].ident;

            switch (token) {
            case NOTOKEN:
                ++data;
                break;
            case PP_DEFINE:
                mode = PrepareDefine;
                break;
            case PP_IFDEF:
                symbols += Symbol(lineNum, PP_IF);
                symbols += Symbol(lineNum, PP_DEFINED);
                continue;
            case PP_IFNDEF:
                symbols += Symbol(lineNum, PP_IF);
                symbols += Symbol(lineNum, PP_NOT);
                symbols += Symbol(lineNum, PP_DEFINED);
                continue;
            case PP_INCLUDE:
                mode = TokenizeInclude;
                break;
            case PP_QUOTE:
                data = skipQuote(data);
                token = PP_STRING_LITERAL;
                break;
            case PP_SINGLEQUOTE:
                while (*data && (*data != '\''
                                 || (*(data-1)=='\\'
                                     && *(data-2)!='\\')))
                    ++data;
                if (*data)
                    ++data;
                token = PP_CHARACTER_LITERAL;
                break;
            case PP_DIGIT:
                while (is_digit_char(*data))
                    ++data;
                if (!*data || *data != '.') {
                    token = PP_INTEGER_LITERAL;
                    if (data - lexem == 1 &&
                        (*data == 'x' || *data == 'X')
                        && *lexem == '0') {
                        ++data;
                        while (is_hex_char(*data))
                            ++data;
                    }
                    break;
                }
                token = PP_FLOATING_LITERAL;
                ++data;
                // fall through
            case PP_FLOATING_LITERAL:
                while (is_digit_char(*data))
                    ++data;
                if (*data == '+' || *data == '-')
                    ++data;
                if (*data == 'e' || *data == 'E') {
                    ++data;
                    while (is_digit_char(*data))
                        ++data;
                }
                if (*data == 'f' || *data == 'F'
                    || *data == 'l' || *data == 'L')
                    ++data;
                break;
            case PP_CHARACTER:
                if (mode == PreparePreprocessorStatement) {
                    // rewind entire token to begin
                    data = lexem;
                    mode = TokenizePreprocessorStatement;
                    continue;
                }
                while (is_ident_char(*data))
                    ++data;
                token = PP_IDENTIFIER;

                if (mode == PrepareDefine) {
                    symbols += Symbol(lineNum, token, input, lexem-begin, data-lexem);
                    // make sure we explicitly add the whitespace here if the next char
                    // is not an opening brace, so we can distinguish correctly between
                    // regular and function macros
                    if (*data != '(')
                        symbols += Symbol(lineNum, WHITESPACE);
                    mode = TokenizeDefine;
                    continue;
                }
                break;
            case PP_C_COMMENT:
                if (*data) {
                    if (*data == '\n')
                        ++lineNum;
                    ++data;
                    if (*data) {
                        if (*data == '\n')
                            ++lineNum;
                        ++data;
                    }
                }
                while (*data && (*(data-1) != '/' || *(data-2) != '*')) {
                    if (*data == '\n')
                        ++lineNum;
                    ++data;
                }
                token = PP_WHITESPACE; // one comment, one whitespace
                // fall through;
            case PP_WHITESPACE:
                while (*data && (*data == ' ' || *data == '\t'))
                    ++data;
                continue; // the preprocessor needs no whitespace
            case PP_CPP_COMMENT:
                while (*data && *data != '\n')
                    ++data;
                continue; // ignore safely, the newline is a separator
            case PP_NEWLINE:
                ++lineNum;
                mode = TokenizeCpp;
                break;
            case PP_BACKSLASH:
            {
                const char *rewind = data;
                while (*data && (*data == ' ' || *data == '\t'))
                    ++data;
                if (*data && *data == '\n') {
                    ++data;
                    continue;
                }
                data = rewind;
            } break;
            case PP_LANGLE:
                if (mode != TokenizeInclude)
                    break;
                token = PP_STRING_LITERAL;
                while (*data && *data != '\n' && *(data-1) != '>')
                    ++data;
                break;
            default:
                break;
            }
            if (mode == PreparePreprocessorStatement)
                continue;
#ifdef USE_LEXEM_STORE
            if (token != PP_IDENTIFIER
                && token != PP_STRING_LITERAL
                && token != PP_FLOATING_LITERAL
                && token != PP_INTEGER_LITERAL)
                symbols += Symbol(lineNum, token);
            else
#endif
                symbols += Symbol(lineNum, token, input, lexem-begin, data-lexem);
        }
    }
    symbols += Symbol(); // eof symbol
    return symbols;
}

Symbols Preprocessor::macroExpand(Preprocessor *that, Symbols &toExpand, int &index,
                                  int lineNum, bool one, const QSet<QByteArray> &excludeSymbols)
{
    SymbolStack symbols;
    SafeSymbols sf;
    sf.symbols = toExpand;
    sf.index = index;
    sf.excludedSymbols = excludeSymbols;
    symbols.push(sf);

    Symbols result;
    if (toExpand.isEmpty())
        return result;

    for (;;) {
        QByteArray macro;
        Symbols newSyms = macroExpandIdentifier(that, symbols, lineNum, &macro);

        if (macro.isEmpty()) {
            result += newSyms;
        } else {
            SafeSymbols sf;
            sf.symbols = newSyms;
            sf.index = 0;
            sf.expandedMacro = macro;
            symbols.push(sf);
        }
        if (!symbols.hasNext() || (one && symbols.size() == 1))
                break;
        symbols.next();
    }

    if (symbols.size())
        index = symbols.top().index;
    else
        index = toExpand.size();

    return result;
}


Symbols Preprocessor::macroExpandIdentifier(Preprocessor *that, SymbolStack &symbols, int lineNum, QByteArray *macroName)
{
    Symbol s = symbols.symbol();

    // not a macro
    if (s.token != PP_IDENTIFIER || !that->macros.contains(s) || symbols.dontReplaceSymbol(s.lexem())) {
        Symbols syms;
        syms += s;
        syms.last().lineNum = lineNum;
        return syms;
    }

    const Macro &macro = that->macros.value(s);
    *macroName = s.lexem();

    Symbols expansion;
    if (!macro.isFunction) {
        expansion = macro.symbols;
    } else {
        bool haveSpace = false;
        while (symbols.test(PP_WHITESPACE)) { haveSpace = true; }
        if (!symbols.test(PP_LPAREN)) {
            *macroName = QByteArray();
            Symbols syms;
            if (haveSpace)
                syms += Symbol(lineNum, PP_WHITESPACE);
            syms += s;
            syms.last().lineNum = lineNum;
            return syms;
        }
        QList<Symbols> arguments;
        while (symbols.hasNext()) {
            Symbols argument;
            // strip leading space
            while (symbols.test(PP_WHITESPACE)) {}
            int nesting = 0;
            bool vararg = macro.isVariadic && (arguments.size() == macro.arguments.size() - 1);
            while (symbols.hasNext()) {
                Token t = symbols.next();
                if (t == PP_LPAREN) {
                    ++nesting;
                } else if (t == PP_RPAREN) {
                    --nesting;
                    if (nesting < 0)
                        break;
                } else if (t == PP_COMMA && nesting == 0) {
                    if (!vararg)
                        break;
                }
                argument += symbols.symbol();
            }
            arguments += argument;

            if (nesting < 0)
                break;
            else if (!symbols.hasNext())
                that->error("missing ')' in macro usage");
        }

        // empty VA_ARGS
        if (macro.isVariadic && arguments.size() == macro.arguments.size() - 1)
            arguments += Symbols();

        if (arguments.size() != macro.arguments.size() &&
            // 0 argument macros are a bit special. They are ok if the
            // argument is pure whitespace or empty
            (macro.arguments.size() != 0 || arguments.size() != 1 || !arguments.at(0).isEmpty()))
            that->warning("Macro argument mismatch.");

        // now replace the macro arguments with the expanded arguments
        enum Mode {
            Normal,
            Hash,
            HashHash
        } mode = Normal;

        for (int i = 0; i < macro.symbols.size(); ++i) {
            const Symbol &s = macro.symbols.at(i);
            if (s.token == HASH || s.token == PP_HASHHASH) {
                mode = (s.token == HASH ? Hash : HashHash);
                continue;
            }
            int index = macro.arguments.indexOf(s);
            if (mode == Normal) {
                if (index >= 0 && index < arguments.size()) {
                    // each argument undoergoes macro expansion if it's not used as part of a # or ##
                    if (i == macro.symbols.size() - 1 || macro.symbols.at(i + 1).token != PP_HASHHASH) {
                        Symbols arg = arguments.at(index);
                        int idx = 1;
                        expansion += macroExpand(that, arg, idx, lineNum, false, symbols.excludeSymbols());
                    } else {
                        expansion += arguments.at(index);
                    }
               } else {
                    expansion += s;
                }
            } else if (mode == Hash) {
                if (index < 0)
                    that->error("'#' is not followed by a macro parameter");

                const Symbols &arg = arguments.at(index);
                QByteArray stringified;
                for (int i = 0; i < arg.size(); ++i) {
                    stringified += arg.at(i).lexem();
                }
                stringified.replace('"', "\\\"");
                stringified.prepend('"');
                stringified.append('"');
                expansion += Symbol(lineNum, STRING_LITERAL, stringified);
            } else if (mode == HashHash){
                if (s.token == WHITESPACE)
                    continue;

                while (expansion.size() && expansion.last().token == PP_WHITESPACE)
                    expansion.pop_back();

                Symbol next = s;
                if (index >= 0) {
                    const Symbols &arg = arguments.at(index);
                    if (arg.size() == 0) {
                        mode = Normal;
                        continue;
                    }
                    next = arg.at(0);
                }

                if (!expansion.isEmpty() && expansion.last().token == s.token) {
                    Symbol last = expansion.last();
                    expansion.pop_back();

                    if (last.token == STRING_LITERAL || s.token == STRING_LITERAL)
                        that->error("Can't concatenate non identifier tokens");

                    QByteArray lexem = last.lexem() + next.lexem();
                    expansion += Symbol(lineNum, last.token, lexem);
                } else {
                    expansion += next;
                }

                if (index >= 0) {
                    const Symbols &arg = arguments.at(index);
                    for (int i = 1; i < arg.size(); ++i)
                        expansion += arg.at(i);
                }
            }
            mode = Normal;
        }
        if (mode != Normal)
            that->error("'#' or '##' found at the end of a macro argument");

    }

    return expansion;
}

void Preprocessor::substituteUntilNewline(Symbols &substituted)
{
    while (hasNext()) {
        Token token = next();
        if (token == PP_IDENTIFIER) {
            substituted += macroExpand(this, symbols, index, symbol().lineNum, true);
        } else if (token == PP_DEFINED) {
            bool braces = test(PP_LPAREN);
            next(PP_IDENTIFIER);
            Symbol definedOrNotDefined = symbol();
            definedOrNotDefined.token = macros.contains(definedOrNotDefined)? PP_MOC_TRUE : PP_MOC_FALSE;
            substituted += definedOrNotDefined;
            if (braces)
                test(PP_RPAREN);
            continue;
        } else if (token == PP_NEWLINE) {
            substituted += symbol();
            break;
        } else {
            substituted += symbol();
        }
    }
}


class PP_Expression : public Parser
{
public:
    int value() { index = 0; return unary_expression_lookup() ?  conditional_expression() : 0; }

    int conditional_expression();
    int logical_OR_expression();
    int logical_AND_expression();
    int inclusive_OR_expression();
    int exclusive_OR_expression();
    int AND_expression();
    int equality_expression();
    int relational_expression();
    int shift_expression();
    int additive_expression();
    int multiplicative_expression();
    int unary_expression();
    bool unary_expression_lookup();
    int primary_expression();
    bool primary_expression_lookup();
};

int PP_Expression::conditional_expression()
{
    int value = logical_OR_expression();
    if (test(PP_QUESTION)) {
        int alt1 = conditional_expression();
        int alt2 = test(PP_COLON) ? conditional_expression() : 0;
        return value ? alt1 : alt2;
    }
    return value;
}

int PP_Expression::logical_OR_expression()
{
    int value = logical_AND_expression();
    if (test(PP_OROR))
        return logical_OR_expression() || value;
    return value;
}

int PP_Expression::logical_AND_expression()
{
    int value = inclusive_OR_expression();
    if (test(PP_ANDAND))
        return logical_AND_expression() && value;
    return value;
}

int PP_Expression::inclusive_OR_expression()
{
    int value = exclusive_OR_expression();
    if (test(PP_OR))
        return value | inclusive_OR_expression();
    return value;
}

int PP_Expression::exclusive_OR_expression()
{
    int value = AND_expression();
    if (test(PP_HAT))
        return value ^ exclusive_OR_expression();
    return value;
}

int PP_Expression::AND_expression()
{
    int value = equality_expression();
    if (test(PP_AND))
        return value & AND_expression();
    return value;
}

int PP_Expression::equality_expression()
{
    int value = relational_expression();
    switch (next()) {
    case PP_EQEQ:
        return value == equality_expression();
    case PP_NE:
        return value != equality_expression();
    default:
        prev();
        return value;
    }
}

int PP_Expression::relational_expression()
{
    int value = shift_expression();
    switch (next()) {
    case PP_LANGLE:
        return value < relational_expression();
    case PP_RANGLE:
        return value > relational_expression();
    case PP_LE:
        return value <= relational_expression();
    case PP_GE:
        return value >= relational_expression();
    default:
        prev();
        return value;
    }
}

int PP_Expression::shift_expression()
{
    int value = additive_expression();
    switch (next()) {
    case PP_LTLT:
        return value << shift_expression();
    case PP_GTGT:
        return value >> shift_expression();
    default:
        prev();
        return value;
    }
}

int PP_Expression::additive_expression()
{
    int value = multiplicative_expression();
    switch (next()) {
    case PP_PLUS:
        return value + additive_expression();
    case PP_MINUS:
        return value - additive_expression();
    default:
        prev();
        return value;
    }
}

int PP_Expression::multiplicative_expression()
{
    int value = unary_expression();
    switch (next()) {
    case PP_STAR:
        return value * multiplicative_expression();
    case PP_PERCENT:
    {
        int remainder = multiplicative_expression();
        return remainder ? value % remainder : 0;
    }
    case PP_SLASH:
    {
        int div = multiplicative_expression();
        return div ? value / div : 0;
    }
    default:
        prev();
        return value;
    };
}

int PP_Expression::unary_expression()
{
    switch (next()) {
    case PP_PLUS:
        return unary_expression();
    case PP_MINUS:
        return -unary_expression();
    case PP_NOT:
        return !unary_expression();
    case PP_TILDE:
        return ~unary_expression();
    case PP_MOC_TRUE:
        return 1;
    case PP_MOC_FALSE:
        return 0;
    default:
        prev();
        return primary_expression();
    }
}

bool PP_Expression::unary_expression_lookup()
{
    Token t = lookup();
    return (primary_expression_lookup()
            || t == PP_PLUS
            || t == PP_MINUS
            || t == PP_NOT
            || t == PP_TILDE
            || t == PP_DEFINED);
}

int PP_Expression::primary_expression()
{
    int value;
    if (test(PP_LPAREN)) {
        value = conditional_expression();
        test(PP_RPAREN);
    } else {
        next();
        value = lexem().toInt(0, 0);
    }
    return value;
}

bool PP_Expression::primary_expression_lookup()
{
    Token t = lookup();
    return (t == PP_IDENTIFIER
            || t == PP_INTEGER_LITERAL
            || t == PP_FLOATING_LITERAL
            || t == PP_MOC_TRUE
            || t == PP_MOC_FALSE
            || t == PP_LPAREN);
}

int Preprocessor::evaluateCondition()
{
    PP_Expression expression;
    expression.currentFilenames = currentFilenames;

    substituteUntilNewline(expression.symbols);

    return expression.value();
}

void Preprocessor::preprocess(const QByteArray &filename, Symbols &preprocessed)
{
    currentFilenames.push(filename);
    preprocessed.reserve(preprocessed.size() + symbols.size());
    while (hasNext()) {
        Token token = next();

        switch (token) {
        case PP_INCLUDE:
        {
            int lineNum = symbol().lineNum;
            QByteArray include;
            bool local = false;
            if (test(PP_STRING_LITERAL)) {
                local = lexem().startsWith('\"');
                include = unquotedLexem();
            } else
                continue;
            until(PP_NEWLINE);

            // #### stringery
            QFileInfo fi;
            if (local)
                fi.setFile(QFileInfo(QString::fromLocal8Bit(filename.constData())).dir(), QString::fromLocal8Bit(include.constData()));
            for (int j = 0; j < Preprocessor::includes.size() && !fi.exists(); ++j) {
                const IncludePath &p = Preprocessor::includes.at(j);
                if (p.isFrameworkPath) {
                    const int slashPos = include.indexOf('/');
                    if (slashPos == -1)
                        continue;
                    QByteArray frameworkCandidate = include.left(slashPos);
                    frameworkCandidate.append(".framework/Headers/");
                    fi.setFile(QString::fromLocal8Bit(QByteArray(p.path + '/' + frameworkCandidate).constData()), QString::fromLocal8Bit(include.mid(slashPos + 1).constData()));
                } else {
                    fi.setFile(QString::fromLocal8Bit(p.path.constData()), QString::fromLocal8Bit(include.constData()));
                }
                // try again, maybe there's a file later in the include paths with the same name
                // (186067)
                if (fi.isDir()) {
                    fi = QFileInfo();
                    continue;
                }
            }

            if (!fi.exists() || fi.isDir())
                continue;
            include = fi.canonicalFilePath().toLocal8Bit();

            if (Preprocessor::preprocessedIncludes.contains(include))
                continue;
            Preprocessor::preprocessedIncludes.insert(include);

            QFile file(QString::fromLocal8Bit(include.constData()));
            if (!file.open(QFile::ReadOnly))
                continue;

            QByteArray input = file.readAll();
            file.close();
            if (input.isEmpty())
                continue;

            Symbols saveSymbols = symbols;
            int saveIndex = index;

            // phase 1: get rid of backslash-newlines
            input = cleaned(input);

            // phase 2: tokenize for the preprocessor
            symbols = tokenize(input);
            input.clear();

            index = 0;

            // phase 3: preprocess conditions and substitute macros
            preprocessed += Symbol(0, MOC_INCLUDE_BEGIN, include);
            preprocess(include, preprocessed);
            preprocessed += Symbol(lineNum, MOC_INCLUDE_END, include);

            symbols = saveSymbols;
            index = saveIndex;
            continue;
        }
        case PP_DEFINE:
        {
            next(IDENTIFIER);
            QByteArray name = lexem();
            Macro macro;
            macro.isVariadic = false;
            Token t = next();
            if (t == LPAREN) {
                // we have a function macro
                macro.isFunction = true;
                parseDefineArguments(&macro);
            } else if (t == PP_WHITESPACE){
                macro.isFunction = false;
            } else {
                error("Moc: internal error");
            }
            int start = index;
            until(PP_NEWLINE);
            macro.symbols.reserve(index - start - 1);

            // remove whitespace where there shouldn't be any:
            // Before and after the macro, after a # and around ##
            Token lastToken = HASH; // skip shitespace at the beginning
            for (int i = start; i < index - 1; ++i) {
                Token token = symbols.at(i).token;
                if (token ==  PP_WHITESPACE || token == WHITESPACE) {
                    if (lastToken == PP_HASH || lastToken == HASH ||
                        lastToken == PP_HASHHASH ||
                        lastToken == PP_WHITESPACE || lastToken == WHITESPACE)
                        continue;
                } else if (token == PP_HASHHASH) {
                    if (!macro.symbols.isEmpty() &&
                        (lastToken ==  PP_WHITESPACE || lastToken == WHITESPACE))
                        macro.symbols.pop_back();
                }
                macro.symbols.append(symbols.at(i));
                lastToken = token;
            }
            // remove trailing whitespace
            while (!macro.symbols.isEmpty() &&
                   (macro.symbols.last().token == PP_WHITESPACE || macro.symbols.last().token == WHITESPACE))
                macro.symbols.pop_back();

            if (!macro.symbols.isEmpty()) {
                if (macro.symbols.first().token == PP_HASHHASH ||
                    macro.symbols.last().token == PP_HASHHASH) {
                    error("'##' cannot appear at either end of a macro expansion");
                }
                if (macro.symbols.last().token == HASH ||
                    macro.symbols.last().token == PP_HASH) {
                    error("'#' is not followed by a macro parameter");
                }
            }
            macros.insert(name, macro);
            continue;
        }
        case PP_UNDEF: {
            next(IDENTIFIER);
            QByteArray name = lexem();
            until(PP_NEWLINE);
            macros.remove(name);
            continue;
        }
        case PP_IDENTIFIER: {
            // substitute macros
            preprocessed += macroExpand(this, symbols, index, symbol().lineNum, true);
            continue;
        }
        case PP_HASH:
            until(PP_NEWLINE);
            continue; // skip unknown preprocessor statement
        case PP_IFDEF:
        case PP_IFNDEF:
        case PP_IF:
            while (!evaluateCondition()) {
                if (!skipBranch())
                    break;
                if (test(PP_ELIF)) {
                } else {
                    until(PP_NEWLINE);
                    break;
                }
            }
            continue;
        case PP_ELIF:
        case PP_ELSE:
            skipUntilEndif();
            // fall through
        case PP_ENDIF:
            until(PP_NEWLINE);
            continue;
        case PP_NEWLINE:
            continue;
        case SIGNALS:
        case SLOTS: {
            Symbol sym = symbol();
            if (macros.contains("QT_NO_KEYWORDS"))
                sym.token = IDENTIFIER;
            else
                sym.token = (token == SIGNALS ? Q_SIGNALS_TOKEN : Q_SLOTS_TOKEN);
            preprocessed += sym;
        } continue;
        default:
            break;
        }
        preprocessed += symbol();
    }

    currentFilenames.pop();
}

Symbols Preprocessor::preprocessed(const QByteArray &filename, QIODevice *file)
{
    QByteArray input = file->readAll();
    if (input.isEmpty())
        return symbols;

    // phase 1: get rid of backslash-newlines
    input = cleaned(input);

    // phase 2: tokenize for the preprocessor
    symbols = tokenize(input);

#if 0
    for (int j = 0; j < symbols.size(); ++j)
        fprintf(stderr, "line %d: %s(%s)\n",
               symbols[j].lineNum,
               symbols[j].lexem().constData(),
               tokenTypeName(symbols[j].token));
#endif

    // phase 3: preprocess conditions and substitute macros
    Symbols result;
    preprocess(filename, result);

#if 0
    for (int j = 0; j < result.size(); ++j)
        fprintf(stderr, "line %d: %s(%s)\n",
               result[j].lineNum,
               result[j].lexem().constData(),
               tokenTypeName(result[j].token));
#endif

    return result;
}

void Preprocessor::parseDefineArguments(Macro *m)
{
    Symbols arguments;
    while (hasNext()) {
        while (test(PP_WHITESPACE)) {}
        Token t = next();
        if (t == PP_RPAREN)
            break;
        if (t != PP_IDENTIFIER) {
            QByteArray l = lexem();
            if (l == "...") {
                m->isVariadic = true;
                arguments += Symbol(symbol().lineNum, PP_IDENTIFIER, "__VA_ARGS__");
                while (test(PP_WHITESPACE)) {}
                if (!test(PP_RPAREN))
                    error("missing ')' in macro argument list");
                break;
            } else if (!is_identifier(l.constData(), l.length())) {
                qDebug() << l;
                error("Unexpected character in macro argument list.");
            }
        }

        Symbol arg = symbol();
        if (arguments.contains(arg))
            error("Duplicate macro parameter.");
        arguments += symbol();

        while (test(PP_WHITESPACE)) {}
        t = next();
        if (t == PP_RPAREN)
            break;
        if (t == PP_COMMA)
            continue;
        if (lexem() == "...") {
            //GCC extension:    #define FOO(x, y...) x(y)
            // The last argument was already parsed. Just mark the macro as variadic.
            m->isVariadic = true;
            while (test(PP_WHITESPACE)) {}
            if (!test(PP_RPAREN))
                error("missing ')' in macro argument list");
            break;
        }
        error("Unexpected character in macro argument list.");
    }
    m->arguments = arguments;
    while (test(PP_WHITESPACE)) {}
}

void Preprocessor::until(Token t)
{
    while(hasNext() && next() != t)
        ;
}

QT_END_NAMESPACE
