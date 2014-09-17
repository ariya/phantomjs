/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include <stdio.h>
#include <string.h>
#include <qbytearray.h>
#include <qlist.h>

struct Keyword
{
    const char *lexem;
    const char *token;
};

static const Keyword pp_keywords[] = {
    { "<", "PP_LANGLE" },
    { ">", "PP_RANGLE" },
    { "(", "PP_LPAREN"},
    { ")", "PP_RPAREN"},
    { ",", "PP_COMMA"},
    { "\n", "PP_NEWLINE"},
    { "#define", "PP_DEFINE"},
    { "#if", "PP_IF"},
    { "#undef", "PP_UNDEF"},
    { "#ifdef", "PP_IFDEF"},
    { "#ifndef", "PP_IFNDEF"},
    { "#elif", "PP_ELIF"},
    { "#else", "PP_ELSE"},
    { "#endif", "PP_ENDIF"},
    { "#include", "PP_INCLUDE"},
    { "defined", "PP_DEFINED"},
    { "+", "PP_PLUS" },
    { "-", "PP_MINUS" },
    { "*", "PP_STAR" },
    { "/", "PP_SLASH" },
    { "%", "PP_PERCENT" },
    { "^", "PP_HAT" },
    { "&", "PP_AND" },
    { "bitand", "PP_AND" },
    { "|", "PP_OR" },
    { "bitor", "PP_OR" },
    { "~", "PP_TILDE" },
    { "compl", "PP_TILDE" },
    { "!", "PP_NOT" },
    { "not", "PP_NOT" },
    { "<<", "PP_LTLT" },
    { ">>", "PP_GTGT" },
    { "==", "PP_EQEQ" },
    { "!=", "PP_NE" },
    { "not_eq", "PP_NE" },
    { "<=", "PP_LE" },
    { ">=", "PP_GE" },
    { "&&", "PP_ANDAND" },
    { "||", "PP_OROR" },
    { "?", "PP_QUESTION" },
    { ":", "PP_COLON" },
    { "##", "PP_HASHHASH"},
    { "%:%:", "PP_HASHHASH"},
    { "#", "PP_HASH"},
    { "\"", "PP_QUOTE"},
    { "\'", "PP_SINGLEQUOTE"},
    { " ", "PP_WHITESPACE" },
    { "\t", "PP_WHITESPACE" },
    { "//", "PP_CPP_COMMENT" },
    { "/*", "PP_C_COMMENT" },
    { "\\", "PP_BACKSLASH" },
    { 0, "PP_NOTOKEN"}
};

static const Keyword keywords[] = {
    { "<", "LANGLE" },
    { ">", "RANGLE" },
    { "(", "LPAREN" },
    { ")", "RPAREN" },
    { "...", "ELIPSIS" },
    { ",", "COMMA" },
    { "[", "LBRACK" },
    { "]", "RBRACK" },
    { "<:", "LBRACK" },
    { ":>", "RBRACK" },
    { "<::", "LANGLE_SCOPE" },
    { "{", "LBRACE" },
    { "<%", "LBRACE" },
    { "}", "RBRACE" },
    { "%>", "RBRACE" },
    { "=", "EQ" },
    { "::", "SCOPE" },
    { ";", "SEMIC" },
    { ":", "COLON" },
    { ".*", "DOTSTAR" },
    { "?", "QUESTION" },
    { ".", "DOT" },
    { "dynamic_cast", "DYNAMIC_CAST" },
    { "static_cast", "STATIC_CAST" },
    { "reinterpret_cast", "REINTERPRET_CAST" },
    { "const_cast", "CONST_CAST" },
    { "typeid", "TYPEID" },
    { "this", "THIS" },
    { "template", "TEMPLATE" },
    { "throw", "THROW" },
    { "try", "TRY" },
    { "catch", "CATCH" },
    { "typedef", "TYPEDEF" },
    { "friend", "FRIEND" },
    { "class", "CLASS" },
    { "namespace", "NAMESPACE" },
    { "enum", "ENUM" },
    { "struct", "STRUCT" },
    { "union", "UNION" },
    { "virtual", "VIRTUAL" },
    { "private", "PRIVATE" },
    { "protected", "PROTECTED" },
    { "public", "PUBLIC" },
    { "export", "EXPORT" },
    { "auto", "AUTO" },
    { "register", "REGISTER" },
    { "extern", "EXTERN" },
    { "mutable", "MUTABLE" },
    { "asm", "ASM" },
    { "using", "USING" },
    { "inline", "INLINE" },
    { "explicit", "EXPLICIT" },
    { "static", "STATIC" },
    { "const", "CONST" },
    { "volatile", "VOLATILE" },
    { "operator", "OPERATOR" },
    { "sizeof", "SIZEOF" },
    { "new", "NEW" },
    { "delete", "DELETE" },
    { "+", "PLUS" },
    { "-", "MINUS" },
    { "*", "STAR" },
    { "/", "SLASH" },
    { "%", "PERCENT" },
    { "^", "HAT" },
    { "&", "AND" },
    { "bitand", "AND" },
    { "|", "OR" },
    { "bitor", "OR" },
    { "~", "TILDE" },
    { "compl", "TILDE" },
    { "!", "NOT" },
    { "not", "NOT" },
    { "+=", "PLUS_EQ" },
    { "-=", "MINUS_EQ" },
    { "*=", "STAR_EQ" },
    { "/=", "SLASH_EQ" },
    { "%=", "PERCENT_EQ" },
    { "^=", "HAT_EQ" },
    { "&=", "AND_EQ" },
    { "|=", "OR_EQ" },
    { "<<", "LTLT" },
    { ">>", "GTGT" },
    { ">>=", "GTGT_EQ" },
    { "<<=", "LTLT_EQ" },
    { "==", "EQEQ" },
    { "!=", "NE" },
    { "not_eq", "NE" },
    { "<=", "LE" },
    { ">=", "GE" },
    { "&&", "ANDAND" },
    { "||", "OROR" },
    { "++", "INCR" },
    { "--", "DECR" },
    { ",", "COMMA" },
    { "->*", "ARROW_STAR" },
    { "->", "ARROW" },
    { "char", "CHAR" },
    { "wchar", "WCHAR" },
    { "bool", "BOOL" },
    { "short", "SHORT" },
    { "int", "INT" },
    { "long", "LONG" },
    { "signed", "SIGNED" },
    { "unsigned", "UNSIGNED" },
    { "float", "FLOAT" },
    { "double", "DOUBLE" },
    { "void", "VOID" },
    { "case", "CASE" },
    { "default", "DEFAULT" },
    { "if", "IF" },
    { "else", "ELSE" },
    { "switch", "SWITCH" },
    { "while", "WHILE" },
    { "do", "DO" },
    { "for", "FOR" },
    { "break", "BREAK" },
    { "continue", "CONTINUE" },
    { "goto", "GOTO" },
    { "return", "RETURN" },
    { "Q_OBJECT", "Q_OBJECT_TOKEN" },
    { "Q_GADGET", "Q_GADGET_TOKEN" },
    { "Q_PROPERTY", "Q_PROPERTY_TOKEN" },
    { "Q_ENUMS", "Q_ENUMS_TOKEN" },
    { "Q_FLAGS", "Q_FLAGS_TOKEN" },
    { "Q_DECLARE_FLAGS", "Q_DECLARE_FLAGS_TOKEN" },
    { "Q_DECLARE_INTERFACE", "Q_DECLARE_INTERFACE_TOKEN" },
    { "Q_DECLARE_METATYPE", "Q_DECLARE_METATYPE_TOKEN" },
    { "Q_DECLARE_EXTENSION_INTERFACE", "Q_DECLARE_INTERFACE_TOKEN" },
    { "Q_SETS", "Q_FLAGS_TOKEN" },
    { "Q_CLASSINFO", "Q_CLASSINFO_TOKEN" },
    { "Q_INTERFACES", "Q_INTERFACES_TOKEN" },
    { "signals", "SIGNALS" },
    { "slots", "SLOTS" },
    { "Q_SIGNALS", "Q_SIGNALS_TOKEN" },
    { "Q_SLOTS", "Q_SLOTS_TOKEN" },
    { "Q_PRIVATE_SLOT", "Q_PRIVATE_SLOT_TOKEN" },
    { "QT_MOC_COMPAT", "Q_MOC_COMPAT_TOKEN" },
    { "QT3_SUPPORT", "Q_QT3_SUPPORT_TOKEN" },
    { "Q_INVOKABLE", "Q_INVOKABLE_TOKEN" },
    { "Q_SIGNAL", "Q_SIGNAL_TOKEN" },
    { "Q_SLOT", "Q_SLOT_TOKEN" },
    { "Q_SCRIPTABLE", "Q_SCRIPTABLE_TOKEN" },
    { "Q_PRIVATE_PROPERTY", "Q_PRIVATE_PROPERTY_TOKEN" },
    { "Q_REVISION", "Q_REVISION_TOKEN" },
    { "\n", "NEWLINE" },
    { "\"", "QUOTE" },
    { "\'", "SINGLEQUOTE" },
    { " ", "WHITESPACE" },
    { "\t", "WHITESPACE" },
    { "#", "HASH" },
    { "\\", "BACKSLASH" },
    { "//", "CPP_COMMENT" },
    { "/*", "C_COMMENT" },
    { 0, "NOTOKEN"}
};


inline bool is_ident_start(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || s == '_'
        );
}

inline bool is_ident_char(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || (s >= '0' && s <= '9')
            || s == '_'
        );
}
struct State
{
    State(const char* token):token(token), nextindex(0),
            defchar(0), defnext(0), ident(0) {
        memset( next, 0, sizeof(next));
    }
    QByteArray token;
    int next[128];
    int nextindex;

    char defchar;
    int defnext;

    const char *ident;

    bool operator==(const State& o) const
        {
            return (token == o.token
                    && nextindex == o.nextindex
                    && defchar == o.defchar
                    && defnext == o.defnext
                    && ident == o.ident);
        }
};

void newState(QList<State> &states, const char *token, const char *lexem, bool pre)
{
    const char * ident = 0;
    if (is_ident_start(*lexem))
        ident = pre?"PP_CHARACTER" : "CHARACTER";
    else if (*lexem == '#')
        ident = pre?"PP_HASH" : "HASH";

    int state = 0;
    while (*lexem) {
        int next = states[state].next[(int)*lexem];
        if (!next) {
            const char * t = 0;
            if (ident)
                t = ident;
            else
                t = pre?"PP_INCOMPLETE":"INCOMPLETE";
            next = states.size();
            states += State(t);
            states[state].next[(int)*lexem] = next;
            states[next].ident = ident;
        }
        state = next;
        ++lexem;
        if (ident && !is_ident_char(*lexem))
            ident = 0;
    }
    states[state].token = token;
}

void newState(QList<State> &states, const char *token,  char lexem)
{
    int next = states[0].next[(int)lexem];
    if (!next) {
        next = states.size();
        states += State(token);
        states[0].next[(int)lexem] = next;
    } else {
        states[next].token = token;
    }
}


void makeTable(const Keyword keywords[])
{
    int i,c;
    bool pre = (keywords == pp_keywords);
    QList<State> states;
    states += State(pre?"PP_NOTOKEN":"NOTOKEN");

    // identifiers
    for (c = 'a'; c <= 'z'; ++c)
        newState(states, pre?"PP_CHARACTER":"CHARACTER", c);
    for (c = 'A'; c <= 'Z'; ++c)
        newState(states, pre?"PP_CHARACTER":"CHARACTER", c);
    c = '_';
    newState(states, pre?"PP_CHARACTER":"CHARACTER", c);

    // add digits
    for (c = '0'; c <= '9'; ++c)
        newState(states, pre?"PP_DIGIT":"DIGIT", c);

    // keywords
    for (i = 0; keywords[i].lexem; ++i)
        newState(states, keywords[i].token, keywords[i].lexem, pre);

    // some floats
    for (c = '0'; c <= '9'; ++c)
        newState(states, pre?"PP_FLOATING_LITERAL":"FLOATING_LITERAL",
                 QByteArray(".") + char(c), pre);

    // simplify table with default transitions
    int transindex = -1;
    for (i = 0; i < states.size(); ++i) {
        int n = 0;
        int defchar = -1;
        for (c = 0; c < 128; ++c)
            if (states[i].next[c]) {
                ++n;
                defchar = c;
            }
        if (!n)
            continue;
        if (n == 1) {
            states[i].defnext = states[i].next[defchar];
            states[i].defchar = defchar;
            continue;
        }
        states[i].nextindex = ++transindex;
    }

#if 1
    // compress table
    int j, k;
    for (i = 0; i < states.size(); ++i) {
        for (j = i + 1; j < states.size(); ++j) {
            if ( states[i] == states[j] ) {
                for (k = 0; k < states.size(); ++k) {
                    if (states[k].defnext == j)
                        states[k].defnext = i;
                    if (states[k].defnext > j)
                        --states[k].defnext;
                    for (c = 0; c < 128; ++c) {
                        if (states[k].next[c] == j)
                            states[k].next[c] = i;
                        if (states[k].next[c] > j)
                            --states[k].next[c];
                    }
                }
                states.removeAt(j);
                --j;
            }
        }
    }
#endif
    printf("static const short %skeyword_trans[][128] = {\n",
           pre?"pp_":"");
    for (i = 0; i < states.size(); ++i) {
        if (i && !states[i].nextindex)
            continue;
        printf("%s    {", i?",\n":"");
        for (c = 0; c < 128; ++c)
            printf("%s%s%d",
                   c?",":"",
                   (!c || c%16)?"":"\n     ",
                   states[i].next[c]
                   );
        printf("}");
    }
    printf("\n};\n\n");

    printf("static const struct\n{\n"
           "   %sToken token;\n"
           "   short next;\n"
           "   char defchar;\n"
           "   short defnext;\n"
           "   %sToken ident;\n"
           "} %skeywords[] = {\n",
           pre ? "PP_":"",
           pre ? "PP_":"",
           pre ? "pp_":"");
    for (i = 0; i < states.size(); ++i) {
        printf("%s    {%s, %d, %d, %d, %s}",
               i?",\n":"",
               states[i].token.data(),
               states[i].nextindex,
               states[i].defchar,
               states[i].defnext,
               states[i].ident?states[i].ident:(pre?"PP_NOTOKEN":"NOTOKEN"));
    }
    printf("\n};\n");
}

int main(int argc, char **)
{
    printf("// auto generated\n"
           "// DO NOT EDIT.\n\n");
    if ( argc > 1 )
        makeTable(pp_keywords);
    else
        makeTable(keywords);
    return 0;
}
