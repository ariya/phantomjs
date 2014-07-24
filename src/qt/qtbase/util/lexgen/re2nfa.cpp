/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
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

#include "re2nfa.h"
#include "tokenizer.cpp"

RE2NFA::RE2NFA(const QMap<QString, NFA> &macros, const QSet<InputType> &maxInputSet, Qt::CaseSensitivity cs)
    : macros(macros), index(0), errorColumn(-1), maxInputSet(maxInputSet), caseSensitivity(cs)
{
}

NFA RE2NFA::parse(const QString &expression, int *errCol)
{
    tokenize(expression);

    if (symbols.isEmpty())
        return NFA();

    index = 0;

    NFA result = parseExpr();
    if (result.isEmpty()) {
        if (errCol)
            *errCol = errorColumn;
    }
    return result;
}

NFA RE2NFA::parseExpr()
{
    NFA value = parseBranch();
    while (test(TOK_OR)) {
        NFA rhs = parseBranch();
        value = NFA::createAlternatingNFA(value, rhs);
    }
    return value;
}

NFA RE2NFA::parseBranch()
{
    NFA value = parsePiece();
    if (!hasNext())
        return value;
    NFA next;
    do {
        next = parsePiece();
        if (!next.isEmpty())
            value = NFA::createConcatenatingNFA(value, next);
    } while (!next.isEmpty() && hasNext());
    return value;
}

NFA RE2NFA::parsePiece()
{
    NFA atom = parseAtom();
    if (atom.isEmpty() || !hasNext())
        return atom;
    return parseMaybeQuantifier(atom);
}

NFA RE2NFA::parseAtom()
{
    // ####
    switch (next()) {
        case TOK_STRING:
            return createCharNFA();
        case TOK_LPAREN: {
            NFA subExpr = parseExpr();
            next(TOK_RPAREN);
            return subExpr;
        }
        case TOK_LBRACE: {
            QString macroName = lexemUntil(TOK_RBRACE);
            QMap<QString, NFA>::ConstIterator macro = macros.find(macroName);
            if (macro == macros.end()) {
                qWarning("Unknown macro '%s' - probably used before defined", qPrintable(macroName));
                return NFA();
            }
            return *macro;
        }
        case TOK_LBRACKET: {
            NFA set = parseSet();
            next(TOK_RBRACKET);
            return set;
        }
        case TOK_SEQUENCE:
            return parseSet2();
        case TOK_DOT:
            return NFA::createSetNFA(maxInputSet);
        default:
            prev();
            return NFA();
    }
}

NFA RE2NFA::parseMaybeQuantifier(const NFA &nfa)
{
    // ####
    switch (next()) {
        case TOK_STAR:
            return NFA::createOptionalNFA(nfa);
        case TOK_QUESTION:
            return NFA::createZeroOrOneNFA(nfa);
        case TOK_PLUS:
            return NFA::createConcatenatingNFA(nfa, NFA::createOptionalNFA(nfa));
        case TOK_LBRACE: {
              const int rewind = index - 1;

              QString lexemBeforeComma;
              QString lexemAfterComma;
              bool seenComma = false;
              forever {
                  if (test(TOK_COMMA)) {
                      if (seenComma) {
                          errorColumn = symbol().column;
                          return NFA();
                      }
                      seenComma = true;
                  } else if (test(TOK_RBRACE)) {
                      break;
                  } else {
                      next(TOK_STRING);
                      if (seenComma)
                          lexemAfterComma += symbol().lexem;
                      else
                          lexemBeforeComma += symbol().lexem;
                  }
              }
              bool isNumber = false;
              int min = lexemBeforeComma.toInt(&isNumber);
              if (!isNumber) {
                  index = rewind;
                  return nfa;
              }
              int max = min;
              if (seenComma) {
                  max = lexemAfterComma.toInt(&isNumber);
                  if (!isNumber) {
                      errorColumn = symbol().column;
                      return NFA();
                  }
              }
              return NFA::applyQuantity(nfa, min, max);
        }
        default:
            prev();
            return nfa;
    }
}

NFA RE2NFA::parseSet()
{
    QSet<InputType> set;
    bool negate = false;

    next(TOK_STRING);

    do {
        Q_ASSERT(symbol().lexem.length() == 1);
        // ###
        QChar ch = symbol().lexem.at(0);
        if (set.isEmpty() && ch == QLatin1Char('^')) {
            negate = true;
            continue;
        }

        // look ahead for ranges like a-z
        bool rangeFound = false;
        if (test(TOK_STRING)) {
            if (symbol().lexem.length() == 1
                && symbol().lexem.at(0) == QLatin1Char('-')) {
                next(TOK_STRING);
                Q_ASSERT(symbol().lexem.length() == 1);
                QChar last = symbol().lexem.at(0);

                if (ch.unicode() > last.unicode())
                    qSwap(ch, last);

                for (ushort i = ch.unicode(); i <= last.unicode(); ++i) {
                    if (caseSensitivity == Qt::CaseInsensitive) {
                        set.insert(QChar(i).toLower().unicode());
                    } else {
                        set.insert(i);
                    }
                }

                rangeFound = true;
            } else {
                prev();
            }
        }

        if (!rangeFound) {
            if (caseSensitivity == Qt::CaseInsensitive) {
                set.insert(ch.toLower().unicode());
            } else {
                set.insert(ch.unicode());
            }
        }
    } while (test(TOK_STRING));

    if (negate) {
        QSet<InputType> negatedSet = maxInputSet;
        negatedSet.subtract(set);
        set = negatedSet;
    }

    return NFA::createSetNFA(set);
}

NFA RE2NFA::parseSet2()
{
    QSet<InputType> set;
    bool negate = false;

    QString str = symbol().lexem;
    // strip off brackets
    str.chop(1);
    str.remove(0, 1);

    int i = 0;
    while (i < str.length()) {
        // ###
        QChar ch = str.at(i++);
        if (set.isEmpty() && ch == QLatin1Char('^')) {
            negate = true;
            continue;
        }

        // look ahead for ranges like a-z
        bool rangeFound = false;
        if (i < str.length() - 1 && str.at(i) == QLatin1Char('-')) {
            ++i;
            QChar last = str.at(i++);

            if (ch.unicode() > last.unicode())
                qSwap(ch, last);

            for (ushort i = ch.unicode(); i <= last.unicode(); ++i) {
                if (caseSensitivity == Qt::CaseInsensitive) {
                    set.insert(QChar(i).toLower().unicode());
                } else {
                    set.insert(i);
                }
            }

            rangeFound = true;
        }

        if (!rangeFound) {
            if (caseSensitivity == Qt::CaseInsensitive) {
                set.insert(ch.toLower().unicode());
            } else {
                set.insert(ch.unicode());
            }
        }
    }

    if (negate) {
        QSet<InputType> negatedSet = maxInputSet;
        negatedSet.subtract(set);
        set = negatedSet;
    }

    return NFA::createSetNFA(set);
}
NFA RE2NFA::createCharNFA()
{
    NFA nfa;
    // ####
    if (caseSensitivity == Qt::CaseInsensitive) {
        nfa = NFA::createStringNFA(symbol().lexem.toLower().toLatin1());
    } else {
        nfa = NFA::createStringNFA(symbol().lexem.toLatin1());
    }
    return nfa;
}

static inline int skipQuote(const QString &str, int pos)
{
    while (pos < str.length()
           && str.at(pos) != QLatin1Char('"')) {
        if (str.at(pos) == QLatin1Char('\\')) {
            ++pos;
            if (pos >= str.length())
                break;
        }
        ++pos;
    }
    if (pos < str.length())
        ++pos;
    return pos;
}

#if 0
static const char*tokStr(Token t)
{
    switch (t) {
        case TOK_INVALID: return "TOK_INVALID";
        case TOK_STRING: return "TOK_STRING";
        case TOK_LBRACE: return "TOK_LBRACE";
        case TOK_RBRACE: return "TOK_RBRACE";
        case TOK_LBRACKET: return "TOK_LBRACKET";
        case TOK_RBRACKET: return "TOK_RBRACKET";
        case TOK_LPAREN: return "TOK_LPAREN";
        case TOK_RPAREN: return "TOK_RPAREN";
        case TOK_COMMA: return "TOK_COMMA";
        case TOK_STAR: return "TOK_STAR";
        case TOK_OR: return "TOK_OR";
        case TOK_QUESTION: return "TOK_QUESTION";
        case TOK_DOT: return "TOK_DOT";
        case TOK_PLUS: return "TOK_PLUS";
        case TOK_SEQUENCE: return "TOK_SEQUENCE";
        case TOK_QUOTED_STRING: return "TOK_QUOTED_STRING";
    }
    return "";
}
#endif

void RE2NFA::tokenize(const QString &input)
{
    symbols.clear();
#if 1
    RegExpTokenizer tokenizer(input);
    Symbol sym;
    int tok = tokenizer.lex();
    while (tok != -1) {
        Symbol sym;
        sym.token = static_cast<Token>(tok);
        sym.lexem = input.mid(tokenizer.lexemStart, tokenizer.lexemLength);

        if (sym.token == TOK_QUOTED_STRING) {
            sym.lexem.chop(1);
            sym.lexem.remove(0, 1);
            sym.token = TOK_STRING;
        }

        if (sym.token == TOK_STRING || sym.token == TOK_SEQUENCE) {
            for (int i = 0; i < sym.lexem.length(); ++i) {
                if (sym.lexem.at(i) == '\\') {
                    if (i >= sym.lexem.length() - 1)
                        break;
                    QChar ch = sym.lexem.at(i + 1);
                    if (ch == QLatin1Char('n')) {
                        ch = '\n';
                    } else if (ch == QLatin1Char('r')) {
                        ch = '\r';
                    } else if (ch == QLatin1Char('t')) {
                        ch = '\t';
                    } else if (ch == QLatin1Char('f')) {
                        ch = '\f';
                    }
                    sym.lexem.replace(i, 2, ch);
                }
            }
        }

        /*
        if (sym.token == TOK_SEQUENCE) {
            Symbol s;
            s.token = TOK_LBRACKET;
            s.lexem = "[";
            symbols.append(s);

            for (int i = 1; i < sym.lexem.length() - 1; ++i) {
                s.token = TOK_STRING;
                s.lexem = sym.lexem.at(i);
                symbols.append(s);
            }

            s.token = TOK_RBRACKET;
            s.lexem = "]";
            symbols.append(s);

            tok = tokenizer.lex();
            continue;
        }
        */

        symbols.append(sym);
        tok = tokenizer.lex();
    }
#else
    int pos = 0;
    bool insideSet = false;
    while (pos < input.length()) {
        QChar ch = input.at(pos);

        Symbol sym;
        sym.column = pos;
        sym.token = TOK_INVALID;
        sym.lexem = QString(ch);
        switch (ch.toLatin1()) {
            case '"': {
                if (insideSet) {
                    sym.token = TOK_STRING;
                    sym.lexem = QString(ch);
                    symbols += sym;
                    ++pos;
                    continue;
                }
                if (pos + 1 >= input.length())
                    return;
                int quoteEnd = skipQuote(input, pos + 1);
                sym.token = TOK_STRING;
                sym.lexem = input.mid(pos + 1, quoteEnd - pos - 2);
                symbols += sym;
                pos = quoteEnd;
                continue;
            }
            case '{':
                sym.token = (insideSet ? TOK_STRING : TOK_LBRACE);
                break;
            case '}':
                sym.token = (insideSet ? TOK_STRING : TOK_RBRACE);
                break;
            case '[':
                insideSet = true;
                sym.token = TOK_LBRACKET;
                break;
            case ']':
                insideSet = false;
                sym.token = TOK_RBRACKET;
                break;
            case '(':
                sym.token = (insideSet ? TOK_STRING : TOK_LPAREN);
                break;
            case ')':
                sym.token = (insideSet ? TOK_STRING : TOK_RPAREN);
                break;
            case ',':
                sym.token = (insideSet ? TOK_STRING : TOK_COMMA);
                break;
            case '*':
                sym.token = (insideSet ? TOK_STRING : TOK_STAR);
                break;
            case '|':
                sym.token = (insideSet ? TOK_STRING : TOK_OR);
                break;
            case '?':
                sym.token = (insideSet ? TOK_STRING : TOK_QUESTION);
                break;
            case '.':
                sym.token = (insideSet ? TOK_STRING : TOK_DOT);
                break;
            case '+':
                sym.token = (insideSet ? TOK_STRING : TOK_PLUS);
                break;
            case '\\':
                ++pos;
                if (pos >= input.length())
                    return;
                ch = input.at(pos);
                if (ch == QLatin1Char('n')) {
                    ch = '\n';
                } else if (ch == QLatin1Char('r')) {
                    ch = '\r';
                } else if (ch == QLatin1Char('t')) {
                    ch = '\t';
                } else if (ch == QLatin1Char('f')) {
                    ch = '\f';
                }
                // fall through
            default:
                sym.token = TOK_STRING;
                sym.lexem = QString(ch);
                symbols += sym;
                ++pos;
                continue;
        }
        symbols += sym;
        ++pos;
    }
#endif
#if 0
    foreach (Symbol s, symbols) {
        qDebug() << "Tok" << tokStr(s.token) << "lexem" << s.lexem;
    }
#endif
}

bool RE2NFA::next(Token t)
{
    if (hasNext() && next() == t)
        return true;
    errorColumn = symbol().column;
    Q_ASSERT(false);
    return false;
}

bool RE2NFA::test(Token t)
{
    if (index >= symbols.count())
        return false;
    if (symbols.at(index).token == t) {
        ++index;
        return true;
    }
    return false;
}

QString RE2NFA::lexemUntil(Token t)
{
    QString lexem;
    while (hasNext() && next() != t)
        lexem += symbol().lexem;
    return lexem;
}

