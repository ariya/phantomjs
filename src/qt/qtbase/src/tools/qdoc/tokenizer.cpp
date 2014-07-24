/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "config.h"
#include "tokenizer.h"

#include <qfile.h>
#include <qhash.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextcodec.h>

#include <ctype.h>
#include <string.h>

QT_BEGIN_NAMESPACE

#define LANGUAGE_CPP                        "Cpp"

/* qmake ignore Q_OBJECT */

/*
  Keep in sync with tokenizer.h.
*/
static const char *kwords[] = {
    "char", "class", "const", "double", "enum", "explicit",
    "friend", "inline", "int", "long", "namespace", "operator",
    "private", "protected", "public", "short", "signals", "signed",
    "slots", "static", "struct", "template", "typedef", "typename",
    "union", "unsigned", "using", "virtual", "void", "volatile",
    "__int64",
    "Q_OBJECT",
    "Q_OVERRIDE",
    "Q_PROPERTY",
    "Q_PRIVATE_PROPERTY",
    "Q_DECLARE_SEQUENTIAL_ITERATOR",
    "Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR",
    "Q_DECLARE_ASSOCIATIVE_ITERATOR",
    "Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR",
    "Q_DECLARE_FLAGS",
    "Q_SIGNALS",
    "Q_SLOTS",
    "QT_COMPAT",
    "QT_COMPAT_CONSTRUCTOR",
    "QT_DEPRECATED",
    "QT_MOC_COMPAT",
    "QT_MODULE",
    "QT3_SUPPORT",
    "QT3_SUPPORT_CONSTRUCTOR",
    "QT3_MOC_SUPPORT",
    "QDOC_PROPERTY"
};

static const int KwordHashTableSize = 4096;
static int kwordHashTable[KwordHashTableSize];

static QHash<QByteArray, bool> *ignoredTokensAndDirectives = 0;

static QRegExp *comment = 0;
static QRegExp *versionX = 0;
static QRegExp *definedX = 0;

static QRegExp *defines = 0;
static QRegExp *falsehoods = 0;

#ifndef QT_NO_TEXTCODEC
static QTextCodec *sourceCodec = 0;
#endif

/*
  This function is a perfect hash function for the 37 keywords of C99
  (with a hash table size of 512). It should perform well on our
  Qt-enhanced C++ subset.
*/
static int hashKword(const char *s, int len)
{
    return (((uchar) s[0]) + (((uchar) s[2]) << 5) +
            (((uchar) s[len - 1]) << 3)) % KwordHashTableSize;
}

static void insertKwordIntoHash(const char *s, int number)
{
    int k = hashKword(s, int(strlen(s)));
    while (kwordHashTable[k]) {
        if (++k == KwordHashTableSize)
            k = 0;
    }
    kwordHashTable[k] = number;
}

Tokenizer::Tokenizer(const Location& loc, QFile &in)
{
    init();
    yyIn = in.readAll();
    yyPos = 0;
    start(loc);
}

Tokenizer::Tokenizer(const Location& loc, const QByteArray &in)
    : yyIn(in)
{
    init();
    yyPos = 0;
    start(loc);
}

Tokenizer::~Tokenizer()
{
    delete[] yyLexBuf1;
    delete[] yyLexBuf2;
}

int Tokenizer::getToken()
{
    char *t = yyPrevLex;
    yyPrevLex = yyLex;
    yyLex = t;

    while (yyCh != EOF) {
        yyTokLoc = yyCurLoc;
        yyLexLen = 0;

        if (isspace(yyCh)) {
            do {
                yyCh = getChar();
            } while (isspace(yyCh));
        }
        else if (isalpha(yyCh) || yyCh == '_') {
            do {
                yyCh = getChar();
            } while (isalnum(yyCh) || yyCh == '_');

            int k = hashKword(yyLex, int(yyLexLen));
            for (;;) {
                int i = kwordHashTable[k];
                if (i == 0) {
                    return Tok_Ident;
                }
                else if (i == -1) {
                    if (!parsingMacro && ignoredTokensAndDirectives->contains(yyLex)) {
                        if (ignoredTokensAndDirectives->value(yyLex)) { // it's a directive
                            int parenDepth = 0;
                            while (yyCh != EOF && (yyCh != ')' || parenDepth > 1)) {
                                if (yyCh == '(')
                                    ++parenDepth;
                                else if (yyCh == ')')
                                    --parenDepth;
                                yyCh = getChar();
                            }
                            if (yyCh == ')')
                                yyCh = getChar();
                        }
                        break;
                    }
                }
                else if (strcmp(yyLex, kwords[i - 1]) == 0) {
                    int ret = (int) Tok_FirstKeyword + i - 1;
                    if (ret != Tok_explicit && ret != Tok_inline && ret != Tok_typename)
                        return ret;
                    break;
                }

                if (++k == KwordHashTableSize)
                    k = 0;
            }
        }
        else if (isdigit(yyCh)) {
            do {
                yyCh = getChar();
            } while (isalnum(yyCh) || yyCh == '.' || yyCh == '+' ||
                     yyCh == '-');
            return Tok_Number;
        }
        else {
            switch (yyCh) {
            case '!':
            case '%':
                yyCh = getChar();
                if (yyCh == '=')
                    yyCh = getChar();
                return Tok_SomeOperator;
            case '"':
                yyCh = getChar();

                while (yyCh != EOF && yyCh != '"') {
                    if (yyCh == '\\')
                        yyCh = getChar();
                    yyCh = getChar();
                }
                yyCh = getChar();

                if (yyCh == EOF)
                    yyTokLoc.warning(tr("Unterminated C++ string literal"),
                                     tr("Maybe you forgot '/*!' at the beginning of the file?"));
                else
                    return Tok_String;
                break;
            case '#':
                return getTokenAfterPreprocessor();
            case '&':
                yyCh = getChar();
                /*
                  Removed check for '&&', only interpret '&=' as an operator.
                  '&&' is also used for an rvalue reference. QTBUG-32675
                 */
                if (yyCh == '=') {
                    yyCh = getChar();
                    return Tok_SomeOperator;
                }
                else {
                    return Tok_Ampersand;
                }
            case '\'':
                yyCh = getChar();
                /*
                  Allow empty character literal. QTBUG-25775
                 */
                if (yyCh == '\'') {
                    yyCh = getChar();
                    break;
                }
                if (yyCh == '\\')
                    yyCh = getChar();
                do {
                    yyCh = getChar();
                } while (yyCh != EOF && yyCh != '\'');

                if (yyCh == EOF) {
                    yyTokLoc.warning(tr("Unterminated C++ character literal"));
                }
                else {
                    yyCh = getChar();
                    return Tok_Number;
                }
                break;
            case '(':
                yyCh = getChar();
                if (yyNumPreprocessorSkipping == 0)
                    yyParenDepth++;
                if (isspace(yyCh)) {
                    do {
                        yyCh = getChar();
                    } while (isspace(yyCh));
                    yyLexLen = 1;
                    yyLex[1] = '\0';
                }
                if (yyCh == '*') {
                    yyCh = getChar();
                    return Tok_LeftParenAster;
                }
                return Tok_LeftParen;
            case ')':
                yyCh = getChar();
                if (yyNumPreprocessorSkipping == 0)
                    yyParenDepth--;
                return Tok_RightParen;
            case '*':
                yyCh = getChar();
                if (yyCh == '=') {
                    yyCh = getChar();
                    return Tok_SomeOperator;
                } else {
                    return Tok_Aster;
                }
            case '^':
                yyCh = getChar();
                if (yyCh == '=') {
                    yyCh = getChar();
                    return Tok_SomeOperator;
                } else {
                    return Tok_Caret;
                }
            case '+':
                yyCh = getChar();
                if (yyCh == '+' || yyCh == '=')
                    yyCh = getChar();
                return Tok_SomeOperator;
            case ',':
                yyCh = getChar();
                return Tok_Comma;
            case '-':
                yyCh = getChar();
                if (yyCh == '-' || yyCh == '=') {
                    yyCh = getChar();
                } else if (yyCh == '>') {
                    yyCh = getChar();
                    if (yyCh == '*')
                        yyCh = getChar();
                }
                return Tok_SomeOperator;
            case '.':
                yyCh = getChar();
                if (yyCh == '*') {
                    yyCh = getChar();
                } else if (yyCh == '.') {
                    do {
                        yyCh = getChar();
                    } while (yyCh == '.');
                    return Tok_Ellipsis;
                } else if (isdigit(yyCh)) {
                    do {
                        yyCh = getChar();
                    } while (isalnum(yyCh) || yyCh == '.' || yyCh == '+' ||
                             yyCh == '-');
                    return Tok_Number;
                }
                return Tok_SomeOperator;
            case '/':
                yyCh = getChar();
                if (yyCh == '/') {
                    do {
                        yyCh = getChar();
                    } while (yyCh != EOF && yyCh != '\n');
                } else if (yyCh == '*') {
                    bool metDoc = false; // empty doc is no doc
                    bool metSlashAsterBang = false;
                    bool metAster = false;
                    bool metAsterSlash = false;

                    yyCh = getChar();
                    if (yyCh == '!')
                        metSlashAsterBang = true;

                    while (!metAsterSlash) {
                        if (yyCh == EOF) {
                            yyTokLoc.warning(tr("Unterminated C++ comment"));
                            break;
                        } else {
                            if (yyCh == '*') {
                                metAster = true;
                            } else if (metAster && yyCh == '/') {
                                metAsterSlash = true;
                            } else {
                                metAster = false;
                                if (isgraph(yyCh))
                                    metDoc = true;
                            }
                        }
                        yyCh = getChar();
                    }
                    if (metSlashAsterBang && metDoc)
                        return Tok_Doc;
                    else if (yyParenDepth > 0)
                        return Tok_Comment;
                } else {
                    if (yyCh == '=')
                        yyCh = getChar();
                    return Tok_SomeOperator;
                }
                break;
            case ':':
                yyCh = getChar();
                if (yyCh == ':') {
                    yyCh = getChar();
                    return Tok_Gulbrandsen;
                } else {
                    return Tok_Colon;
                }
            case ';':
                yyCh = getChar();
                return Tok_Semicolon;
            case '<':
                yyCh = getChar();
                if (yyCh == '<') {
                    yyCh = getChar();
                    if (yyCh == '=')
                        yyCh = getChar();
                    return Tok_SomeOperator;
                } else if (yyCh == '=') {
                    yyCh = getChar();
                    return Tok_SomeOperator;
                } else {
                    return Tok_LeftAngle;
                }
            case '=':
                yyCh = getChar();
                if (yyCh == '=') {
                    yyCh = getChar();
                    return Tok_SomeOperator;
                } else {
                    return Tok_Equal;
                }
            case '>':
                yyCh = getChar();
                if (yyCh == '>') {
                    yyCh = getChar();
                    if (yyCh == '=')
                        yyCh = getChar();
                    return Tok_SomeOperator;
                } else if (yyCh == '=') {
                    yyCh = getChar();
                    return Tok_SomeOperator;
                } else {
                    return Tok_RightAngle;
                }
            case '?':
                yyCh = getChar();
                return Tok_SomeOperator;
            case '[':
                yyCh = getChar();
                if (yyNumPreprocessorSkipping == 0)
                    yyBracketDepth++;
                return Tok_LeftBracket;
            case '\\':
                yyCh = getChar();
                yyCh = getChar(); // skip one character
                break;
            case ']':
                yyCh = getChar();
                if (yyNumPreprocessorSkipping == 0)
                    yyBracketDepth--;
                return Tok_RightBracket;
            case '{':
                yyCh = getChar();
                if (yyNumPreprocessorSkipping == 0)
                    yyBraceDepth++;
                return Tok_LeftBrace;
            case '}':
                yyCh = getChar();
                if (yyNumPreprocessorSkipping == 0)
                    yyBraceDepth--;
                return Tok_RightBrace;
            case '|':
                yyCh = getChar();
                if (yyCh == '|' || yyCh == '=')
                    yyCh = getChar();
                return Tok_SomeOperator;
            case '~':
                yyCh = getChar();
                return Tok_Tilde;
            case '@':
                yyCh = getChar();
                return Tok_At;
            default:
                // ### We should really prevent qdoc from looking at snippet files rather than
                // ### suppress warnings when reading them.
                if (yyNumPreprocessorSkipping == 0 && !(yyTokLoc.fileName().endsWith(".qdoc") ||
                                                        yyTokLoc.fileName().endsWith(".js"))) {
                    yyTokLoc.warning(tr("Hostile character 0x%1 in C++ source")
                                     .arg((uchar)yyCh, 1, 16));
                }
                yyCh = getChar();
            }
        }
    }

    if (yyPreprocessorSkipping.count() > 1) {
        yyTokLoc.warning(tr("Expected #endif before end of file"));
        // clear it out or we get an infinite loop!
        while (!yyPreprocessorSkipping.isEmpty()) {
            popSkipping();
        }
    }

    strcpy(yyLex, "end-of-input");
    yyLexLen = strlen(yyLex);
    return Tok_Eoi;
}

void Tokenizer::initialize(const Config &config)
{
    QString versionSym = config.getString(CONFIG_VERSIONSYM);

    QString sourceEncoding = config.getString(CONFIG_SOURCEENCODING);
    if (sourceEncoding.isEmpty())
        sourceEncoding = QLatin1String("ISO-8859-1");
#ifndef QT_NO_TEXTCODEC
    sourceCodec = QTextCodec::codecForName(sourceEncoding.toLocal8Bit());
#endif

    comment = new QRegExp("/(?:\\*.*\\*/|/.*\n|/[^\n]*$)");
    comment->setMinimal(true);
    versionX = new QRegExp("$cannot possibly match^");
    if (!versionSym.isEmpty())
        versionX->setPattern("[ \t]*(?:" + QRegExp::escape(versionSym)
                             + ")[ \t]+\"([^\"]*)\"[ \t]*");
    definedX = new QRegExp("defined ?\\(?([A-Z_0-9a-z]+) ?\\)");

    QStringList d = config.getStringList(CONFIG_DEFINES);
    d += "qdoc";
    defines = new QRegExp(d.join('|'));
    falsehoods = new QRegExp(config.getStringList(CONFIG_FALSEHOODS).join('|'));

    memset(kwordHashTable, 0, sizeof(kwordHashTable));
    for (int i = 0; i < Tok_LastKeyword - Tok_FirstKeyword + 1; i++)
        insertKwordIntoHash(kwords[i], i + 1);

    ignoredTokensAndDirectives = new QHash<QByteArray, bool>;

    QStringList tokens = config.getStringList(LANGUAGE_CPP + Config::dot + CONFIG_IGNORETOKENS);
    foreach (const QString &t, tokens) {
        const QByteArray tb = t.toLatin1();
        ignoredTokensAndDirectives->insert(tb, false);
        insertKwordIntoHash(tb.data(), -1);
    }

    QStringList directives = config.getStringList(LANGUAGE_CPP + Config::dot
                                                  + CONFIG_IGNOREDIRECTIVES);
    foreach (const QString &d, directives) {
        const QByteArray db = d.toLatin1();
        ignoredTokensAndDirectives->insert(db, true);
        insertKwordIntoHash(db.data(), -1);
    }
}

void Tokenizer::terminate()
{
    delete comment;
    comment = 0;
    delete versionX;
    versionX = 0;
    delete definedX;
    definedX = 0;
    delete defines;
    defines = 0;
    delete falsehoods;
    falsehoods = 0;
    delete ignoredTokensAndDirectives;
    ignoredTokensAndDirectives = 0;
}

void Tokenizer::init()
{
    yyLexBuf1 = new char[(int) yyLexBufSize];
    yyLexBuf2 = new char[(int) yyLexBufSize];
    yyPrevLex = yyLexBuf1;
    yyPrevLex[0] = '\0';
    yyLex = yyLexBuf2;
    yyLex[0] = '\0';
    yyLexLen = 0;
    yyPreprocessorSkipping.push(false);
    yyNumPreprocessorSkipping = 0;
    yyBraceDepth = 0;
    yyParenDepth = 0;
    yyBracketDepth = 0;
    yyCh = '\0';
    parsingMacro = false;
}

void Tokenizer::start(const Location& loc)
{
    yyTokLoc = loc;
    yyCurLoc = loc;
    yyCurLoc.start();
    strcpy(yyPrevLex, "beginning-of-input");
    strcpy(yyLex, "beginning-of-input");
    yyLexLen = strlen(yyLex);
    yyBraceDepth = 0;
    yyParenDepth = 0;
    yyBracketDepth = 0;
    yyCh = '\0';
    yyCh = getChar();
}

/*
  Returns the next token, if # was met.  This function interprets the
  preprocessor directive, skips over any #ifdef'd out tokens, and returns the
  token after all of that.
*/
int Tokenizer::getTokenAfterPreprocessor()
{
    yyCh = getChar();
    while (isspace(yyCh) && yyCh != '\n')
        yyCh = getChar();

    /*
      #directive condition
    */
    QString directive;
    QString condition;

    while (isalpha(yyCh)) {
        directive += QChar(yyCh);
        yyCh = getChar();
    }
    if (!directive.isEmpty()) {
        while (yyCh != EOF && yyCh != '\n') {
            if (yyCh == '\\') {
                yyCh = getChar();
                if (yyCh == '\r')
                    yyCh = getChar();
            }
            condition += yyCh;
            yyCh = getChar();
        }
        condition.remove(*comment);
        condition = condition.simplified();

        /*
          The #if, #ifdef, #ifndef, #elif, #else, and #endif
          directives have an effect on the skipping stack.  For
          instance, if the code processed so far is

              #if 1
              #if 0
              #if 1
              // ...
              #else

          the skipping stack contains, from bottom to top, false true
          true (assuming 0 is false and 1 is true).  If at least one
          entry of the stack is true, the tokens are skipped.

          This mechanism is simple yet hard to understand.
        */
        if (directive[0] == QChar('i')) {
            if (directive == QString("if"))
                pushSkipping(!isTrue(condition));
            else if (directive == QString("ifdef"))
                pushSkipping(!defines->exactMatch(condition));
            else if (directive == QString("ifndef"))
                pushSkipping(defines->exactMatch(condition));
        } else if (directive[0] == QChar('e')) {
            if (directive == QString("elif")) {
                bool old = popSkipping();
                if (old)
                    pushSkipping(!isTrue(condition));
                else
                    pushSkipping(true);
            } else if (directive == QString("else")) {
                pushSkipping(!popSkipping());
            } else if (directive == QString("endif")) {
                popSkipping();
            }
        } else if (directive == QString("define")) {
            if (versionX->exactMatch(condition))
                yyVersion = versionX->cap(1);
        }
    }

    int tok;
    do {
        /*
          We set yyLex now, and after getToken() this will be
          yyPrevLex. This way, we skip over the preprocessor
          directive.
        */
        qstrcpy(yyLex, yyPrevLex);

        /*
          If getToken() meets another #, it will call
          getTokenAfterPreprocessor() once again, which could in turn
          call getToken() again, etc. Unless there are 10,000 or so
          preprocessor directives in a row, this shouldn't overflow
          the stack.
        */
        tok = getToken();
    } while (yyNumPreprocessorSkipping > 0 && tok != Tok_Eoi);
    return tok;
}

/*
  Pushes a new skipping value onto the stack.  This corresponds to entering a
  new #if block.
*/
void Tokenizer::pushSkipping(bool skip)
{
    yyPreprocessorSkipping.push(skip);
    if (skip)
        yyNumPreprocessorSkipping++;
}

/*
  Pops a skipping value from the stack.  This corresponds to reaching a #endif.
*/
bool Tokenizer::popSkipping()
{
    if (yyPreprocessorSkipping.isEmpty()) {
        yyTokLoc.warning(tr("Unexpected #elif, #else or #endif"));
        return true;
    }

    bool skip = yyPreprocessorSkipping.pop();
    if (skip)
        yyNumPreprocessorSkipping--;
    return skip;
}

/*
  Returns \c true if the condition evaluates as true, otherwise false.  The
  condition is represented by a string.  Unsophisticated parsing techniques are
  used.  The preprocessing method could be named StriNg-Oriented PreProcessing,
  as SNOBOL stands for StriNg-Oriented symBOlic Language.
*/
bool Tokenizer::isTrue(const QString &condition)
{
    int firstOr = -1;
    int firstAnd = -1;
    int parenDepth = 0;

    /*
      Find the first logical operator at top level, but be careful
      about precedence. Examples:

          X || Y          // the or
          X || Y || Z     // the leftmost or
          X || Y && Z     // the or
          X && Y || Z     // the or
          (X || Y) && Z   // the and
    */
    for (int i = 0; i < (int) condition.length() - 1; i++) {
        QChar ch = condition[i];
        if (ch == QChar('(')) {
            parenDepth++;
        } else if (ch == QChar(')')) {
            parenDepth--;
        } else if (parenDepth == 0) {
            if (condition[i + 1] == ch) {
                if (ch == QChar('|')) {
                    firstOr = i;
                    break;
                } else if (ch == QChar('&')) {
                    if (firstAnd == -1)
                        firstAnd = i;
                }
            }
        }
    }
    if (firstOr != -1)
        return isTrue(condition.left(firstOr)) ||
                isTrue(condition.mid(firstOr + 2));
    if (firstAnd != -1)
        return isTrue(condition.left(firstAnd)) &&
                isTrue(condition.mid(firstAnd + 2));

    QString t = condition.simplified();
    if (t.isEmpty())
        return true;

    if (t[0] == QChar('!'))
        return !isTrue(t.mid(1));
    if (t[0] == QChar('(') && t.endsWith(QChar(')')))
        return isTrue(t.mid(1, t.length() - 2));

    if (definedX->exactMatch(t))
        return defines->exactMatch(definedX->cap(1));
    else
        return !falsehoods->exactMatch(t);
}

QString Tokenizer::lexeme() const
{
#ifndef QT_NO_TEXTCODEC
    return sourceCodec->toUnicode(yyLex);
#else
    return QString::fromUtf8(yyLex);
#endif
}

QString Tokenizer::previousLexeme() const
{
#ifndef QT_NO_TEXTCODEC
    return sourceCodec->toUnicode(yyPrevLex);
#else
    return QString::fromUtf8(yyPrevLex);
#endif
}

QT_END_NAMESPACE
