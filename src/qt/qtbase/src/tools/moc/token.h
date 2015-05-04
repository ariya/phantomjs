/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TOKEN_H
#define TOKEN_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#define FOR_ALL_TOKENS(F) \
    F(NOTOKEN) \
    F(IDENTIFIER) \
    F(INTEGER_LITERAL) \
    F(CHARACTER_LITERAL) \
    F(STRING_LITERAL) \
    F(BOOLEAN_LITERAL) \
    F(HEADER_NAME) \
    F(LANGLE) \
    F(RANGLE) \
    F(LPAREN) \
    F(RPAREN) \
    F(ELIPSIS) \
    F(LBRACK) \
    F(RBRACK) \
    F(LBRACE) \
    F(RBRACE) \
    F(EQ) \
    F(SCOPE) \
    F(SEMIC) \
    F(COLON) \
    F(DOTSTAR) \
    F(QUESTION) \
    F(DOT) \
    F(DYNAMIC_CAST) \
    F(STATIC_CAST) \
    F(REINTERPRET_CAST) \
    F(CONST_CAST) \
    F(TYPEID) \
    F(THIS) \
    F(TEMPLATE) \
    F(THROW) \
    F(TRY) \
    F(CATCH) \
    F(TYPEDEF) \
    F(FRIEND) \
    F(CLASS) \
    F(NAMESPACE) \
    F(ENUM) \
    F(STRUCT) \
    F(UNION) \
    F(VIRTUAL) \
    F(PRIVATE) \
    F(PROTECTED) \
    F(PUBLIC) \
    F(EXPORT) \
    F(AUTO) \
    F(REGISTER) \
    F(EXTERN) \
    F(MUTABLE) \
    F(ASM) \
    F(USING) \
    F(INLINE) \
    F(EXPLICIT) \
    F(STATIC) \
    F(CONST) \
    F(VOLATILE) \
    F(OPERATOR) \
    F(SIZEOF) \
    F(NEW) \
    F(DELETE) \
    F(PLUS) \
    F(MINUS) \
    F(STAR) \
    F(SLASH) \
    F(PERCENT) \
    F(HAT) \
    F(AND) \
    F(OR) \
    F(TILDE) \
    F(NOT) \
    F(PLUS_EQ) \
    F(MINUS_EQ) \
    F(STAR_EQ) \
    F(SLASH_EQ) \
    F(PERCENT_EQ) \
    F(HAT_EQ) \
    F(AND_EQ) \
    F(OR_EQ) \
    F(LTLT) \
    F(GTGT) \
    F(GTGT_EQ) \
    F(LTLT_EQ) \
    F(EQEQ) \
    F(NE) \
    F(LE) \
    F(GE) \
    F(ANDAND) \
    F(OROR) \
    F(INCR) \
    F(DECR) \
    F(COMMA) \
    F(ARROW_STAR) \
    F(ARROW) \
    F(CHAR) \
    F(WCHAR) \
    F(BOOL) \
    F(SHORT) \
    F(INT) \
    F(LONG) \
    F(SIGNED) \
    F(UNSIGNED) \
    F(FLOAT) \
    F(DOUBLE) \
    F(VOID) \
    F(CASE) \
    F(DEFAULT) \
    F(IF) \
    F(ELSE) \
    F(SWITCH) \
    F(WHILE) \
    F(DO) \
    F(FOR) \
    F(BREAK) \
    F(CONTINUE) \
    F(GOTO) \
    F(SIGNALS) \
    F(SLOTS) \
    F(RETURN) \
    F(Q_OBJECT_TOKEN) \
    F(Q_GADGET_TOKEN) \
    F(Q_PROPERTY_TOKEN) \
    F(Q_PLUGIN_METADATA_TOKEN) \
    F(Q_ENUMS_TOKEN) \
    F(Q_FLAGS_TOKEN) \
    F(Q_DECLARE_FLAGS_TOKEN) \
    F(Q_DECLARE_INTERFACE_TOKEN) \
    F(Q_DECLARE_METATYPE_TOKEN) \
    F(Q_CLASSINFO_TOKEN) \
    F(Q_INTERFACES_TOKEN) \
    F(Q_SIGNALS_TOKEN) \
    F(Q_SLOTS_TOKEN) \
    F(Q_SIGNAL_TOKEN) \
    F(Q_SLOT_TOKEN) \
    F(Q_PRIVATE_SLOT_TOKEN) \
    F(Q_MOC_COMPAT_TOKEN) \
    F(Q_INVOKABLE_TOKEN) \
    F(Q_SCRIPTABLE_TOKEN) \
    F(Q_PRIVATE_PROPERTY_TOKEN) \
    F(Q_REVISION_TOKEN) \
    F(SPECIAL_TREATMENT_MARK) \
    F(MOC_INCLUDE_BEGIN) \
    F(MOC_INCLUDE_END) \
    F(CPP_COMMENT) \
    F(C_COMMENT) \
    F(FLOATING_LITERAL) \
    F(HASH) \
    F(QUOTE) \
    F(SINGLEQUOTE) \
    F(LANGLE_SCOPE) \
    F(DIGIT) \
    F(CHARACTER) \
    F(NEWLINE) \
    F(WHITESPACE) \
    F(BACKSLASH) \
    F(INCOMPLETE) \
    F(PP_DEFINE) \
    F(PP_UNDEF) \
    F(PP_IF) \
    F(PP_IFDEF) \
    F(PP_IFNDEF) \
    F(PP_ELIF) \
    F(PP_ELSE) \
    F(PP_ENDIF) \
    F(PP_INCLUDE) \
    F(PP_HASHHASH) \
    F(PP_HASH) \
    F(PP_DEFINED) \
    F(PP_INCOMPLETE) \
    F(PP_MOC_TRUE) \
    F(PP_MOC_FALSE)


enum Token {

#define CREATE_ENUM_VALUE(Name) Name,
    FOR_ALL_TOKENS(CREATE_ENUM_VALUE)
#undef CREATE_ENUM_VALUE

    // aliases
    PP_AND = AND,
    PP_ANDAND = ANDAND,
    PP_BACKSLASH = BACKSLASH,
    PP_CHARACTER = CHARACTER,
    PP_CHARACTER_LITERAL = CHARACTER_LITERAL,
    PP_COLON = COLON,
    PP_COMMA = COMMA,
    PP_CPP_COMMENT = CPP_COMMENT,
    PP_C_COMMENT = C_COMMENT,
    PP_DIGIT = DIGIT,
    PP_EQEQ = EQEQ,
    PP_FLOATING_LITERAL = FLOATING_LITERAL,
    PP_GE = GE,
    PP_GTGT = GTGT,
    PP_HAT = HAT,
    PP_IDENTIFIER = IDENTIFIER,
    PP_INTEGER_LITERAL = INTEGER_LITERAL,
    PP_LANGLE = LANGLE,
    PP_LE = LE,
    PP_LPAREN = LPAREN,
    PP_LTLT = LTLT,
    PP_MINUS = MINUS,
    PP_NE = NE,
    PP_NEWLINE = NEWLINE,
    PP_NOTOKEN = NOTOKEN,
    PP_NOT = NOT,
    PP_OR = OR,
    PP_OROR = OROR,
    PP_PERCENT = PERCENT,
    PP_PLUS = PLUS,
    PP_QUESTION = QUESTION,
    PP_QUOTE = QUOTE,
    PP_RANGLE = RANGLE,
    PP_RPAREN = RPAREN,
    PP_SINGLEQUOTE = SINGLEQUOTE,
    PP_SLASH = SLASH,
    PP_STAR = STAR,
    PP_STRING_LITERAL = STRING_LITERAL,
    PP_TILDE = TILDE,
    PP_WHITESPACE = WHITESPACE,
    Q_META_TOKEN_BEGIN = Q_OBJECT_TOKEN,
    Q_META_TOKEN_END = SPECIAL_TREATMENT_MARK
};

// for debugging only
#if defined(DEBUG_MOC)
const char *tokenTypeName(Token t);
#endif

typedef Token PP_Token;

QT_END_NAMESPACE

#endif // TOKEN_H
